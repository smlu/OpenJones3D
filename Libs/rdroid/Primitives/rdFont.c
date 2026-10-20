#include "rdFont.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#include <stdio.h>

#define RDFONT_MAXGLYPHS  256u
#define RDFONT_MAXNAMELEN 256u


bool rdFont_bStartup = false;
bool rdFont_bOpen    = false;

bool rdFont_bFontColorSet = false;
rdLightMode rdFont_lightingMode;
rdFontColor rdFont_aFontColors;

static void J3DAPI rdFont_DrawChar(size_t chr, float x, float y, float z, const rdFont* pFont);
static void J3DAPI rdFont_DrawCharScaled(size_t chr, float x, float y, float z, const rdFont* pFont, float fontSizePt); // New

void rdFont_InstallHooks(void)
{
    J3D_HOOKFUNC(rdFont_Load);
    J3D_HOOKFUNC(rdFont_LoadEntry);
    J3D_HOOKFUNC(rdFont_LoadMaterial);
    J3D_HOOKFUNC(rdFont_Free);
    J3D_HOOKFUNC(rdFont_SetFontColor);
    J3D_HOOKFUNC(rdFont_GetWrapLine);
    J3D_HOOKFUNC(rdFont_GetTextWidth);
    J3D_HOOKFUNC(rdFont_DrawTextLine);
    J3D_HOOKFUNC(rdFont_DrawChar);
    J3D_HOOKFUNC(rdFont_DrawTextLineClipped);
    J3D_HOOKFUNC(rdFont_Startup);
    J3D_HOOKFUNC(rdFont_Shutdown);
    J3D_HOOKFUNC(rdFont_Open);
    J3D_HOOKFUNC(rdFont_Close);
}

void rdFont_ResetGlobals(void)
{

}

void rdFont_Startup(void)
{
    if ( rdFont_bStartup )
    {
        RDLOG_STATUS("Warning: System already initialized!\n");
        return; // Added
    }

    rdFont_bStartup = true;
}

void rdFont_Shutdown(void)
{
    if ( rdFont_bOpen )
    {
        rdFont_Close();
    }

    if ( !rdFont_bStartup )
    {
        RDLOG_STATUS("Warning: System already shutdown!\n");
        return;
    }

    rdFont_bStartup = false;
}

int rdFont_Open(void)
{
    if ( rdFont_bOpen )
    {
        RDLOG_STATUS("Warning: System already open!\n");
        return -2;
    }

    rdFont_bOpen = true;
    return 0;
}

void rdFont_Close(void)
{
    if ( !rdFont_bOpen )
    {
        RDLOG_STATUS("Warning: System already closed!\n");
        return;
    }

    rdFont_bOpen = false;
}

int J3DAPI rdFont_Write(const char* pFilename, const rdFont* pFont, tVBuffer*** paTextures)
{
    if ( !pFilename || !pFont->pName )
    {
        return -3;
    }

    bool bGcf = streq(&pFilename[strlen(pFilename) - 4], ".gcf");
    bool bMat = streq(&pFilename[strlen(pFilename) - 4], ".mat");
    if ( !bGcf && !bMat )
    {
        return -5;
    }

    char aMatFilename[256] = { 0 };
    char aGcfFilename[256] = { 0 };

    if ( bGcf )
    {
        STD_STRCPY(aGcfFilename, pFilename);
        STD_STRNCPY(aMatFilename, pFilename, strlen(pFilename) - 3); // 3 - size of extension
        STD_STRCAT(aMatFilename, "mat");
    }
    else // mat
    {
        STD_STRCPY(aMatFilename, pFilename);
        STD_STRNCPY(aGcfFilename, pFilename, strlen(pFilename) - 3); // 3 - size of extension
        STD_STRCAT(aGcfFilename, "gcf");
    }

    if ( !rdFont_WriteEntry(aGcfFilename, pFont) && !rdFont_WriteMaterial(aMatFilename, pFont->pMaterial, paTextures) )
    {
        return 0;
    }

    remove(aGcfFilename);
    remove(aMatFilename);
    RDLOG_ERROR("Error: Unable to write file '%s'.\n", pFilename);
    return -1;
}

int J3DAPI rdFont_WriteEntry(const char* pFilename, const rdFont* pFont)
{
    // Static san. check
    static_assert(sizeof(rdGlyphMetrics) == 24, "sizeof(rdGlyphMetrics) == 24");
    static_assert(sizeof(rdGlyphMetrics) == 24, "sizeof(rdGlyphMetrics) == 24"); // makes sure that 

    tFileHandle fh = rdroid_g_pHS->pFileOpen(pFilename, "wb+");
    if ( !fh )
    {
        RDLOG_ERROR("Error: Unable to open file '%s' for writing.\n", pFilename);
        return -4;
    }
    else if ( rdroid_g_pHS->pFileWrite(fh, &pFont->lineSpacing, sizeof(pFont->lineSpacing)) != sizeof(pFont->lineSpacing)
        || rdroid_g_pHS->pFileWrite(fh, pFont->pName, RDFONT_MAXNAMELEN) != RDFONT_MAXNAMELEN
        || rdroid_g_pHS->pFileWrite(fh, &pFont->fontSize, sizeof(pFont->fontSize)) != sizeof(pFont->fontSize)
        || rdroid_g_pHS->pFileWrite(fh, pFont->aGlyphs, sizeof(rdGlyphMetrics) * RDFONT_MAXGLYPHS) != sizeof(rdGlyphMetrics) * RDFONT_MAXGLYPHS )
    {
        rdroid_g_pHS->pFileClose(fh);
        RDLOG_ERROR("Error: Unable to write file '%s'.\n", pFilename);
        return -1;
    }

    rdroid_g_pHS->pFileClose(fh);
    return 0;
}

int J3DAPI rdFont_WriteMaterial(const char* pFilename, const rdMaterial* pMat, tVBuffer*** paTextures)
{
    if ( rdMaterial_Write(pFilename, pMat, paTextures, /*numMipmaps=*/1u) )
    {
        return -1;
    }

    return 0;
}

rdFont* J3DAPI rdFont_Load(const char* pFilename)
{
    rdFont* pFont = (rdFont*)STDMALLOC(sizeof(rdFont));
    if ( !pFont || !pFilename )
    {
        return NULL;
    }

    memset(pFont, 0, sizeof(rdFont));

    bool bGcf = streq(&pFilename[strlen(pFilename) - 4], ".gcf");
    bool bMat = streq(&pFilename[strlen(pFilename) - 4], ".mat");
    if ( !bGcf && !bMat )
    {
        return NULL;
    }

    char aGcfFilename[256] = { 0 };
    char aMatFilename[256] = { 0 };
    if ( bGcf )
    {
        STD_STRCPY(aGcfFilename, pFilename);
        STD_STRNCPY(aMatFilename, pFilename, strlen(pFilename) - 3); // 3 - size of extension
        STD_STRCAT(aMatFilename, "mat");
    }
    else // mat
    {
        STD_STRCPY(aMatFilename, pFilename);
        STD_STRNCPY(aGcfFilename, pFilename, strlen(pFilename) - 3); // 3 - size of extension
        STD_STRCAT(aGcfFilename, "gcf");
    }

    if ( !rdFont_LoadEntry(aGcfFilename, pFont) && !rdFont_LoadMaterial(aMatFilename, pFont) )
    {
        return pFont;
    }

    // Failure
    if ( !pFont )
    {
        return NULL;
    }

    if ( pFont->pMaterial )
    {
        stdMemory_Free(pFont->pMaterial);
    }

    stdMemory_Free(pFont->aGlyphs);
    stdMemory_Free(pFont->pName);
    stdMemory_Free(pFont);
    return NULL;
}

int J3DAPI rdFont_LoadEntry(const char* pFilename, rdFont* pFont)
{
    // Static san. check
    static_assert(sizeof(rdGlyphMetrics) == 24, "sizeof(rdGlyphMetrics) == 24");
    static_assert(sizeof(rdGlyphMetrics) == 24, "sizeof(rdGlyphMetrics) == 24"); // makes sure that 

    pFont->aGlyphs = (rdGlyphMetrics*)STDMALLOC(sizeof(rdGlyphMetrics) * RDFONT_MAXGLYPHS);
    pFont->pName   = (char*)STDMALLOC(RDFONT_MAXNAMELEN);
    if ( !pFont->aGlyphs && !pFont->pName )
    {
        return -1;
    }

    tFileHandle fh = rdroid_g_pHS->pFileOpen(pFilename, "rb");
    if ( !fh )
    {
        RDLOG_ERROR("Error: Unable to open file '%s'.\n", pFilename);
        return -4;
    }
    else if ( rdroid_g_pHS->pFileRead(fh, &pFont->lineSpacing, sizeof(pFont->lineSpacing)) != sizeof(pFont->lineSpacing)
        || rdroid_g_pHS->pFileRead(fh, pFont->pName, RDFONT_MAXNAMELEN) != RDFONT_MAXNAMELEN
        || rdroid_g_pHS->pFileRead(fh, &pFont->fontSize, sizeof(pFont->fontSize)) != sizeof(pFont->fontSize)
        || rdroid_g_pHS->pFileRead(fh, pFont->aGlyphs, sizeof(rdGlyphMetrics) * RDFONT_MAXGLYPHS) != sizeof(rdGlyphMetrics) * RDFONT_MAXGLYPHS )
    {
        rdroid_g_pHS->pFileClose(fh);
        RDLOG_ERROR("Error: Unable to write file '%s'.\n", pFilename);
        return -1;
    }

    rdroid_g_pHS->pFileClose(fh);
    return 0;
}

int J3DAPI rdFont_LoadMaterial(const char* pFilename, rdFont* pFont)
{
    pFont->pMaterial = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
    if ( !pFont->pMaterial )
    {
        RDLOG_ERROR("Error: Bad memory allocation for '%s'\n", pFilename);
        return -1;

    }

    return rdMaterial_LoadEntry(pFilename, pFont->pMaterial);
}

void J3DAPI rdFont_Free(rdFont* pFont)
{
    rdMaterial_FreeEntry(pFont->pMaterial);
    stdMemory_Free(pFont->pMaterial);
    stdMemory_Free(pFont->aGlyphs);
    stdMemory_Free(pFont->pName);
    stdMemory_Free(pFont);
}

void J3DAPI rdFont_SetFontColorDiffuse(const rdFontColor apColor)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(rdFont_aFontColors); ++i )
    {
        rdVector_Copy4(&rdFont_aFontColors[i], &apColor[i]);
    }

    rdFont_bFontColorSet = true;
    rdFont_lightingMode  = RD_LIGHTING_DIFFUSE;
}

void J3DAPI rdFont_SetFontColor(const rdFontColor apColor)
{
    for ( size_t i = 0; i < STD_ARRAYLEN(rdFont_aFontColors); ++i )
    {
        rdVector_Copy4(&rdFont_aFontColors[i], &apColor[i]);
    }

    rdFont_bFontColorSet = true;
    rdFont_lightingMode  = RD_LIGHTING_GOURAUD;
}

rdFontColor* rdFont_DuplicateFontColor(void)
{
    rdFontColor* aColor = (rdFontColor*)STDMALLOC(sizeof(rdFontColor));
    if ( !aColor )
    {
        return NULL;
    }

    for ( size_t i = 0; i < STD_ARRAYLEN(rdFont_aFontColors); ++i )
    {
        rdVector_Copy4(aColor[i], &rdFont_aFontColors[i]);
    }

    return aColor;
}

size_t J3DAPI rdFont_GetTextWidthEx(const char* pText, const rdFont* pFont, float fontSizePt)
{
    if ( !pFont || !pText )
    {
        return 0;
    }

    float fontScale = fontSizePt / (float)pFont->fontSize;
    float textWidth = 0.0f;
    const size_t texLen = strlen(pText);

    for ( size_t i = 0; i < texLen; ++i )
    {
        size_t curChar = (uint8_t)pText[i];
        if ( pFont->aGlyphs[curChar].baselineOriginY == -pFont->fontSize )
        {
            curChar = '?';
        }

        textWidth = (float)pFont->aGlyphs[curChar].baselineOriginX * fontScale + textWidth;
    }

    return (size_t)textWidth;
}

size_t J3DAPI rdFont_GetTextWidth(const char* pText, const rdFont* pFont)
{
    return rdFont_GetTextWidthEx(pText, pFont, (float)pFont->fontSize);
}

const char* J3DAPI rdFont_GetWrapLineEx(const char* pText, const rdFont* pFont, float widthScale, float fontSizePt)
{
    if ( !pFont || !pText )
    {
        return NULL;
    }

    float fontScale = fontSizePt / (float)pFont->fontSize;
    float textWidth = 0.0f;
    const char* pTextEnd = NULL;
    const size_t textLen = strlen(pText);

    for ( size_t i = 0; i < textLen; ++i )
    {
        size_t curChar = (uint8_t)pText[i];
        if ( pFont->aGlyphs[curChar].baselineOriginY == -pFont->fontSize )
        {
            curChar = '?';
        }

        textWidth = (float)pFont->aGlyphs[curChar].baselineOriginX * fontScale + textWidth;
        if ( RD_REF_WIDTH * widthScale < textWidth )
        {
            if ( pTextEnd )
            {
                return pTextEnd + 1;
            }

            return pText;
        }

        if ( curChar == ' ' )
        {
            pTextEnd = &pText[i];
        }
    }

    return NULL;
}

const char* J3DAPI rdFont_GetWrapLine(const char* pText, const rdFont* pFont, float widthScale)
{
    return rdFont_GetWrapLineEx(pText, pFont, widthScale, (float)pFont->fontSize);
}

void J3DAPI rdFont_DrawTextLineEx(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags, float fontSizePt)
{
    int bAlignRight = alignFlags & RDFONT_ALIGNRIGHT;
    int bAlignCenter = alignFlags & RDFONT_ALIGNCENTER;

    if ( pFont && bAlignCenter ^ bAlignRight ^ alignFlags & RDFONT_ALIGNLEFT )
    {
        size_t textLen = strlen(pText);

        uint32_t width, height;
        stdDisplay_GetBackBufferSize(&width, &height);

        double swidth  = (double)width / RD_REF_WIDTH;
        double sheight = (double)height / RD_REF_HEIGHT;

        float fontScale = fontSizePt / (float)pFont->fontSize; // Added

        double curPosX = (double)width * x;
        double curPosY = (double)height * y;
        curPosY += (double)pFont->fontSize * sheight * fontScale; // Altered: Added multiply by fontScale

        if ( bAlignCenter || bAlignRight )
        {
            double textWidth = 0.0f;
            for ( size_t i = 0; i < textLen; ++i )
            {
                size_t curChar = (uint8_t)pText[i]; // Note, has to cast to uint8_t first for the case of negative char to remain within range [0,255]
                if ( pFont->aGlyphs[curChar].baselineOriginY == -pFont->fontSize )
                {
                    curChar = '?';
                }

                textWidth += (double)pFont->aGlyphs[curChar].baselineOriginX * swidth * fontScale; // Altered: Added multiply by fontScale
            }

            double offsetX = textWidth;
            if ( bAlignCenter && !bAlignRight )
            {
                offsetX /= 2.0f;
            }

            curPosX = curPosX - (double)(int)offsetX;
        }

        for ( size_t i = 0; i < textLen; ++i )
        {
            size_t curChar = (uint8_t)pText[i]; // Note, has to cast to uint8_t first for the case of negative char to remain within range [0,255]
            if ( curChar == 160 ) // 160 - no-break space
            {
                curChar = ' ';
            }
            else if ( pFont->aGlyphs[curChar].baselineOriginY == -pFont->fontSize )
            {
                curChar = '?';
            }

            if ( pFont->aGlyphs[curChar].baselineOriginY != -1 )
            {
                double nextPosX = (double)pFont->aGlyphs[curChar].baselineOriginX * swidth * fontScale + curPosX; // Altered: Added multiply by fontScale
                if ( nextPosX >= 0.0f )
                {
                    if ( RD_REF_WIDTH * swidth >= nextPosX && curPosX >= 0.0f && RD_REF_WIDTH * swidth >= curPosX )
                    {
                        rdFont_DrawCharScaled(curChar, (float)curPosX, (float)curPosY, z, pFont, fontSizePt);
                    }
                }

                curPosX += (double)pFont->aGlyphs[curChar].baselineOriginX * swidth * fontScale; // Altered: Added multiply by fontScale
            }
        }
    }
}

void J3DAPI rdFont_DrawTextLine(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags)
{
    rdFont_DrawTextLineEx(pText, x, y, z, pFont, alignFlags, (float)pFont->fontSize);
}

void J3DAPI rdFont_DrawChar(size_t chr, float x, float y, float z, const rdFont* pFont)
{
    rdFont_DrawCharScaled(chr, x, y, z, pFont, pFont->fontSize);
}

void J3DAPI rdFont_DrawTextLineClippedEx(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags, float fontSizePt)
{
    int bAlignRight = alignFlags & RDFONT_ALIGNRIGHT;
    int bAlignCenter = alignFlags & RDFONT_ALIGNCENTER;
    if ( pFont && bAlignCenter ^ bAlignRight ^ alignFlags & RDFONT_ALIGNLEFT )
    {
        uint32_t width, height;
        stdDisplay_GetBackBufferSize(&width, &height);
        double swidth = (double)width / RD_REF_WIDTH;
        double sheight = (double)height / RD_REF_HEIGHT;

        float fontScale = fontSizePt / (float)pFont->fontSize; // Added

        double curPosX = (double)width * x;
        double curPosY = (double)height * y;
        curPosY += (double)pFont->fontSize * sheight * fontScale; // Altered: Added multiply by fontScale

        size_t textLen = strlen(pText);
        if ( bAlignCenter || bAlignRight )
        {
            double textWidth = 0.0f;
            for ( size_t i = 0; i < textLen; ++i )
            {
                size_t curChar = (uint8_t)pText[i]; // Note, has to cast to uint8_t first for the case of negative char to remain within range [0,255]
                if ( pFont->aGlyphs[curChar].baselineOriginY == -pFont->fontSize )
                {
                    curChar = '?';
                }
                textWidth += (double)pFont->aGlyphs[curChar].baselineOriginX * swidth * fontScale; // Altered: Added multiply by fontScale
            }
            double offsetX = textWidth;
            if ( bAlignCenter && !bAlignRight )
            {
                offsetX /= 2.0f;
            }

            curPosX = curPosX - (double)(int)offsetX;
        }

        for ( size_t i = 0; i < textLen; ++i )
        {
            size_t curChar = (uint8_t)pText[i]; // Note, has to cast to uint8_t first for the case of negative char to remain within range [0,255]
            if ( pFont->aGlyphs[curChar].baselineOriginY == -pFont->fontSize )
            {
                curChar = '?';
            }
            else
            {
                double nextPosX = (double)pFont->aGlyphs[curChar].baselineOriginX * swidth * fontScale + curPosX; // Altered: Added multiply by fontScale
                if ( nextPosX >= 0.0f )
                {
                    if ( RD_REF_WIDTH * swidth >= nextPosX && curPosX >= 0.0f && RD_REF_WIDTH * swidth >= curPosX
                        && curPosY >= 0 && RD_REF_HEIGHT * sheight >= curPosY )
                    {
                        rdFont_DrawCharScaled(curChar, (float)curPosX, (float)curPosY, z, pFont, fontSizePt);
                    }
                }
            }
            curPosX += (double)pFont->aGlyphs[curChar].baselineOriginX * swidth * fontScale; // Altered: Added multiply by fontScale
        }
    }
}

void J3DAPI rdFont_DrawTextLineClipped(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags)
{
    rdFont_DrawTextLineClippedEx(pText, x, y, z, pFont, alignFlags, (float)pFont->fontSize);
}

void J3DAPI rdFont_DrawCharScaled(size_t chr, float x, float y, float z, const rdFont* pFont, float fontSizePt)
{
    // This function is part of the OG rdFont_DrawChar with added font size scale

    // Added: Calculate scale factor based on desired pt size vs atlas pt size
    float fontScale = fontSizePt / (float)pFont->fontSize;

    float gwidth = pFont->aGlyphs[chr].right - pFont->aGlyphs[chr].left;
    float gheight = pFont->aGlyphs[chr].bottom - pFont->aGlyphs[chr].top;

    rdCacheProcEntry* pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        return;
    }

    uint32_t width, height;
    stdDisplay_GetBackBufferSize(&width, &height);
    float swidth  = (float)width / RD_REF_WIDTH;
    float sheight = (float)height / RD_REF_HEIGHT;

    // Altered: font size
    float scaledWidth  = gwidth * 256.0f * swidth * fontScale;
    float scaledHeight = gheight * 256.0f * sheight * fontScale;

    // Apply font scale
    y = y - (float)pFont->aGlyphs[chr].baselineOriginY * sheight * fontScale;

    pPoly->flags        = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    pPoly->lightingMode = rdFont_lightingMode;
    pPoly->numVertices  = 4;
    pPoly->pMaterial    = pFont->pMaterial;
    pPoly->matCelNum    = 0;

    LPD3DTLVERTEX pCurVert = pPoly->aVertices;
    pCurVert->sx  = x;
    pCurVert->sy  = y;
    pCurVert->sz  = z;
    pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X1;
    pCurVert->tu  = pFont->aGlyphs[chr].left;
    pCurVert->tv  = pFont->aGlyphs[chr].top;
    ++pCurVert;

    pCurVert->sx  = scaledWidth + x;
    pCurVert->sy  = y;
    pCurVert->sz  = z;
    pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X1;
    pCurVert->tu  = pFont->aGlyphs[chr].right;
    pCurVert->tv  = pFont->aGlyphs[chr].top;
    ++pCurVert;

    pCurVert->sx  = scaledWidth + x;
    pCurVert->sy  = scaledHeight + y;
    pCurVert->sz  = z;
    pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X1;
    pCurVert->tu  = pFont->aGlyphs[chr].right;
    pCurVert->tv  = pFont->aGlyphs[chr].bottom;
    ++pCurVert;

    pCurVert->sx  = x;
    pCurVert->sy  = scaledHeight + y;
    pCurVert->sz  = z;
    pCurVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X1;
    pCurVert->tu  = pFont->aGlyphs[chr].left;
    pCurVert->tv  = pFont->aGlyphs[chr].bottom;

    if ( rdFont_bFontColorSet )
    {
        if ( rdFont_lightingMode == RD_LIGHTING_DIFFUSE )
        {
            pPoly->extraLight = rdFont_aFontColors[0];
        }
        else
        {
            rdVector_Set4(&pPoly->extraLight, 0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    for ( size_t i = 0; i < pPoly->numVertices; ++i )
    {
        if ( rdFont_bFontColorSet )
        {
            if ( rdFont_lightingMode == RD_LIGHTING_GOURAUD )
            {
                pPoly->aVertIntensities[i] = rdFont_aFontColors[i];
            }
            else
            {
                rdVector_Set4(&pPoly->aVertIntensities[i], 0.0f, 0.0f, 0.0f, 1.0f);
            }
        }
    }

    rdCache_AddAlphaProcFace(pPoly->numVertices);
}
