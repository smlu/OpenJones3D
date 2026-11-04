#ifndef SITH_SITHPLAYER_H
#define SITH_SITHPLAYER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

#include <wchar.h>

J3D_EXTERN_C_START

#define SITHPLAYER_MAX_PLAYERS 24

#define SITHPLAYER_DEFAULTLOCALPLAYERNUM 0

#define SITHPLAYER_IMPFIRE_OFF -1

extern int sithPlayer_g_bPlayerInPor;
// extern int sithPlayer_g_bPlayerInPor ;

extern int sithPlayer_g_impFireType;
// extern int sithPlayer_g_impFireType ;

extern size_t sithPlayer_g_playerNum;
// extern size_t sithPlayer_g_playerNum;

extern size_t sithPlayer_g_numPlayers;
// extern size_t sithPlayer_g_numPlayers;

extern SithThing* sithPlayer_g_pLocalPlayerThing;
// extern SithThing *sithPlayer_g_pLocalPlayerThing;

extern SithPlayer* sithPlayer_g_pLocalPlayer;
// extern SithPlayer *sithPlayer_g_pLocalPlayer;

extern int sithPlayer_g_bPlayerInvulnerable;
// extern int sithPlayer_g_bPlayerInvulnerable;

extern int sithPlayer_g_bInAetheriumSector;
// extern int sithPlayer_g_bInAetheriumSector;

extern int sithPlayer_g_bGuybrush;
// extern int sithPlayer_g_bGuybrush;

extern int sithPlayer_g_curLevelNum;
// extern int sithPlayer_g_curLevelNum;

extern int sithPlayer_g_bBonusMapBought;
// extern int sithPlayer_g_bBonusMapBought;

extern SithPlayer sithPlayer_g_aPlayers[SITHPLAYER_MAX_PLAYERS];
// extern SithPlayer sithPlayer_g_aPlayers[24];

extern float sithPlayer_g_impState;
// extern float sithPlayer_g_impState;

void J3DAPI sithPlayer_Open(const wchar_t* awName);
void sithPlayer_Close(void);

void J3DAPI sithPlayer_NewPlayer(SithThing* pPlayer);
void J3DAPI sithPlayer_Reset(size_t playerNum);

void J3DAPI sithPlayer_PlacePlayers(SithWorld* pWorld);
void J3DAPI sithPlayer_HidePlayer(size_t playerNum);
int J3DAPI sithPlayer_ShowPlayer(size_t playerNum, DPID id);

void J3DAPI sithPlayer_SetLocalPlayer(size_t playerNum);
const wchar_t* sithPlayer_GetLocalPlayerName(void);

void J3DAPI sithPlayer_Update(SithPlayer* pPlayer, float secDetaTime);

int J3DAPI sithPlayer_GetPlayerNumByName(wchar_t* pwName); // Added
int J3DAPI sithPlayer_GetPlayerNum(DPID playerId);
int J3DAPI sithPlayer_GetThingPlayerNum(const SithThing* pThing);
int J3DAPI sithPlayer_GetThingPlayerNumByIndex(int thingIdx); // Added

void J3DAPI sithPlayer_PlayerKilledAction(SithThing* pPlayerThing, const SithThing* pSrcThing);
void J3DAPI sithPlayer_KillPlayer(SithThing* pPlayerThing);

float J3DAPI sithPlayer_GetInvItemAmount(size_t binIndex); // Added
void J3DAPI sithPlayer_SetInvItemAmount(size_t binIndex, float amount); // Added
void J3DAPI sithPlayer_SetInvItemAvailable(size_t binIndex, int bAvailable); // Added
bool J3DAPI sithPlayer_IsInvItemAvailable(size_t binIndex); // Added

int J3DAPI sithPlayer_ToggleGuybrush();

void J3DAPI sithPlayer_IMPStartFiring(int fireType);
void J3DAPI sithPlayer_IMPEndFiring(int fireType);

void sithPlayer_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPLAYER_H
