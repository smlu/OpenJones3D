#include "rdMaterial.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Main/rdroid.h>
#include <rdroid/RTI/symbols.h>

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#define RDMAT_MAGIC        "MAT "
#define RDMAT_VERSION      50
#define RDMAT_TYPE_TEXTURE 2

rdMaterialLoaderFunc rdMaterial_pMaterialsLoader     = NULL;
rdMaterialUnloaderFunc rdMaterial_pMaterialsUnloader = NULL;

void rdMaterial_InstallHooks(void)
{
    J3D_HOOKFUNC(rdMaterial_RegisterLoader);
    J3D_HOOKFUNC(rdMaterial_Load);
    J3D_HOOKFUNC(rdMaterial_LoadEntry);
    J3D_HOOKFUNC(rdMaterial_Free);
    J3D_HOOKFUNC(rdMaterial_FreeEntry);
    J3D_HOOKFUNC(rdMaterial_GetMaterialMemUsage);
    J3D_HOOKFUNC(rdMaterial_GetMipSize);
}

void rdMaterial_ResetGlobals(void)
{}

rdMaterialLoaderFunc J3DAPI rdMaterial_RegisterLoader(rdMaterialLoaderFunc pFunc)
{
    rdMaterialLoaderFunc pPrevFunc = rdMaterial_pMaterialsLoader;
    rdMaterial_pMaterialsLoader = pFunc;
    return pPrevFunc;
}

rdMaterialUnloaderFunc J3DAPI rdMaterial_RegisterUnloader(rdMaterialUnloaderFunc pFunc)
{
    rdMaterialUnloaderFunc pPrevFunc = rdMaterial_pMaterialsUnloader;
    rdMaterial_pMaterialsUnloader = pFunc;
    return pPrevFunc;
}

rdMaterial* J3DAPI rdMaterial_Load(const char* pFilename)
{
    if ( rdMaterial_pMaterialsLoader )
    {
        return rdMaterial_pMaterialsLoader(pFilename);
    }

    rdMaterial* pMaterial = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
    if ( !pMaterial )
    {
        RDLOG_ERROR("Error: Bad memory allocation for '%s'\n", pFilename);
        return NULL;
    }

    if ( rdMaterial_LoadEntry(pFilename, pMaterial) )
    {
        // Failed to load material
        rdMaterial_Free(pMaterial);
        return NULL;
    }

    return pMaterial;
}

int J3DAPI rdMaterial_LoadEntry(const char* pFilename, rdMaterial* pMat)
{
    // Clear input mat
    memset(pMat, 0, sizeof(rdMaterial));

    tFileHandle fh = rdroid_g_pHS->pFileOpen(pFilename, "rb");
    if ( !fh )
    {
        RDLOG_ERROR("Error: Invalid load filename '%s'.\n", pFilename);
        return 1;
    }

    rdMatHeader header;
    if ( rdroid_g_pHS->pFileRead(fh, &header, sizeof(rdMatHeader)) != sizeof(rdMatHeader) )
    {
        goto error;
    }

    if ( strncmp(header.magic, RDMAT_MAGIC, 4) != 0 )
    {
        RDLOG_ERROR("Error: Bad header for '%s'\n", pFilename);

    format_error:
        RDLOG_ERROR("Error: Invalid format for '%s'\n", pFilename);
        goto error;
    }

    if ( header.version != RDMAT_VERSION )
    {
        RDLOG_ERROR("Error: Bad version %d for '%s'\n", header.version, pFilename);
        goto format_error;
    }

    if ( header.numCels != header.numTextures )
    {
        RDLOG_ERROR("Error: Old style mat for '%s'\n", pFilename);
        goto format_error;
    }

    pMat->numCels    = header.numCels;
    pMat->formatType = std3D_GetColorFormat(&header.colorInfo);

    int bColorKey = 0;
    LPDDCOLORKEY pColorKey;
    ColorInfo ci;
    std3D_GetTextureFormat(pMat->formatType, &ci, &bColorKey, &pColorKey);

    rdMatCelInfo celInfo;
    rdMatTextureCelInfo texcelInfo;
    for ( size_t i = 0; i < pMat->numCels; ++i )
    {
        if ( rdroid_g_pHS->pFileRead(fh, &celInfo, sizeof(rdMatCelInfo)) != sizeof(rdMatCelInfo)
            || ((celInfo.type & 0x8) != 0 && rdroid_g_pHS->pFileRead(fh, &texcelInfo, sizeof(rdMatTextureCelInfo)) != sizeof(rdMatTextureCelInfo)) )
        {
            goto error;
        }
    }

    pMat->aTextures = NULL;
    if ( pMat->numCels )
    {
        pMat->aTextures = (tSystemTexture*)STDMALLOC(sizeof(tSystemTexture) * pMat->numCels);
        if ( !pMat->aTextures )
        {
        bad_alloc_error:
            RDLOG_ERROR("Error: Bad memory allocation for '%s'\n", pFilename);
            goto error;
        }

        memset(pMat->aTextures, 0, sizeof(tSystemTexture) * pMat->numCels);
    }

    // Read in cel textures
    for ( size_t i = 0; i < pMat->numCels; ++i )
    {
        rdMatTextureHeader texHeader;
        if ( rdroid_g_pHS->pFileRead(fh, &texHeader, sizeof(rdMatTextureHeader)) != sizeof(rdMatTextureHeader) )
        {
            goto error;
        }

        pMat->width  = texHeader.width;
        pMat->height = texHeader.height;

        tRasterInfo rasterInfo;
        rasterInfo.width  = texHeader.width;
        rasterInfo.height = texHeader.height;
        memcpy(&rasterInfo.colorInfo, &header.colorInfo, sizeof(rasterInfo.colorInfo));

        tVBuffer** apVBuffers = NULL;
        if ( texHeader.numMipLevels )
        {
            apVBuffers = (tVBuffer**)STDMALLOC(sizeof(tVBuffer**) * texHeader.numMipLevels);
            if ( !apVBuffers )
            {
                goto bad_alloc_error;
            }

            memset(apVBuffers, 0, sizeof(tVBuffer**) * texHeader.numMipLevels);
        }

        // Read in cel mipmaps
        for ( size_t j = 0; j < texHeader.numMipLevels; ++j )
        {
            tVBuffer* pVBuffer = stdDisplay_VBufferNew(&rasterInfo, /*bUseVSurface=*/0, /*bUseVideoMemory=*/0);
            apVBuffers[j] = pVBuffer;
            if ( !apVBuffers[j] )
            {
                goto bad_alloc_error;
            }

            stdDisplay_VBufferLock(apVBuffers[j]);
            pVBuffer = apVBuffers[j];
            if ( pVBuffer->rasterInfo.size != rdroid_g_pHS->pFileRead(fh, pVBuffer->pPixels, pVBuffer->rasterInfo.size) )
            {
                goto error;
            }

            stdDisplay_VBufferUnlock(apVBuffers[j]);
            if ( apVBuffers[j]->rasterInfo.colorInfo.colorMode && std3D_GetNumTextureFormats() )
            {
                apVBuffers[j] = stdDisplay_VBufferConvertColorFormat(&ci, apVBuffers[j], bColorKey, pColorKey);
            }

            rasterInfo.width  >>= 1; // /2
            rasterInfo.height >>= 1;
        }

        std3D_AllocSystemTexture(&pMat->aTextures[i], apVBuffers, texHeader.numMipLevels, pMat->formatType);
        for ( size_t j = 0; j < texHeader.numMipLevels; ++j )
        {
            stdDisplay_VBufferFree(apVBuffers[j]);
        }

        stdMemory_Free(apVBuffers);
    }

    if ( (header.type & 1) == 0 )
    {
        const char* pName = stdFileFromPath(pFilename);
        STD_STRCPY(pMat->aName, pName);
        rdroid_g_pHS->pFileClose(fh);
        return 0;
    }


error:
    if ( fh )
    {
        rdroid_g_pHS->pFileClose(fh);
    }

    return 1;
}

void J3DAPI rdMaterial_Free(rdMaterial* pMaterial)
{
    RD_ASSERTREL(pMaterial != NULL);
    if ( rdMaterial_pMaterialsUnloader )
    {
        rdMaterial_pMaterialsUnloader(pMaterial);
    }
    else
    {
        rdMaterial_FreeEntry(pMaterial);
        stdMemory_Free(pMaterial);
    }
}

void J3DAPI rdMaterial_FreeEntry(rdMaterial* pMaterial)
{
    RD_ASSERTREL(pMaterial != NULL);

    if ( pMaterial->aTextures )
    {
        for ( size_t i = 0; i < pMaterial->numCels; ++i ) {
            std3D_ClearSystemTexture(&pMaterial->aTextures[i]);
        }
        stdMemory_Free(pMaterial->aTextures);
    }

    memset(pMaterial, 0, sizeof(rdMaterial));
}

int J3DAPI rdMaterial_Write(const char* pFilename, const rdMaterial* pMaterial, tVBuffer*** paTextures, size_t numMipLevels)
{
    rdMatHeader header;
    STD_STRNCPY(header.magic, RDMAT_MAGIC, 4u);
    header.version     = RDMAT_VERSION;
    header.type        = RDMAT_TYPE_TEXTURE;
    header.numCels     = pMaterial->numCels;
    header.numTextures = pMaterial->numCels;
    memcpy(&header.colorInfo, &(**paTextures)->rasterInfo.colorInfo, sizeof(header.colorInfo));

    tFileHandle fh = rdroid_g_pHS->pFileOpen(pFilename, "wb+");
    if ( !fh )
    {
        RDLOG_ERROR("Error: Invalid save filename '%s'.\n", pFilename);
        goto error;
    }

    if ( rdroid_g_pHS->pFileWrite(fh, &header, sizeof(rdMatHeader)) != sizeof(rdMatHeader) )
    {
    error:
        if ( fh ) {
            rdroid_g_pHS->pFileClose(fh);
        }

        return 1;
    }

    rdMatCelInfo celInfo = { 0 };
    celInfo.type     = 0x8;
    celInfo.unknown4 = 1.0f;

    for ( size_t i = 0; i < pMaterial->numCels; ++i )
    {
        if ( rdroid_g_pHS->pFileWrite(fh, &celInfo, sizeof(rdMatCelInfo)) != sizeof(rdMatCelInfo) )
        {
            goto error;
        }

        rdMatTextureCelInfo texcelInfo = { 0 };
        texcelInfo.unknown0 = 0;
        texcelInfo.celNum   = i;
        if ( rdroid_g_pHS->pFileWrite(fh, &texcelInfo, sizeof(rdMatTextureCelInfo)) != sizeof(rdMatTextureCelInfo) )
        {
            goto error;
        }
    }

    for ( size_t i = 0; i < pMaterial->numCels; ++i )
    {
        rdMatTextureHeader texHeader = { 0 };
        texHeader.width  = pMaterial->width;
        texHeader.height = pMaterial->height;
        texHeader.numMipLevels = numMipLevels;

        if ( rdroid_g_pHS->pFileWrite(fh, &texHeader, sizeof(rdMatTextureHeader)) != sizeof(rdMatTextureHeader) )
        {
            goto error;
        }

        for ( size_t j = 0; j < numMipLevels; ++j )
        {
            tVBuffer* pBuffer = paTextures[i][j];
            stdDisplay_VBufferLock(pBuffer);

            if ( pBuffer->rasterInfo.size != rdroid_g_pHS->pFileWrite(fh, pBuffer->pPixels, pBuffer->rasterInfo.size) )
            {
                goto error;
            }

            stdDisplay_VBufferUnlock(pBuffer);
        }
    }

    rdroid_g_pHS->pFileClose(fh);
    return 0;
}

size_t J3DAPI rdMaterial_GetMaterialMemUsage(const rdMaterial* pMaterial)
{
    size_t mipLevels = std3D_GetMipMapCount(pMaterial->aTextures);
    return (sizeof(tSystemTexture) * pMaterial->numCels)
        + (pMaterial->numCels * rdMaterial_GetMipSize(pMaterial->width, pMaterial->height, mipLevels))
        + sizeof(rdMaterial);
}

size_t J3DAPI rdMaterial_GetMipSize(int width, int height, size_t mipLevels)
{
    size_t size = 0;
    size_t curSize = height * width;
    for ( size_t i = 0; i < mipLevels && curSize; ++i )// Added: Added check for  curSize != 0
    {
        size += curSize;
        curSize >>= 2;
    }

    return size;
}
