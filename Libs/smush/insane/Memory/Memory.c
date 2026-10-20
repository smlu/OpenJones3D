// Memory.c: the memory services (`MEMORY=insane`, `MEMORY=insane_debug`) (original 0x004EAE10-0x004EB210).
#include <smush/insane/Memory/Memory.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/addresses.h>
#include <smush/RTI/symbols.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static LecModule memory_module = { memory_Register, memory_Unregister, NULL, NULL };
static MemoryInterface memory_plain; // Set in memory_Register: with the DLL CRT (Debug configuration) the address of dllimport malloc/free/realloc is not a constant
static MemoryDebugInterface memory_debug = { memory_debugAlloc, memory_debugFree, memory_debugRealloc };

static LecList memory_list;
static LecShare* memory_pServices;
static void* memory_pData;
static int memory_total;
static int memory_maxTotal;

int J3DAPI memory_Register(LecShare* pShare, void* pParam)
{
    memory_pServices = pShare;
    memory_pData     = pParam;
    memory_plain.Alloc   = malloc;
    memory_plain.Free    = free;
    memory_plain.Realloc = realloc;
    memory_plain.pReserved = NULL;
    pShare->Publish("LECMODULE=insane_memory", &memory_module);
    memory_pServices->Publish("MEMORY=insane", &memory_plain);
    memory_pServices->Publish("MEMORY=insane_debug", &memory_debug);
    memory_total = 0;
    LecList_Init(&memory_list);
    return 0;
}

void memory_Unregister(void)
{
    memory_pServices->Unpublish("MEMORY=insane_debug");
    memory_pServices->Unpublish("MEMORY=insane");
    memory_pServices->Unpublish("LECMODULE=insane_memory");

    if ( memory_list.head.pNext != &memory_list.tail )
    {
        printf("\nMaximum Usage = %ld\n", memory_maxTotal);
        printf("\nRequest Summary\n\n");
        while ( memory_list.head.pNext != &memory_list.tail )
        {
            MemoryRecord* pRecord = (MemoryRecord*)memory_list.head.pNext;
            printf("%-16s,\tline %4d:\t%8ld bytes, [%08lx]\tstatus = %s\n", pRecord->pFile, pRecord->line, pRecord->size, (unsigned long)(uintptr_t)pRecord->pBlock,
                   pRecord->refs > 0 ? "Unfreed" : pRecord->refs == 0 ? "Freed" : "Freed Twice");
            LecList_Remove(&pRecord->link);
            free(pRecord);
        }
    }
}

void* J3DAPI memory_debugAlloc(int size, const char* pFile, int line)
{
    char* pRaw = malloc(size + 8);
    if ( !pRaw )
    {
        return NULL;
    }

    memory_Track(pRaw, size, pFile, line, "Alloc");
    *(int*)pRaw = size;
    memory_total += size + 4;
    return pRaw + 4;
}

int J3DAPI memory_Track(char* pRaw, int size, const char* pFile, int line, const char* pOp)
{
    *(char**)(pRaw + size + 4) = pRaw;
    if ( memory_total > memory_maxTotal )
    {
        memory_maxTotal = memory_total;
    }

    printf("%-16s,\tline %4d:\t%s %8ld [%08lx] (usage = %ld)\n", pFile, line, pOp, size, (unsigned long)(uintptr_t)pRaw, memory_total);

    for ( MemoryRecord* pRecord = (MemoryRecord*)memory_list.head.pNext; &pRecord->link != &memory_list.tail; pRecord = (MemoryRecord*)pRecord->link.pNext )
    {
        if ( pRaw == pRecord->pBlock )
        {
            pRecord->refs++;
            return 0;
        }
    }

    MemoryRecord* pRecord = malloc(sizeof(MemoryRecord));
    if ( pRecord )
    {
        pRecord->pBlock = pRaw;
        pRecord->size   = size;
        pRecord->line   = line;
        pRecord->pFile  = pFile;
        pRecord->refs   = 1;
        LecList_Insert(&pRecord->link, &memory_list.head, 0);
    }

    return 0;
}

void J3DAPI memory_debugFree(void* pBlock, const char* pFile, int line)
{
    char* pRaw = (char*)pBlock - 4;

    memory_total -= *(int*)pRaw + 4;
    if ( !memory_Check(pRaw, *(int*)pRaw, pFile, line, "Free") )
    {
        free(pRaw);
    }
}

int J3DAPI memory_Check(char* pRaw, int size, const char* pFile, int line, const char* pOp)
{
    int bOverwritten = *(char**)(pRaw + size + 4) != pRaw;

    for ( MemoryRecord* pRecord = (MemoryRecord*)memory_list.head.pNext; &pRecord->link != &memory_list.tail; pRecord = (MemoryRecord*)pRecord->link.pNext )
    {
        if ( pRaw == pRecord->pBlock )
        {
            if ( --pRecord->refs < 0 )
            {
                printf("%-16s,\tline %4d:\t%s  %8ld [%08lx] (usage = %ld)  %s FREE TWICE!!\n", pFile, line, pOp, size, (unsigned long)(uintptr_t)pRaw, memory_total,
                       bOverwritten ? "OVERWRITTEN!!" : "");
                return -1;
            }

            printf("%-16s,\tline %4d:\t%s  %8ld [%08lx] (usage = %ld)  %s \n", pFile, line, pOp, size, (unsigned long)(uintptr_t)pRaw, memory_total,
                   bOverwritten ? "OVERWRITTEN!!" : "");
            return 0;
        }
    }

    printf("%-16s,\tline %4d:\t%s  %8ld [%08lx] (usage = %ld)  FREEING INVALID CHUNK!\n", pFile, line, pOp, size, (unsigned long)(uintptr_t)pRaw, memory_total);
    return -2;
}

void* J3DAPI memory_debugRealloc(void* pBlock, int size, const char* pFile, int line)
{
    char* pRaw = pBlock;
    int base = 0;

    // TODO: [BUG] A NULL pBlock is not handled: it is dereferenced below (and base is not set).
    if ( pRaw )
    {
        pRaw -= 4;
        base = memory_total - *(int*)pRaw - 4;
    }

    if ( memory_Check(pRaw, *(int*)pRaw, pFile, line, "Re-Free") )
    {
        return NULL;
    }

    char* pNew = realloc(pRaw, size + 8);
    if ( !pNew )
    {
        memory_Track(pRaw, *(int*)pRaw, pFile, line, "Re-UnAlloc");
        return NULL;
    }

    memory_Track(pNew, size, pFile, line, "Re-Alloc");
    *(int*)pNew = size;
    memory_total = base + size + 4;
    return pNew + 4;
}

void Memory_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(memory_Register);
    J3D_HOOKFUNC(memory_Unregister);
    J3D_HOOKFUNC(memory_debugAlloc);
    J3D_HOOKFUNC(memory_Track);
    J3D_HOOKFUNC(memory_debugFree);
    J3D_HOOKFUNC(memory_Check);
    J3D_HOOKFUNC(memory_debugRealloc);
}

void Memory_ResetGlobals(void)
{
    memset(&memory_list, 0, sizeof(memory_list));
    memory_pServices = NULL;
    memory_pData     = NULL;
    memory_total     = 0;
    memory_maxTotal  = 0;
}
