#ifndef STD_STDCOMM_H
#define STD_STDCOMM_H
#include <j3dcore/j3d.h>
#include <std/types.h>
#include <std/RTI/addresses.h>

#include <wchar.h>

J3D_EXTERN_C_START

#define STDCOMM_MAX_PLAYERS 24U
#define STDCOMM_MAX_GAMES   32U

int32_t J3DAPI stdComm_Send(DPID idFrom, DPID idTo, const void* pData, uint32_t size, uint32_t flags);
int32_t J3DAPI stdComm_Receive(DPID* pSender, void* pData, size_t* pLength);

int J3DAPI stdComm_GetGame(size_t gameNum, StdCommGame* pGame); // Added
HRESULT J3DAPI stdComm_CreateGame(const StdCommGame* pSettings); // Added
HRESULT J3DAPI stdComm_SetGameParams(StdCommGame* pSettings); // Added
int J3DAPI stdComm_JoinGame(size_t gameNum, const wchar_t* pPassword); // Added
void stdComm_CloseGame(void);

int J3DAPI stdComm_RejoinSession(DPID* pPlayerId, const wchar_t* pPlayerName);
int J3DAPI stdComm_GetSessionSettings(StdCommGame* pSettings);
int J3DAPI stdComm_IsGameActive(void);
int J3DAPI stdComm_IsGameHost(void);
int J3DAPI stdComm_UpdatePlayers(size_t gameNum);
DPID J3DAPI stdComm_CreatePlayer(const wchar_t* pPlayerName);
void J3DAPI stdComm_DestroyPlayer(DPID playerId);
size_t stdComm_GetNumPlayers(void);
int J3DAPI stdComm_VerifyPlayer(DPID id);
DPID J3DAPI stdComm_GetPlayerID(size_t playerNum);

// Helper hooking functions
void stdComm_InstallHooks(void);
void stdComm_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // STD_STDCOMM_H
