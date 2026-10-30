#ifndef SITH_SITHMULTI_H
#define SITH_SITHMULTI_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHMULTI_CHECKPLAYER_TASKID 2u

#define sithMulti_g_serverId J3D_DECL_FAR_VAR(sithMulti_g_serverId, DPID)
// extern DPID sithMulti_g_serverId;

#define sithMulti_g_message J3D_DECL_FAR_VAR(sithMulti_g_message, SithMessage)
// extern SithMessage sithMulti_g_message;

void sithMulti_OpenGame(void); // Added
void sithMulti_CloseGame();

int J3DAPI sithMulti_CheckPlayers(int msecTime, SithEventParams* pParam);

size_t J3DAPI sithMulti_GetTickRate(void); // Added: from debug
void J3DAPI sithMulti_SetTickRate(size_t tickRate); // Added: from debug

void J3DAPI sithMulti_ProcessPlayerLost(DPID idPlayer);
void J3DAPI sithMulti_RemovePlayer(unsigned int playerNum);
signed int J3DAPI sithMulti_SendWelcome(DPID idPlayer, int playerNum, DPID idTo);
int J3DAPI sithMulti_ProcessWelcome(const SithMessage* pMsg);
void J3DAPI sithMulti_ProcessPlayerJoin(int playerNum);
void J3DAPI sithMulti_SyncPlayers(DPID idTo, uint32_t dpFlags);
signed int J3DAPI sithMulti_ProcessSyncPlayers(const SithMessage* pMsg);
int J3DAPI sithMulti_ProcessJoinRequest(const SithMessage* pMsg);
signed int J3DAPI sithMulti_FinishJoining(SithMultiJoinStatus code, float progress, int playerId);
int J3DAPI sithMulti_ProcessChat(const SithMessage* pMsg);

int J3DAPI sithMulti_Ping(DPID idTo); // Added: from debug
int J3DAPI sithMulti_ProcessPing(const SithMessage* pMsg);
int J3DAPI sithMulti_ProcessPong(const SithMessage* pMsg);

void J3DAPI sithMulti_QuitPlayer(DPID id);
int J3DAPI sithMulti_ProcessQuit(const SithMessage* pMsg);

void J3DAPI sithMulti_Update(unsigned int msecDeltaTime);
void J3DAPI sithMulti_SyncScores();
int J3DAPI sithMulti_GetPlayerIndexByID(DPID idPlayer);
void J3DAPI sithMulti_ProcessKilledPlayer(const SithPlayer* pPlayer, const SithThing* pPlayerThing, const SithThing* pKiller);
void J3DAPI sithMulti_QuitGame(unsigned int msecTime, int state);
size_t J3DAPI sithMulti_Respawn(SithThing* pPlayer);
void J3DAPI sithMulti_RemoveStaticThing(int guid);
void J3DAPI sithMulti_UpdateSurfaces();
void J3DAPI sithMulti_UpdateSectors();
void J3DAPI sithMulti_UpdateThings();
void J3DAPI sithMulti_UpdateRemovals();
void J3DAPI sithMulti_StartWelcomingPlayer(DPID playerId);
void J3DAPI sithMulti_StopWelcomingPlayer(int bError);

// Helper hooking functions
void sithMulti_InstallHooks(void);
void sithMulti_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHMULTI_H
