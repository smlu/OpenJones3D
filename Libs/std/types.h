#ifndef STD_TYPES_H
#define STD_TYPES_H

#if defined(J3D_DIRECTX6)
#undef DIRECT3D_VERSION
#include <d3d.h>
#include <ddraw.h>
#include <DirectX6/dplay.h>

#include <external/DirectX61c/include/dplay.h>
#elif defined(J3D_DIRECTX9)
#undef DIRECT3D_VERSION
#ifdef J3D_DEBUG
#define D3D_DEBUG_INFO // TODO: comment out when done
#endif
#include <d3d9.h>
#elif defined(J3D_OPENGL)
#include <float.h>
#include <glad/gl.h>
#include <SDL3/SDL.h>
#else
#error "Unsupported GAPI"
#endif

#include <stdint.h>
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

#if defined(J3D_DIRECTX6)
// Direct3D & DirectDraw
typedef DDCAPS_DX6 tSysDisplayDeviceCaps;
typedef IDirectDraw4 tSysDisplayDevice;

typedef D3DDEVICEDESC tSysDevice3DDesc;
typedef IDirect3DDevice3 tSysDevice3D;

typedef DDPIXELFORMAT tSysPixelFormat;
typedef DDSURFACEDESC2 tSysSurfaceDesc;
typedef IDirectDrawSurface4 tSysSurface;
typedef IDirect3DTexture2 tSysTexture;

#elif defined(J3D_DIRECTX9)
// DirectPlay
#define DPID_ALLPLAYERS   0
#define DPSEND_GUARANTEED 0x00000001
#define DPERR_GENERIC     E_FAIL
typedef DWORD DPID;

// Direct3D stuff
#define D3DRGBA D3DCOLOR_COLORVALUE
#define RGBA_MAKE D3DCOLOR_ARGB
#define D3DRGB(r, g , b) \
D3DCOLOR_XRGB((DWORD)((r)*255.f),(DWORD)((g)*255.f),(DWORD)((b)*255.f))

typedef D3DCAPS9 tSysDisplayDeviceCaps;
typedef IDirect3DDevice9 tSysDisplayDevice;

typedef D3DCAPS9 tSysDevice3DDesc;
typedef IDirect3DDevice9 tSysDevice3D;

typedef D3DFORMAT tSysPixelFormat;
typedef D3DSURFACE_DESC tSysSurfaceDesc;
typedef IDirect3DSurface9 tSysSurface;
typedef IDirect3DTexture9 tSysTexture;
typedef void* LPDDCOLORKEY;

// The FVF format for D3DTLVERTEX
#define D3DTLVERTEX_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

#elif defined(J3D_OPENGL)

#define DPID_ALLPLAYERS   0
#define DPSEND_GUARANTEED 0x00000001
#define DPERR_GENERIC     E_FAIL
// Direct3D stuff
#define D3DRGBA D3DCOLOR_COLORVALUE
#define RGBA_MAKE D3DCOLOR_ARGB
#define D3DRGB(r, g , b) \
D3DCOLOR_XRGB((DWORD)((b)*255.f),(DWORD)((g)*255.f),(DWORD)((r)*255.f))

// maps floating point channels (0.f to 1.f range) to D3DCOLOR
#define D3DCOLOR_COLORVALUE(r,g,b,a) \
D3DCOLOR_RGBA((DWORD)((b)*255.f),(DWORD)((g)*255.f),(DWORD)((r)*255.f),(DWORD)((a)*255.f))

// maps unsigned 8 bits/channel to D3DCOLOR
#define D3DCOLOR_XRGB(r,g,b)   D3DCOLOR_ARGB(0xff,r,g,b)
#define D3DCOLOR_ARGB(a,r,g,b) \
((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)

typedef DWORD D3DCOLOR;
typedef DWORD DPID;
typedef void* LPDDCOLORKEY;
typedef void* tSysDisplayDeviceCaps;
typedef void* tSysDevice3D;
typedef void* tSysDevice3DDesc;

typedef struct _GLRECT
{
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
} GLRECT;


typedef struct sGLTexture
{
    GLuint id;
} GLTexture;

typedef struct sSysPixelFormat
{
    GLint glInternalFormat;
    GLenum glFormat;
    GLenum glType;
} tSysPixelFormat;

typedef GLTexture tSysTexture;

typedef enum sTextureUnits
{
    TU_DEFAULT = 0,
    TU_SCENE   = 1,
    TU_DEPTH   = 2,
    TU_3D_DRAW = 3
} GLTextureUnit;

#endif // J3D_DIRECTX6

#if defined(J3D_DIRECTX9) || defined(J3D_OPENGL)

typedef struct sD3DTLVERTEX
{
    /* Screen coordinates */
    float sx;
    float sy;
    float sz;

    /* Reciprocal of homogeneous w */
    float rhw;

    /* Vertex color */
    D3DCOLOR color;

    /* Specular component of vertex */
    D3DCOLOR specular;

    /* Texture coordinates */
    float tu;
    float tv;
} D3DTLVERTEX, *LPD3DTLVERTEX;

#endif

typedef enum eStdColorFormatType
{
    STDCOLOR_FORMAT_RGB            = 0,
    STDCOLOR_FORMAT_RGBA_1BITALPHA = 1, // e.g.: RGBA5551
    STDCOLOR_FORMAT_RGBA           = 2,
} StdColorFormatType;

typedef enum eColorMode J3D_ENUM_TYPE(int32_t)
{
    STDCOLOR_PAL  = 0,
    STDCOLOR_RGB  = 1,
    STDCOLOR_RGBA = 2,
} tColorMode;

typedef enum eStd3DMipmapFilterType
{
    STD3D_MIPMAPFILTER_NONE      = 0,
    STD3D_MIPMAPFILTER_BILINEAR  = 1,
    STD3D_MIPMAPFILTER_TRILINEAR = 2,
} Std3DMipmapFilterType;

typedef enum eStdControlAxisFlag
{
    STDCONTROL_AXIS_REGISTERED  = 0x1,
    STDCONTROL_AXIS_ENABLED     = 0x2,
    STDCONTROL_AXIS_HASDEADZONE = 0x8,
    STDCONTROL_AXIS_POSITIVE    = 0x10,
    STDCONTROL_AXIS_NEGATIVE    = 0x20,
    STDCONTROL_AXIS_GAMEPAD     = 0x80,
} StdControlAxisFlag;

typedef enum eStd3DRenderState
{
    STD3D_RS_UNKNOWN_1             = 0x1,
    STD3D_RS_UNKNOWN_2             = 0x2,
    STD3D_RS_ALPHAREF_SET          = 0x4,
    STD3D_RS_UNKNOWN_8             = 0x8,
    STD3D_RS_SUBPIXEL_CORRECTION   = 0x10,
    STD3D_RS_TEXFILTER_BILINEAR    = 0x80,
    STD3D_RS_UNKNOWN_100           = 0x100,
    STD3D_RS_UNKNOWN_200           = 0x200,
    STD3D_RS_UNKNOWN_400           = 0x400,
    STD3D_RS_TEX_CPAMP_U           = 0x800,
    STD3D_RS_TEX_CPAMP_V           = 0x1000,
    STD3D_RS_ZWRITE_DISABLED       = 0x2000,
    STD3D_RS_FOG_ENABLED           = 0x8000,
    STD3D_RS_TEXFILTER_ANISOTROPIC = 0x10000 // Added
} Std3DRenderState;

typedef enum eVBufferType
{
    VBUFFER_SOFTWARE = 0, // Raw pixels allocated on heap
    VBUFFER_HARDWARE = 1, // hardware surface
} VBufferType;

// File handle type
typedef uintptr_t tFileHandle;

// Time type
typedef unsigned int tStdTime;

typedef struct sLinkListNode tLinkListNode;
typedef struct sMemoryHeader tMemoryHeader;
typedef struct sMemoryBlock tMemoryBlock;
typedef struct sSystemTexture tSystemTexture;
typedef struct sGob Gob;

typedef unsigned int (J3DAPI*tHashFunc)(const char*, signed int);
typedef int (J3DAPI*tPrintfFunc)(const char* pFormat, ...);
typedef void (J3DAPI*tAssertFunc)(const char*, const char*, int);
typedef int (J3DAPI*tAtExitFunc)(void (*func)(void));
typedef void* (J3DAPI*tMallocFunc)(size_t size);
typedef void (J3DAPI*tFreeFunc)(void* pData);
typedef void* (J3DAPI*tReallocFunc)(void* pData, size_t newSize);
typedef unsigned int (J3DAPI*tGetTimeMsecFunc)();
typedef tFileHandle (J3DAPI*tFileOpenFunc)(const char* pFilename, const char* mode);
typedef int (J3DAPI*tFileCloseFunc)(tFileHandle fh);
typedef size_t (J3DAPI*tFileReadFunc)(tFileHandle, void*, size_t);
typedef char* (J3DAPI*tFileGetsFunc)(tFileHandle fh, char* pStr, size_t size);
typedef size_t (J3DAPI*tFileWriteFunc)(tFileHandle fh, const void* pData, size_t size);
typedef int (J3DAPI*tFileEOFFunc)(tFileHandle fh);
typedef int (J3DAPI*tFileTellFunc)(tFileHandle fh);
typedef int (J3DAPI*tFileSeekFunc)(tFileHandle fh, int offset, int origin);
typedef size_t (J3DAPI*tFileSizeFunc)(const char* pFilename);
typedef int (*tFilePrintfFunc)(tFileHandle fh, const char* pFormat, ...);
typedef wchar_t* (J3DAPI*tFileGetwsFunc)(tFileHandle fh, wchar_t* pOutStr, size_t size);
typedef void* (J3DAPI*tAllocHandleFunc)(size_t);
typedef void (J3DAPI*tFreeHandleFunc)(void*);
typedef void* (J3DAPI*tReallocHandleFunc)(void* ptr, size_t newSize);
typedef int (J3DAPI*tLockHandleFunc)(int);
typedef void (J3DAPI*tUnlockHandleFunc)();

typedef struct sStdRect
{
    int left;
    int top;
    int right;
    int bottom;
} StdRect;

static_assert(sizeof(StdRect) == 16, "sizeof(StdRect) == 16");

typedef struct sGobFileHeader
{
    char signature[4];
    int version;
    int dirOffset;
} GobFileHeader;

static_assert(sizeof(GobFileHeader) == 12, "sizeof(GobFileHeader) == 12");

typedef struct sGobFileEntry
{
    int offset;
    int size;
    char aName[128];
} tGobFileEntry;

static_assert(sizeof(tGobFileEntry) == 136, "sizeof(tGobFileEntry) == 136");

typedef struct sGobFileDirectory
{
    uint32_t numEntries;
    tGobFileEntry* aEntries;
} GobFileDirectory;

static_assert(sizeof(GobFileDirectory) == 8, "sizeof(GobFileDirectory) == 8");

struct sLinkListNode
{
    tLinkListNode* prev;
    tLinkListNode* next;
    const char* name;
    void* data;
};

static_assert(sizeof(tLinkListNode) == 16, "sizeof(tLinkListNode) == 16");

typedef struct sHashTable
{
    size_t numNodes;
    tLinkListNode* paNodes;
    tHashFunc pfHashFunc;
} tHashTable;

static_assert(sizeof(tHashTable) == 12, "sizeof(tHashTable) == 12");

typedef struct sGobFileHandle
{
    int bUsed;
    Gob* pGob;
    tGobFileEntry* pEntry;
    int offset;
} GobFileHandle;

static_assert(sizeof(GobFileHandle) == 16, "sizeof(GobFileHandle) == 16");

struct sGob
{
    char aFilePath[128];
    tFileHandle hGobFile;
    GobFileDirectory directory;
    tHashTable* pDirHash;
    int numHandles;
    GobFileHandle* aHandles;
    GobFileHandle* pCurHandle;
    int bFileMap;
    LPVOID pBase;
    HANDLE hFile;
    HANDLE hMapFile;
};

static_assert(sizeof(Gob) == 172, "sizeof(Gob) == 172");

typedef struct sGobFileHandles
{
    int numHandles;
    GobFileHandle* aHandles;
} GobFileHandles;

static_assert(sizeof(GobFileHandles) == 8, "sizeof(GobFileHandles) == 8");

typedef struct sFindFileData
{
    float unknown;
    int nFoundFiles;
    char aSearchFilter[128];
    HANDLE handle;
} FindFileData;

static_assert(sizeof(FindFileData) == 140, "sizeof(FindFileData) == 140");

typedef struct sHostServices
{
    float unknown1;
    tPrintfFunc pStatusPrint;
    tPrintfFunc pMessagePrint;
    tPrintfFunc pWarningPrint;
    tPrintfFunc pErrorPrint;
    tPrintfFunc pDebugPrint;
    tAssertFunc pAssert;
    tAtExitFunc pAtExit;
    tMallocFunc pMalloc;
    tFreeFunc pFree;
    tReallocFunc pRealloc;
    tGetTimeMsecFunc pGetTimeMsec;
    tFileOpenFunc pFileOpen;
    tFileCloseFunc pFileClose;
    tFileReadFunc pFileRead;
    tFileGetsFunc pFileGets;
    tFileWriteFunc pFileWrite;
    tFileEOFFunc pFileEOF;
    tFileTellFunc pFileTell;
    tFileSeekFunc pFileSeek;
    tFileSizeFunc pFileSize;
    tFilePrintfFunc pFilePrintf;
    tFileGetwsFunc pFileGetws;
    tAllocHandleFunc pAllocHandle;
    tFreeHandleFunc pFreeHandle;
    tReallocHandleFunc pReallocHandle;
    tLockHandleFunc pLockHandle;
    tUnlockHandleFunc pUnlockHandle;
} tHostServices;

static_assert(sizeof(tHostServices) == 112, "sizeof(tHostServices) == 112");

struct sMemoryHeader
{
    int number;
    uint32_t id;
    size_t size;
    const char* pFilename;
    size_t line;
    tMemoryHeader* pPrev;
    tMemoryHeader* pNext;
    uint32_t magic;
};

static_assert(sizeof(tMemoryHeader) == 32, "sizeof(tMemoryHeader) == 32");

typedef struct sColorInfo
{
    tColorMode colorMode;
    uint32_t bpp;
    uint32_t redBPP;
    uint32_t greenBPP;
    uint32_t blueBPP;
    int32_t redPosShift;
    int32_t greenPosShift;
    int32_t bluePosShift;
    int32_t redPosShiftRight;
    int32_t greenPosShiftRight;
    int32_t bluePosShiftRight;
    uint32_t alphaBPP;
    int32_t alphaPosShift;
    int32_t alphaPosShiftRight;
} ColorInfo;

static_assert(sizeof(ColorInfo) == 56, "sizeof(ColorInfo) == 56");

typedef struct sFoundFileInfo
{
    char aName[260];
    int bIsDirectory;
    uint32_t lastChanged;
} tFoundFileInfo;

static_assert(sizeof(tFoundFileInfo) == 268, "sizeof(tFoundFileInfo) == 268");

typedef struct sStringTableNode
{
    const char* pKey;
    wchar_t* value;
    int unknown;
} tStringTableNode;

static_assert(sizeof(tStringTableNode) == 12, "sizeof(tStringTableNode) == 12");

typedef struct sStringTable
{
    uint32_t nMsgs;
    tStringTableNode* pData;
    tHashTable* pHashtbl;
    int magic;
} tStringTable;

static_assert(sizeof(tStringTable) == 16, "sizeof(tStringTable) == 16");

typedef struct sRasterInfo
{
    uint32_t width;
    uint32_t height;
    size_t size;
    size_t rowSize;
    size_t rowWidth;
    ColorInfo colorInfo;
} tRasterInfo;

static_assert(sizeof(tRasterInfo) == 76, "sizeof(tRasterInfo) == 76");

#if defined (J3D_OPENGL)
typedef struct sVSurface
{
    GLuint fbo;
    GLuint colorTex;
    GLuint depthTex;

    GLuint msaaFbo;
    GLuint msaaColorTex;
    GLuint msaaDepthTex;

    bool skipMSAA;
} tVSurface;
#else
typedef struct sVSurface
{
    tSysSurface* pSysSurface;
    tSysSurfaceDesc desc;
} tVSurface;
#endif


//static_assert(sizeof(tVSurface) == 128, "sizeof(tVSurface) == 128");

typedef struct sVBuffer
{
    VBufferType type;
    size_t lockRefCount;
    int bVideoMemory;
    tRasterInfo rasterInfo;
    uint8_t* pPixels;
    int unknown1; // could be another surface option
    tVSurface surface;
} tVBuffer;

//static_assert(sizeof(tVBuffer) == 224, "sizeof(tVBuffer) == 224");

struct sSystemTexture
{
#ifdef J3D_DIRECTX6
    tSysSurfaceDesc desc;
    tSysTexture* pTexture;
#else
    // VBuffer storage - completely device independent
    tVBuffer** apMipmaps;   // Array of VBuffer pointers (one per mip level)
    size_t numMipLevels;    // Number of mip levels
    tSysPixelFormat format; // DirectX format for video memory texture
#endif
    tSysTexture* pCachedTexture;
    size_t textureSize;
    size_t frameNum;
    tSystemTexture* pPrevCachedTexture;
    tSystemTexture* pNextCachedTexture;
};

//static_assert(sizeof(tSystemTexture) == 148, "sizeof(tSystemTexture) == 148");

typedef struct sCircularBuffer
{
    size_t numAllocated;
    uint8_t* paElements;
    size_t iFirst;
    size_t numValidElements;
    size_t elementSize;
} tCircularBuffer;

static_assert(sizeof(tCircularBuffer) == 20, "sizeof(tCircularBuffer) == 20");

typedef struct sStdDisplayDevice
{
    char aDeviceName[128];
    char aDriverName[128];
    int bHAL;
    int bGuidNotSet;
    int bWindowRenderNotSupported;
    size_t totalVideoMemory;
    size_t freeVideoMemory;
    tSysDisplayDeviceCaps caps;
    GUID guid;
} StdDisplayDevice;

//static_assert(sizeof(StdDisplayDevice) == 672, "sizeof(StdDisplayDevice) == 672");

typedef struct sStdVideoMode
{
    float aspectRatio;
    tRasterInfo rasterInfo;
    uint32_t refreshRate;
#if defined(J3D_OPENGL)
    SDL_PixelFormat format;
    SDL_DisplayMode pDisplayMode;
#endif
} StdVideoMode;

//static_assert(sizeof(StdVideoMode) == 80, "sizeof(StdVideoMode) == 80");

typedef struct sDevice3D
{
    int bHAL;
    int bTexturePerspectiveSupported;
    int hasZBuffer;
    int bColorkeyTextureSupported;
    int bAlphaTextureSupported;
    int bStippledShadeSupported;
    int bAlphaBlendSupported;
    int bSqareOnlyTexture;
    size_t minTexWidth;
    size_t minTexHeight;
    size_t maxTexWidth;
    size_t maxTexHeight;
    size_t maxVertexCount;
    char deviceName[128];
    char deviceDescription[128];
    size_t totalMemory;
    size_t availableMemory;
    tSysDevice3DDesc d3dDesc;
    GUID duid;
    int unknown146;
    int unknown147;
    int unknown148;
    int unknown149;
    int unknown150;
    int unknown151;
    int unknown152;
    int unknown153;
    int unknown154;
    int unknown155;
    int unknown156;
    int unknown157;
    int unknown158;
    int unknown159;
    int unknown160;
    int unknown161;
    int unknown162;
    int unknown163;
    int unknown164;
    int unknown165;
    int unknown166;
    int unknown167;
    int unknown168;
    int unknown169;
    int unknown170;
    int unknown171;
    int unknown172;
    int unknown173;
    int unknown174;
    int unknown175;
    int unknown176;
    int unknown177;
    int unknown178;
    int unknown179;
    int unknown180;
    int unknown181;
    int unknown182;
    int unknown183;
    int unknown184;
    int unknown185;
    int unknown186;
    int unknown187;
    int unknown188;
    int unknown189;
    int unknown190;
    int unknown191;
    int unknown192;
    int unknown193;
    int unknown194;
    int unknown195;
    int unknown196;
    int unknown197;
    int unknown198;
    int unknown199;
    int unknown200;
    int unknown201;
    int unknown202;
    int unknown203;
    int unknown204;
    int unknown205;
    int unknown206;
    int unknown207;
    int unknown208;
    int unknown209;
    int unknown210;
    int unknown211;
    int unknown212;
    int unknown213;
    int unknown214;
    int unknown215;
    int unknown216;
    int unknown217;
    bool bAnisotropicFilteringSupported; // Added
    bool bMipmapAutoGenSupported;        // Added
} Device3D;

//static_assert(sizeof(Device3D) == 872, "sizeof(Device3D) == 872");

typedef struct sDXStatus
{
    HRESULT code;
    const char* text;
} DXStatus;

static_assert(sizeof(DXStatus) == 8, "sizeof(DXStatus) == 8");

typedef struct sStdDisplayInfo
{
    StdDisplayDevice displayDevice;
    size_t numModes;
    StdVideoMode* aModes;
    size_t numDevices;
    Device3D* aDevices;
} StdDisplayInfo;

//static_assert(sizeof(StdDisplayInfo) == 688, "sizeof(StdDisplayInfo) == 688");

typedef struct sStdTextureFormat
{
    ColorInfo ci;
    int bColorKey;
    LPDDCOLORKEY pColorKey;
    tSysPixelFormat ddPixelFmt;
} StdTextureFormat;

//static_assert(sizeof(StdTextureFormat) == 96, "sizeof(StdTextureFormat) == 96");

typedef struct sStdDisplayEnvironment
{
    size_t numInfos;
    StdDisplayInfo* aDisplayInfos;
} StdDisplayEnvironment;

static_assert(sizeof(StdDisplayEnvironment) == 8, "sizeof(StdDisplayEnvironment) == 8");

typedef struct sStdControlAxis
{
    StdControlAxisFlag flags;
    int min;
    int max;
    int center;
    int deadzoneThreshold;
    float scale;
} StdControlAxis;

static_assert(sizeof(StdControlAxis) == 24, "sizeof(StdControlAxis) == 24");

typedef struct sStdFadeFactor
{
    int bEnabled;
    float factor;
} tStdFadeFactor;

static_assert(sizeof(tStdFadeFactor) == 8, "sizeof(tStdFadeFactor) == 8");

typedef struct sMemoryHeap
{
    tMemoryHeader header;
    void* pMemory;
} tMemoryHeap;

static_assert(sizeof(tMemoryHeap) == 36, "sizeof(tMemoryHeap) == 36");

typedef struct sMemoryState
{
    size_t totalBytes;
    size_t totalAllocs;
    size_t maxBytes;
    tMemoryHeader header;
} tMemoryState;

static_assert(sizeof(tMemoryState) == 44, "sizeof(tMemoryState) == 44");

typedef struct sMemoryBlockHeader
{
    size_t size;
    tMemoryBlock* pBlock;
} tMemoryBlockHeader;

static_assert(sizeof(tMemoryBlockHeader) == 8, "sizeof(tMemoryBlockHeader) == 8");

struct sMemoryBlock
{
    tMemoryBlockHeader* pFirst;       // Start of the block sequence
    tMemoryBlockHeader* pLargestFree; // Cache of the largest free block
    size_t availableMem;
    int bAllocated;
};

static_assert(sizeof(tMemoryBlock) == 16, "sizeof(tMemoryBlock) == 16");

typedef struct sStdCommPlayerInfo
{
    wchar_t aName[20];
    DPID id;
} StdCommPlayerInfo;

static_assert(sizeof(StdCommPlayerInfo) == 44, "sizeof(StdCommPlayerInfo) == 44");

typedef struct sStdCommSessionSettings
{
    GUID guid;
    int maxPlayers;
    int numCurrentPlayers;
    wchar_t aSessionName[64];
    char aSomething[64];
    wchar_t aPassword[64];
    int opt1;
    int opt2;
    int opt3;
} StdCommGame;

static_assert(sizeof(StdCommGame) == 356, "sizeof(StdCommGame) == 356");

J3D_EXTERN_C_END
#endif //STD_TYPES_H
