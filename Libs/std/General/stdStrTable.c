#include <j3dcore/j3dhook.h>
#include <std/RTI/symbols.h>

#include "std.h"
#include "stdHashtbl.h"
#include "stdMemory.h"
#include "stdStrTable.h"
#include "stdUtil.h"

#include <errno.h>
#include <limits.h>

static wchar_t stdStrTable_aBuffer[64] = { 0 };

static void stdStrTable_FreeData(tStringTable* pStrTable);

void stdStrTable_InstallHooks(void)
{
    J3D_HOOKFUNC(stdStrTable_Load);
    J3D_HOOKFUNC(stdStrTable_ParseLiteral);
    J3D_HOOKFUNC(stdStrTable_Free);
    J3D_HOOKFUNC(stdStrTable_GetValue);
    J3D_HOOKFUNC(stdStrTable_GetValueOrKey);
    J3D_HOOKFUNC(stdStrTable_ReadLine);
}

void stdStrTable_ResetGlobals(void)
{}

int J3DAPI stdStrTable_Load(tStringTable* pStrTable, const char* pFilename)
{
    char aLine[276] = { 0 };
    char aBuf[256]  = { 0 };

    STD_ASSERTREL(pStrTable != NULL);
    STD_ASSERTREL(pStrTable->magic != 'sTbl');
    STD_ASSERTREL(pFilename != NULL);

    pStrTable->nMsgs    = 0;
    pStrTable->pData    = NULL;
    pStrTable->pHashtbl = NULL;
    pStrTable->magic    = 0;

    tFileHandle fh = std_g_pHS->pFileOpen(pFilename, "rt");
    if ( !fh )
    {
        return 0;
    }

    int numMsgs = 0;
    if ( !stdStrTable_ReadLine(fh, aLine, sizeof(aLine) - 1) || sscanf_s(aLine, "MSGS %d", &numMsgs) != 1 )
    {
        std_g_pHS->pFileClose(fh);
        STDLOG_ERROR("Bad 'MSGS n' line in string table file '%s'\n", pFilename);
        return 0;
    }

    // Fixed: Reject invalid message counts before calculating allocation sizes.
    if ( numMsgs < 0 || (size_t)numMsgs > SIZE_MAX / sizeof(tStringTableNode) )
    {
        std_g_pHS->pFileClose(fh);
        STDLOG_ERROR("Invalid message count in string table file '%s'.\n", pFilename);
        return 0;
    }

    size_t hashSize = (size_t)numMsgs + (size_t)numMsgs / 2u;
    if ( hashSize > INT_MAX )
    {
        std_g_pHS->pFileClose(fh);
        STDLOG_ERROR("Message count is too large in string table file '%s'.\n", pFilename);
        return 0;
    }

    pStrTable->nMsgs = (uint32_t)numMsgs;

    int bSuccess = 0;

    if ( numMsgs )
    {
        pStrTable->pData = (tStringTableNode*)STDMALLOC(sizeof(tStringTableNode) * (size_t)numMsgs);
        if ( !pStrTable->pData )
        {
            // Altered: Preserve the original allocation diagnostic before asserting and cleaning up the partial table.
            STDLOG_ERROR("Out of memory--cannot load string table");
            STD_ASSERTREL(pStrTable->pData != NULL);
            goto cleanup;
        }

        STD_ZEROMEM(pStrTable->pData, sizeof(tStringTableNode) * (size_t)numMsgs);
    }

    pStrTable->pHashtbl = stdHashtbl_New(hashSize);
    if ( !pStrTable->pHashtbl )
    {
        // Altered: Preserve the original allocation diagnostic before asserting and cleaning up the partial table.
        STDLOG_ERROR("Out of memory--cannot load string table");
        STD_ASSERTREL(pStrTable->pHashtbl != NULL);
        goto cleanup;
    }

    bSuccess = 1;

    tStringTableNode* aData = pStrTable->pData;
    for ( int i = 0; bSuccess && (i < numMsgs); i++ )
    {
        bSuccess = stdStrTable_ReadLine(fh, aLine, sizeof(aLine) - 1);
        if ( bSuccess && strncmpi(aLine, "end", 3u) == 0 ) // hit end
        {
            bSuccess = 0;
            pStrTable->nMsgs = i;
            STDLOG_ERROR("Premature 'END' found after only %d lines in '%s'.  Check number in 'MSGS xxx' header.\n", i, pFilename);
        }

        if ( bSuccess )
        {
            char* pCur = stdUtil_ParseLiteral(aLine, aBuf, sizeof(aBuf));
            if ( pCur && aBuf[0] )
            {
                size_t keySize = strlen(aBuf) + 1u;
                char* pKey = (char*)STDMALLOC(keySize);
                if ( !pKey )
                {
                    STDLOG_FATAL("Out of memory--cannot load string table");
                    bSuccess = 0;
                    break;
                }

                stdUtil_StringCopy(pKey, keySize, aBuf);
                aData[i].pKey = pKey;

                pCur = stdUtil_StringSplit(pCur, aBuf, sizeof(aBuf), " \t");
                if ( pCur )
                {
                    errno = 0;
                    char* pNumberEnd;
                    long numericValue = strtol(aBuf, &pNumberEnd, 10);
                    // Fixed: Reject malformed or overflowing decimal numeric fields instead of silently accepting them as zero.
                    if ( errno == ERANGE || pNumberEnd == aBuf || *pNumberEnd || numericValue < INT_MIN || numericValue > INT_MAX )
                    {
                        bSuccess = 0;
                        STDLOG_ERROR("Cannot understand this line in string table '%s'.\n   >>> %s\n", pFilename, aLine);
                        continue;
                    }

                    aData[i].unknown = (int)numericValue;

                    char* pValBegin;
                    char* pValEnd;
                    stdStrTable_ParseLiteral(pCur, &pValBegin, &pValEnd);
                    // Fixed: Require valid quoted value bounds before copying to the temp buffer and assigning aData[i].value.
                    if ( pValBegin && pValEnd )
                    {
                        memset(aBuf, 0, sizeof(aBuf));
                        stdUtil_StringNumCopy(aBuf, sizeof(aBuf), pValBegin, (pValEnd - pValBegin));
                        aData[i].value = stdUtil_ToWString(aBuf);
                        if ( !aData[i].value )
                        {
                            STDLOG_FATAL("Out of memory--cannot load string table");
                            bSuccess = 0;
                            break;
                        }

                        if ( !stdHashtbl_Add(pStrTable->pHashtbl, aData[i].pKey, &aData[i]) )
                        {
                            // Fixed: A duplicate key or hash-node allocation failure makes the table incomplete.
                            bSuccess = 0;
                            if ( stdHashtbl_Find(pStrTable->pHashtbl, aData[i].pKey) )
                            {
                                STDLOG_ERROR("The key '%s' is in the string table '%s' more than once.\n   >>>%s\n", aData[i].pKey, pFilename, aLine);
                            }
                            else
                            {
                                STDLOG_FATAL("Out of memory--cannot load string table");
                            }
                        }
                    }
                    else
                    {
                        bSuccess = 0;
                        STDLOG_ERROR("Cannot understand this line in string table '%s'.\n   >>> %s\n", pFilename, aLine);
                    }
                }
                else
                {
                    // Fixed: Treat malformed entry lines as load failures, not logged successes.
                    bSuccess = 0;
                    STDLOG_ERROR("Cannot understand this line in string table '%s'.\n   >>> %s\n", pFilename, aLine);
                }
            }
            else
            {
                // Fixed: Treat malformed entry lines as load failures, not logged successes.
                bSuccess = 0;
                STDLOG_ERROR("Cannot understand this line in string table '%s'.\n   >>> %s\n", pFilename, aLine);
            }
        }
    }

    if ( bSuccess )
    {
        aLine[0] = 0;
        stdStrTable_ReadLine(fh, aLine, STD_ARRAYLEN(aLine) - 1);
        const char* ptok = strtok(aLine, " \t\n\r");
        // Fixed: Missing trailing END can leave no token at EOF; reject it instead of dereferencing NULL.
        if ( !ptok || !streqi(ptok, "end") )
        {
            bSuccess = 0;
            STDLOG_ERROR("'END' not found in '%s'.  Enlarge number in 'MSGS xxx' header.\n", pFilename);
        }
    }

    if ( bSuccess )
    {
        pStrTable->magic = 'sTbl';
    }

cleanup:
    std_g_pHS->pFileClose(fh);
    // Fixed: Release every partial allocation when parsing or allocation fails.
    if ( !bSuccess )
    {
        stdStrTable_FreeData(pStrTable);
    }

    return bSuccess;
}

char* J3DAPI stdStrTable_ParseLiteral(const char* pStr, char** ppBegin, char** ppEnd)
{
    char* pCur;

    *ppBegin = NULL;
    *ppEnd = NULL;

    pCur = strchr(pStr, '"');
    // Fixed: Reject strings with no opening quote instead of forming a pointer from NULL.
    if ( !pCur )
    {
        return NULL;
    }

    pCur = pCur + 1;
    *ppBegin = pCur;
    while ( pCur )
    {
        pCur = strchr(pCur, '"');
        if ( !pCur )
        {
            break;
        }

        *ppEnd = pCur;
        if ( *pCur == '"' )
        {
            ++pCur;
        }
    }

    return pCur;
}

void J3DAPI stdStrTable_Free(tStringTable* pStrTable)
{
    STD_ASSERTREL(pStrTable != NULL);
    // Added: Keep guard-enabled builds from dereferencing a NULL table after preserving the original assertion.
    STD_GUARD_VOID(pStrTable != NULL);

    if ( pStrTable->magic == 'sTbl' )
    {
        stdStrTable_FreeData(pStrTable);
    }
}

// Added: Shared cleanup for complete tables and partially loaded table data.
static void stdStrTable_FreeData(tStringTable* pStrTable)
{
    uint32_t nMsgs          = pStrTable->nMsgs;
    tStringTableNode* aData = pStrTable->pData;
    tHashTable* pHashtbl    = pStrTable->pHashtbl;

    pStrTable->magic    = 0;
    pStrTable->nMsgs    = 0;
    pStrTable->pData    = NULL;
    pStrTable->pHashtbl = NULL;

    if ( pHashtbl )
    {
        stdHashtbl_Free(pHashtbl);
    }

    if ( aData )
    {
        for ( uint32_t i = 0; i < nMsgs; ++i )
        {
            if ( aData[i].value )
            {
                STDFREE(aData[i].value);
            }

            if ( aData[i].pKey )
            {
                STDFREE((void*)aData[i].pKey);
            }
        }

        STDFREE(aData);
    }
}

wchar_t* J3DAPI stdStrTable_GetValue(const tStringTable* pStrTable, const char* pKey)
{
    tStringTableNode* pStrNode;
    STD_ASSERTREL(pStrTable != NULL);

    if ( pStrTable->nMsgs && (pStrNode = (tStringTableNode*)stdHashtbl_Find(pStrTable->pHashtbl, pKey)) != 0 )
    {
        return pStrNode->value;
    }

    return NULL;
}

wchar_t* J3DAPI stdStrTable_GetValueOrKey(const tStringTable* pStrTable, const char* pKey)
{
    wchar_t* pVal;

    STD_ASSERTREL(pStrTable != NULL);
    pVal = stdStrTable_GetValue(pStrTable, pKey);
    if ( pVal )
    {
        return pVal;
    }

    STD_TOWSTR(stdStrTable_aBuffer, pKey);
    return stdStrTable_aBuffer;
}

int J3DAPI stdStrTable_ReadLine(tFileHandle fh, char* pStr, int size)
{
    char* pReadStr = NULL;
    bool bFinish = false;
    while ( !bFinish )
    {
        pReadStr = std_g_pHS->pFileGets(fh, pStr, size);
        // Fixed: Stop before scanning stale line contents after EOF or a read failure.
        if ( !pReadStr )
        {
            break;
        }

        if ( pReadStr != NULL && strchr(pStr, '\n') == NULL ) // Added: Added check for no data read from file
        {
            char aBuf[64] = { 0 };
            char* pDiscardedLine;
            do
            {
                pDiscardedLine = std_g_pHS->pFileGets(fh, aBuf, sizeof(aBuf));
            } while ( pDiscardedLine != NULL && strchr(aBuf, '\n') == NULL ); // Fixed: Added check for no data read from file. This fixes potential infinitive loop bug when there is no line break at the end of the file
        }

        // Skip spaces
        char* pch = pStr;
        for ( ; isspace((unsigned char)*pch); ++pch )
        {
            ;
        }

        if ( *pch != '#' && *pch && *pch != '\r' && *pch != '\n' )
        {
            bFinish = true;
        }
    };

    // Fixed: Report EOF/read failure so callers cannot parse stale line contents.
    return pReadStr != NULL;
}
