// Added: Null version of std/Win95/DX9/std3DX9.c (the Direct3D9 renderer). Draw calls,
// render-state, fog and Z-buffer services are no-ops; the device list reports one 3D
// device and the texture-format table holds the six RGB/RGBA software formats the DX9
// backend checks. System textures keep the exact DX9 ownership model: mipmap VBuffers are
// created and copied through stdDisplay_VBufferNew/Lock, and the texture cache stays an
// empty list because nothing is ever uploaded to a device.
#include <math.h>

#include <j3dcore/j3d.h>

#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include <std/General/std.h>
#include <std/General/stdColor.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#define STD3D_MAXTEXTURESIZE 4096u

// Fixed: E_INVALIDARG carries no Windows header on the Null platform; the constant is the
// exact value std3DX9.c's SetProjection returns on bad arguments.
#ifndef E_INVALIDARG
#define E_INVALIDARG ((int)0x80070057L)
#endif

float std3D_g_fogDensity  = 0.0f;
size_t std3D_g_maxVertices = 0u;

static bool std3D_bStartup           = false;
static bool std3D_bOpen              = false;
static bool std3D_bFindAllD3Devices  = false;
static bool std3D_bHasRGBTextureFormat = false;
static bool std3D_bRenderFog         = true;

static size_t std3D_numDevices     = 0;
static size_t std3D_curDevice      = 0;
static Device3D std3D_aDevices[16];

static size_t std3D_numTextureFormats = 0;
static StdTextureFormat std3D_aTextureFormats[16];

static size_t std3D_frameCount    = 1;
static Std3DMipmapFilterType std3D_mipmapFilter = -1;

static void std3D_InitTextureFormats(void);
static int std3D_BuildDeviceList(void);

void std3D_ResetGlobals(void)
{
    float fogDensity = 1.0f;
    memcpy(&std3D_g_fogDensity, &fogDensity, sizeof(std3D_g_fogDensity));
    STD_ZEROMEM(&std3D_g_maxVertices, sizeof(std3D_g_maxVertices));
}

// Added: The DX9 texture-format set of InitTextureFormats: six software RGB/RGBA formats.
static void std3D_InitTextureFormats(void)
{
    std3D_numTextureFormats = 0;
    std3D_bHasRGBTextureFormat = false;

    static const struct
    {
        ColorInfo colorInfo;
        const char* pName;
    } formats[] =
    {
        { stdColor_cfRGB888,   "24-bit RGB" },
        { stdColor_cfRGB8888,  "32-bit RGB" },
        { stdColor_cfARGB8888, "32-bit ARGB" },
        { stdColor_cfRGB565,   "16-bit RGB" },
        { stdColor_cfARGB5551, "16-bit RGBA5551" },
        { stdColor_cfARGB4444, "16-bit ARGB4444" }
    };

    for ( size_t i = 0; i < STD_ARRAYLEN(formats); ++i )
    {
        StdTextureFormat* pFormat = &std3D_aTextureFormats[std3D_numTextureFormats];
        STD_ZEROMEM(pFormat, sizeof(*pFormat));
        pFormat->ci = formats[i].colorInfo;
        pFormat->bColorKey = 0;
        pFormat->pColorKey = NULL;

        if ( formats[i].colorInfo.colorMode == STDCOLOR_RGB )
        {
            std3D_bHasRGBTextureFormat = true;
        }

        STDLOG_STATUS("Found texture format: %s (%u:%u:%u:%u)\n",
            formats[i].pName,
            pFormat->ci.redBPP, pFormat->ci.greenBPP, pFormat->ci.blueBPP, pFormat->ci.alphaBPP);
        ++std3D_numTextureFormats;
    }
}

int std3D_Startup(void)
{
    if ( std3D_bStartup )
    {
        return 1;
    }

    STD_ZEROMEM(std3D_aTextureFormats, sizeof(std3D_aTextureFormats));
    STD_ZEROMEM(std3D_aDevices, sizeof(std3D_aDevices));

    if ( !std3D_BuildDeviceList() )
    {
        STDLOG_ERROR("Error building device list.\n");
        return 0;
    }

    if ( std3D_numDevices == 0 )
    {
        return 0;
    }

    std3D_bStartup = true;
    return 1;
}

void std3D_Shutdown(void)
{
    if ( !std3D_bStartup )
    {
        STDLOG_WARNING("Warning: System already shutdown!\n");
        return;
    }

    if ( std3D_bOpen )
    {
        std3D_Close();
    }

    STD_ZEROMEM(std3D_aTextureFormats, sizeof(std3D_aTextureFormats));
    STD_ZEROMEM(std3D_aDevices, sizeof(std3D_aDevices));

    std3D_numDevices = 0;
    std3D_bStartup   = false;
}

const Device3D* std3D_GetCurrentDevice(void)
{
    return std3D_aDevices[0].maxVertexCount ? &std3D_aDevices[std3D_curDevice] : NULL;
}

size_t std3D_GetNumDevices(void)
{
    return std3D_numDevices;
}

const Device3D* std3D_GetAllDevices(void)
{
    return std3D_aDevices;
}

void J3DAPI std3D_SetFindAllDevices(int bFindAll)
{
    std3D_bFindAllD3Devices = bFindAll != 0;
}

// Added: The DX9 BuildDeviceList minus Direct3D: all capability answers come from the
// software feature set, and the memory numbers mirror the caps-derived DX9 accounting.
static int std3D_BuildDeviceList(void)
{
    std3D_numDevices = 0;

    StdDisplayDevice displayDevice = { 0 };
    if ( !stdDisplay_IsOpen() || stdDisplay_GetCurrentDevice(&displayDevice) )
    {
        return 0;
    }

    if ( !std3D_bFindAllD3Devices && !displayDevice.bHAL )
    {
        return 0;
    }

    Device3D* pD3DDriver = &std3D_aDevices[0];
    STD_ZEROMEM(pD3DDriver, sizeof(*pD3DDriver));

    stdUtil_StringCopy(pD3DDriver->deviceDescription, STD_ARRAYLEN(pD3DDriver->deviceDescription), displayDevice.aDeviceName);
    stdUtil_StringCopy(pD3DDriver->deviceName, STD_ARRAYLEN(pD3DDriver->deviceName), displayDevice.aDriverName);

    pD3DDriver->bHAL                         = displayDevice.bHAL;
    pD3DDriver->bTexturePerspectiveSupported = 1;
    pD3DDriver->hasZBuffer                   = 1;
    pD3DDriver->bColorkeyTextureSupported    = 1;
    pD3DDriver->bAlphaTextureSupported       = 1;
    pD3DDriver->bStippledShadeSupported      = 0;
    pD3DDriver->bAlphaBlendSupported         = 1;
    pD3DDriver->bSqareOnlyTexture            = 0;
    pD3DDriver->minTexWidth                  = 1;
    pD3DDriver->minTexHeight                 = 1;
    pD3DDriver->maxTexWidth                  = STD3D_MAXTEXTURESIZE;
    pD3DDriver->maxTexHeight                 = STD3D_MAXTEXTURESIZE;
    pD3DDriver->bAnisotropicFilteringSupported = false;
    pD3DDriver->bMipmapAutoGenSupported        = false;
    pD3DDriver->bMSAASupported                 = false;

    pD3DDriver->maxVertexCount = 65535u;

    pD3DDriver->totalMemory     = displayDevice.totalVideoMemory;
    pD3DDriver->availableMemory = displayDevice.freeVideoMemory;

    std3D_numDevices = 1u;

    STDLOG_STATUS("Found |%s|%s|%s|%s| Null D3D Device\n",
        pD3DDriver->hasZBuffer ? "Z" : "Non-Z",
        pD3DDriver->bAlphaTextureSupported ? "Alpha" : "No Alpha",
        pD3DDriver->bStippledShadeSupported ? "Stippled" : "Blend",
        pD3DDriver->bColorkeyTextureSupported ? "Colorkey" : "No Colorkey"
    );
    STDLOG_STATUS("Description: %s [%s]\n", pD3DDriver->deviceName, pD3DDriver->deviceDescription);

    return 1;
}

int J3DAPI std3D_Open(size_t deviceNum)
{
    STD_ASSERTREL(std3D_bStartup);
    if ( std3D_bOpen )
    {
        STDLOG_ERROR("Warning: System already open!\n");
        return 0;
    }

    // Added: Same display-open requirement as the DX9 std3D_Open.
    if ( !stdDisplay_IsOpen() )
    {
        STDLOG_ERROR("std3D_Open: Display system is not opened!\n");
        return 0;
    }

    if ( deviceNum >= std3D_numDevices )
    {
        return 0;
    }

    std3D_curDevice = deviceNum;

    // Initialize texture formats (the InitSystem part of the DX9 std3D_Open).
    std3D_InitTextureFormats();
    if ( !std3D_numTextureFormats || !std3D_bHasRGBTextureFormat )
    {
        STDLOG_ERROR("Error no texture formats found.\n");
        return 0;
    }

    std3D_g_maxVertices = std3D_aDevices[deviceNum].maxVertexCount;
    if ( std3D_g_maxVertices == 0 )
    {
        std3D_g_maxVertices = STD3D_MAXVERTICES;
    }
    STDLOG_STATUS("Max vertices: %zu.\n", std3D_g_maxVertices);

    std3D_bOpen = true;
    return 1;
}

void std3D_Close(void)
{
    if ( !std3D_bOpen )
    {
        STDLOG_WARNING("Warning: System already closed!\n");
        return;
    }

    std3D_mipmapFilter           = -1;
    std3D_numTextureFormats      = 0;
    std3D_curDevice              = 0;
    std3D_bHasRGBTextureFormat   = false;
    std3D_bOpen                  = false;
}

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, LPDDCOLORKEY* ppColorKey)
{
    size_t formatIndex;
    if ( type == STDCOLOR_FORMAT_RGBA_1BITALPHA )
    {
        formatIndex = 4; // ARGB5551
    }
    else if ( type == STDCOLOR_FORMAT_RGBA )
    {
        formatIndex = 2; // ARGB8888
    }
    else
    {
        formatIndex = 1; // RGB8888
    }

    STD_ASSERTREL(formatIndex < std3D_numTextureFormats);

    // Added: No color key is ever supported on the Null platform.
    *pbColorKeySet = std3D_aTextureFormats[formatIndex].bColorKey;
    *ppColorKey    = std3D_aTextureFormats[formatIndex].pColorKey;
    *pDest         = std3D_aTextureFormats[formatIndex].ci;
}

StdColorFormatType J3DAPI std3D_GetColorFormat(const ColorInfo* pCi)
{
    if ( pCi->alphaBPP == 0 )
    {
        return STDCOLOR_FORMAT_RGB;
    }

    if ( pCi->alphaBPP == 1 )
    {
        return STDCOLOR_FORMAT_RGBA_1BITALPHA;
    }

    return STDCOLOR_FORMAT_RGBA;
}

size_t std3D_GetNumTextureFormats(void)
{
    return std3D_numTextureFormats;
}

int std3D_StartScene(void)
{
    ++std3D_frameCount;
    return 0;
}

void std3D_EndScene(void)
{
}

void J3DAPI std3D_DrawRenderList(tSysTexture* pTex, Std3DRenderState rdflags, StdVertexTL* aVerts, size_t numVerts, uint16_t* aIndices, size_t numIndices)
{
    J3D_UNUSED(pTex);
    J3D_UNUSED(rdflags);
    J3D_UNUSED(aVerts);
    J3D_UNUSED(numVerts);
    J3D_UNUSED(aIndices);
    J3D_UNUSED(numIndices);
}

void J3DAPI std3D_DrawLineStrip(StdVertexTL* aVerts, size_t numVerts)
{
    J3D_UNUSED(aVerts);
    J3D_UNUSED(numVerts);
}

void J3DAPI std3D_DrawPointList(StdVertexTL* aVerts, size_t numVerts)
{
    J3D_UNUSED(aVerts);
    J3D_UNUSED(numVerts);
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    J3D_UNUSED(rdflags);
}

void std3D_SetWireframeRenderState(void)
{
}

void J3DAPI std3D_GetValidDimensions(uint32_t width, uint32_t height, uint32_t* pOutWidth, uint32_t* pOutHeight)
{
    uint32_t texWidth  = STDMATH_CLAMP(width, 1u, STD3D_MAXTEXTURESIZE);
    uint32_t texHeight = STDMATH_CLAMP(height, 1u, STD3D_MAXTEXTURESIZE);

    // Added: The one Null device never requires square textures.
    *pOutWidth  = texWidth;
    *pOutHeight = texHeight;
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType)
{
    STD_ZEROMEM(pTexture, sizeof(tSystemTexture));

    if ( !std3D_numTextureFormats )
    {
        return;
    }

    // Get mipmap buffer at LOD 0
    tVBuffer* pVBuffer = *apVBuffers;
    uint32_t texHeight = 0, texWidth = 0;
    std3D_GetValidDimensions(pVBuffer->rasterInfo.width, pVBuffer->rasterInfo.height, &texWidth, &texHeight);
    while ( numMipLevels > 1 && (pVBuffer->rasterInfo.width > texWidth || pVBuffer->rasterInfo.height > texHeight) )
    {
        --numMipLevels;
        pVBuffer = *++apVBuffers;
        std3D_GetValidDimensions(pVBuffer->rasterInfo.width, pVBuffer->rasterInfo.height, &texWidth, &texHeight);
    }

    size_t texSize = (pVBuffer->rasterInfo.colorInfo.bpp * texHeight * texWidth) / 8;
    if ( std3D_mipmapFilter == STD3D_MIPMAPFILTER_NONE )
    {
        numMipLevels = 1;
    }

    pTexture->apMipmaps = (tVBuffer**)STDMALLOC(numMipLevels * sizeof(tVBuffer*));
    if ( !pTexture->apMipmaps )
    {
        STDLOG_ERROR("Failed to allocate memory for VBuffer array.\n");
        goto error;
    }

    pTexture->numMipLevels = numMipLevels;
    pTexture->textureSize  = texSize;

    // Create owned VBuffer copies for each mip level, as the DX9 AllocSystemTexture does.
    for ( size_t mmNum = 0; mmNum < numMipLevels; ++mmNum )
    {
        pTexture->apMipmaps[mmNum] = stdDisplay_VBufferNew(&apVBuffers[mmNum]->rasterInfo, /*bUseVSurface=*/0, /*bUseVideoMemory=*/0);
        if ( !pTexture->apMipmaps[mmNum] )
        {
            STDLOG_ERROR("Failed to create VBuffer for mip level %zu.\n", mmNum);
            goto error;
        }

        tColorMode colorMode = apVBuffers[mmNum]->rasterInfo.colorInfo.colorMode;
        if ( colorMode )
        {
            if ( colorMode > STDCOLOR_PAL && colorMode <= STDCOLOR_RGBA )
            {
                stdDisplay_VBufferLock(apVBuffers[mmNum]);
                stdDisplay_VBufferLock(pTexture->apMipmaps[mmNum]);

                size_t pixelDataSize = apVBuffers[mmNum]->rasterInfo.height * apVBuffers[mmNum]->rasterInfo.rowSize;
                memcpy(pTexture->apMipmaps[mmNum]->pPixels, apVBuffers[mmNum]->pPixels, pixelDataSize);

                stdDisplay_VBufferUnlock(pTexture->apMipmaps[mmNum]);
                stdDisplay_VBufferUnlock(apVBuffers[mmNum]);
            }
        }
        else
        {
            STDLOG_ERROR("Can't use paletized textures.\n");
            goto error;
        }
    }

    return;

error:
    if ( pTexture->apMipmaps )
    {
        stdMemory_Free(pTexture->apMipmaps);
    }
    STD_ZEROMEM(pTexture, sizeof(tSystemTexture));

    STDLOG_ERROR("Done error exit from std3D_AllocSystemTexture.\n");
    return;
}

void J3DAPI std3D_ClearSystemTexture(tSystemTexture* pTex)
{
    while ( pTex->numMipLevels > 0 )
    {
        stdDisplay_VBufferFree(pTex->apMipmaps[--pTex->numMipLevels]);
    }

    if ( pTex->apMipmaps )
    {
        stdMemory_Free(pTex->apMipmaps);
        pTex->apMipmaps = NULL;
    }

    STD_ZEROMEM(pTex, sizeof(tSystemTexture));
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format)
{
    J3D_UNUSED(format);

    STD_ASSERTREL(pCacheTexture);

    if ( pCacheTexture->numMipLevels == 0 || !pCacheTexture->apMipmaps )
    {
        STDLOG_ERROR("No Source texture.\n");
        return;
    }

    // Added: The DX9 AddToTextureCache uploads the mipmap pixels into a Direct3D managed
    // texture and links the system texture into the cache list. Nothing is uploaded on
    // the Null platform, so the mip VBuffers stay exactly where AllocSystemTexture left
    // them and no cache entry exists (pCachedTexture stays NULL, as the DX9 failure path).
}

void std3D_ResetTextureCache(void)
{
    // Added: Nothing is ever cached on the Null platform; only the frame counter resets,
    // like the tail of the DX9 cache reset.
    std3D_frameCount = 1;
}

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture)
{
    if ( !pTexture )
    {
        return 0;
    }

    return pTexture->numMipLevels;
}

void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture)
{
    // Added: The DX9 version relinks the texture in the cache list; with an empty cache
    // only the frame stamp is updated.
    pTexture->frameNum = std3D_frameCount;
}

size_t J3DAPI std3D_FindClosestFormat(const ColorInfo* pMatch)
{
    if ( !std3D_numTextureFormats )
    {
        return 0;
    }

    size_t bestMatchLevel = 0;
    size_t closestMatch = 0;
    for ( size_t i = 0; i < std3D_numTextureFormats; ++i )
    {
        StdTextureFormat* pFormat = &std3D_aTextureFormats[i];
        size_t matchLevel = 0;

        if ( pFormat->ci.colorMode == pMatch->colorMode )
        {
            matchLevel = 1;
            if ( pFormat->ci.bpp == pMatch->bpp )
            {
                matchLevel = 2;
                if ( pMatch->colorMode == STDCOLOR_RGB )
                {
                    if ( pFormat->ci.redBPP == pMatch->redBPP && pFormat->ci.greenBPP == pMatch->greenBPP && pFormat->ci.blueBPP == pMatch->blueBPP )
                    {
                        return i;
                    }
                }
                else
                {
                    if ( pMatch->colorMode != STDCOLOR_RGBA )
                    {
                        return i;
                    }

                    if ( pFormat->ci.colorMode == STDCOLOR_RGBA )
                    {
                        matchLevel = 3;
                    }

                    if ( pMatch->alphaBPP == 1 )
                    {
                        if ( pFormat->ci.alphaBPP == 1 )
                        {
                            return i;
                        }
                    }
                    else
                    {
                        if ( pFormat->ci.alphaBPP != 1 )
                        {
                            return i;
                        }
                    }
                }
            }
        }

        if ( matchLevel > bestMatchLevel )
        {
            bestMatchLevel = matchLevel;
            closestMatch   = i;
        }
    }

    return closestMatch;
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType mode)
{
    // Added: DX9 SetMipmapFilter returns the (already applied) HRESULT as int, 0 is
    // success; the Null backend always applies.
    std3D_mipmapFilter = mode;
    return 0;
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane)
{
    // Added: The DX9 validation, without the D3D transform store (nothing consumes the
    // matrix on the Null platform).
    float distance = farPlane - nearPlane;
    if ( fabs(distance) < 0.009999999776482582 )
    {
        return E_INVALIDARG;
    }

    float hfov = fov / 2.0f;
    float sinhfov = sinf(hfov);
    if ( fabs(sinhfov) < 0.009999999776482582f )
    {
        return E_INVALIDARG;
    }

    return 0;
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    // Added: Fog state is stored without device validation; nothing consumes it on the
    // Null platform, but the global stays the same observable the engine reads.
    std3D_bRenderFog   = bEnabled != 0;
    std3D_g_fogDensity = density;
}

void J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    J3D_UNUSED(red);
    J3D_UNUSED(green);
    J3D_UNUSED(blue);
    J3D_UNUSED(startDepth);
    J3D_UNUSED(endDepth);
}

void std3D_ClearZBuffer(void)
{
}

StdDisplayEnvironment* J3DAPI std3D_BuildDisplayEnvironment()
{
    StdDisplayEnvironment* pDeviceList = (StdDisplayEnvironment*)STDMALLOC(sizeof(StdDisplayEnvironment));
    STD_ZEROMEM(pDeviceList, sizeof(StdDisplayEnvironment));

    if ( !stdDisplay_Startup() )
    {
        STDLOG_ERROR("Error starting stdDisplay system.\n");
        std3D_FreeDisplayEnvironment(pDeviceList);
        return NULL;
    }

    pDeviceList->numInfos      = stdDisplay_GetNumDevices();
    pDeviceList->aDisplayInfos = NULL;

    if ( pDeviceList->numInfos )
    {
        pDeviceList->aDisplayInfos = (StdDisplayInfo*)STDMALLOC(sizeof(StdDisplayInfo) * pDeviceList->numInfos);

        StdDisplayInfo* pCurInfo = pDeviceList->aDisplayInfos;
        for ( size_t deviceNum = 0; deviceNum < pDeviceList->numInfos; ++deviceNum )
        {
            STD_ZEROMEM(pCurInfo, sizeof(StdDisplayInfo));

            if ( stdDisplay_GetDevice(deviceNum, &pCurInfo->displayDevice) )
            {
                STDLOG_ERROR("Error getting stdDisplay device.\n");
                std3D_FreeDisplayEnvironment(pDeviceList);
                return NULL;
            }

            if ( !stdDisplay_Open(deviceNum) )
            {
                STDLOG_ERROR("Error opening stdDisplay device.\n");
                std3D_FreeDisplayEnvironment(pDeviceList);
                return NULL;
            }

            pCurInfo->numModes = stdDisplay_GetNumVideoModes();
            pCurInfo->aModes   = NULL;

            if ( pCurInfo->numModes )
            {
                pCurInfo->aModes = (StdVideoMode*)STDMALLOC(sizeof(StdVideoMode) * pCurInfo->numModes);
                StdVideoMode* pCurMode = pCurInfo->aModes;
                for ( size_t modeNum = 0; modeNum < pCurInfo->numModes; ++modeNum )
                {
                    if ( !stdDisplay_GetVideoMode(modeNum, pCurMode) )
                    {
                        ++pCurMode;
                    }
                }

                pCurInfo->numDevices = 0;
                pCurInfo->aDevices   = NULL;
                if ( pCurInfo->displayDevice.bHAL )
                {
                    if ( !std3D_Startup() )
                    {
                        STDLOG_ERROR("Error starting std3D system.\n");
                        std3D_FreeDisplayEnvironment(pDeviceList);
                        return NULL;
                    }

                    pCurInfo->numDevices = std3D_GetNumDevices();
                    if ( pCurInfo->numDevices )
                    {
                        size_t listSize = sizeof(Device3D) * pCurInfo->numDevices;
                        pCurInfo->aDevices = (Device3D*)STDMALLOC(listSize);
                        memcpy(pCurInfo->aDevices, std3D_GetAllDevices(), listSize);
                    }

                    std3D_Shutdown();
                }
            }

            stdDisplay_Close();
            ++pCurInfo;
        }
    }

    stdDisplay_Shutdown();
    return pDeviceList;
}

void J3DAPI std3D_FreeDisplayEnvironment(StdDisplayEnvironment* pEnv)
{
    if ( pEnv->aDisplayInfos )
    {
        StdDisplayInfo* pCurDevice = pEnv->aDisplayInfos;
        for ( size_t i = 0; i < pEnv->numInfos; ++i )
        {
            if ( pCurDevice->aDevices )
            {
                stdMemory_Free(pCurDevice->aDevices);
                pCurDevice->aDevices = NULL;
            }

            if ( pCurDevice->aModes )
            {
                stdMemory_Free(pCurDevice->aModes);
                pCurDevice->aModes = NULL;
            }

            ++pCurDevice;
        }

        stdMemory_Free(pEnv->aDisplayInfos);
        pEnv->aDisplayInfos = NULL;
    }

    stdMemory_Free(pEnv);
}
