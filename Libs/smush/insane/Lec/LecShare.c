// LecShare.c: the LEC share table (module registry) and log (original 0x004EB210-0x004EBDE0).
#include <smush/insane/Lec/LecShare.h>
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>
#include <smush/RTI/symbols.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <std/General/stdUtil.h>

#define LEC_SEP(c) ((c) < 0x21 || (c) > 0x7F || (c) == ',' || (c) == ';')

static LecShare lec_table = {
    LecShare_Publish,
    (void (*)(const char*))LecShare_Unpublish,
    (void* (*)(const char*, void*))LecShare_Subscribe,
    (void (*)(void*))LecShare_Unsubscribe,
    (void*)LecShare_BatchSubscribe,
    (void*)LecShare_BatchUnsubscribe,
    LecShare_Enumerate,
    LecShare_Count,
    (void (*)(const char*, ...))LecShare_NullError
};

static char lec_nameBuf[0x64];
static char lec_parseBuf[0x2000];

static struct
{
    int inited;
    const char* pA;
    const char* pB;
    LecEntry* pCursor;
    int count;
    LecEntry entries[LECSHARE_MAXENTRIES];
    LecEntry* pSearch;
} lec_state;

static int lec_enumIndex;
static char lec_ring[20][200];
static int lec_ringNext;
static int lec_ringUsed;

LecShare* J3DAPI LecShare_Init(LecErrorFn pError, LecModuleEntry* pModules, const char* pA, const char* pB)
{
    if ( pError )
    {
        lec_table.ReportError = pError;
    }

    LecShare_Reset(pA, pB);
    lec_state.inited = 0;
    lec_state.pSearch = NULL;

    if ( pModules )
    {
        while ( pModules->pfnRegister )
        {
            pModules->pfnRegister(&lec_table, pModules->pParam);
            pModules++;
        }
    }

    lec_state.inited = 1;
    return &lec_table;
}

int LecShare_UnregisterAll(void)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];

    while ( n-- != 0 )
    {
        if ( p->pName && LecShare_NameLen(p->pName) == 9 && LecShare_NameNCmp("LECMODULE", p->pName, LecShare_NameLen(p->pName)) == 0 )
        {
            if ( ((LecModule*)p->pInterface)->Unregister )
            {
                ((LecModule*)p->pInterface)->Unregister();
            }
        }

        p--;
    }

    return 0;
}

int LecShare_Startup(void)
{
    LecEntry* p = lec_state.entries;
    int n = lec_state.count;
    int rc = 0;

    lec_state.pCursor = NULL;
    while ( n-- != 0 )
    {
        if ( p->pName && LecShare_NameLen(p->pName) == 9 && LecShare_NameNCmp("LECMODULE", p->pName, LecShare_NameLen(p->pName)) == 0 )
        {
            if ( ((LecModule*)p->pInterface)->Startup )
            {
                rc = ((LecModule*)p->pInterface)->Startup();
                if ( rc )
                {
                    if ( lec_table.ReportError )
                    {
                        lec_table.ReportError("LecShare_Startup: Failed with module %s.", p->pName);
                    }

                    LecShare_Shutdown();
                    return rc;
                }
            }

            lec_state.pCursor = p;
        }

        p++;
    }

    return rc;
}

int LecShare_Shutdown(void)
{
    if ( lec_state.pCursor == NULL )
    {
        return 1;
    }

    LecEntry* p = ++lec_state.pCursor;
    int n = (int)(p - lec_state.entries);
    p--;

    while ( n-- != 0 )
    {
        if ( p->pName && LecShare_NameLen(p->pName) == 9 && LecShare_NameNCmp("LECMODULE", p->pName, LecShare_NameLen(p->pName)) == 0 )
        {
            if ( ((LecModule*)p->pInterface)->Shutdown )
            {
                ((LecModule*)p->pInterface)->Shutdown();
            }
        }

        p--;
    }

    return 0;
}

void J3DAPI LecShare_Reset(const char* pA, const char* pB)
{
    memset(&lec_state, 0, sizeof(lec_state));
    lec_state.pA = pA;
    lec_state.pB = pB;
}

int J3DAPI LecShare_Publish(const char* pName, void* pInterface)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];
    int i = n;

    lec_state.pSearch = NULL;
    while ( i-- != 0 )
    {
        if ( p->pName == NULL )
        {
            p->pName = pName;
            p->pInterface = pInterface;
            p->refs = 0;
            return 0;
        }

        p--;
    }

    if ( (unsigned int)n >= LECSHARE_MAXENTRIES )
    {
        return 1;
    }

    lec_state.count = n + 1;
    lec_state.entries[n].pName = pName;
    lec_state.entries[n].pInterface = pInterface;
    lec_state.entries[n].refs = 0;
    return 0;
}

int J3DAPI LecShare_Unpublish(const char* pName)
{
    int n = lec_state.count;

    lec_state.pSearch = NULL;
    LecShare_Strip((char*)pName);

    LecEntry* p = &lec_state.entries[n - 1];
    while ( n-- != 0 )
    {
        if ( p->pName && LecShare_NameCmp(p->pName, pName) == 0 )
        {
            int refs = p->refs;
            memset(p, 0, sizeof(*p));
            return refs;
        }

        p--;
    }

    return 1;
}

void* J3DAPI LecShare_Subscribe(const char* pName, const char* pPreferred)
{
    char buf[0x64];
    const char* pProvider = NULL;

    if ( !lec_state.inited )
    {
        return NULL;
    }

    lec_state.pSearch = NULL;
    if ( lec_state.pB )
    {
        pProvider = (const char*)(intptr_t)LecShare_Match(pName, lec_state.pB);
    }

    if ( !pProvider )
    {
        if ( pPreferred )
        {
            pProvider = (const char*)(intptr_t)LecShare_Match(pName, pPreferred);
        }

        if ( !pProvider )
        {
            if ( lec_state.pA )
            {
                pProvider = (const char*)(intptr_t)LecShare_Match(pName, lec_state.pA);
            }
        }
    }

    int len = LecShare_NameLen(pName);
    STD_STRNCPY(buf, pName, len);
    buf[len] = 0;

    if ( !pProvider )
    {
        if ( !LecShare_Count(buf) )
        {
            return NULL;
        }

        STD_STRNCPY(buf, (const char*)LecShare_Enumerate(buf, 0), 99);
    }
    else
    {
        STD_STRCAT(buf, "=");
        STD_STRCAT(buf, pProvider);
    }

    LecEntry* pEntry = LecShare_Find(buf);
    if ( pEntry )
    {
        pEntry->refs++;
        return pEntry->pInterface;
    }

    return NULL;
}

int J3DAPI LecShare_Unsubscribe(void* pInterface)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];
    int i = n;

    lec_state.pSearch = NULL;
    while ( i-- != 0 )
    {
        if ( pInterface == p->pInterface )
        {
            p->refs--;
            return 0;
        }

        p--;
    }

    return 1;
}

int J3DAPI LecShare_BatchSubscribe(LecBatch* pTable, const char* pDefault)
{
    for ( LecBatch* p = pTable; p->ppOut; p++ )
    {
        if ( p->pPreferred && *p->pPreferred )
        {
            *p->ppOut = LecShare_Subscribe(p->pName, p->pPreferred);
        }
        else
        {
            *p->ppOut = NULL;
        }

        if ( *p->ppOut == NULL && pDefault && *pDefault )
        {
            *p->ppOut = LecShare_Subscribe(p->pName, pDefault);
        }

        if ( *p->ppOut == NULL )
        {
            *p->ppOut = LecShare_Subscribe(p->pName, NULL);
        }

        if ( *p->ppOut == NULL )
        {
            if ( lec_table.ReportError )
            {
                lec_table.ReportError("LecShare:BatchSubscribe Couldn't subscribe to %s interface.", p->pName);
            }

            return 1;
        }
    }

    return 0;
}

int J3DAPI LecShare_BatchUnsubscribe(LecBatch* pTable)
{
    if ( pTable->ppOut == NULL )
    {
        return 1;
    }

    LecBatch* p = pTable + 1;
    int n = 1;
    while ( p->ppOut )
    {
        p++;
        n++;
    }

    p--;
    while ( n-- != 0 )
    {
        if ( LecShare_Unsubscribe(*p->ppOut) )
        {
            if ( lec_table.ReportError )
            {
                lec_table.ReportError("LecShare:BatchUnsubscribe Couldn't unsubscribe from %s interface.", p->pName);
            }

            return 1;
        }

        *p->ppOut = NULL;
        p--;
    }

    return 0;
}

void* J3DAPI LecShare_Enumerate(const char* pName, int index)
{
    int step = 1;

    if ( lec_state.count == 0 || index < 0 )
    {
        return NULL;
    }

    int n = lec_state.count;
    LecEntry* p;
    if ( lec_state.pSearch && ((!pName && lec_nameBuf[0] == 0) || (pName && LecShare_NameCmp(pName, lec_nameBuf) == 0)) )
    {
        if ( index == lec_enumIndex )
        {
            return (void*)lec_state.pSearch->pName;
        }

        int want;
        if ( index < lec_enumIndex )
        {
            step = -1;
            n = (int)(lec_state.pSearch - lec_state.entries);
            want = index;
            index = lec_enumIndex - index - 1;
            lec_enumIndex = want;
        }
        else
        {
            n -= (int)(lec_state.pSearch - lec_state.entries) + 1;
            want = index;
            index -= lec_enumIndex + 1;
            lec_enumIndex = want;
        }

        p = lec_state.pSearch + step;
    }
    else
    {
        p = lec_state.entries;
        lec_enumIndex = index;
    }

    if ( pName )
    {
        STD_STRNCPY(lec_nameBuf, pName, 99);
        lec_nameBuf[99] = 0;
    }
    else
    {
        lec_nameBuf[0] = 0;
    }

    while ( n-- != 0 )
    {
        if ( p->pName )
        {
            if ( pName == NULL || ((int)strlen(pName) == LecShare_NameLen(p->pName) && LecShare_NameNCmp(pName, p->pName, LecShare_NameLen(p->pName)) == 0) )
            {
                if ( index-- == 0 )
                {
                    lec_state.pSearch = p;
                    return (void*)p->pName;
                }
            }
        }

        p += step;
    }

    lec_state.pSearch = NULL;
    return NULL;
}

int J3DAPI LecShare_Count(const char* pName)
{
    int count = 0;
    int i = lec_state.count;
    LecEntry* p = &lec_state.entries[i - 1];

    while ( i-- != 0 )
    {
        if ( p->pName )
        {
            if ( pName == NULL || ((int)strlen(pName) == LecShare_NameLen(p->pName) && LecShare_NameNCmp(pName, p->pName, LecShare_NameLen(p->pName)) == 0) )
            {
                count++;
            }
        }

        p--;
    }

    return count;
}

LecEntry* J3DAPI LecShare_Find(const char* pName)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];

    // TODO: [BUG] NameCmp is called with the name of free entries (NULL) as well, which crashes if the table has a free slot.
    while ( n-- != 0 )
    {
        if ( LecShare_NameCmp(p->pName, pName) == 0 )
        {
            return p;
        }

        p--;
    }

    return NULL;
}

int J3DAPI LecShare_Match(const char* pName, const char* pList)
{
    const char* pResult = NULL;
    int found = 0;

    size_t len = strlen(pList) < 0x1FFF ? strlen(pList) : 0x1FFF;
    STD_STRNCPY(lec_parseBuf, pList, len);
    lec_parseBuf[len] = 0;

    char* p = lec_parseBuf;
    while ( *p )
    {
        while ( *p && LEC_SEP(*p) )
        {
            p++;
        }

        const char* q = pName;
        while ( *p && *q && toupper(*q) == toupper(*p) )
        {
            p++;
            q++;
        }

        if ( *q == 0 && *p == '.' )
        {
            q = pName;
        }

        int atEnd = !(*p && *p >= 0x21 && *p <= 0x7F && *p != '=');
        while ( *p && *p != '=' )
        {
            p++;
        }

        if ( *p == 0 )
        {
            return (int)(intptr_t)pResult;
        }

        p++;
        while ( *p && LEC_SEP(*p) )
        {
            p++;
        }

        if ( atEnd && *q == 0 )
        {
            pResult = p;
        }
        else if ( *q == '.' )
        {
            pResult = p;
            found = 1;
        }

        while ( *p && !LEC_SEP(*p) )
        {
            p++;
        }

        if ( *q == 0 || found )
        {
            if ( *p == 0 )
            {
                return (int)(intptr_t)pResult;
            }

            *p++ = 0;
            if ( !found )
            {
                return (int)(intptr_t)pResult;
            }

            found = 0;
        }

        while ( *p && LEC_SEP(*p) )
        {
            p++;
        }
    }

    return (int)(intptr_t)pResult;
}

int J3DAPI LecShare_NameLen(const char* pName)
{
    const char* p = pName;
    while ( *p )
    {
        if ( *p == '.' || *p == '=' )
        {
            break;
        }

        p++;
    }

    if ( p > pName && *p == '=' )
    {
        do
        {
            p--;
        } while ( p > pName && *p && LEC_SEP(*p) );

        p++;
    }

    return (int)(p - pName);
}

int J3DAPI LecShare_NullError(const char* pFormat, ...)
{
    (void)pFormat;
    return 0;
}

void J3DAPI LecShare_Strip(char* pName)
{
    char* pDst = pName;
    char* pSrc = pName;

    if ( *pSrc )
    {
        do
        {
            if ( *pSrc >= 0x21 && *pSrc <= 0x7F )
            {
                // Added: don't write if the character is unchanged. The original code stripped (and so wrote) the caller's literal names in
                // place, which are in writable memory in the original executable, but in read-only memory in this build.
                if ( *pDst != *pSrc )
                {
                    *pDst = *pSrc;
                }

                pDst++;
            }
        } while ( *++pSrc );
    }

    if ( *pDst != 0 ) // Added: see above
    {
        *pDst = 0;
    }
}

int J3DAPI LecShare_NameCmp(const char* pA, const char* pB)
{
    int result = -1;
    for ( ;; )
    {
        if ( *pA == 0 && *pB == 0 )
        {
            break;
        }

        result = toupper(*pA++) - toupper(*pB++);
        if ( result )
        {
            break;
        }
    }

    return result;
}

int J3DAPI LecShare_NameNCmp(const char* pA, const char* pB, int n)
{
    int result = -1;
    while ( n-- != 0 )
    {
        if ( *pA == 0 && *pB == 0 )
        {
            break;
        }

        result = toupper(*pA++) - toupper(*pB++);
        if ( result )
        {
            break;
        }
    }

    return result;
}

void J3DAPI lec_Log(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    // Changed: the original wrote with vsprintf, i.e. a message of 200 bytes or more overflowed the ring slot (/sdl does not allow vsprintf)
    vsnprintf(lec_ring[lec_ringNext], sizeof(lec_ring[0]), pFormat, args);
    va_end(args);

    if ( lec_ringNext == 19 )
    {
        lec_ringNext = 0;
    }
    else
    {
        lec_ringNext++;
    }

    if ( lec_ringUsed < 19 )
    {
        lec_ringUsed++;
    }
}

void LecShare_InstallHooks(void)
{
    // Uncomment only lines for functions that have full definition and doesn't call original function (non-thunk functions)

    J3D_HOOKFUNC(LecShare_Init);
    J3D_HOOKFUNC(LecShare_UnregisterAll);
    J3D_HOOKFUNC(LecShare_Startup);
    J3D_HOOKFUNC(LecShare_Shutdown);
    J3D_HOOKFUNC(LecShare_Reset);
    J3D_HOOKFUNC(LecShare_Publish);
    J3D_HOOKFUNC(LecShare_Unpublish);
    J3D_HOOKFUNC(LecShare_Subscribe);
    J3D_HOOKFUNC(LecShare_Unsubscribe);
    J3D_HOOKFUNC(LecShare_BatchSubscribe);
    J3D_HOOKFUNC(LecShare_BatchUnsubscribe);
    J3D_HOOKFUNC(LecShare_Enumerate);
    J3D_HOOKFUNC(LecShare_Count);
    J3D_HOOKFUNC(LecShare_Find);
    J3D_HOOKFUNC(LecShare_Match);
    J3D_HOOKFUNC(LecShare_NameLen);
    J3D_HOOKFUNC(LecShare_Strip);
    J3D_HOOKFUNC(LecShare_NameCmp);
    J3D_HOOKFUNC(LecShare_NameNCmp);
    J3D_HOOKFUNC(lec_Log);
}

void LecShare_ResetGlobals(void)
{
    memset(&lec_state, 0, sizeof(lec_state));
    memset(lec_nameBuf, 0, sizeof(lec_nameBuf));
    memset(lec_parseBuf, 0, sizeof(lec_parseBuf));
    memset(lec_ring, 0, sizeof(lec_ring));
    lec_enumIndex = 0;
    lec_ringNext  = 0;
    lec_ringUsed  = 0;
    lec_table.ReportError = (void (*)(const char*, ...))LecShare_NullError;
}
