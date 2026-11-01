#ifndef SMUSH_INSANE_LEC_H
#define SMUSH_INSANE_LEC_H
#include <assert.h>
#include <j3dcore/j3d.h>

/*
 * LucasArts LEC module framework, as INSANE's modules use it.
 * The original has no names for any of this: the member names below are descriptive inferences drawn from the
 * calls the module code makes. Every module's registration function receives the share table and an opaque
 * argument, publishes `LECMODULE=<module>` and one interface name, and keeps both in file statics.
 */

J3D_EXTERN_C_START

typedef struct sLecShare
{
    int   (*Publish)(const char* pName, void* pInterface);   /* 0x00 */
    void  (*Unpublish)(const char* pName);                    /* 0x04 */
    void* (*Subscribe)(const char* pName, void* pParam);      /* 0x08 */
    void  (*Unsubscribe)(void* pInterface);                   /* 0x0C */
    void* pfn10;                                              /* 0x10 BatchSubscribe (LecShare.c: `int (LecBatch*, const char*)`, SmushPlay calls it with a table and 0) */
    void* pfn14;                                              /* 0x14 BatchUnsubscribe (LecShare.c: `int (LecBatch*)`) */
    void* (*Enumerate)(const char* pName, int index);         /* 0x18 */
    int   (*Count)(const char* pName);                        /* 0x1C */
    void  (*ReportError)(const char* pFormat, ...);           /* 0x20 (printf-style) */
} LecShare;
static_assert(sizeof(LecShare) == 0x24, "sizeof(LecShare) == 0x24");

/* `VIRTHERC.debug`: INSANE's diagnostic-print service (slot roles inferred from the calls; only Print is seen in use). */
typedef struct sLecDebug
{
    void* pfn00;
    void* pfn04;
    void* pfn08;
    void  (*Print)(const char* pMessage);                     /* 0x0C */
} LecDebug;
static_assert(sizeof(LecDebug) == 0x10, "sizeof(LecDebug) == 0x10");

/* A module's descriptor: the four entry points the framework calls (`{register, unregister, startup, shutdown}`). */
typedef struct sLecModule
{
    int  (*Register)(LecShare* pShare, void* pParam);
    void (*Unregister)(void);
    int  (*Startup)(void);
    void (*Shutdown)(void);
} LecModule;
static_assert(sizeof(LecModule) == 0x10, "sizeof(LecModule) == 0x10");

/*
 * The original passes its `__FILE__` (E:\smush\INSANE\SOURCE\...) to the debug allocator and to assertions.
 * It only ends up in the allocator's records (printed by memory_Track, which has no console in the game).
 */
#define INSANE_FILE __FILE__

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_LEC_H
