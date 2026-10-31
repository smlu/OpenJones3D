#ifndef SITH_SITHPLAYER_H
#define SITH_SITHPLAYER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHPLAYER_MAX_PLAYERS 24

#define SITHPLAYER_DEFAULTLOCALPLAYERNUM 0

#define SITHPLAYER_IMPFIRE_OFF -1

#if defined(J3D_STANDALONE)
extern int sithPlayer_g_bPlayerInPor;
#else
#define sithPlayer_g_bPlayerInPor J3D_DECL_FAR_VAR(sithPlayer_g_bPlayerInPor, int)
#endif
// extern int sithPlayer_g_bPlayerInPor ;

#if defined(J3D_STANDALONE)
extern int sithPlayer_g_impFireType;
#else
#define sithPlayer_g_impFireType J3D_DECL_FAR_VAR(sithPlayer_g_impFireType, int)
#endif
// extern int sithPlayer_g_impFireType ;

#if defined(J3D_STANDALONE)
extern size_t sithPlayer_g_playerNum;
#else
#define sithPlayer_g_playerNum J3D_DECL_FAR_VAR(sithPlayer_g_playerNum, size_t)
#endif
// extern size_t sithPlayer_g_playerNum;

#if defined(J3D_STANDALONE)
extern size_t sithPlayer_g_numPlayers;
#else
#define sithPlayer_g_numPlayers J3D_DECL_FAR_VAR(sithPlayer_g_numPlayers, size_t)
#endif
// extern size_t sithPlayer_g_numPlayers;

#if defined(J3D_STANDALONE)
extern SithThing* sithPlayer_g_pLocalPlayerThing;
#else
#define sithPlayer_g_pLocalPlayerThing J3D_DECL_FAR_VAR(sithPlayer_g_pLocalPlayerThing, SithThing*)
#endif
// extern SithThing *sithPlayer_g_pLocalPlayerThing;

#if defined(J3D_STANDALONE)
extern SithPlayer* sithPlayer_g_pLocalPlayer;
#else
#define sithPlayer_g_pLocalPlayer J3D_DECL_FAR_VAR(sithPlayer_g_pLocalPlayer, SithPlayer*)
#endif
// extern SithPlayer *sithPlayer_g_pLocalPlayer;

#if defined(J3D_STANDALONE)
extern int sithPlayer_g_bPlayerInvulnerable;
#else
#define sithPlayer_g_bPlayerInvulnerable J3D_DECL_FAR_VAR(sithPlayer_g_bPlayerInvulnerable, int)
#endif
// extern int sithPlayer_g_bPlayerInvulnerable;

#if defined(J3D_STANDALONE)
extern int sithPlayer_g_bInAetheriumSector;
#else
#define sithPlayer_g_bInAetheriumSector J3D_DECL_FAR_VAR(sithPlayer_g_bInAetheriumSector, int)
#endif
// extern int sithPlayer_g_bInAetheriumSector;

#if defined(J3D_STANDALONE)
extern int sithPlayer_g_bGuybrush;
#else
#define sithPlayer_g_bGuybrush J3D_DECL_FAR_VAR(sithPlayer_g_bGuybrush, int)
#endif
// extern int sithPlayer_g_bGuybrush;

#if defined(J3D_STANDALONE)
extern int sithPlayer_g_curLevelNum;
#else
#define sithPlayer_g_curLevelNum J3D_DECL_FAR_VAR(sithPlayer_g_curLevelNum, int)
#endif
// extern int sithPlayer_g_curLevelNum;

#if defined(J3D_STANDALONE)
extern int sithPlayer_g_bBonusMapBought;
#else
#define sithPlayer_g_bBonusMapBought J3D_DECL_FAR_VAR(sithPlayer_g_bBonusMapBought, int)
#endif
// extern int sithPlayer_g_bBonusMapBought;

#if defined(J3D_STANDALONE)
extern SithPlayer sithPlayer_g_aPlayers[SITHPLAYER_MAX_PLAYERS];
#else
#define sithPlayer_g_aPlayers J3D_DECL_FAR_ARRAYVAR(sithPlayer_g_aPlayers, SithPlayer(*)[SITHPLAYER_MAX_PLAYERS])
#endif
// extern SithPlayer sithPlayer_g_aPlayers[24];

#if defined(J3D_STANDALONE)
extern float sithPlayer_g_impState;
#else
#define sithPlayer_g_impState J3D_DECL_FAR_VAR(sithPlayer_g_impState, float)
#endif
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

// Helper hooking functions
void sithPlayer_InstallHooks(void);
void sithPlayer_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHPLAYER_H
