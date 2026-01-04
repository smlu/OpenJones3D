#include "sithComm.h"
#include <j3dcore/j3dhook.h>

#include <sith/AI/sithAIUtil.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/RTI/symbols.h>
#include <sith/World/sithVoice.h>

#include <std/General/stdConffile.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

static SithMessageProcessFunc sithMessage_aTypeFuncs[65] = { 0 };

static bool sithMessage_bSturtup             = false;
static bool sithMessage_bStopProcessMessages = false;
static size_t sithMessage_totalSentDataSize  = 0;
static size_t sithMessage_totalNetSent       = 0;
static DPID sithMessage_senderId             = 0;
static SithMessage outmsg                    = { 0 };

int J3DAPI sithMessage_NetWrite(const SithMessage* pMsg, DPID idTo, uint32_t flags);
int J3DAPI sithMessage_ReceiveMessage(SithMessage* pMsg, DPID* pSender);
int J3DAPI sithMessage_ProcessCheckMembershipList(const SithMessage* pMsg);
void sithMessage_InitMessageHandlers(void);
int J3DAPI sithMessage_FileWrite(const SithMessage* pMessage);

void sithComm_InstallHooks(void)
{
    J3D_HOOKFUNC(sithMessage_Startup);
    J3D_HOOKFUNC(sithMessage_Shutdown);
    J3D_HOOKFUNC(sithMessage_SendMessage);
    J3D_HOOKFUNC(sithMessage_ProcessMessages);
    J3D_HOOKFUNC(sithMessage_StopProcessMessages);
    J3D_HOOKFUNC(sithMessage_GetSenderID);
    J3D_HOOKFUNC(sithMessage_NetWrite);
    J3D_HOOKFUNC(sithMessage_ReceiveMessage);
    J3D_HOOKFUNC(sithMessage_CloseGame);
    J3D_HOOKFUNC(sithMessage_CheckMembershipList);
    J3D_HOOKFUNC(sithMessage_ProcessCheckMembershipList);
    J3D_HOOKFUNC(sithMessage_RegisterFunction);
    J3D_HOOKFUNC(sithMessage_Process);
    J3D_HOOKFUNC(sithMessage_InitMessageHandlers);
    J3D_HOOKFUNC(sithMessage_FileWrite);
}

void sithComm_ResetGlobals(void)
{
    memset(&sithMessage_g_localPlayerId, 0, sizeof(sithMessage_g_localPlayerId));
    memset(&sithMessage_g_outputstream, 0, sizeof(sithMessage_g_outputstream));
    memset(&sithMessage_g_inputstream, 0, sizeof(sithMessage_g_inputstream));
}

void sithMessage_Startup(void)
{
    if ( sithMessage_bSturtup )
    {
        SITHLOG_ERROR("Multiple startup on message system.\n");
        return;
    }

    sithMessage_totalSentDataSize    = 0;
    sithMessage_totalNetSent         = 0;
    sithMessage_senderId             = 0;
    sithMessage_bStopProcessMessages = false;

    memset(sithMessage_aTypeFuncs, 0, sizeof(sithMessage_aTypeFuncs));
    sithMessage_InitMessageHandlers();

    sithMessage_bSturtup = true;
}

void sithMessage_Shutdown(void)
{
    if ( !sithMessage_bSturtup )
    {
        SITHLOG_ERROR("Shutdown of message system w/o startup.\n");
    }

    sithMessage_totalSentDataSize    = 0;
    sithMessage_totalNetSent         = 0;
    sithMessage_senderId             = 0;
    sithMessage_bStopProcessMessages = 0;
    memset(sithMessage_aTypeFuncs, 0, sizeof(sithMessage_aTypeFuncs));

    sithMessage_bSturtup = false;
}

int J3DAPI sithMessage_SendMessage(SithMessage* pMessage, DPID idTo, SithMessageStream outstream, uint32_t dwDPFlags)
{
    SITH_ASSERTREL(pMessage != NULL);
    SITH_ASSERTREL((pMessage->type >= 0) && (pMessage->type < STD_ARRAYLEN(sithMessage_aTypeFuncs)));
    SITH_ASSERTREL(pMessage->length <= STD_ARRAYLEN(pMessage->data));

    uint32_t stream = sithMessage_g_outputstream & outstream;
    if ( stream == SITHMESSAGE_STREAM_NONE )
    {
        return 1;
    }

    pMessage->msecTime = sithTime_g_msecGameTime;
    if ( (stream & SITHMESSAGE_STREAM_NET) != 0 )
    {
        return sithMessage_NetWrite(pMessage, idTo, dwDPFlags);
    }

    if ( (stream & SITHMESSAGE_STREAM_FILE) != 0 )
    {
        return sithMessage_FileWrite(pMessage);
    }

    return 1; // Added: return error
}

void sithMessage_ProcessMessages(void)
{
    if ( sithMessage_g_inputstream == SITHMESSAGE_STREAM_NONE )
    {
        SITHLOG_ERROR("Called ProcessMessages without any input stream to read from.\n");
    }

    sithMessage_bStopProcessMessages = false;
    while ( !sithMessage_bStopProcessMessages && sithMessage_ReceiveMessage(&sithMulti_g_message, &sithMessage_senderId) == 1 )
    {
        int playerNum = sithPlayer_GetPlayerNum(sithMessage_senderId);
        if ( playerNum >= 0 )
        {
            sithPlayer_g_aPlayers[playerNum].msecLastCommTime = sithTime_g_msecGameTime;
            sithMessage_Process(&sithMulti_g_message);
            continue;
        }

        if ( sithMulti_g_message.type != SITHDSS_WELCOME
            && sithMulti_g_message.type != SITHDSS_JOIN
            && sithMulti_g_message.type != SITHDSS_SYNCPLAYERS
            && (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) == 0 )
        {
            SITHLOG_ERROR("Ignored message type %d from non-active sender %x.\n", sithMulti_g_message.type, sithMessage_senderId);

            if ( stdComm_IsGameHost() )
            {
                sithMulti_QuitPlayer(sithMessage_senderId);
            }
        }
        else
        {
            sithMessage_Process(&sithMulti_g_message);
        }
    }
}

void sithMessage_StopProcessMessages(void)
{
    sithMessage_bStopProcessMessages = true;
}

DPID sithMessage_GetSenderID(void)
{
    return sithMessage_senderId;
}

int J3DAPI sithMessage_NetWrite(const SithMessage* pMsg, DPID idTo, uint32_t flags)
{
    SITH_ASSERTREL(pMsg); // Fixed: Moved this assert to the start, before accessing the pMsg

    int bFailed = 0;
    uint32_t dataSize = pMsg->length + sizeof(pMsg->type); // 4 is the size of message type
    if ( idTo == SITHMESSAGE_SENDTOJOINEDPLAYERS )
    {
        for ( size_t i = 0; i < sithPlayer_g_numPlayers; ++i )
        {
            if ( (sithPlayer_g_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && sithPlayer_g_aPlayers[i].playerNetId != sithMessage_g_localPlayerId )
            {
                int hr = stdComm_Send(sithMessage_g_localPlayerId, sithPlayer_g_aPlayers[i].playerNetId, &pMsg->type, dataSize, flags);
                if ( hr )
                {
                    bFailed = hr;
                }

                ++sithMessage_totalNetSent;
                sithMessage_totalSentDataSize += dataSize;
            }
        }
    }
    else
    {
        bFailed = stdComm_Send(sithMessage_g_localPlayerId, idTo, &pMsg->type, dataSize, flags);
        ++sithMessage_totalNetSent;
        sithMessage_totalSentDataSize += dataSize;
    }

    return bFailed;
}

int J3DAPI sithMessage_ReceiveMessage(SithMessage* pMsg, DPID* pSender)
{
    SITH_ASSERTREL(pMsg != NULL); // Fixed: Moved this assert to beginning of scope before accessing pMsg

    size_t dataLen = STD_ARRAYLEN(pMsg->data);
    int res = stdComm_Receive(pSender, &pMsg->type, &dataLen);
    if ( res == -1 ) // no more messages
    {
        return 0;
    }

    if ( res == 0 ) // success
    {
        pMsg->length = dataLen - sizeof(pMsg->type);
        pMsg->msecTime = sithTime_g_msecGameTime;
        return 1;
    }

    if ( (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) != 0 )
    {
        return 0;
    }

    switch ( res )
    {
        case 2: // Destroy player or group
            sithMulti_ProcessPlayerLost(*pSender);
            break;

        case 5: // Create player or group
            if ( stdComm_IsGameHost() )
            {
                sithMulti_SyncPlayers(*pSender, 1u);
            }

            break;

        case 8: // host
            sithMulti_g_serverId = sithMessage_g_localPlayerId;
            break;
    }

    return 0;
}

int J3DAPI sithMessage_JoinGame(size_t gameNum, const wchar_t* pPassword)
{
    int res = stdComm_JoinGame(gameNum, pPassword);
    if ( res )
    {
        SITHLOG_ERROR("Problem opening game session %d.\n", gameNum);
        return res;
    }

    const wchar_t* pPlayerName = sithPlayer_GetLocalPlayerName();
    sithMessage_g_localPlayerId = stdComm_CreatePlayer(pPlayerName);
    if ( !sithMessage_g_localPlayerId )
    {
        SITHLOG_ERROR("Create Local Player failed.\n");
        stdComm_CloseGame();
        sithMessage_g_localPlayerId = 0;
        return DPERR_GENERIC;
    }

    SITHLOG_STATUS("Joined game %d as player %d.\n", gameNum, sithMessage_g_localPlayerId);
    return 0;
}

void sithMessage_CloseGame(void)
{
    if ( sithMessage_g_localPlayerId )
    {
        stdComm_DestroyPlayer(sithMessage_g_localPlayerId);
    }

    stdComm_CloseGame();
    sithMessage_g_localPlayerId = 0;
}

int J3DAPI sithMessage_StartGame(StdCommGame* pSettings)
{
    SITH_ASSERTREL(pSettings);
    if ( sithMessage_g_localPlayerId )
    {
        int res = stdComm_SetGameParams(pSettings);
        if ( res )
        {
            stdComm_CloseGame();
            SITHLOG_ERROR("Could not set session params for game %S.\n", pSettings->aSessionName);
            return res;
        }
        return 0;
    }

    int res = stdComm_CreateGame(pSettings);
    if ( res )
    {
        SITHLOG_ERROR("Could not create new game %S.\n", pSettings->aSessionName);
        return res;
    }

    sithMessage_g_localPlayerId = stdComm_CreatePlayer(sithPlayer_GetLocalPlayerName());
    if ( !sithMessage_g_localPlayerId )
    {
        SITHLOG_ERROR("Create Local Player failed.\n");
        stdComm_CloseGame();
        sithMessage_g_localPlayerId = 0;
        return DPERR_GENERIC;
    }

    SITHLOG_STATUS("Started game '%S' as player %d.\n", pSettings->aSessionName, sithMessage_g_localPlayerId);
    return 0;
}

int sithMessage_sub_465EA8(void) // Found in debug version of Indy3D
{
    if ( 0 ) // Note, looks like it could be a #if 0 macro
    {
        sithMessage_g_localPlayerId = 0;
        stdComm_CloseGame();
    }
    return 1;
}

void sithMessage_Flush(void)
{
    size_t msgLen  = STD_ARRAYLEN(sithMulti_g_message.data);
    size_t numMsgs = 0;
    while ( 1 )
    {
        DPID idSender;
        int res = stdComm_Receive(&idSender, sithMulti_g_message.data, &msgLen);
        if ( res == -1 ) // no more messages
        {
            break;
        }

        ++numMsgs;

        SITHLOG_STATUS("Flushed message type %d from player %x.\n", sithMulti_g_message.type, idSender);
    }

    SITHLOG_STATUS("Flushed %d pending messages from the queue.\n", numMsgs);
}

void J3DAPI sithMessage_CheckMembershipList(DPID idTo)
{
    if ( stdComm_UpdatePlayers(0) )
    {
        SITHLOG_WARNING("Unable to UpdatePlayers List.\n");
        return;
    }

    SITHDSS_STARTOUTEX(&outmsg, SITHDSS_CHECKMEMBERSHIPLIST); // Why using outmsg object and not sithMulti_g_message?

    SITHDSS_PUSHUINT8(stdComm_GetNumPlayers());
    //outmsg.data[0] = stdComm_GetNumPlayers();

    //uint32_t* pCurOut = (uint32_t*)&outmsg.data[1];
    for ( size_t i = 0; i < stdComm_GetNumPlayers(); ++i )
    {
        SITHDSS_PUSHUINT32(stdComm_GetPlayerID(i));
        //*pCurOut++ = stdComm_GetPlayerID(i);
    }

    /*outmsg.type = SITHDSS_CHECKMEMBERSHIPLIST;
    outmsg.length = (char*)pCurOut - (char*)outmsg.data;*/
    SITHDSS_ENDOUT;
    sithMessage_SendMessage(&outmsg, idTo, 1, DPSEND_GUARANTEED);
}

int J3DAPI sithMessage_ProcessCheckMembershipList(const SithMessage* pMsg)
{
    if ( stdComm_IsGameHost() || (sithMain_g_sith_mode.subModeFlags & SITH_SUBMODE_SYNC) == 0 )
    {
        SITHLOG_STATUS("Ignoring MembershipList check from %x.\n", sithMessage_senderId);
        return 1;
    }

    if ( stdComm_UpdatePlayers(0) )
    {
        SITHLOG_WARNING("Unable to UpdatePlayers List.\n");
        return 0;
    }

    SITHDSS_STARTIN(pMsg);

    size_t numPlayers = SITHDSS_POPUINT8();
    //size_t numPlayers = pMsg->data[0];

    //uint32_t* pCurIn = (uint32_t*)&pMsg->data[1];
    for ( size_t i = 0; i < numPlayers; ++i )
    {
        DPID playerId = SITHDSS_POPUINT32();//*pCurIn++;
        if ( stdComm_VerifyPlayer(playerId) )
        {
            stdComm_RejoinSession(&sithMessage_g_localPlayerId, sithPlayer_GetLocalPlayerName());
            SITHLOG_ERROR("--- Session memebership problem detected --- rejoining session.\n");
            return 1;
        }
    }

    SITHDSS_ENDIN;
    SITHLOG_STATUS("We verified %d players in our session successfully.\n", numPlayers);
    return 1;
}

void J3DAPI sithMessage_RegisterFunction(int type, SithMessageProcessFunc pFunc)
{
    SITH_ASSERTREL((type >= 0) && (type < STD_ARRAYLEN(sithMessage_aTypeFuncs)));
    sithMessage_aTypeFuncs[type] = pFunc;
}

int J3DAPI sithMessage_Process(const SithMessage* pMessage)
{
    SITH_ASSERTREL(pMessage != NULL);

    uint32_t type = pMessage->type;
    if ( type >= STD_ARRAYLEN(sithMessage_aTypeFuncs) )
    {
        SITHLOG_ERROR("Message contained illegal type field %d.\n", type);
        return 1;
    }

    if ( !sithMessage_aTypeFuncs[type] )
    {
        SITHLOG_ERROR("Warning: message type %d has no registered function to process.  Discarded\n", type);
        return 1;
    }

    return sithMessage_aTypeFuncs[type](pMessage);
}

void sithMessage_InitMessageHandlers(void)
{
    sithMessage_RegisterFunction(SITHDSS_POS, sithDSSThing_ProcessPos);
    sithMessage_RegisterFunction(SITHDSS_FIRE, sithDSSThing_ProcessFire);
    sithMessage_RegisterFunction(SITHDSS_JOIN, sithMulti_ProcessJoinRequest);
    sithMessage_RegisterFunction(SITHDSS_WELCOME, sithMulti_ProcessWelcome);
    sithMessage_RegisterFunction(SITHDSS_DEATH, sithDSSThing_ProcessDeath);
    sithMessage_RegisterFunction(SITHDSS_DAMAGE, sithDSSThing_ProcessDamage);
    sithMessage_RegisterFunction(SITHDSS_COGMESSAGE, sithDSSCog_ProcessMessage);
    sithMessage_RegisterFunction(SITHDSS_THINGSTATE, sithDSSThing_ProcessStateUpdate);
    sithMessage_RegisterFunction(SITHDSS_PLAYSOUND, sithDSSThing_ProcessPlaySound);
    sithMessage_RegisterFunction(SITHDSS_PLAYKEY, sithDSSThing_ProcessPlayKey);
    sithMessage_RegisterFunction(SITHDSS_THINGFULLDESC, sithDSSThing_ProcessThingFullDescription);
    sithMessage_RegisterFunction(SITHDSS_COGSTATE, sithDSSCog_ProcessCogState);
    sithMessage_RegisterFunction(SITHDSS_SURFACESTATUS, sithDSS_ProcessSurfaceStatus);
    sithMessage_RegisterFunction(SITHDSS_AISTATUS, sithDSS_ProcessAIStatus);
    sithMessage_RegisterFunction(SITHDSS_INVENTORY, sithDSS_ProcessInventory);
    sithMessage_RegisterFunction(SITHDSS_ANIMSTATUS, sithDSS_ProcessAnimStatus);
    sithMessage_RegisterFunction(SITHDSS_SECTORSTATUS, sithDSS_ProcessSectorStatus);
    sithMessage_RegisterFunction(SITHDSS_SYNCSEENSECTORS, sithDSS_ProcessSyncSeenSecors);
    sithMessage_RegisterFunction(SITHDSS_PATHMOVE, sithDSSThing_ProcessPathMove);
    sithMessage_RegisterFunction(SITHDSS_PUPPETSTATUS, sithDSS_ProcessPuppetStatus);
    sithMessage_RegisterFunction(SITHDSS_SYNCPLAYERS, sithMulti_ProcessSyncPlayers);
    sithMessage_RegisterFunction(SITHDSS_ATTACHMENT, sithDSSThing_ProcessAttachment);
    sithMessage_RegisterFunction(SITHDSS_SYNCTASKEVENTS, sithDSS_ProcessSyncTaskEvents);
    sithMessage_RegisterFunction(SITHDSS_SYNCCAMERAS, sithDSS_ProcessSyncCameras);
    sithMessage_RegisterFunction(SITHDSS_TAKE, sithDSSThing_ProcessTake);
    sithMessage_RegisterFunction(SITHDSS_TAKE2, sithDSSThing_ProcessTake);
    sithMessage_RegisterFunction(SITHDSS_STOPKEY, sithDSSThing_ProcessStopKey);
    sithMessage_RegisterFunction(SITHDSS_STOPSOUND, sithDSSThing_ProcessStopSound);
    sithMessage_RegisterFunction(SITHDSS_CREATETHING, sithDSSThing_ProcessCreateThing);
    sithMessage_RegisterFunction(SITHDSS_SYNCGAMESTATE, sithDSS_ProcessSyncGameState);
    sithMessage_RegisterFunction(SITHDSS_CHAT, sithMulti_ProcessChat);
    sithMessage_RegisterFunction(SITHDSS_DESTROYTHING, sithDSSThing_ProcessDestroyThing);
    sithMessage_RegisterFunction(SITHDSS_SECTORFLAGS, sithDSS_ProcessSectorFlags);
    sithMessage_RegisterFunction(SITHDSS_PLAYSOUNDMODE, sithDSSThing_ProcessPlaySoundMode);
    sithMessage_RegisterFunction(SITHDSS_PLAYKEYMODE, sithDSSThing_ProcessPlayKeyMode);
    sithMessage_RegisterFunction(SITHDSS_SETMODEL, sithDSSThing_ProcessSetModel);
    sithMessage_RegisterFunction(SITHDSS_PING, sithMulti_ProcessPing);
    sithMessage_RegisterFunction(SITHDSS_PONG, sithMulti_ProcessPong);
    sithMessage_RegisterFunction(SITHDSS_CHECKMEMBERSHIPLIST, sithMessage_ProcessCheckMembershipList);
    sithMessage_RegisterFunction(SITHDSS_QUIT, sithMulti_ProcessQuit);
    sithMessage_RegisterFunction(SITHDSS_MOVEPOS, sithDSSThing_ProcessMovePos);
    sithMessage_RegisterFunction(SITHDSS_UNKNOWN_44, sithDSS_sub_4B3790);
    sithMessage_RegisterFunction(SITHDSS_VOICESTATE, sithVoice_ProcessVoiceState);
    sithMessage_RegisterFunction(SITHDSS_VEHICLECONTROLSTATE, sithDSS_ProcessVehicleControlsState);
    sithMessage_RegisterFunction(SITHDSS_WHIPSTATUS, sithWhip_Restore);
    sithMessage_RegisterFunction(SITHDSS_SYNCWPNT, sithAIUtil_ProcessSyncWpnts);
}

int J3DAPI sithMessage_FileWrite(const SithMessage* pMessage)
{
    static_assert(sizeof(pMessage->type) == 2, "sizeof(pMessage->type) == 2");
    static_assert(sizeof(pMessage->length) == 4, "sizeof(pMessage->length) == 4");

    if ( stdConffile_Write(&pMessage->type, sizeof(pMessage->type)) )
    {
        return 1;
    }

    if ( stdConffile_Write(&pMessage->length, sizeof(pMessage->length)) )
    {
        return 1;
    }

    return stdConffile_Write(pMessage->data, pMessage->length) != 0;
}
