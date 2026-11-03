#ifndef SITH_SITHCOMM_H
#define SITH_SITHCOMM_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>
#include <std/types.h>

#include <wchar.h>

J3D_EXTERN_C_START

typedef enum eSithMessageStream
{
    SITHMESSAGE_STREAM_NONE = 0x00,
    SITHMESSAGE_STREAM_NET  = 0x01,
    SITHMESSAGE_STREAM_FILE = 0x04,
    SITHMESSAGE_STREAM_ALL  = 0xFF,
} SithMessageStream;

extern DPID sithMessage_g_localPlayerId;
// extern DPID sithMessage_g_localPlayerId;

extern SithMessageStream sithMessage_g_outputstream;
// extern int sithMessage_g_outputstream;

extern SithMessageStream sithMessage_g_inputstream;
// extern int sithMessage_g_inputstream;

// NOTE: all sithMessage functions should be in sithComm module
void sithMessage_Startup(void);
void sithMessage_Shutdown(void);

int J3DAPI sithMessage_SendMessage(SithMessage* pMessage, DPID idTo, SithMessageStream outstream, uint32_t dwDPFlags);

void J3DAPI sithMessage_ProcessMessages();
int J3DAPI sithMessage_Process(const SithMessage* pMessage);
void J3DAPI sithMessage_StopProcessMessages();
void sithMessage_Flush(void); // Added, discards any inbound message

DPID sithMessage_GetSenderID(void);

int J3DAPI sithMessage_StartGame(StdCommGame* pSettings); // Added
int J3DAPI sithMessage_JoinGame(size_t gameNum, const wchar_t* pPassword); // Added
void J3DAPI sithMessage_CheckMembershipList(DPID idTo);
void sithMessage_CloseGame(void);

void J3DAPI sithMessage_RegisterFunction(int type, SithMessageProcessFunc pFunc);

// Helper hooking functions
void sithComm_InstallHooks(void);
void sithComm_ResetGlobals(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCOMM_H
