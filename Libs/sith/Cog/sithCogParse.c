#include "sithCogParse.h"
#include <j3dcore/j3dhook.h>

#include <rdroid/Math/rdVector.h>
#include <rdroid/types.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Cog/sithCogYacc.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithThing.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <stdio.h>

#define SITHCOGPARSE_SYNTAXTREESIZE 8096u

static bool bSkipYYRestart = true;
static SithCogScript* pCurrentScript;

static uint32_t codepos;
static int32_t* pCurrentCode;

static size_t nextlabel = 1;
static int32_t aLabelTable[1024];

static SithCogSyntaxNode* pCurrentRoot;

static size_t numTreeNodes;
static size_t syntaxTreeSize;
static SithCogSyntaxNode* pSyntaxTree;

static SithCogSymbolTable* pCurrentSymbols;

void sithCogParse_InstallHooks(void)
{
    J3D_HOOKFUNC(sithCogParse_FreeParseTree);
    J3D_HOOKFUNC(sithCogParse_Load);
    J3D_HOOKFUNC(sithCogParse_ParseSectionCode);
    J3D_HOOKFUNC(sithCogParse_DuplicateSymbolTable);
    J3D_HOOKFUNC(sithCogParse_AllocSymbolTable);
    J3D_HOOKFUNC(sithCogParse_FreeSymbolTable);
    J3D_HOOKFUNC(sithCogParse_AddSymbol);
    J3D_HOOKFUNC(sithCogParse_SetSymbolValue);
    J3D_HOOKFUNC(sithCogParse_GetSymbol);
    J3D_HOOKFUNC(sithCogParse_GetSymbolByID);
    J3D_HOOKFUNC(sithCogParse_GetSymbolLabel);
    J3D_HOOKFUNC(sithCogParse_MakeLeafNode);
    J3D_HOOKFUNC(sithCogParse_MakeVectorLeafNode);
    J3D_HOOKFUNC(sithCogParse_MakeNode);
    J3D_HOOKFUNC(sithCogParse_LexerSetSymbol);
    J3D_HOOKFUNC(sithCogParse_LexerSetString);
    J3D_HOOKFUNC(sithCogParse_LexerSetVector);
    J3D_HOOKFUNC(sithCogParse_GetNextLabel);
    J3D_HOOKFUNC(sithCogParse_GetNextNode);
    J3D_HOOKFUNC(sithCogParse_ResetTreeNodes);
    J3D_HOOKFUNC(sithCogParse_GenerateLabelTable);
    J3D_HOOKFUNC(sithCogParse_GenerateCode);
    J3D_HOOKFUNC(sithCogParse_ParseSymbols);
    J3D_HOOKFUNC(sithCogParse_ParseSymbolRef);
    J3D_HOOKFUNC(sithCogParse_ParseFlex);
    J3D_HOOKFUNC(sithCogParse_ParseInt);
    J3D_HOOKFUNC(sithCogParse_ParseVector);
    J3D_HOOKFUNC(sithCogParse_ParseMessage);
    J3D_HOOKFUNC(sithCogParse_ResolveMessageHandlers);
}

void sithCogParse_ResetGlobals(void)
{}

void sithCogParse_FreeParseTree(void)
{
    if ( pSyntaxTree )
    {
        STDFREE(pSyntaxTree);
        pSyntaxTree    = NULL; // Altered: Assigned NULL
        syntaxTreeSize = 0;
        numTreeNodes   = 0;
    }
}

int J3DAPI sithCogParse_Load(const char* pFilename, SithCogScript* pScript, int bParseDescription)
{
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(!pScript->pSymbolTable);

    if ( !stdConffile_Open(pFilename) )
    {
        SITHLOG_ERROR("Open failed for file %s.\n", pFilename);
        return 0;
    }

    // Clear script & name
    STD_ZEROMEM(pScript, sizeof(SithCogScript));
    STD_STRCPY(pScript->aName, stdFileFromPath(pFilename));

    // Prepare parser
    nextlabel = 1;
    STD_FILLMEM(aLabelTable, -1, sizeof(aLabelTable));

    if ( !stdConffile_ReadArgs() )
    {
        goto eof_error;
    }

    // Parse optional flags
    if ( streq(stdConffile_g_entry.aArgs[0].argName, "flags") ) // TODO: case-insensitive cmp?
    {
        if ( sscanf_s(stdConffile_g_entry.aArgs[0].argValue, "%x", &pScript->flags) != 1 || !stdConffile_ReadArgs() )
        {
            goto eof_error;
        }
    }

   // Parse symbol section
    if ( !streq(stdConffile_g_entry.aArgs[0].argValue, "symbols") ) // TODO: case-insensitive cmp?
    {
        goto syntax_error;
    }

    pScript->pSymbolTable = sithCogParse_AllocSymbolTable(SITHCOGSCRIPT_MAXSYMREFS);
    if ( !pScript->pSymbolTable )
    {
        SITHLOG_ERROR("Memory alloc failed.\n");
        goto error;
    }

    if ( !sithCogParse_ParseSymbols(pScript, bParseDescription) )
    {
        goto syntax_error;
    }

    // Now parse code section
    if ( !stdConffile_ReadArgs() )
    {
        goto eof_error;
    }

    if ( !streq(stdConffile_g_entry.aArgs[0].argValue, "code") // TODO: case-insensitive cmp?
        || !sithCogParse_ParseSectionCode(pScript) )
    {
        goto syntax_error;
    }

    // Success!
    sithCogParse_ResolveMessageHandlers(pScript);
    stdConffile_Close();
    return 1;

syntax_error:
    SITHLOG_ERROR("Syntax error.\n");
    goto error;

eof_error:
    SITHLOG_ERROR("Unexpected EOF.\n");
    goto error;

error:
    if ( pScript->pSymbolTable )
    {
        sithCogParse_FreeSymbolTable(pScript->pSymbolTable);
        pScript->pSymbolTable = NULL;
    }

    if ( pCurrentRoot )
    {
        sithCogParse_ResetTreeNodes();
        pCurrentRoot = NULL;
    }

    stdConffile_Close();
    return 0;
}

int J3DAPI sithCogParse_ParseSectionCode(SithCogScript* pScript)
{
    SITH_ASSERTREL(pCurrentRoot == NULL);

    tFileHandle fh = stdConffile_GetFileHandle();
    if ( !fh )
    {
        goto error;
    }

    yyin = fh;

    pCurrentScript  = pScript;
    pCurrentSymbols = pScript->pSymbolTable;

    if ( bSkipYYRestart )
    {
        bSkipYYRestart = false;
    }
    else
    {
        yyrestart(fh);
    }

    yylinenum = stdConffile_GetLineNumber(); // Fixed: Was set to 1 which resulted in wrong line number in case of an error.
    int ret = yyparse();
    if ( ret )
    {
        SITHLOG_ERROR("COG parse returned %d.\n", ret);
        goto error;
    }

        // Build label table
    codepos = 0;
    sithCogParse_GenerateLabelTable(pCurrentRoot);

    pScript->pCode = (int32_t*)STDMALLOC(sizeof(int32_t) * codepos + sizeof(int32_t));
    if ( !pScript->pCode )
    {
        SITHLOG_ERROR("Malloc error for COG code.\n");
        goto error;
    }

    // Generate code
    pScript->codeSize = codepos + 1;

    codepos = 0;
    pCurrentCode = pScript->pCode;
    sithCogParse_GenerateCode(pCurrentRoot);

    pScript->pCode[pScript->codeSize - 1] = SITHCOGEXEC_OPCODE_RET;

    // Success
    sithCogParse_ResetTreeNodes();
    pCurrentRoot = NULL;
    return 1;

error:
    if ( pCurrentRoot )
    {
        sithCogParse_ResetTreeNodes();
        pCurrentRoot = NULL;
    }

    codepos = 0;
    return 0;
}

SithCogSymbolTable* J3DAPI sithCogParse_DuplicateSymbolTable(const SithCogSymbolTable* pSource)
{
    SITH_ASSERTREL(pSource);

    SithCogSymbolTable* pTable = (SithCogSymbolTable*)STDMALLOC(sizeof(SithCogSymbolTable));
    if ( !pTable )
    {
        goto error;
    }

    STD_ZEROMEM(pTable, sizeof(SithCogSymbolTable));
    pTable->aSymbols = (SithCogSymbol*)STDMALLOC(sizeof(SithCogSymbol) * pSource->numUsedSymbols);
    if ( !pTable->aSymbols )
    {
        goto error;
    }

    STD_COPYMEM(pTable->aSymbols, pSource->aSymbols, sizeof(SithCogSymbol) * pSource->numUsedSymbols);
    pTable->tableSize      = pSource->numUsedSymbols;
    pTable->numUsedSymbols = pSource->numUsedSymbols;
    pTable->bIsCopy        = 1;
    return pTable;

error:
    SITHLOG_ERROR("Error while duplicating symbol table.\n");
    return NULL;
}

SithCogSymbolTable* J3DAPI sithCogParse_AllocSymbolTable(size_t numElements)
{
    SITH_ASSERTREL(numElements > 0);
    SithCogSymbolTable* pTable = (SithCogSymbolTable*)STDMALLOC(sizeof(SithCogSymbolTable));
    if ( !pTable )
    {
        goto error;
    }

    STD_ZEROMEM(pTable, sizeof(SithCogSymbolTable));
    pTable->aSymbols = (SithCogSymbol*)STDMALLOC(sizeof(SithCogSymbol) * numElements);
    if ( !pTable->aSymbols )
    {
        goto error;
    }

    pTable->pHashtbl = stdHashtbl_New(2 * numElements);
    if ( !pTable->pHashtbl )
    {
        goto error;
    }

    STD_ZEROMEM(pTable->aSymbols, sizeof(SithCogSymbol) * numElements);
    pTable->tableSize      = numElements;
    pTable->numUsedSymbols = 0;
    pTable->bIsCopy        = 0;
    return pTable;

error:
    SITHLOG_ERROR("Failed to create memory for symbol table.\n");
    if ( !pTable )
    {
        return NULL;
    }

    if ( pTable->aSymbols )
    {
        STDFREE(pTable->aSymbols);
    }

    if ( pTable->pHashtbl )
    {
        stdHashtbl_Free(pTable->pHashtbl);
    }

    STDFREE(pTable);
    return NULL;
}

void J3DAPI sithCogParse_ReallocSymbolTable(SithCogSymbolTable* pTable)
{
    SITH_ASSERTREL(pTable != NULL);
    if ( pTable->pHashtbl )
    {
        stdHashtbl_Free(pTable->pHashtbl);
        pTable->pHashtbl = NULL;
    }

    if ( pTable->tableSize > pTable->numUsedSymbols )
    {
        pTable->aSymbols = (SithCogSymbol*)STDREALLOC(pTable->aSymbols, sizeof(SithCogSymbol) * pTable->numUsedSymbols);
        pTable->tableSize = pTable->numUsedSymbols;
    }

    for ( size_t i = 0; i < pTable->tableSize; ++i )
    {
        if ( pTable->aSymbols[i].pName )
        {
            STDFREE(pTable->aSymbols[i].pName);
            pTable->aSymbols[i].pName = NULL;
        }
    }
}

void J3DAPI sithCogParse_FreeSymbolTable(SithCogSymbolTable* pTable)
{
    SITH_ASSERTREL(pTable != NULL);
    if ( pTable->pHashtbl )
    {
        stdHashtbl_Free(pTable->pHashtbl);
        pTable->pHashtbl = NULL;
    }

    if ( pTable->aSymbols )
    {
        SITH_ASSERTREL(pTable->tableSize > 0);
        if ( !pTable->bIsCopy )
        {
            for ( size_t i = 0; i < pTable->tableSize; ++i )
            {
                if ( pTable->aSymbols[i].pName )
                {
                    STDFREE(pTable->aSymbols[i].pName);
                }

                if ( pTable->aSymbols[i].value.type == SITHCOG_VALUE_STRING )
                {
                    SITH_ASSERTREL(pTable->aSymbols[i].value.val.pString);
                    STDFREE(pTable->aSymbols[i].value.val.pString);
                    pTable->aSymbols[i].value.val.pString = NULL;
                }
            }
        }

        STDFREE(pTable->aSymbols);
        pTable->aSymbols = NULL;
    }

    STDFREE(pTable);
}

SithCogSymbol* J3DAPI sithCogParse_AddSymbol(SithCogSymbolTable* pTable, const char* pName)
{
    SITH_ASSERTREL(pTable != NULL);
    SITH_ASSERTREL(pTable->aSymbols != NULL);

    if ( pTable->tableSize <= pTable->numUsedSymbols )
    {
        SITHLOG_ERROR("No space for COG symbol %s.\n", pName);
        return NULL;
    }

    SithCogSymbol* pSymbol = &pTable->aSymbols[pTable->numUsedSymbols++];
    if ( pName )
    {
        const size_t nameLen = strlen(pName) + 1;
        pSymbol->pName = (char*)STDMALLOC(nameLen);
        stdUtil_StringCopy(pSymbol->pName, nameLen, pName);
        if ( pTable->pHashtbl )
        {
            stdHashtbl_Add(pTable->pHashtbl, pSymbol->pName, pSymbol);
        }
    }

    pSymbol->label = sithCogParse_GetNextLabel();
    pSymbol->id    = pTable->firstId + pSymbol - pTable->aSymbols;
    return pSymbol;
}

void J3DAPI sithCogParse_SetSymbolValue(SithCogSymbol* pSymbol, const SithCogSymbolValue* pValue)
{
    SITH_ASSERTREL(pSymbol != NULL);
    SITH_ASSERTREL(pValue != NULL);
    STD_COPYMEM(&pSymbol->value, pValue, sizeof(pSymbol->value));
}

SithCogSymbol* J3DAPI sithCogParse_GetSymbol(const SithCogSymbolTable* pLocal, const char* pName)
{
    SITH_ASSERTREL(pLocal != NULL);
    SITH_ASSERTREL(pLocal->aSymbols != NULL);
    SITH_ASSERTREL(pName != NULL);

    if ( pLocal->pHashtbl )
    {
        SithCogSymbol* pSymbol = (SithCogSymbol*)stdHashtbl_Find(pLocal->pHashtbl, pName);
        if ( pSymbol )
        {
            return pSymbol;
        }
    }

    // Get symbol from global table
    if ( !sithCog_g_pSymbolTable )
    {
        return NULL;
    }

    if ( pLocal == sithCog_g_pSymbolTable )
    {
        return NULL;
    }

    if ( !sithCog_g_pSymbolTable->pHashtbl )
    {
        return NULL;
    }

    SithCogSymbol* pSymbol = (SithCogSymbol*)stdHashtbl_Find(sithCog_g_pSymbolTable->pHashtbl, pName);
    if ( pSymbol )
    {
        return pSymbol;
    }

    return NULL;
}

SithCogSymbol* J3DAPI sithCogParse_GetSymbolByID(const SithCogSymbolTable* pTable, size_t symbolId)
{
    if ( symbolId >= SITHCOG_GLOBALSYMBOLSTARTID )
    {
        pTable = sithCog_g_pSymbolTable;
        symbolId -= SITHCOG_GLOBALSYMBOLSTARTID;
    }

    if ( pTable && symbolId < pTable->numUsedSymbols )
    {
        return &pTable->aSymbols[symbolId];
    }

    return NULL;
}

uint32_t J3DAPI sithCogParse_GetSymbolLabel(size_t symbolId)
{
    SithCogSymbol* pSymbol = sithCogParse_GetSymbolByID(pCurrentSymbols, symbolId);
    SITH_ASSERTREL(pSymbol);
    return pSymbol->label;
}

SithCogSyntaxNode* J3DAPI sithCogParse_MakeLeafNode(SithCogExecOpcode opcode, int symbolId)
{
    SithCogSyntaxNode* pNewNode = sithCogParse_GetNextNode();
    if ( !pNewNode )
    {
        return pNewNode;
    }

    pNewNode->opcode = opcode;
    pNewNode->value  = symbolId;

    pCurrentRoot = pNewNode;
    return pNewNode;
}

SithCogSyntaxNode* J3DAPI sithCogParse_MakeVectorLeafNode(SithCogExecOpcode opcode, const rdVector3* pVect)
{
    SithCogSyntaxNode* pNewNode = sithCogParse_GetNextNode();
    if ( !pNewNode ) // Fixed: was !sithCogParse_GetNextNode
    {
        return pNewNode;
    }

    pNewNode->opcode   = opcode;
    pNewNode->vecValue = *pVect;

    pCurrentRoot = pNewNode;
    return pNewNode;
}

SithCogSyntaxNode* J3DAPI sithCogParse_MakeNode(SithCogSyntaxNode* pLeft, SithCogSyntaxNode* pRight, SithCogExecOpcode opcode, int value)
{
    SithCogSyntaxNode* pNewNode = sithCogParse_GetNextNode();
    if ( !pNewNode )
    {
        return pNewNode;
    }

    pNewNode->opcode = opcode;
    pNewNode->value  = value;
    pNewNode->pLeft  = pLeft;
    pNewNode->pRight = pRight;

    pCurrentRoot = pNewNode;
    return pNewNode;
}

void J3DAPI sithCogParse_LexerSetSymbol(const char* pName)
{
    SITH_ASSERTREL(pName && pCurrentSymbols);
    stdUtil_ToLower((char*)pName);// Note, was calling _strlwr((char*)pName);
    SithCogSymbol* pSym = sithCogParse_GetSymbol(pCurrentSymbols, pName);
    if ( pSym )
    {
        yylval.intValue = pSym->id;
        return;
    }

    // Sym not found, make float val
    SithCogSymbol* pNewSym = sithCogParse_AddSymbol(pCurrentSymbols, pName);
    if ( !pNewSym )
    {
        SITHLOG_ERROR("symbol %s not defined, cannot add new symbol.\n", pName);
        return;
    }

    SITHLOG_ERROR("Parsing script %s: Symbol %s not defined.  Assuming float=0.0f\n", pCurrentScript->aName, pName);
    SITHLOG_DEBUG("Parsing script %s: Symbol %s not defined.  Assuming float=0.0f\n", pCurrentScript->aName, pName);
    fprintf(stdout, "Parsing script %s: Symbol %s not defined.  Assuming float=0.0f\n", pCurrentScript->aName, pName);

    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_FLOAT;
    value.val.floatValue = 0.0f;
    sithCogParse_SetSymbolValue(pNewSym, &value);

    yylval.intValue = pNewSym->id;
}

void J3DAPI sithCogParse_LexerSetString(const char* pString)
{
    SITH_ASSERTREL(pString && pCurrentSymbols);

    SithCogSymbol* pNewSym = sithCogParse_AddSymbol(pCurrentSymbols, NULL);
    if ( pNewSym )
    {
        pNewSym->value.type = SITHCOG_VALUE_STRING;
        size_t strLen = strlen(pString) - 1;
        pNewSym->value.val.pString = (char*)STDMALLOC(strLen);
        if ( pNewSym->value.val.pString )
        {
            stdUtil_StringNumCopy(pNewSym->value.val.pString, strLen, pString + 1, strLen - 1); // Note, pString is quoted, we strip off quote here
        }

        yylval.intValue = pNewSym->id;
    }
}

void J3DAPI sithCogParse_LexerSetVector(const char* pString)
{
    SITH_ASSERTREL(pString);

    rdVector3 vec;
    sscanf_s(pString, "'%f %f %f'", &vec.x, &vec.y, &vec.z);
    yylval.vecValue = vec;
}

uint32_t sithCogParse_GetNextLabel(void)
{
    SITH_ASSERTREL(nextlabel <= STD_ARRAYLEN(aLabelTable));
    return nextlabel++;
}

SithCogSyntaxNode* sithCogParse_GetNextNode(void)
{
    if ( !pSyntaxTree )
    {
        pSyntaxTree = (SithCogSyntaxNode*)STDMALLOC(SITHCOGPARSE_SYNTAXTREESIZE * sizeof(SithCogSyntaxNode));
        if ( !pSyntaxTree )
        {
            return NULL;
        }

        syntaxTreeSize = SITHCOGPARSE_SYNTAXTREESIZE;
    }

    if ( numTreeNodes == syntaxTreeSize )
    {
        SITHLOG_ERROR("Max # of nodes %d exceeded, expanding size to %d.\n", syntaxTreeSize, 2 * syntaxTreeSize);
        pSyntaxTree = (SithCogSyntaxNode*)STDREALLOC(pSyntaxTree, 2 * sizeof(int32_t) * syntaxTreeSize);
        if ( !pSyntaxTree )
        {
            return NULL;
        }

        syntaxTreeSize *= 2;
    }

    SithCogSyntaxNode* pNode = &pSyntaxTree[numTreeNodes++];
    STD_ZEROMEM(pNode, sizeof(SithCogSyntaxNode));
    return pNode;
}

void sithCogParse_ResetTreeNodes(void)
{
    numTreeNodes = 0;
}

void J3DAPI sithCogParse_GenerateLabelTable(const SithCogSyntaxNode* pRoot)
{
    if ( pRoot->parentLabel )
    {
        aLabelTable[pRoot->parentLabel] = codepos;
    }

    if ( pRoot->pLeft )
    {
        sithCogParse_GenerateLabelTable(pRoot->pLeft);
    }

    if ( pRoot->pRight )
    {
        sithCogParse_GenerateLabelTable(pRoot->pRight);
    }

    switch ( pRoot->opcode )
    {
        case SITHCOGEXEC_OPCODE_NOP:
            break;

        case SITHCOGEXEC_OPCODE_PUSHINT:
        case SITHCOGEXEC_OPCODE_PUSHFLOAT:
        case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
        case SITHCOGEXEC_OPCODE_JZ:
        case SITHCOGEXEC_OPCODE_JNZ:
        case SITHCOGEXEC_OPCODE_JMP:
        case SITHCOGEXEC_OPCODE_CALL:
            codepos += 2; // opcode + value in sizeof(int32_t)
            break;

        case SITHCOGEXEC_OPCODE_PUSHVECTOR:
            codepos += 4; // opcode + vector size in sizeof(int32_t)
            break;

        default:
            ++codepos;
            break;
    }

    if ( pRoot->childLabel )
    {
        aLabelTable[pRoot->childLabel] = codepos;
    }
}

void J3DAPI sithCogParse_GenerateCode(const SithCogSyntaxNode* pNode)
{
    if ( pNode->pLeft )
    {
        sithCogParse_GenerateCode(pNode->pLeft);
    }

    if ( pNode->pRight )
    {
        sithCogParse_GenerateCode(pNode->pRight);
    }

    if ( pNode->opcode )
    {
        pCurrentCode[codepos++] = pNode->opcode;
        switch ( pNode->opcode )
        {
            case SITHCOGEXEC_OPCODE_PUSHINT:
            case SITHCOGEXEC_OPCODE_PUSHFLOAT:
            case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
                pCurrentCode[codepos++] = pNode->value;
                break;

            case SITHCOGEXEC_OPCODE_PUSHVECTOR:
                rdVector_Copy3((rdVector3*)&pCurrentCode[codepos], &pNode->vecValue);
                codepos += 3;
                break;

            case SITHCOGEXEC_OPCODE_JZ:
            case SITHCOGEXEC_OPCODE_JNZ:
            case SITHCOGEXEC_OPCODE_JMP:
            case SITHCOGEXEC_OPCODE_CALL:
                pCurrentCode[codepos++] = aLabelTable[pNode->value];
                break;

            default:
                return;
        }
    }
}

int J3DAPI sithCogParse_ParseSymbols(SithCogScript* pScript, int bParseDescription)
{
    while ( stdConffile_ReadArgs() && !streq(stdConffile_g_entry.aArgs[0].argValue, "end") ) // TODO: case-insensitive cmp?
    {
        if ( pScript->pSymbolTable->numUsedSymbols >= pScript->pSymbolTable->tableSize )
        {
            SITHLOG_ERROR("Tried to define too many symbols (%d allocated)\n", pScript->pSymbolTable->tableSize);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "thing") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_THING, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "surface") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_SURFACE, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "sector") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_SECTOR, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "sound") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_SOUND, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "template") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_TEMPLATE, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "model") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_MODEL, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "keyframe") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_KEYFRAME, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "cog") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_COG, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "message") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseMessage(pScript);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "material") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_MATERIAL, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "flex") || streq(stdConffile_g_entry.aArgs[0].argValue, "float") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseFlex(pScript, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "int") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseInt(pScript, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "vector") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseVector(pScript, bParseDescription);
        }
        else if ( streq(stdConffile_g_entry.aArgs[0].argValue, "ai") ) // TODO: case-insensitive cmp?
        {
            sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_AICLASS, bParseDescription);
        }
        else
        {
            if ( stdConffile_g_entry.numArgs > 1 )
            {
                SITHLOG_ERROR("Cog %s: Symbol type %s not valid.  Symbol %s ignored.\n", pScript->aName, stdConffile_g_entry.aArgs[0].argValue, stdConffile_g_entry.aArgs[1].argValue);
            }
            else // Added: Fixes out of bound read when numArgs < 2
            {
                SITHLOG_ERROR("Cog %s: Syntax error in symbols section line: %d\n", pScript->aName, stdConffile_GetLineNumber());
            }
        }
    }

    return 1;
}

int J3DAPI sithCogParse_ParseSymbolRef(SithCogScript* pScript, SithCogSymbolRefType symbolType, int bParseDescription)
{
    if ( pScript->numSymbolRefs >= SITHCOGSCRIPT_MAXSYMREFS )
    {
        SITHLOG_ERROR("Script %s: Max references exceeded.\n", pScript->aName);
        return 0;
    }
    if ( stdConffile_g_entry.numArgs < 2u )
    {
        SITHLOG_ERROR("Script %s: Syntax error in symbol definition, line %d.\n", pScript->aName, stdConffile_GetLineNumber());
        return 0;
    }

    SithCogSymbol* pNewSym = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_g_entry.aArgs[1].argName);
    if ( !pNewSym )
    {
        SITHLOG_ERROR("Script %s: Could not add new symbol, line %d.\n", pScript->aName, stdConffile_GetLineNumber());
        return 0;
    }

    SithCogSymbolValue value;
    value.type         = SITHCOG_VALUE_INT;
    value.val.intValue = 0;
    sithCogParse_SetSymbolValue(pNewSym, &value);

    SithCogSymbolRef* pRef = &pScript->aSymRefs[pScript->numSymbolRefs];
    STD_ZEROMEM(pRef, sizeof(SithCogSymbolRef));

    pRef->type     = symbolType;
    pRef->mask     = SITHTHING_TYPEMASK(SITH_THING_FREE, SITH_THING_PLAYER);
    pRef->symbolId = pNewSym - pScript->pSymbolTable->aSymbols;

    for ( size_t i = 2; i < stdConffile_g_entry.numArgs; ++i )
    {
        if ( streq(stdConffile_g_entry.aArgs[i].argName, "local") ) // TODO: case-insensitive cmp?
        {
            pRef->bLocal |= 1u;
        }
        else if ( bParseDescription && streq(stdConffile_g_entry.aArgs[i].argName, "desc") ) // TODO: case-insensitive cmp?
        {
            if ( pRef->pDescription )
            {
                STDFREE(pRef->pDescription);
            }

            const char* pArg = stdConffile_g_entry.aArgs[i].argValue;
            size_t descLen = strlen(pArg) + 1;
            pRef->pDescription = (char*)STDMALLOC(descLen);
            stdUtil_StringCopy(pRef->pDescription, descLen, pArg);
        }
        else if ( streq(stdConffile_g_entry.aArgs[i].argName, "mask") ) // TODO: case-insensitive cmp?
        {
            sscanf_s(stdConffile_g_entry.aArgs[i].argValue, "%x", &pRef->mask);
        }
        else if ( streq(stdConffile_g_entry.aArgs[i].argName, "linkid") ) // TODO: case-insensitive cmp?
        {
            pRef->linkId = atoi(stdConffile_g_entry.aArgs[i].argValue);
        }
        else if ( streq(stdConffile_g_entry.aArgs[i].argName, "nolink") ) // TODO: case-insensitive cmp?
        {
            pRef->linkId = -1;
        }
    }

    if ( stdConffile_g_entry.aArgs[1].argValue
        && stdConffile_g_entry.aArgs[1].argValue != stdConffile_g_entry.aArgs[1].argName )
    {
        STD_STRCPY(pScript->aSymRefs[pScript->numSymbolRefs].aValue, stdConffile_g_entry.aArgs[1].argValue);
    }

    ++pScript->numSymbolRefs;
    return 1;
}

int J3DAPI sithCogParse_ParseFlex(SithCogScript* pScript, int bParseDescription)
{
    SithCogSymbol* pSymbol = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_g_entry.aArgs[1].argName);
    if ( !pSymbol )
    {
        SITHLOG_ERROR("Could not add new symbol to Cog, line %d.\n", stdConffile_GetLineNumber());
        return 0;
    }

    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_FLOAT;
    value.val.floatValue = atof(stdConffile_g_entry.aArgs[1].argValue);
    sithCogParse_SetSymbolValue(pSymbol, &value);

    char* pDesc = NULL;
    for ( size_t i = 2; i < stdConffile_g_entry.numArgs; ++i )
    {
        if ( streq(stdConffile_g_entry.aArgs[i].argName, "local") ) // TODO: case-insensitive cmp?
        {
            return 1;
        }

        if ( bParseDescription )
        {
            if ( streq(stdConffile_g_entry.aArgs[i].argName, "desc") ) // TODO: case-insensitive cmp?
            {
                const char* pArg = stdConffile_g_entry.aArgs[i].argValue;
                size_t descLen = strlen(pArg) + 1;
                pDesc = (char*)STDMALLOC(descLen);
                stdUtil_StringCopy(pDesc, descLen, pArg);
            }
        }
    }

    size_t idx = pScript->numSymbolRefs;
    pScript->aSymRefs[idx].type         = SITHCOG_SYM_REF_FLEX;
    pScript->aSymRefs[idx].linkId       = -1;
    pScript->aSymRefs[idx].symbolId     = pSymbol->id;
    pScript->aSymRefs[idx].pDescription = pDesc;

    ++pScript->numSymbolRefs;
    return 1;
}

int J3DAPI sithCogParse_ParseInt(SithCogScript* pScript, int bParseDescription)
{
    SithCogSymbol* pSym = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_g_entry.aArgs[1].argName);
    if ( !pSym )
    {
        SITHLOG_ERROR("Could not add new symbol to Cog, line %d.\n", stdConffile_GetLineNumber());
        return 0;
    }

    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_INT;
    value.val.intValue = atoi(stdConffile_g_entry.aArgs[1].argValue);
    sithCogParse_SetSymbolValue(pSym, &value);

    char* pDesc = NULL;
    for ( size_t i = 2; i < stdConffile_g_entry.numArgs; ++i )
    {
        if ( streq(stdConffile_g_entry.aArgs[i].argName, "local") ) // TODO: case-insensitive cmp?
        {
            return 1;
        }

        if ( bParseDescription )
        {
            if ( streq(stdConffile_g_entry.aArgs[i].argName, "desc") ) // TODO: case-insensitive cmp?
            {
                const char* pArg = stdConffile_g_entry.aArgs[i].argValue;
                size_t descLen = strlen(pArg) + 1;
                pDesc = (char*)STDMALLOC(descLen);
                stdUtil_StringCopy(pDesc, descLen, pArg);
            }
        }
    }

    size_t index = pScript->numSymbolRefs;
    pScript->aSymRefs[index].type         = SITHCOG_SYM_REF_INT;
    pScript->aSymRefs[index].linkId       = -1;
    pScript->aSymRefs[index].symbolId     = pSym - pScript->pSymbolTable->aSymbols;
    pScript->aSymRefs[index].pDescription = pDesc;

    ++pScript->numSymbolRefs;
    return 1;
}

int J3DAPI sithCogParse_ParseVector(SithCogScript* pScript, int bParseDescription)
{
    SithCogSymbol* pSym = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_g_entry.aArgs[1].argName);
    if ( !pSym )
    {
        SITHLOG_ERROR("Could not add new symbol to Cog, line %d.\n", stdConffile_GetLineNumber());
        return 0;
    }

    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_VECTOR;
    STD_ZEROMEM(&value.val, sizeof(value.val)); // TODO: Parse vector
    sithCogParse_SetSymbolValue(pSym, &value);

    char* pDesc = NULL;
    for ( size_t i = 2; i < stdConffile_g_entry.numArgs; ++i )
    {
        if ( streq(stdConffile_g_entry.aArgs[i].argName, "local") ) // TODO: case-insensitive cmp?
        {
            return 1;
        }

        if ( bParseDescription )
        {
            if ( streq(stdConffile_g_entry.aArgs[i].argName, "desc") ) // TODO: case-insensitive cmp?
            {
                const char* pArg = stdConffile_g_entry.aArgs[i].argValue;
                size_t descLen = strlen(pArg) + 1;
                pDesc = (char*)STDMALLOC(descLen);
                stdUtil_StringCopy(pDesc, descLen, pArg);
            }
        }
    }

    size_t index = pScript->numSymbolRefs;
    pScript->aSymRefs[index].type         = SITHCOG_SYM_REF_VECTOR;
    pScript->aSymRefs[index].linkId       = -1;
    pScript->aSymRefs[index].symbolId     = pSym - pScript->pSymbolTable->aSymbols;
    pScript->aSymRefs[index].pDescription = pDesc;
    ++pScript->numSymbolRefs;
    return 1;
}

int J3DAPI sithCogParse_ParseMessage(SithCogScript* pScript)
{
    if ( pScript->numHandlers == STD_ARRAYLEN(pScript->aHandlers) )
    {
        SITHLOG_ERROR("Too many message handlers in script %s.\n", pScript->aName);
        return 0;
    }

    SithCogSymbol* pMsgTypeSym = sithCogParse_GetSymbol(sithCog_g_pSymbolTable, stdConffile_g_entry.aArgs[1].argValue);
    if ( !pMsgTypeSym )
    {
        SITHLOG_ERROR("Cog message type %s is not recognized, %s.\n", stdConffile_g_entry.aArgs[1].argValue, pScript->aName);
        return 0;
    }

    SithCogSymbol* pSymbol = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_g_entry.aArgs[1].argValue);
    if ( !pSymbol )
    {
        return 0;
    }

    pSymbol->value.type = SITHCOG_VALUE_INT;
    pSymbol->value.val.intValue = pMsgTypeSym->value.val.intValue;

    pScript->aHandlers[pScript->numHandlers].type    = pMsgTypeSym->value.val.intValue;
    pScript->aHandlers[pScript->numHandlers++].label = pSymbol->label;
    return 1;
}

void J3DAPI sithCogParse_ResolveMessageHandlers(SithCogScript* pScript)
{
    for ( size_t i = 0; i < pScript->numHandlers; ++i )
    {
        pScript->aHandlers[i].codeOffset = aLabelTable[pScript->aHandlers[i].label];
    }
}