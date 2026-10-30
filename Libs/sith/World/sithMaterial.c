#include "sithMaterial.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Engine/rdMaterial.h>

#include <sith/RTI/symbols.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdConfig.h>
#include <std/General/stdFnames.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#define SITHMATERIAL_TABLESIZE                1024u
#define SITHMATERIAL_EXTRABUFFERSIZE          64u

static bool sithMaterial_bMaterialStartup  = false; // Added
static tHashTable* sithMaterial_pHashtable = NULL;  // Fixed: Init to 0

static size_t sithMaterial_staticBufferExtraCapacity = 32; // Added

static bool sithMaterial_bCndLoadExternal = J3D_QOL_VALUE(true, false); // Added


rdMaterial* J3DAPI sithMaterial_CacheFind(const char* pName);
void J3DAPI sithMaterial_CacheAdd(rdMaterial* pMat);
int J3DAPI sithMaterial_CacheRemove(const rdMaterial* pMat);

static inline size_t sithMaterial_GetTextureByteSize(size_t width, size_t height, size_t bpp, size_t numMipLevels);
static inline size_t sithMaterial_GetTotalPixelDataSize(size_t width, size_t height, size_t bpp, size_t numMipLevels, size_t numCells);
static inline size_t sithMaterial_GetCndMaterialPixelDataSize(const CndMaterialInfo* pInfo);
static bool sithMaterial_FileExists(const char* pFilename);

void sithMaterial_InstallHooks(void)
{
    J3D_HOOKFUNC(sithMaterial_Startup);
    J3D_HOOKFUNC(sithMaterial_Shutdown);
    J3D_HOOKFUNC(sithMaterial_FreeWorldMaterials);
    J3D_HOOKFUNC(sithMaterial_ReadMaterialsListText);
    J3D_HOOKFUNC(sithMaterial_WriteMaterialsListBinary);
    J3D_HOOKFUNC(sithMaterial_ReadMaterialsListBinary);
    J3D_HOOKFUNC(sithMaterial_Load);
    J3D_HOOKFUNC(sithMaterial_GetMaterialByIndex);
    J3D_HOOKFUNC(sithMaterial_AllocWorldMaterials);
    J3D_HOOKFUNC(sithMaterial_CacheFind);
    J3D_HOOKFUNC(sithMaterial_CacheAdd);
    J3D_HOOKFUNC(sithMaterial_CacheRemove);
}

void sithMaterial_ResetGlobals(void)
{}

int sithMaterial_Startup(void)
{
    // Added
    if ( sithMaterial_bMaterialStartup )
    {
        SITHLOG_ERROR("Multiple startup on material system.\n");
        return 1;
    }

    sithMaterial_pHashtable = stdHashtbl_New(SITHMATERIAL_TABLESIZE);
    if ( !sithMaterial_pHashtable ) // Added
    {
        SITHLOG_ERROR("Could not allocate material hashtable.\n");
        return 1;
    }

    // Added: Load config values
    sithMaterial_staticBufferExtraCapacity = stdConfig_GetInt(SITHMATERIAL_CFG_STATICWORLDMATERIALS_EXTRACAPACITY, sithMaterial_staticBufferExtraCapacity);
    if ( !stdConfig_Contains(SITHMATERIAL_CFG_STATICWORLDMATERIALS_EXTRACAPACITY) )
    {
        stdConfig_SetInt(SITHMATERIAL_CFG_STATICWORLDMATERIALS_EXTRACAPACITY, sithMaterial_staticBufferExtraCapacity);
    }

    sithMaterial_bCndLoadExternal = stdConfig_GetBool(SITHMATERIAL_CFG_CNDWORLDMATERIALS_LOADEXTERNAL, sithMaterial_bCndLoadExternal);
    if ( !stdConfig_Contains(SITHMATERIAL_CFG_CNDWORLDMATERIALS_LOADEXTERNAL) )
    {
        stdConfig_SetBool(SITHMATERIAL_CFG_CNDWORLDMATERIALS_LOADEXTERNAL, sithMaterial_bCndLoadExternal);
    }

    sithMaterial_bMaterialStartup = true;
    return 0;
}

void sithMaterial_Shutdown(void)
{
    // Added
    if ( !sithMaterial_bMaterialStartup )
    {
        SITHLOG_ERROR("Material system not started.\n");
        return;
    }

    if ( sithMaterial_pHashtable )
    {
        stdHashtbl_Free(sithMaterial_pHashtable);
        sithMaterial_pHashtable = NULL;
    }

    sithMaterial_bMaterialStartup = false;
}

void J3DAPI sithMaterial_FreeWorldMaterials(SithWorld* pWorld)
{
    SITH_ASSERTREL(pWorld != NULL);

    if ( !pWorld->sizeMaterials )
    {
        SITH_ASSERTREL(pWorld->aMaterials == NULL);
        return;
    }

    for ( size_t i = 0; i < pWorld->numMaterials; ++i )
    {
        sithMaterial_CacheRemove(&pWorld->aMaterials[i]);
        rdMaterial_FreeEntry(&pWorld->aMaterials[i]);
    }

    if ( pWorld->aMaterials )
    {
        STDFREE(pWorld->aMaterials);
    }

    pWorld->aMaterials   = NULL;
    pWorld->numMaterials = 0;

    if ( pWorld->apMatArray )
    {
        STDFREE(pWorld->apMatArray);
    }

    pWorld->apMatArray = NULL;
}

int J3DAPI sithMaterial_WriteMaterialsListText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("##### Material information #####\n")
        || stdConffile_WriteLine("SECTION: MATERIALS\n\n")
        || stdConffile_Printf("World materials %d\n\n", pWorld->numMaterials + SITHMATERIAL_EXTRABUFFERSIZE)
        || stdConffile_WriteLine("#num:\tmat:\n") )
    {
        return 1;
    }

    for ( size_t i = 0; i < pWorld->numMaterials; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aMaterials[i].aName) )
        {
            return 1;
        }
    }

    return stdConffile_WriteLine("end\n") || stdConffile_WriteLine("################################\n\n\n");
}

int J3DAPI sithMaterial_ReadMaterialsListText(SithWorld* pWorld, int bSkip)
{
    SITH_ASSERTREL(pWorld != NULL);
    if ( bSkip )
    {
        return 1;
    }

    int nRead = 0;
    size_t  numMaterials;
    if ( nRead = stdConffile_ScanLine(" world materials %d", &numMaterials), nRead != 1 )
    {
        if ( nRead < 0 )
        {
            SITHLOG_ERROR("Read error in materials section, line %d.\n", stdConffile_GetLineNumber());
        }
        else
        {
            SITHLOG_ERROR("Syntax error - expected world materials XX on line %d.\n", stdConffile_GetLineNumber());
        }

        return 1;
    }

    sithWorld_UpdateLoadProgress(5.0f); // Note, original this call was made between conffile ReadLine and line scan

    SITH_ASSERTREL(numMaterials > 0); // Fixed: Move this assert before below step calculation
    float progressStep = 45.0f / (float)numMaterials;

    if ( sithMaterial_AllocWorldMaterials(pWorld, numMaterials) )
    {
        // TODO: Maybe add alloc error log
        return 1;
    }

    if ( pWorld->apMatArray )
    {
        STDFREE(pWorld->apMatArray);
    }

    pWorld->apMatArray = (rdMaterial**)STDMALLOC(sizeof(pWorld->apMatArray) * pWorld->sizeMaterials); // Altered: Use pWorld->sizeMaterials instead of numMaterials to allocate apMatArray
    if ( !pWorld->apMatArray )
    {
        // TODO: Maybe add alloc error log
        return 1;
    }

    size_t curMatNum = 0;
    while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") )
    {
        rdMaterial* pMat = sithMaterial_Load(stdConffile_g_entry.aArgs[1].argValue);
        if ( !pMat )
        {
            SITHLOG_ERROR("Material load failure %s.\n", stdConffile_g_entry.aArgs[1].argValue);
        }

        pWorld->apMatArray[curMatNum++] = pMat;

        float progress = (float)curMatNum * progressStep + 5.0f;
        sithWorld_UpdateLoadProgress(progress);
    }

    // Removed: The SITHMATERIAL_EXTRABUFFERSIZE constraint was removed

    // Success
    sithWorld_UpdateLoadProgress(50.0f);
    SITHLOG_STATUS("Allocated %d materials, %d used.\n", pWorld->sizeMaterials, pWorld->numMaterials);
    return 0;
}

int J3DAPI sithMaterial_WriteMaterialsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    int bError             = 1;
    tFileHandle fhMat      = 0;
    uint8_t* aPixelBuffers = NULL;

    size_t sizeInfos  = sizeof(CndMaterialInfo) * pWorld->numMaterials;
    CndMaterialInfo* aMatInfos = (CndMaterialInfo*)STDMALLOC(sizeInfos);
    if ( !aMatInfos )
    {
        goto error;
    }

    STD_ZEROMEM(aMatInfos, sizeInfos);
    CndMaterialInfo* pCurInfo = aMatInfos;

    // Calculate the required pixeldata buffer size for all materials

    size_t sizePixelBuffers = 0;
    for ( size_t i = 0; i < pWorld->numMaterials; ++i )
    {
        const rdMaterial* pMat = &pWorld->aMaterials[i];
        pCurInfo->numMipLevels = std3D_GetMipMapCount(pMat->aTextures);

        // Altered: Raised pixel size to 4 bytes from 2 bytes to allow 32 bit textures
        size_t texSize = sithMaterial_GetTotalPixelDataSize(pMat->width, pMat->height, /*bpp=*/32, pCurInfo->numMipLevels, pMat->numCels);
        sizePixelBuffers += texSize;
        ++pCurInfo;
    }

    aPixelBuffers = (uint8_t*)STDMALLOC(sizePixelBuffers);
    if ( !aPixelBuffers )
    {
        goto error;
    }

    STD_ZEROMEM(aPixelBuffers, sizePixelBuffers);
    uint8_t* pCurPixelBuffer = aPixelBuffers;

    // Reset cur info pointer
    pCurInfo = aMatInfos;
    sizePixelBuffers = 0; // Added: Reset to 0 as the exact buffer size will be recalculated in the following loop

    // Loop goes over all world materials and collects for each material it's raster info and pixeldata
    for ( size_t i = 0; i < pWorld->numMaterials; ++i )
    {
        const rdMaterial* pMat = &pWorld->aMaterials[i];

        // Load material data from file
        char aOutPath[128];
        stdFnames_MakePath(aOutPath, STD_ARRAYLEN(aOutPath), "mat", pMat->aName);

        fhMat = sith_g_pHS->pFileOpen(aOutPath, "rb");
        if ( !fhMat )
        {
            SITHLOG_ERROR("Unable to open material '%s'.\n", aOutPath);
            goto error;
        }

        pCurInfo->numCels = pMat->numCels;
        pCurInfo->width   = pMat->width;
        pCurInfo->height  = pMat->height;

        STD_STRCPY(pCurInfo->aName, pMat->aName);

        // Read header & copy colorInfo
        rdMatHeader matHeader;
        if ( sith_g_pHS->pFileRead(fhMat, &matHeader, sizeof(matHeader)) != sizeof(matHeader) )
        {
            goto error;
        }
        STD_COPYMEM(&pCurInfo->colorInfo, &matHeader.colorInfo, sizeof(pCurInfo->colorInfo));

        // Skip records
        if ( sith_g_pHS->pFileSeek(fhMat, sizeof(rdMatRecordHeader) * pMat->numCels, 1) ) // 1 - seek from cur pos
        {
            goto error;
        }

        // Now read pixledata if mat from mat file.
        // TODO: maybe a check should be made for colorInfo.bpp <= 32?
        size_t texSize = sithMaterial_GetTextureByteSize(pMat->width, pMat->height, matHeader.colorInfo.bpp, pCurInfo->numMipLevels); // Fixed: Using actual pixel size from first texture; was hardcoded to 2 bytes - 16 bpp
        for ( size_t j = 0; j < pMat->numCels; ++j )
        {
            if ( sith_g_pHS->pFileSeek(fhMat, sizeof(rdMatTextureHeader), 1) )
            {
                goto error;
            }

            size_t nRead = sith_g_pHS->pFileRead(fhMat, pCurPixelBuffer, texSize);
            if ( texSize != nRead )
            {
                goto error;
            }

            pCurPixelBuffer += texSize;
        }

        // Added: Adds correct mat pixledata size to buffer. This allows having different pixel sizes. Before was hardcoded to 16 bpp
        sizePixelBuffers += texSize * pMat->numCels;

        sith_g_pHS->pFileClose(fhMat);
        fhMat = 0;

        ++pCurInfo;
    }

    // Write material section to CND file
    size_t nWritten = sith_g_pHS->pFileWrite(fh, &sizePixelBuffers, sizeof(uint32_t));  // pixeldata buffer size
    if ( nWritten == sizeof(uint32_t) )
    {
        // Write mat infos
        nWritten = sith_g_pHS->pFileWrite(fh, aMatInfos, sizeInfos);
        if ( nWritten == sizeInfos )
        {
            // Write pixeldata buffer
            nWritten = sith_g_pHS->pFileWrite(fh, aPixelBuffers, sizePixelBuffers);
            if ( nWritten == sizePixelBuffers )
            {
                bError = 0; // Success
            }
        }
    }

error:
    if ( fhMat )
    {
        sith_g_pHS->pFileClose(fhMat);
    }

    if ( aMatInfos )
    {
        STDFREE(aMatInfos);
    }

    if ( aPixelBuffers )
    {
        STDFREE(aPixelBuffers);
    }

    return bError;
}

size_t sithMaterial_GetTextureByteSize(size_t width, size_t height, size_t bpp, size_t numMipLevels)
{
    return (bpp / 8) * rdMaterial_GetMipSize(width, height, numMipLevels);
}

size_t sithMaterial_GetTotalPixelDataSize(size_t width, size_t height, size_t bpp, size_t numMipLevels, size_t numCells)
{
    return sithMaterial_GetTextureByteSize(width, height, bpp, numMipLevels) * numCells;
}

size_t sithMaterial_GetCndMaterialPixelDataSize(const CndMaterialInfo* pInfo)
{
    return  sithMaterial_GetTotalPixelDataSize(pInfo->width, pInfo->height, pInfo->colorInfo.bpp, pInfo->numMipLevels, pInfo->numCels);
}

bool sithMaterial_FileExists(const char* pFilename)
{
    char aPath[128] = { 0 };
    SITH_ASSERT(strlen(pFilename) < STD_ARRAYLEN(aPath)); // Changed: Moved this check down here
    stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "mat", pFilename);
    return stdUtil_FileExists(aPath);
}

int J3DAPI sithMaterial_ReadMaterialsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    int bError                 = 1;
    CndMaterialInfo* aMatInfos = NULL;
    uint8_t* aPixelBuffers     = NULL;
    tVBuffer** apBuffers       = NULL;
    tRasterInfo rasterInfo     = { 0 };

    size_t numMaterials = pWorld->numMaterials; // Note, must be cached because alloc function resets it to 0
    if ( sithMaterial_AllocWorldMaterials(pWorld, pWorld->sizeMaterials) )
    {
        goto error;
    }

    // Free existing mat pointer array
    if ( pWorld->apMatArray )
    {
        STDFREE(pWorld->apMatArray);
    }

    // Allocate mat pointer array and world material array
    // Altered: Moved after call to sithMaterial_AllocWorldMaterials as it can alter the sizeMaterials value,
    //          and to match with the allocation in sithMaterial_ReadMaterialsListText.
    pWorld->apMatArray = (rdMaterial**)STDMALLOC(sizeof(pWorld->apMatArray) * pWorld->sizeMaterials);
    if ( !pWorld->apMatArray )
    {
        goto error;
    }

    size_t sizeInfos = sizeof(CndMaterialInfo) * numMaterials;
    aMatInfos = (CndMaterialInfo*)STDMALLOC(sizeof(CndMaterialInfo) * numMaterials);
    if ( !aMatInfos )
    {
        goto error;
    }
    STD_ZEROMEM(aMatInfos, sizeInfos);

    // Read the size of pixeldata buffer from CND file
    size_t sizePixelBuffers;
    size_t nRead = sith_g_pHS->pFileRead(fh, &sizePixelBuffers, sizeof(uint32_t));
    if ( nRead != sizeof(uint32_t) )
    {
        goto error;
    }

    // Alloc pixeldata buffer
    aPixelBuffers = (uint8_t*)STDMALLOC(sizePixelBuffers);
    if ( !aPixelBuffers )
    {
        goto error;
    }
    STD_ZEROMEM(aPixelBuffers, sizePixelBuffers);

    // Read pixeldata buffer from CND file 
    nRead = sith_g_pHS->pFileRead(fh, aMatInfos, sizeInfos);
    if ( nRead != sizeInfos )
    {
        goto error;
    }

    // Read pixeldata buffer from CND file
    nRead = sith_g_pHS->pFileRead(fh, aPixelBuffers, sizePixelBuffers);
    if ( nRead != sizePixelBuffers )
    {
        goto error;
    }

    // Now construct materials form mat infos and pixeldata

    size_t curWorldMatIdx = 0;
    uint8_t* pCurPixelBuffer = aPixelBuffers;

    for ( size_t matNum = 0; matNum < numMaterials; matNum++ )
    {
        const CndMaterialInfo* pCurInfo = &aMatInfos[matNum];

        rdMaterial* pMat = sithMaterial_CacheFind(pCurInfo->aName);
        if ( pMat )
        {
            // Material already loaded, skip 
            // Altered: Replaced inline code with new sithMaterial_GetCndMaterialPixelDataSize function
            // Fixed: Calculate exact pixel size. Was hardcoded to 2 bytes - 16 bpp
            // Fixed: Use all texture data from pCurInfo. OG was using width, height and num cels from pMat
            //        which could lead to incorrect size calculation if the cached material 
            //        had different dimensions or num cels than the one being loaded.
            size_t texSize   = sithMaterial_GetCndMaterialPixelDataSize(pCurInfo);
            pCurPixelBuffer += texSize; // Increase cur pixeldata pos
            pWorld->apMatArray[matNum] = pMat;
            continue;
        }
        else
        {
            // Added: Try loading materials form file system first
            if ( sithMaterial_bCndLoadExternal && sithMaterial_FileExists(pCurInfo->aName) )
            {
                pMat = sithMaterial_Load(pCurInfo->aName);
                if ( pMat )
                {
                    size_t texSize   = sithMaterial_GetCndMaterialPixelDataSize(pCurInfo);
                    pCurPixelBuffer += texSize; // Increase cur pixeldata pos
                    pWorld->apMatArray[matNum] = pMat;
                    curWorldMatIdx++;
                    continue;
                }
            }
        }

        // Material not loaded in the system yet, load it from pixeldata buffer

        SITH_ASSERT(curWorldMatIdx == pWorld->numMaterials);
        pMat = &pWorld->aMaterials[curWorldMatIdx++]; // TODO: curWorldMatIdx would probably be replaced by pWorld->numMaterials
        pWorld->apMatArray[matNum] = pMat;

        STD_STRCPY(pMat->aName, pCurInfo->aName);

        pMat->numCels    = pCurInfo->numCels;
        pMat->formatType = std3D_GetColorFormat(&pCurInfo->colorInfo);

        ColorInfo desiredColorFormat;
        int bHasColorKey;
        LPDDCOLORKEY pColorKey;
        std3D_GetTextureFormat(pMat->formatType, &desiredColorFormat, &bHasColorKey, &pColorKey);

        // Fixed: Use correct color format type, since it might be different than stored texture format (e.g. RGBA5551 -> RGBA8888; STDCOLOR_FORMAT_RGBA_1BITALPHA -> STDCOLOR_FORMAT_RGBA).
        //        This fixes rendering issue where converted 1-bit alpha texture will still be interpreted as 1-bit alpha texture and
        //        when alpha reaches certain threshold (0xA0 - 0.627 or lower) the polygon becomes invisible (not rendered). See std3D_SetRenderState.
        // 
        //        Note: Due to this change all 1-bit alpha textures of 3DO models that were changed to 32bit format will be pushed to alpha buffer of rdCache (see rdModel3_DrawFace).
        //              Since there might be now more alpha polygons to render than in the OG version the rdCahce alpha buffer had to be increased or risking some polygons not being rendered, i.e.: transparent adjoin surfaces.
        //              Example of this issue is intro cutscene of 9 - Olmec Valley level, where river is briefly not rendered due too small alpha buffer size.
        pMat->formatType = std3D_GetColorFormat(&desiredColorFormat);

        pMat->width     = pCurInfo->width;
        pMat->height    = pCurInfo->height;
        pMat->aTextures = NULL;

        if ( pMat->numCels )
        {
            pMat->aTextures = (tSystemTexture*)STDMALLOC(sizeof(tSystemTexture) * pMat->numCels);
            if ( !pMat->aTextures )
            {
                goto error;
            }
            STD_ZEROMEM(pMat->aTextures, sizeof(tSystemTexture) * pMat->numCels);
        }

        apBuffers = (tVBuffer**)STDMALLOC(sizeof(apBuffers) * pCurInfo->numMipLevels);
        if ( !apBuffers )
        {
            goto error;
        }
        STD_ZEROMEM(apBuffers, sizeof(apBuffers) * pCurInfo->numMipLevels);

        // Alloc texture MipMap VBuffers
        STD_COPYMEM(&rasterInfo.colorInfo, &pCurInfo->colorInfo, sizeof(rasterInfo.colorInfo));
        for ( size_t i = 0; i < pCurInfo->numMipLevels; ++i )
        {
            rasterInfo.width  = pMat->width >> i;
            rasterInfo.height = pMat->height >> i;
            apBuffers[i] = stdDisplay_VBufferNew(&rasterInfo, /*bUseVSurface=*/0, /*bUseVideoMemory=*/0);
            if ( !apBuffers[i] )
            {
                goto error;
            }
        }

        // Construct material from pixeldata 
        for ( size_t i = 0; i < pMat->numCels; ++i )
        {
            for ( size_t j = 0; j < pCurInfo->numMipLevels; ++j )
            {
                STD_COPYMEM(&apBuffers[j]->rasterInfo.colorInfo, &rasterInfo.colorInfo, sizeof(apBuffers[j]->rasterInfo.colorInfo));

                // Fixed: Update row size and size before calling to match the new color info
                apBuffers[j]->rasterInfo.rowSize = (apBuffers[j]->rasterInfo.colorInfo.bpp / 8) * apBuffers[j]->rasterInfo.width;
                apBuffers[j]->rasterInfo.size    = apBuffers[j]->rasterInfo.rowSize * apBuffers[j]->rasterInfo.height;

                stdDisplay_VBufferLock(apBuffers[j]);
                STD_COPYMEM(apBuffers[j]->pPixels, pCurPixelBuffer, apBuffers[j]->rasterInfo.size);
                stdDisplay_VBufferUnlock(apBuffers[j]);

                // Now convert pixeldata to desired color format
                pCurPixelBuffer += apBuffers[j]->rasterInfo.size; // Fixed: Increment pCurPixelBuffer before assigning new tVBuffer to apBuffers[j]
                apBuffers[j] = stdDisplay_VBufferConvertColorFormat(&desiredColorFormat, apBuffers[j], bHasColorKey, pColorKey);
            }

            std3D_AllocSystemTexture(&pMat->aTextures[i], apBuffers, pCurInfo->numMipLevels, pMat->formatType);
        }

        for ( size_t i = 0; i < pCurInfo->numMipLevels; ++i )
        {
            stdDisplay_VBufferFree(apBuffers[i]);
            apBuffers[i] = 0;
        }

        STDFREE(apBuffers);
        apBuffers = NULL;

        sithMaterial_CacheAdd(pMat);

        pMat->num = pWorld->numMaterials;
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pMat->num = SITHWORLD_STATICINDEX(pMat->num);
        }

        ++pWorld->numMaterials;

    }

    bError = 0; // Success

error:
    if ( aMatInfos )
    {
        STDFREE(aMatInfos);
    }

    if ( aPixelBuffers )
    {
        STDFREE(aPixelBuffers);
    }

    if ( apBuffers )
    {
        STDFREE(apBuffers);
    }

    return bError;
}

rdMaterial* J3DAPI sithMaterial_Load(const char* pName)
{
    SithWorld* pWorld = sithWorld_g_pLastLoadedWorld;
    SITH_ASSERTREL(pWorld != NULL);

    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(pWorld->aMaterials != NULL);

    rdMaterial* pMat = sithMaterial_CacheFind(pName);
    if ( pMat )
    {
        return pMat;
    }

    if ( !pName[0] ) // Check if first char is 0
    {
        return NULL;
    }

    SITH_ASSERTREL(pWorld->numMaterials <= pWorld->sizeMaterials);
    if ( pWorld->numMaterials >= (unsigned int)pWorld->sizeMaterials )
    {
        SITHLOG_ERROR("No space left to load new material '%s'.\n", pName);
        return NULL;
    }

    char aPath[128] = { 0 };
    SITH_ASSERTREL(strlen(pName) < STD_ARRAYLEN(aPath)); // Changed: Moved this check down here
    stdFnames_MakePath(aPath, STD_ARRAYLEN(aPath), "mat", pName);

    pMat = &pWorld->aMaterials[pWorld->numMaterials];
    if ( rdMaterial_LoadEntry(aPath, pMat) )
    {
        // TODO: [DEAD] Following dead code (in if statement) was found in debug version and is never executed in release version
        if ( false )
        {
            SITHLOG_ERROR("Material %s not found, trying default.\n", pName);
            return sithMaterial_Load("dflt.mat");
        }

        SITHLOG_ERROR("Material %s not found.\n", pName);
        return NULL;
    }

    sithMaterial_CacheAdd(pMat);

    pMat->num = pWorld->numMaterials;
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        pMat->num = SITHWORLD_STATICINDEX(pMat->num);
    }

    ++pWorld->numMaterials;
    return pMat;
}

rdMaterial* J3DAPI sithMaterial_GetMaterialByIndex(int index)
{
    SithWorld* pWorld = sithWorld_g_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        pWorld = sithWorld_g_pStaticWorld;
        index = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( pWorld && index >= 0 && index < pWorld->numMaterials )
    {
        return &pWorld->aMaterials[index];
    }

    return NULL;
}

int J3DAPI sithMaterial_AllocWorldMaterials(SithWorld* pWorld, size_t numMaterials)
{
    SITH_ASSERTREL(pWorld->aMaterials == NULL);

    // Added: Add extra buffer capacity for static worlds
    //        Make sure the material buffer is enough big to load in extra textures of original HD models.
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        numMaterials += sithMaterial_staticBufferExtraCapacity;
    }

    pWorld->aMaterials = (rdMaterial*)STDMALLOC(sizeof(rdMaterial) * numMaterials);
    if ( !pWorld->aMaterials )
    {
        return 1;
    }

    STD_ZEROMEM(pWorld->aMaterials, sizeof(rdMaterial) * numMaterials);

    pWorld->sizeMaterials = numMaterials;
    pWorld->numMaterials  = 0;

    if ( sithMaterial_pHashtable )
    {
        return 0;
    }

    sithMaterial_pHashtable = stdHashtbl_New(SITHMATERIAL_TABLESIZE); // ???, startup should already alloc table
    if ( !sithMaterial_pHashtable )
    {
        STDFREE(pWorld->aMaterials);
        return 1;
    }

    return 0;
}

rdMaterial* J3DAPI sithMaterial_CacheFind(const char* pName)
{
    SITH_ASSERTREL(sithMaterial_pHashtable);
    return (rdMaterial*)stdHashtbl_Find(sithMaterial_pHashtable, pName);
}

void J3DAPI sithMaterial_CacheAdd(rdMaterial* pMat)
{
    SITH_ASSERTREL(sithMaterial_pHashtable);
    stdHashtbl_Add(sithMaterial_pHashtable, pMat->aName, pMat);
}

int J3DAPI sithMaterial_CacheRemove(const rdMaterial* pMat)
{
    SITH_ASSERTREL(sithMaterial_pHashtable);
    SITH_ASSERTREL(pMat->aName);
    return stdHashtbl_Remove(sithMaterial_pHashtable, pMat->aName);
}