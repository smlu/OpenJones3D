#ifndef SMUSH_INSANE_LEC_LECSHARE_H
#define SMUSH_INSANE_LEC_LECSHARE_H
#include <assert.h>
#include <j3dcore/j3d.h>
#include <smush/insane/lec.h>

/*
 * The LEC framework's start-up interface (LecShare.c), called by the host (SmushPlay.c): the module list LecShare_Init
 * registers, the batch-subscription table the share table's BatchSubscribe/BatchUnsubscribe slots take, and the log
 * every module can print to.
 */

J3D_EXTERN_C_START

#define LECSHARE_MAXENTRIES 512

typedef struct sLecModuleEntry
{
    int (*pfnRegister)(LecShare* pShare, void* pParam);
    void* pParam;
} LecModuleEntry;
static_assert(sizeof(LecModuleEntry) == 8, "sizeof(LecModuleEntry) == 8");

typedef struct sLecBatch
{
    void** ppOut;
    const char* pName;
    const char* pPreferred;
} LecBatch;
static_assert(sizeof(LecBatch) == 0xC, "sizeof(LecBatch) == 0xC");

typedef void (*LecErrorFn)(const char* pFormat, ...);

/* One published interface in the share table (LecShare.c keeps LECSHARE_MAXENTRIES of them in its state block). */
typedef struct sLecEntry
{
    const char* pName;
    void* pInterface;
    int refs;
} LecEntry;
static_assert(sizeof(LecEntry) == 0xC, "sizeof(LecEntry) == 0xC");

LecShare* J3DAPI LecShare_Init(LecErrorFn pError, LecModuleEntry* pModules, const char* pA, const char* pB);
int LecShare_UnregisterAll(void);
int LecShare_Startup(void);
int LecShare_Shutdown(void);
void J3DAPI LecShare_Reset(const char* pA, const char* pB);
int J3DAPI LecShare_Publish(const char* pName, void* pInterface);
int J3DAPI LecShare_Unpublish(const char* pName);
void* J3DAPI LecShare_Subscribe(const char* pName, const char* pPreferred);
int J3DAPI LecShare_Unsubscribe(void* pInterface);
int J3DAPI LecShare_BatchSubscribe(LecBatch* pTable, const char* pDefault);
int J3DAPI LecShare_BatchUnsubscribe(LecBatch* pTable);
void* J3DAPI LecShare_Enumerate(const char* pName, int index);
int J3DAPI LecShare_Count(const char* pName);
LecEntry* J3DAPI LecShare_Find(const char* pName);
int J3DAPI LecShare_Match(const char* pName, const char* pList);
int J3DAPI LecShare_NameLen(const char* pName);
int J3DAPI LecShare_NullError(const char* pFormat, ...); // Folded onto 0x004E74A0; no address of its own, never hook (see RTI/addresses.h)
void J3DAPI LecShare_Strip(char* pName);
int J3DAPI LecShare_NameCmp(const char* pA, const char* pB);
int J3DAPI LecShare_NameNCmp(const char* pA, const char* pB, int n);
void J3DAPI lec_Log(const char* pFormat, ...);

void LecShare_InstallHooks(void);
void LecShare_ResetGlobals(void);

J3D_EXTERN_C_END

#endif // SMUSH_INSANE_LEC_LECSHARE_H
