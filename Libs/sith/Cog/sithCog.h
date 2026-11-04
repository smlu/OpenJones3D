#ifndef SITH_SITHCOG_H
#define SITH_SITHCOG_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHCOG_TASKID              4u
#define SITHCOG_NORETURNVALUE      -9999
#define SITHCOG_GLOBALSYMBOLSTARTID SITHCOGSCRIPT_MAXSYMREFS

extern SithCogSymbolTable* sithCog_g_pSymbolTable;
// extern SithCogSymbolTable *sithCog_g_pSymbolTable;

extern SithCog* sithCog_g_pMasterCog;
// extern SithCog *sithCog_g_pMasterCog;

int sithCog_Startup(void);
void sithCog_Shutdown(void);

// On error 1 is returned otherwise 0
int J3DAPI sithCog_Open(SithWorld* pWorld);
void sithCog_Close(void);

void J3DAPI sithCog_FreeWorldCogs(SithWorld* pWorld);
void J3DAPI sithCog_FreeScriptEntry(SithCogScript* pScript);
void J3DAPI sithCog_FreeEntry(SithCog* pCog);
void J3DAPI sithCog_UpdateThingTimer(SithThing* pThing);

void J3DAPI sithCog_BroadcastMessage(SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx);
void J3DAPI sithCog_BroadcastMessageEx(SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int param0, int param1, int param2, int param3);

void J3DAPI sithCog_SendMessage(SithCog* pCog, SithCogMsgType messageType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId);

// The return value SITHCOG_NORETURNVALUE ('-9999') means no return value was returned after COG was executed.
// Same value is returned when COG is paused, disabled or message handler for the type doesn't exist.
// In this case no message is sent further to COG.
// In case message was sent over net via sithDSSCog_SendMessage the return value is 0.
int J3DAPI sithCog_SendMessageEx(SithCog* pCog, SithCogMsgType messageType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId, int param0, int param1, int param2, int param3);

int J3DAPI sithCog_AllocWorldCogScripts(SithWorld* pWorld, size_t numCogScripts);
int J3DAPI sithCog_AllocWorldCogs(SithWorld* pWorld, size_t sizeCogs);

// Functions for loading & writing COGs from/to NDY/CND files
int J3DAPI sithCog_WriteCogsListText(const SithWorld* pWorld); // Added from debug
int J3DAPI sithCog_ReadCogsListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithCog_WriteCogsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithCog_ReadCogsListBinary(tFileHandle fh, SithWorld* pWorld);

// On success 0 is returned and 1 on error
int J3DAPI sithCog_ParseSymbolRef(SithWorld* pWorld, SithCogSymbol* pSymbol, const SithCogSymbolRef* pRef, const char* pString);

SithCog* J3DAPI sithCog_Load(SithWorld* pWorld, const char* pName); // Loads COG from cache or COG script file
SithCog* J3DAPI sithCog_GetCogByIndex(int idx);
SithCog* J3DAPI sithCog_GetCog(const char* pName);

int J3DAPI sithCog_LinkCogToThing(const SithWorld* pWorld, SithCog* pCog, SithThing* pThing, int linkId, int mask);
int J3DAPI sithCog_LinkCogToSurface(SithCog* pCog, SithSurface* pSurface, int linkId, int mask);
signed int J3DAPI sithCog_LinkCogToSector(const SithWorld* pWorld, SithCog* pCog, SithSector* pSector, int linkId, int mask);

void sithCog_FixupLinksToThings(void);

int J3DAPI sithCog_ThingSendMessage(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType msgType);
int J3DAPI sithCog_ThingSendMessageEx(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3);
int J3DAPI sithCog_SectorSendMessage(const SithSector* pSector, const SithThing* pThing, SithCogMsgType msgType);
int J3DAPI sithCog_SectorSendMessageEx(const SithSector* pSector, const SithThing* pThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3);
int J3DAPI sithCog_SurfaceSendMessage(const SithSurface* pSurf, const SithThing* pSrcThing, SithCogMsgType messageType);
int J3DAPI sithCog_SurfaceSendMessageEx(const SithSurface* pSurf, const SithThing* pSrcThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3);

// Functions for loading & writing COG Scripts from/to NDY/CND files
int J3DAPI sithCog_WriteCogScriptsListText(const SithWorld* pWorld);// Added
int J3DAPI sithCog_ReadCogScriptsListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithCog_WriteCogScriptsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithCog_ReadCogScriptsListBinary(tFileHandle fh, SithWorld* pWorld);

SithCogScript* J3DAPI sithCog_LoadScript(SithWorld* pWorld, const char* pName); // Loads COG script

void J3DAPI sithCog_ProcessCog(SithCog* pCog);
void sithCog_ProcessCogs(void);
int J3DAPI sithCog_CogStatus(const SithConsoleCommand* pFunc, const char* pArg); // Added

// Maybe following should be all called register* functions
void J3DAPI sithCog_AddIntSymbol(SithCogSymbolTable* pTbl, int val, const char* pName);
void J3DAPI sithCog_AddFloatSymbol(SithCogSymbolTable* pTbl, const char* pName, float value);
int J3DAPI sithCog_RegisterFunction(SithCogSymbolTable* pTable, SithCogFunctionType pFunction, const char* pName);

int J3DAPI sithCog_IsThingLinked(const SithThing* pThing);

// Helper hooking functions
void sithCog_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOG_H
