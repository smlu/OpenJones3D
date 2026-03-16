#ifndef RDROID_TYPES_H
#define RDROID_TYPES_H
#include <stdint.h>
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define RDCAMERA_MAX_LIGHTS 128u

#define RDCLIP_MAXWORKVERTS  80u
#define RDQCLIP_MAXWORKVERTS 80u

#define RDKEYFRAME_MAX_MARKERS 16u

#define RDPUPPET_MAX_TRACKS   8u
#define RDPUPPET_MAX_KFNODES  64u

typedef enum erdThingType
{
    RD_THING_NONE     = 0,
    RD_THING_MODEL3   = 1,
    RD_THING_CAMERA   = 2,
    RD_THING_LIGHT    = 3,
    RD_THING_SPRITE3  = 4,
    RD_THING_PARTICLE = 5,
    RD_THING_POLYLINE = 6,
} rdThingType;

typedef enum erdLightMode
{
    RD_LIGHTING_NONE    = 0,
    RD_LIGHTING_LIT     = 1,
    RD_LIGHTING_DIFFUSE = 2,
    RD_LIGHTING_GOURAUD = 3,
} rdLightMode;

typedef enum eRdFrustumCull
{
    RDFRUSTUMCULL_INSIDE    = 0,
    RDFRUSTUMCULL_INTERSECT = 1,
    RDFRUSTUMCULL_OUTSIDE   = 2,
} RdFrustumCull;

typedef enum erdFaceFlags J3D_ENUM_TYPE(int32_t)
{
    RD_FF_DOUBLE_SIDED       = 0x1,
    RD_FF_TEX_TRANSLUCENT    = 0x2,
    RD_FF_TEX_CLAMP_X        = 0x4,
    RD_FF_TEX_CLAMP_Y        = 0x8,
    RD_FF_TEX_FILTER_NEAREST = 0x10,
    RD_FF_ZWRITE_DISABLED    = 0x20,
    RD_FF_3DO_LEDGE          = 0x40,
    RD_FF_UNKNOWN_80         = 0x80,
    RD_FF_FOG_ENABLED        = 0x100,
    RD_FF_3DO_WHIP_AIM       = 0x200,
    RD_FF_BLEND_ENABLED      = 0x400 //added
} rdFaceFlags;

typedef enum erdGeometryMode
{
    RD_GEOMETRY_NONE      = 0,
    RD_GEOMETRY_VERTEX    = 1,
    RD_GEOMETRY_WIREFRAME = 2,
    RD_GEOMETRY_SOLID     = 3,
    RD_GEOMETRY_FULL      = 4,
} rdGeometryMode;

typedef enum erdCameraProjType
{
    RDCAMERA_PROJECT_ORTHO       = 0,
    RDCAMERA_PROJECT_PERSPECTIVE = 1,
} rdCameraProjType;

typedef enum erdPuppetTrackStatus
{
    RDPUPPET_TRACK_UNKNOWN_1                   = 0x1,
    RDPUPPET_TRACK_PLAYING                     = 0x2,
    RDPUPPET_TRACK_FADEIN                      = 0x4,
    RDPUPPET_TRACK_FADEOUT                     = 0x8,
    RDPUPPET_TRACK_PAUSED                      = 0x10,
    RDPUPPET_TRACK_NOLOOP                      = 0x20,
    RDPUPPET_TRACK_PAUSE_ON_LAST_FRAME         = 0x40,
    RDPUPPET_TRACK_FADEOUT_NOLOOP              = 0x80,
    RDPUPPET_TRACK_FADEOUT_PAUSE_ON_LAST_FRAME = 0x100,
    RDPUPPET_TRACK_NOISESET                    = 0x1000
} rdPuppetTrackStatus;

typedef enum erdKeyMarkerType
{
    RDKEYMARKER_LEFTFOOT             = 1,
    RDKEYMARKER_RIGHTFOOT            = 2,
    RDKEYMARKER_ATTACK               = 3,
    RDKEYMARKER_SWING                = 4,
    RDKEYMARKER_SWINGFINISH          = 5,
    RDKEYMARKER_SWIMLEFT             = 6,
    RDKEYMARKER_TREAD                = 7,
    RDKEYMARKER_RUNLEFTFOOT          = 8,
    RDKEYMARKER_RUNRIGHTFOOT         = 9,
    RDKEYMARKER_DIED                 = 10,
    RDKEYMARKER_JUMP                 = 11,
    RDKEYMARKER_JUMPUP               = 12,
    RDKEYMARKER_SWIMRIGHT            = 13,
    RDKEYMARKER_DUCK                 = 14,
    RDKEYMARKER_CLIMB                = 15,
    RDKEYMARKER_ACTIVATE             = 16,
    RDKEYMARKER_CRAWL                = 17,
    RDKEYMARKER_RUNJUMPLAND          = 18,
    RDKEYMARKER_ACTIVATERIGHTARM     = 19,
    RDKEYMARKER_ACTIVATERIGHTARMREST = 20,
    RDKEYMARKER_PLACERIGHTARM        = 21,
    RDKEYMARKER_PLACERIGHTARMREST    = 22,
    RDKEYMARKER_REACHRIGHTARM        = 23,
    RDKEYMARKER_REACHRIGHTARMREST    = 24,
    RDKEYMARKER_PICKUP               = 25,
    RDKEYMARKER_DROP                 = 26,
    RDKEYMARKER_MOVE                 = 27,
    RDKEYMARKER_INVENTORYPULL        = 28,
    RDKEYMARKER_INVENTORYPUT         = 29,
    RDKEYMARKER_ATTACKFINISH         = 30,
    RDKEYMARKER_TURNOFF              = 31,
    RDKEYMARKER_ROW                  = 32,
    RDKEYMARKER_ROWFINISH            = 33,
    RDKEYMARKER_LEFTHAND             = 34,
    RDKEYMARKER_RIGHTHAND            = 35,
} rdKeyMarkerType;

typedef enum erdKeyframeFlags
{
    RDKEYFRAME_PUPPET_CONTROLLED   = 0x1,
    RDKEYFRAME_NOLOOP              = 0x2,
    RDKEYFRAME_PAUSE_ON_LAST_FRAME = 0x4,
    RDKEYFRAME_RESTART_ACTIVE      = 0x8,
    RDKEYFRAME_DISABLE_FADEIN      = 0x10,
    RDKEYFRAME_FADEOUT_NOLOOP      = 0x20,
    RDKEYFRAME_FORCEMOVE           = 0x40,
} rdKeyframeFlags;

typedef enum erdRoidFlags
{
    RDROID_BACKFACE_CULLING_ENABLED = 0x1,
    RDROID_USE_AMBIENT_CAMERA_LIGHT = 0x2,
} rdRoidFlags;

typedef enum erdClipOutcode
{
    RDCLIP_OUTCODE_LEFT   = 0x1,
    RDCLIP_OUTCODE_RIGHT  = 0x10,
    RDCLIP_OUTCODE_BOTTOM = 0x100,
    RDCLIP_OUTCODE_TOP    = 0x1000,
} rdClipOutcode;

// New flags not from OG engine
typedef enum eRdPolyLineFlags
{
    RDPOLYLINE_UVTILE   = 0x01, // tile texture instead of stretch
    RDPOLYLINE_UVROTATE = 0x02  // rotate uvs for 90 degrees
} rdPolylineFlags;


typedef struct srdCacheProcEntry rdCacheProcEntry;
typedef struct srdFace rdFace;
typedef struct srdKeyframe rdKeyframe;
typedef struct srdMaterial rdMaterial;
typedef struct srdModel3 rdModel3;
typedef struct srdModel3HNode rdModel3HNode;
typedef struct srdVector3 rdVector3;
typedef struct srdVector4 rdVector4;
typedef struct sSithThing SithThing;
typedef struct srdThing rdThing;

typedef void (J3DAPI*rdCameraProjectFunc)(rdVector3* dest, const rdVector3* src);
typedef void (J3DAPI*rdCameraProjectListFunc)(rdVector3* aDest, const rdVector3* aSrc, size_t size);
typedef void (J3DAPI*rdPuppetTrackCallback)(SithThing* pThing, int trackNum, rdKeyMarkerType markerType);
typedef rdKeyframe* (J3DAPI*rdKeyframeLoadFunc)(const char* pFilename);
typedef void (J3DAPI*rdKeyframeUnloadFunc)(rdKeyframe* pKeyframe);
typedef rdModel3* (J3DAPI*rdModel3LoaderFunc)(const char* pFilename, int bSkipLoadingDefaultModel);
typedef void (J3DAPI*rdModel3UnloaderFunc)(rdModel3*);

typedef rdMaterial* (J3DAPI*rdMaterialLoaderFunc)(const char* pFilename);
typedef void (J3DAPI*rdMaterialUnloaderFunc)(rdMaterial* pMaterial);
typedef void (J3DAPI*rdModel3DrawFaceFunc)(const rdFace* pFace, const rdVector3* pTransformedVertices, int bBackFace, const rdVector4* pMeshColor);
typedef int (J3DAPI*rdCacheSortFunc)(const rdCacheProcEntry*, const rdCacheProcEntry*);

struct srdMaterial
{
    char aName[64];
    int num;
    StdColorFormatType formatType;
    uint32_t width;
    uint32_t height;
    size_t curCelNum;
    size_t numCels;
    tSystemTexture* aTextures;
};

static_assert(sizeof(rdMaterial) == 92, "sizeof(rdMaterial) == 92");

typedef struct srdGlyphMetrics
{
    float left;
    float top;
    float right;
    float bottom;
    int baselineOriginY;
    int baselineOriginX;
} rdGlyphMetrics;

static_assert(sizeof(rdGlyphMetrics) == 24, "sizeof(rdGlyphMetrics) == 24");

typedef struct srdFont
{
    rdGlyphMetrics* aGlyphs;
    int lineSpacing;
    char* pName;
    int32_t fontSize;
    rdMaterial* pMaterial;
} rdFont;

static_assert(sizeof(rdFont) == 20, "sizeof(rdFont) == 20");

typedef struct srdMatHeader // Originally was probably named tMaterialHeader
{
    char magic[4];
    int version;
    int type;
    uint32_t numCels;
    uint32_t numTextures;
    ColorInfo colorInfo;
} rdMatHeader;

static_assert(sizeof(rdMatHeader) == 76, "sizeof(rdMatHeader) == 76");

struct srdVector3
{
    union
    {
        float x;
        float red;
        float pitch;
    };

    union
    {
        float y;
        float green;
        float yaw;
    };

    union
    {
        float z;
        float blue;
        float roll;
    };
};

static_assert(sizeof(struct srdVector3) == 12, "sizeof(struct srdVector3) == 12");

typedef struct srdMatrix34
{
    rdVector3 rvec;
    rdVector3 lvec;
    rdVector3 uvec;
    rdVector3 dvec;
} rdMatrix34;

static_assert(sizeof(rdMatrix34) == 48, "sizeof(rdMatrix34) == 48");

typedef struct srdClipFrustum
{
    int bClipFar;
    float nearPlane;
    float farPlane;
    float orthoLeftPlane;
    float orthoTopPlane;
    float orthoRightPlane;
    float orthoBottomPlane;
    float topPlane;
    float bottomPlane;
    float leftPlane;
    float rightPlane;
    rdVector3 leftPlaneNormal;
    rdVector3 rightPlaneNormal;
    rdVector3 topPlaneNormal;
    rdVector3 bottomPlaneNormal;
} rdClipFrustum;

static_assert(sizeof(rdClipFrustum) == 92, "sizeof(rdClipFrustum) == 92");

struct srdVector4
{
    union
    {
        float x;
        float red;
    };

    union
    {
        float y;
        float green;
    };

    union
    {
        float z;
        float blue;
    };

    union
    {
        float w;
        float alpha;
    };
};

static_assert(sizeof(struct srdVector4) == sizeof(rdVector3) + sizeof(float), "sizeof(struct srdVector4) == sizeof(rdVector3) + sizeof(float)");

typedef struct srdVector2
{
    float x;
    float y;
} rdVector2;

static_assert(sizeof(rdVector2) == 8, "sizeof(rdVector2) == 8");

typedef struct srdCanvas
{
    int flags;
    tVBuffer* pVBuffer;
    rdVector2 center;
    StdRect rect;
} rdCanvas;

typedef struct srdLight
{
    int num;
    int type;
    int bEnabled;
    rdVector3 direction;
    rdVector4 color;
    float minRadius;
    float maxRadius;
} rdLight;

typedef struct srdCamera
{
    rdCameraProjType projectType;
    rdCanvas* pCanvas;
    rdMatrix34 viewMatrix; // Camera view matrix (for converting world to view space)
    float fov;
    float focalLength;
    float aspectRatio;
    float orthoScale;
    rdClipFrustum* pFrustum;
    rdCameraProjectFunc pfProject;
    rdCameraProjectListFunc pfProjectList;
    float invNearClipPlane;
    float invFarClipPlane;
    rdVector4 ambientLight;
    size_t numLights;
    rdLight* aLights[RDCAMERA_MAX_LIGHTS];
    rdVector3 aLightPositions[RDCAMERA_MAX_LIGHTS];
    float attenuationMin;
    float attenuationMax;
} rdCamera;

static_assert(sizeof(rdCamera) == 2168, "sizeof(rdCamera) == 2168");

struct srdModel3HNode
{
    char aName[128];
    size_t num;
    int type;
    int meshIdx;
    rdModel3HNode* pParent;
    size_t numChildren;
    rdModel3HNode* pChild;
    rdModel3HNode* pSibling;
    rdVector3 pivot;
    rdVector3 pos;
    rdVector3 pyr;
    rdMatrix34 meshOrient;
};

static_assert(sizeof(rdModel3HNode) == 240, "sizeof(rdModel3HNode) == 240");

struct srdFace
{
    size_t num;
    rdFaceFlags flags;
    rdGeometryMode geometryMode;
    rdLightMode lightingMode;
    size_t numVertices;
    int* aVertices;
    int* aTexVertices;
    rdMaterial* pMaterial;
    int matCelNum;
    rdVector2 texVertOffset;
    rdVector4 extraLight;
    rdVector3 normal;
};

static_assert(sizeof(rdFace) == 72, "sizeof(rdFace) == 72");

typedef struct srdModel3Mesh
{
    char name[64];
    size_t num;
    rdGeometryMode geoMode;
    rdLightMode lightMode;
    rdVector3* apVertices;
    rdVector2* apTexVertices;
    rdVector4* aVertColors;
    rdVector4* aLightIntensities;
    rdVector4 meshColor;
    rdFace* aFaces;
    rdVector3* aVertNormals;
    size_t numVertices;
    size_t numTexVertices;
    size_t numFaces;
    float radius;
    int someFaceFlags;
} rdModel3Mesh;

static_assert(sizeof(rdModel3Mesh) == 136, "sizeof(rdModel3Mesh) == 136");

typedef struct srdModel3GeoSet
{
    size_t numMeshes;
    rdModel3Mesh* aMeshes;
} rdModel3GeoSet;

static_assert(sizeof(rdModel3GeoSet) == 8, "sizeof(rdModel3GeoSet) == 8");

struct srdModel3
{
    char aName[64];
    size_t num;
    rdModel3GeoSet aGeos[4];
    size_t numGeos;
    rdMaterial** apMaterials;
    size_t numMaterials;
    size_t curGeoNum;
    size_t numHNodes;
    rdModel3HNode* aHierarchyNodes;
    float radius;
    float size;
    rdVector3 insertOffset;
};

static_assert(sizeof(rdModel3) == 144, "sizeof(rdModel3) == 144");

typedef struct srdPolyline
{
    char aName[64];
    float length;
    float baseRadius;
    float tipRadius;
    rdGeometryMode geoMode;
    rdLightMode lightMode;
    rdFace face;
    rdVector2* apUVs;

    rdPolylineFlags flags; // Added: new field
} rdPolyline;

typedef struct srdParticle
{
    char aName[64];
    rdLightMode lightningMode;
    size_t numVertices;
    rdVector3* aVerticies;
    int* aVertMatCelNums;
    rdVector4* aExtraLights;
    float size;
    float sizeHalf;
    rdMaterial* pMaterial;
    float radius;
    rdVector3 insertOffset;
} rdParticle;

static_assert(sizeof(rdParticle) == 112, "sizeof(rdParticle) == 112");

typedef struct srdSprite3
{
    char aName[64];
    int type;
    float radius;
    int unknown18;
    int unknown19;
    float width;
    float height;
    float widthHalf;
    float heightHalf;
    rdFace face;
    rdVector2* aTexVerts;
    rdVector3 offset;
    float rollAngle;
} rdSprite3;

typedef union srdThingData
{
    rdModel3* pModel3;
    rdPolyline* pPolyline;
    rdSprite3* pSprite3;
    rdParticle* pParticle;
    rdCamera* pCamera;
    rdLight* pLight;
} rdThingData;
static_assert(sizeof(rdThingData) == 4, "sizeof(rdThingData) == 4");

typedef struct srdKeyframeNodeEntry
{
    float frame;
    uint32_t flags;
    rdVector3 pos;
    rdVector3 rot;
    rdVector3 dpos;
    rdVector3 drot;
} rdKeyframeNodeEntry;

static_assert(sizeof(rdKeyframeNodeEntry) == 56, "sizeof(rdKeyframeNodeEntry) == 56");

typedef struct srdKeyframeNode
{
    char aMeshName[64];
    int nodeNum;
    size_t numEntries;
    rdKeyframeNodeEntry* aEntries;
} rdKeyframeNode;

static_assert(sizeof(rdKeyframeNode) == 76, "sizeof(rdKeyframeNode) == 76");

struct srdKeyframe
{
    char aName[64];
    int idx;
    rdKeyframeFlags flags;
    int type;
    float fps;
    size_t numFrames;
    size_t numJoints;
    rdKeyframeNode* aNodes;
    size_t numMarkers;
    float aMarkerFrames[RDKEYFRAME_MAX_MARKERS];
    rdKeyMarkerType aMarkerTypes[RDKEYFRAME_MAX_MARKERS];
};

static_assert(sizeof(rdKeyframe) == 224, "sizeof(rdKeyframe) == 224");

typedef struct srdPuppetTrack
{
    rdPuppetTrackStatus status;
    float playbackSpeed;    // Altered: Repurposed to playback speed of type float. Originally the usage was unknown the var might be 32 bit integer or 32 bit float
    int lowPriority;
    int highPriority;
    float fps;
    float noise;
    float blendWeight;       // Blend/fade level (0-1)
    float fadeRate;          // Rate of fade in blend weight per second
    size_t aCurKfNodeEntryNums[RDPUPPET_MAX_KFNODES]; // TODO: make this field rt variable
    float curFrame;
    float prevFrame;
    rdKeyframe* pKFTrack;
    rdPuppetTrackCallback pfCallback;
    uint32_t guid;
} rdPuppetTrack;

static_assert(sizeof(rdPuppetTrack) == 308, "sizeof(rdPuppetTrack) == 308");

typedef struct srdPuppet
{
    bool bPaused; // was int
    rdThing* pThing;
    rdPuppetTrack aTracks[RDPUPPET_MAX_TRACKS];
} rdPuppet;

static_assert(sizeof(rdPuppet) == 2472, "sizeof(rdPuppet) == 2472");

struct srdThing
{
    rdThingType type;
    rdThingData data;
    rdPuppet* pPuppet;
    int bSkipBuildingJoints;
    int rdFrameNum;
    rdMatrix34* paJointMatrices;
    rdVector3* apTweakedAngles;
    uint32_t* paJointAmputationFlags;
    int matCelNum;
    int geosetNum;
    rdLightMode lightMode;
    RdFrustumCull frustumCull;
    SithThing* pThing;
};

typedef struct srdRGB
{
    int red;
    int green;
    int blue;
} rdRGB;

typedef struct srdBox3
{
    rdVector3 v0;
    rdVector3 v1;
} rdBox3;

typedef struct srdMatRecordHeader
{
    int type;
    int TransparentColorNum;
    int Unknown1;
    int Unknown2;
    int Unknown3;
    int Unknown4;
    int Unknown5;
    int Unknown6;
    float Unknown7;
    uint32_t textureNum;
} rdMatRecordHeader;

static_assert(sizeof(rdMatRecordHeader) == 40, "sizeof(rdMatRecordHeader) == 40");

typedef struct srdWallLine
{
    float startX;
    float startY;
    float deltaX;
    float deltaY;
    int unknown4;
    rdMaterial* pMaterial;
    rdVector4 color;
} rdWallLine;

struct srdCacheProcEntry
{
    rdFaceFlags flags;
    rdLightMode lightingMode;
    size_t numVertices;
    LPD3DTLVERTEX aVertices;
    rdVector4* aVertIntensities;
    rdMaterial* pMaterial;
    int32_t matCelNum;
    rdVector4 extraLight;
    float distance;
#ifdef J3D_OPENGL
    std3DVertexSpace vertexSpace;
    GLShaderProgram* pShader;
    bool bGPULighting;
#endif
};

//static_assert(sizeof(rdCacheProcEntry) == 48, "sizeof(rdCacheProcEntry) == 48");

typedef struct srdMatCelInfo
{
    int type;
    int colorIdx;
    int unknown1;
    int unknown2;
    int unknown3;
    float unknown4;
} rdMatCelInfo;

static_assert(sizeof(rdMatCelInfo) == 24, "sizeof(rdMatCelInfo) == 24");

typedef struct srdMatTextureHeader
{
    int width;
    int height;
    int transparentBool;
    int unknown1;
    int unknown2;
    uint32_t numMipLevels;
} rdMatTextureHeader;

static_assert(sizeof(rdMatTextureHeader) == 24, "sizeof(rdMatTextureHeader) == 24");


typedef struct srdMatTextureCelInfo
{
    int unknown0;
    int unknown1;
    float unknown2;
    int celNum;
} rdMatTextureCelInfo;

typedef struct srdWallpaper
{
    char aName[64];
    rdCamera* pCamera;
    rdMaterial aMaterials[6];
    int aMatCelNums[6];
} rdWallpaper;

typedef struct srdPrimit3
{
    size_t numVertices;
    int* aVertIdxs;
    int* aTexVertIdxs;
    rdVector3* aVertices;
    rdVector2* aTexVertices;
    rdVector4* aVertLights;
    rdVector4* aVertIntensities;
    int unknown2;
} rdPrimit3;

#ifdef J3D_OPENGL
typedef enum erdDrawType
{
    RD_DRAW_GEOMETRY = 0,
    RD_DRAW_MODEL    = 1,
    RD_DRAW_SPRITE   = 2,
    RD_DRAW_PARTICLE = 3,
    RD_DRAW_POLYLINE = 4,
    RD_DRAW_SHADOW   = 5
} rdDrawType;

typedef struct srdGeoFacePayload
{
    size_t faceNum;
    std3DVertexSpace vertexSpace;
} rdGeoFacePayload;

typedef struct srdModelFacePayload
{
    size_t faceNum;
    const rdMatrix34* modelMatrix;
    rdLight sectorLight;
    rdVector3 lightPosition;
} rdModelFacePayload;

typedef struct srdSpritePayload
{
    rdMatrix34 modelMatrix;
    rdVector3 spriteOffset;
    rdVector2 spriteSize;

    int spriteType;
} rdSpritePayload;

typedef struct srdParticlePayload
{
    rdVector3 particlePos;
    float particleHalfSize;
} rdParticlePayload;

typedef struct sPolyLinePayload
{
    rdVector3 vertices[4];
    rdVector2 texCoords[4];
} rdPolyLinePayload, rdShadowPayload;

typedef struct srdPayload
{
    rdDrawType type;

    rdFaceFlags flags;
    rdLightMode lightingMode;
    rdMaterial* pMaterial;
    int32_t matCelNum;
    rdVector4 extraLight;
    uint64_t sortKey;

    Std3DRenderState rdFlags;
    GLShaderProgram* pShader;
    tSysTexture* pTex;


    float distance;

    void* payload;
} rdPayload;
#endif

J3D_EXTERN_C_END
#endif //RDROID_TYPES_H
