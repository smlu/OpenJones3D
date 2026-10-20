// Frame.c: the animation frame service (`ANIMFRAME=insane`): parses and decodes frames through the frame-object decoders (original 0x004E4230-0x004E4D50).
#include <smush/insane/Animate/Frame.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>

#include <stddef.h>
#include <string.h>
#include <smush/insane/File/Framestream.h>

// The debug service's print function is printf-style
typedef void (*AnimFrameDebugPrintFunc)(const char* pFormat, ...);

#define ANIMFRAME_DEBUG_PARSE   0x80000
#define ANIMFRAME_DEBUG_BADHEAD 0x20000
#define ANIMFRAME_DEBUG_UNKNOWN 0x40000

static int animframe_lastDecoder = -1;
static int animframe_cursor      = 8;
static LecModule animframe_module = { animframe_Register, animframe_Unregister, animframe_Startup, animframe_Shutdown };
static AnimFrameTable animframe_interface = {
    animframe_ResetDecoders, animframe_Parse, animframe_Decode, animframe_Draw, animframe_Load, animframe_Fetch
};
static AnimFrameDebug animframe_debug = { animframe_DebugName, animframe_SetDebug, animframe_GetDebug };

static MemoryDebugInterface* animframe_pMemory;
static void* animframe_pBitmap;
static LecShare* animframe_pShare;
static LecDebug* animframe_pDebug;
static AnimFrameDisplayService* animframe_pDisplay;

static int animframe_flags[ANIMFRAME_MAXDECODERS]             = { 0 };
static FlobDecoder* animframe_decoders[ANIMFRAME_MAXDECODERS] = { 0 };
static int animframe_tags[ANIMFRAME_MAXDECODERS]              = { 0 };
static void* animframe_pParam                                 = NULL;
static char animframe_palette[0x400]                          = { 0 };
static int animframe_debugMask                                = 0;
static int animframe_count                                    = 0;
static uint8_t* animframe_pOverlay                            = NULL;
static uint8_t* animframe_pPreload                            = NULL;
static int animframe_overlaySize                              = 0;
static int animframe_preloadSize                              = 0;
static int animframe_loaded                                   = 0;
static int animframe_fade                                     = 0;
static int animframe_lastFragment                             = 0;
static int animframe_fragTag                                  = 0;

void Frame_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(animframe_Register);
    J3D_HOOKFUNC(animframe_Unregister);
    J3D_HOOKFUNC(animframe_Startup);
    J3D_HOOKFUNC(animframe_Shutdown);
    J3D_HOOKFUNC(animframe_ResetDecoders);
    J3D_HOOKFUNC(animframe_Parse);
    J3D_HOOKFUNC(animframe_Decode);
    J3D_HOOKFUNC(animframe_Load);
    J3D_HOOKFUNC(animframe_Fetch);
    J3D_HOOKFUNC(animframe_Draw);
    J3D_HOOKFUNC(animframe_DebugName);
    J3D_HOOKFUNC(animframe_SetDebug);
    J3D_HOOKFUNC(animframe_GetDebug);
}

void Frame_ResetGlobals(void)
{
    animframe_lastDecoder = -1;
    animframe_cursor      = 8;
    animframe_pMemory     = NULL;
    animframe_pBitmap     = NULL;
    animframe_pShare      = NULL;
    animframe_pDebug      = NULL;
    animframe_pDisplay    = NULL;
    memset(animframe_flags, 0, sizeof(animframe_flags));
    memset(animframe_decoders, 0, sizeof(animframe_decoders));
    memset(animframe_tags, 0, sizeof(animframe_tags));
    animframe_pParam = NULL;
    memset(animframe_palette, 0, sizeof(animframe_palette));
    animframe_debugMask    = 0;
    animframe_count        = 0;
    animframe_pOverlay     = NULL;
    animframe_pPreload     = NULL;
    animframe_overlaySize  = 0;
    animframe_preloadSize  = 0;
    animframe_loaded       = 0;
    animframe_fade         = 0;
    animframe_lastFragment = 0;
    animframe_fragTag      = 0;
}

int J3DAPI animframe_Register(LecShare* pShare, void* pParam)
{
    animframe_pShare = pShare;
    animframe_pParam = pParam;
    pShare->Publish("LECMODULE=insane_animframe", &animframe_module);
    animframe_pShare->Publish("ANIMFRAME=insane", &animframe_interface);
    animframe_pShare->Publish("DEBUG=animframe", &animframe_debug);
    return 0;
}

void animframe_Unregister(void)
{
    animframe_pShare->Unpublish("ANIMFRAME=insane");
    animframe_pShare->Unpublish("LECMODULE=insane_animframe");
    animframe_pShare->Unpublish("DEBUG=animframe");
}

int animframe_Startup(void)
{
    animframe_pBitmap = animframe_pShare->Subscribe("BITMAP.buffers", animframe_pParam);
    animframe_pMemory = (MemoryDebugInterface*)animframe_pShare->Subscribe("MEMORY.workspace", animframe_pParam);
    if ( animframe_pBitmap == NULL )
    {
        animframe_pShare->ReportError("AnimFrame Module: Cannot allocate bitmap services.");
        return -1;
    }

    if ( animframe_pMemory == NULL )
    {
        animframe_pShare->ReportError("AnimFrame Module: Cannot allocate memory services.");
        return -1;
    }

    animframe_pDebug   = (LecDebug*)animframe_pShare->Subscribe("VIRTHERC.debug", animframe_pParam);
    animframe_pDisplay = (AnimFrameDisplayService*)animframe_pShare->Subscribe("VIDEO.display", animframe_pParam);
    if ( animframe_overlaySize )
    {
        animframe_pOverlay = animframe_pMemory->Alloc(animframe_overlaySize, INSANE_FILE, __LINE__);
        int rc = animframe_pOverlay ? 0 : -1;
        if ( rc )
        {
            animframe_pShare->ReportError("AnimFrame Module: Can't allocate overlay buffer (%ld bytes)", animframe_overlaySize);
            return rc;
        }
    }

    if ( animframe_preloadSize )
    {
        animframe_pPreload = animframe_pMemory->Alloc(animframe_preloadSize, INSANE_FILE, __LINE__);
        int rc = animframe_pPreload ? 0 : -1;
        if ( rc )
        {
            animframe_pShare->ReportError("AnimFrame Module: Can't allocate preload buffer (%ld bytes)", animframe_preloadSize);
            return rc;
        }
    }

    memset(animframe_decoders, 0, sizeof(animframe_decoders));
    animframe_count = animframe_pShare->Count("FLOBDEC");
    if ( animframe_count > ANIMFRAME_MAXDECODERS )
    {
        animframe_count = ANIMFRAME_MAXDECODERS;
    }

    for ( int i = 0; i < animframe_count; i++ )
    {
        void* pName = animframe_pShare->Enumerate("FLOBDEC", i);
        if ( pName )
        {
            // TODO: [BUG] A failed subscription is dereferenced below
            animframe_decoders[i] = (FlobDecoder*)animframe_pShare->Subscribe("FLOBDEC.temp", pName);
            animframe_tags[i]     = animframe_decoders[i]->GetTag();
            animframe_flags[i]    = animframe_decoders[i]->GetFlags();
        }
    }

    return 0;
}

void animframe_Shutdown(void)
{
    animframe_pShare->Unsubscribe(animframe_pBitmap);
    animframe_pShare->Unsubscribe(animframe_pMemory);
    if ( animframe_pDisplay )
    {
        animframe_pShare->Unsubscribe(animframe_pDisplay);
    }

    if ( animframe_pDebug )
    {
        animframe_pShare->Unsubscribe(animframe_pDebug);
    }

    if ( animframe_pOverlay )
    {
        animframe_pMemory->Free(animframe_pOverlay, INSANE_FILE, __LINE__);
        animframe_pOverlay = NULL;
    }

    if ( animframe_pPreload )
    {
        animframe_pMemory->Free(animframe_pPreload, INSANE_FILE, __LINE__);
        animframe_pPreload = NULL;
    }

    for ( int i = 0; i < animframe_count; i++ )
    {
        if ( animframe_decoders[i] )
        {
            animframe_pShare->Unsubscribe(animframe_decoders[i]);
        }

        animframe_decoders[i] = NULL;
    }

    animframe_count = 0;
}

void animframe_ResetDecoders(void)
{
    for ( int i = 0; i < animframe_count; i++ )
    {
        if ( animframe_decoders[i] )
        {
            animframe_decoders[i]->Reset();
        }
    }
}

int J3DAPI animframe_Parse(const uint8_t* pData, int size)
{
    while ( size > 4 )
    {
        uint32_t tag = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
        uint32_t len = (pData[4] << 24) | (pData[5] << 16) | (pData[6] << 8) | pData[7];

        for ( int i = 0; i < animframe_count; i++ )
        {
            if ( tag != 0 && tag == (uint32_t)animframe_tags[i] && animframe_decoders[i] )
            {
                animframe_decoders[i]->Parse(pData + 8, len);
            }
        }

        size += -8 - len;
        pData += 8 + len;
        if ( len & 1 )
        {
            pData++;
            size--;
        }
    }

    return 0;
}

uint32_t J3DAPI animframe_Decode(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, uint32_t flags, int a9, int a10, const uint8_t* pData, int size)
{
    int seen        = 0;
    uint32_t result = 0;
    int mode        = (flags & 0x30) | ((flags >> 13) & 4);
    int i;

    while ( size > 4 )
    {
        uint32_t tag = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
        uint32_t len = (pData[4] << 24) | (pData[5] << 16) | (pData[6] << 8) | pData[7];

        if ( animframe_pDebug && (animframe_debugMask & ANIMFRAME_DEBUG_PARSE) )
        {
            ((AnimFrameDebugPrintFunc)animframe_pDebug->Print)("Parsing Block '%c%c%c%c' (%ld)\n", pData[0], pData[1], pData[2], pData[3], len);
        }

        for ( i = 0; i < animframe_count; i++ )
        {
            if ( tag != 0 && tag == (uint32_t)animframe_tags[i] && animframe_decoders[i] )
            {
                if ( seen && animframe_pOverlay && (animframe_flags[i] & 0x10) )
                {
                    if ( len + 8 < (uint32_t)animframe_overlaySize )
                    {
                        framestream_Copy(animframe_pOverlay, pData, len + 8);
                        animframe_lastDecoder = i;
                    }
                    else
                    {
                        animframe_lastDecoder = -1;
                    }

                    seen = 0;
                }

                uint32_t ret = animframe_decoders[i]->Decode(pBitmap, a2, a3, a4, pPalette, pData + 8, len, mode, a9, a10);
                if ( ret & 1 )
                {
                    *pOutA |= 1;
                }

                if ( ret & 2 )
                {
                    *pOutB |= 1;
                }

                if ( ret & 4 )
                {
                    *pOutB |= 2;
                }

                if ( ret & 0x80 )
                {
                    result |= 2;
                }

                break;
            }
        }

        pData += 8;
        if ( i >= animframe_count )
        {
            switch ( tag )
            {
                case 'load':
                    animframe_Load((const int16_t*)pData, len);
                    break;

                case 'ftch':
                    if ( animframe_pOverlay && animframe_lastDecoder >= 0 )
                    {
                        uint32_t ret = animframe_decoders[animframe_lastDecoder]->Decode(pBitmap, a2, a3, a4, pPalette, animframe_pOverlay + 8,
                            (animframe_pOverlay[4] << 24) | (animframe_pOverlay[5] << 16) | (animframe_pOverlay[6] << 8) | animframe_pOverlay[7],
                            mode, a9, a10);
                        if ( ret & 1 )
                        {
                            *pOutA |= 1;
                        }

                        if ( ret & 2 )
                        {
                            *pOutB |= 1;
                        }

                        if ( ret & 4 )
                        {
                            *pOutB |= 2;
                        }
                    }

                    break;

                case 'npal':
                    if ( *(const int16_t*)(pData + 0xC) == 0 )
                    {
                        framestream_Copy((char*)pPalette + (*(const int16_t*)(pData + 0xE) << 4), pData + 0x12, *(const int16_t*)(pData + 0x10) << 2);
                        *pOutB |= *(const int*)(pData + 8);
                        animframe_fade = 0;
                    }
                    else
                    {
                        framestream_Copy(animframe_palette, pPalette, 0x400);
                        framestream_Copy(animframe_palette + (*(const int16_t*)(pData + 0xE) << 4), pData + 0x12, *(const int16_t*)(pData + 0x10) << 2);
                        animframe_fade = *(const int16_t*)(pData + 0xC);
                    }

                    break;

                case 'stor':
                    seen = 1;
                    break;

                default:
                    // TODO: [BUG] pData is already past the chunk header, so the messages print the chunk's size bytes instead of its tag
                    if ( animframe_pDebug && (animframe_debugMask & ANIMFRAME_DEBUG_UNKNOWN) )
                    {
                        ((AnimFrameDebugPrintFunc)animframe_pDebug->Print)("Found Unknown Block '%c%c%c%c'\n", pData[-4], pData[-3], pData[-2], pData[-1]);
                    }

                    // The tag characters are expected to be in the range '0'..'z'
                    if ( (uint8_t)tag < '0' || (uint8_t)tag > 'z' || (uint8_t)(tag >> 8) < '0' || (uint8_t)(tag >> 8) > 'z'
                        || (uint8_t)(tag >> 16) < '0' || (uint8_t)(tag >> 16) > 'z' || (uint8_t)(tag >> 24) < '0' || (uint8_t)(tag >> 24) > 'z' )
                    {
                        if ( animframe_pDebug && (animframe_debugMask & ANIMFRAME_DEBUG_BADHEAD) )
                        {
                            ((AnimFrameDebugPrintFunc)animframe_pDebug->Print)("Bad Header Found '%c%c%c%c'\n", pData[-4], pData[-3], pData[-2], pData[-1]);
                        }

                        return result | 0x8000;
                    }

                    break;
            }
        }

        pData += len;
        size -= len + 8;
        if ( len & 1 )
        {
            pData++;
            size--;
        }
    }

    if ( animframe_fade )
    {
        uint32_t* pCur = (uint32_t*)pPalette;
        uint32_t* pTgt = (uint32_t*)animframe_palette;

        *pOutB |= 1;
        for ( i = 0; i < 256; i++ )
        {
            uint32_t c  = pCur[i];
            uint32_t b0 = c & 0xFF;
            uint32_t b1 = (c >> 8) & 0xFF;
            uint32_t b2 = (c >> 16) & 0xFF;
            uint32_t b3 = c >> 24;
            uint32_t t  = pTgt[i];

            b0 = (uint8_t)(b0 + ((t & 0xFF) - b0) / (uint32_t)animframe_fade);
            b1 = (uint8_t)(b1 + (((t >> 8) & 0xFF) - b1) / (uint32_t)animframe_fade);
            b2 = (uint8_t)(b2 + (((t >> 16) & 0xFF) - b2) / (uint32_t)animframe_fade);
            b3 = (uint8_t)(b3 + ((t >> 24) - b3) / (uint32_t)animframe_fade);
            pCur[i] = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
        }

        animframe_fade--;
    }

    return result;
}

void J3DAPI animframe_Load(const int16_t* pData, int size)
{
    int seq  = pData[1];
    int base = pData[0];

    if ( !seq )
    {
        animframe_cursor       = 8;
        animframe_loaded       = 0;
        animframe_lastFragment = -1;
        animframe_fragTag      = base;
    }

    if ( seq == animframe_lastFragment + 1 )
    {
        if ( animframe_loaded + size < animframe_preloadSize )
        {
            framestream_Copy(animframe_pPreload + animframe_loaded, (const char*)pData + 10, size - 10);
            animframe_loaded += size - 10;
        }

        animframe_lastFragment = seq;
    }
}

void* J3DAPI animframe_Fetch(int n)
{
    if ( n < 0 )
    {
        if ( animframe_cursor - n <= animframe_loaded )
        {
            return animframe_pPreload - n + animframe_cursor;
        }
    }
    else
    {
        if ( animframe_cursor + n <= animframe_loaded )
        {
            animframe_cursor += n;
            return animframe_pPreload - n + animframe_cursor;
        }

        animframe_cursor = 8;
    }

    return NULL;
}

void J3DAPI animframe_Draw(void* pBitmap, int a2, int a3, int a4, void* pPalette, int a6, int a7, int a8)
{
    if ( animframe_pDisplay && (a7 & 2) )
    {
        if ( pBitmap == NULL || animframe_pDisplay->GetInfo()->f20 == 0 )
        {
            if ( !(a8 & 0x2000) )
            {
                animframe_pDisplay->SetPalette(0, 0, 0x100);
            }
        }
    }

    if ( (a6 & 1) && !(a8 & 0x4000) && animframe_pDisplay )
    {
        animframe_pDisplay->Blit(pBitmap, a3, a4, a2);
    }

    if ( (a7 & 1) && !(a8 & 0x2000) && animframe_pDisplay )
    {
        animframe_pDisplay->Present(pPalette);
    }
}

const char* J3DAPI animframe_DebugName(int index)
{
    switch ( index )
    {
        case 0:
            return "Insane AnimFrame";

        case 1:
            return "Insane AnimFrame Debug Interface";

        case 2:
            return "This is the debugging interface for the Insane Animation Frame module.";
    }

    return "";
}

void J3DAPI animframe_SetDebug(int flags)
{
    animframe_debugMask = flags;
}

int animframe_GetDebug(void)
{
    return animframe_debugMask;
}
