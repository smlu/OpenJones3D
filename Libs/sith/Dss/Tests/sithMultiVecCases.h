// Cases for the sithMulti.c functions (see sithMultiVec.h). Included by sithMultiVec.h.
//
// Every case starts from the state msv_RunCase builds: host (player 0, net id MSV_NETID(0), also the server), four
// players in the game, game time MSV_GAMETIME, clock time MSV_CLOCKTIME, the default module state of
// msv_DefaultState. The var of a case selects the variant described above its case function.
#ifndef SITHMULTIVECCASES_H
#define SITHMULTIVECCASES_H

// ---------------------------------------------------------------------------------------------------------------
// Game close, player checks, player removal
// ---------------------------------------------------------------------------------------------------------------
// 0 client welcoming (no stop), 1 host not welcoming, 2 host welcoming a player, 3 host welcoming net id 0,
// 4 every stream bit set and a sync time, 5 net stream already off
static int msvc_CloseGame(DssvCtx* cx)
{
    switch ( MSV_VAR )
    {
        case 0: msv_SetClient(); MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = MSV_NEWPLAYER; break;
        case 2: MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = MSV_NEWPLAYER; break;
        case 3: MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = 0; break;
        case 4:
            sithMessage_g_outputstream = (SithMessageStream)0xFFFFFFFFu;
            sithMessage_g_inputstream  = (SithMessageStream)0xFFFFFFFFu;
            MSV_S->msecSyncTime        = 0x4444;
            break;
        case 5:
            sithMessage_g_outputstream = (SithMessageStream)0xFE;
            sithMessage_g_inputstream  = (SithMessageStream)0;
            break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_CloseGame)();
    return 0;
}

// Timeouts are relative to the game time (45000 ms, the check is '>'), the score sync interval to the clock time
// (10000 ms, '<'). 0 nobody timed out, 1 player 2 exactly at the timeout, 2 one ms past it, 3 the local player timed
// out (never kicked), 4 a timed out player that has not joined, 5 24 players with players 1, 7 and 23 timed out,
// 6 score sync one ms past the interval, 7 exactly at it, 8 client with the server exactly at the timeout, 9 one ms
// past it, 10 no world, 11 no local player thing, 12 syncing, 13 no players, 14 last comm time that wraps,
// 15 last score sync time that wraps, 16 client with a server comm time that wraps, 17 game time 0
static int msvc_CheckPlayers(DssvCtx* cx)
{
    MSV_S->msecLastSyncScoreTime = MSV_CLOCKTIME;
    switch ( MSV_VAR )
    {
        case 1: MSV_P(2)->msecLastCommTime = MSV_GAMETIME - 45000; break;
        case 2: MSV_P(2)->msecLastCommTime = MSV_GAMETIME - 45001; break;
        case 3: MSV_P(0)->msecLastCommTime = 0; break;
        case 4: MSV_P(2)->msecLastCommTime = 0; MSV_P(2)->flags &= ~SITH_PLAYER_JOINEDGAME; break;
        case 5:
            msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS);
            MSV_P(1)->msecLastCommTime  = 1;
            MSV_P(7)->msecLastCommTime  = MSV_GAMETIME - 45001;
            MSV_P(23)->msecLastCommTime = 0;
            break;
        case 6: MSV_S->msecLastSyncScoreTime = MSV_CLOCKTIME - 10001; break;
        case 7: MSV_S->msecLastSyncScoreTime = MSV_CLOCKTIME - 10000; break;
        case 8: msv_SetClient(); MSV_P(0)->msecLastCommTime = MSV_GAMETIME - 45000; break;
        case 9: msv_SetClient(); MSV_P(0)->msecLastCommTime = MSV_GAMETIME - 45001; break;
        case 10: sithWorld_g_pCurrentWorld = NULL; break;
        case 11: sithPlayer_g_pLocalPlayerThing = NULL; break;
        case 12: sithMain_g_sith_mode.subModeFlags = SITH_SUBMODE_SYNC; break;
        case 13: msv_BuildPlayers(0, 0); sithPlayer_g_pLocalPlayerThing = &MSV_A->things[0]; break;
        case 14: MSV_P(2)->msecLastCommTime = 0xFFFFFFFFu - 1000; break;
        case 15: MSV_S->msecLastSyncScoreTime = 0xFFFFF000u; break;
        case 16: msv_SetClient(); MSV_P(0)->msecLastCommTime = 0xFFFFFFFFu - 1000; break;
        case 17:
            sithTime_g_msecGameTime = 0;
            for ( unsigned i = 0; i < 4; ++i ) { MSV_P(i)->msecLastCommTime = 0; }
            break;
        default: break;
    }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_CheckPlayers)(12345, NULL);
}

// 0 a player, 1 an unknown id, 2 the player being welcomed (not in the game), 3 a player while another one is
// welcomed, 4 the server, 5 no players, 6 net id 0 while welcoming net id 0, 7 client losing the server
static int msvc_ProcessPlayerLost(DssvCtx* cx)
{
    DPID id = MSV_NETID(2);
    switch ( MSV_VAR )
    {
        case 1: id = 0x9999; break;
        case 2: id = 0x9999; MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = 0x9999; break;
        case 3: MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = 0x9999; break;
        case 4: id = MSV_NETID(0); break;
        case 5: id = MSV_NETID(0); sithPlayer_g_numPlayers = 0; break;
        case 6: id = 0; MSV_P(3)->playerNetId = 0; MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = 0; break;
        case 7: id = MSV_NETID(0); msv_SetClient(); break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_ProcessPlayerLost)(id);
    return 0;
}

// 0 player 2, 1 player 2 on a client, 2 the server, 3 playerNum == numPlayers (assert), 4 15 character name,
// 5 63 character name, 6 empty name, 7 player 23 of 24, 8 the server is player 2
static int msvc_RemovePlayer(DssvCtx* cx)
{
    unsigned num = 2;
    wchar_t awName[64];
    switch ( MSV_VAR )
    {
        case 1: msv_SetClient(); break;
        case 2: num = 0; break;
        case 3: msv_BuildPlayers(5, 4); num = 4; break;
        case 4: msv_SetName(2, msv_awName15, 0); break;
        case 5: msv_MakeName(awName, 63, L'C'); msv_SetName(2, awName, 0); break;
        case 6: msv_SetName(2, L"", 0); break;
        case 7: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); num = 23; break;
        case 8: sithMulti_g_serverId = MSV_NETID(2); break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_RemovePlayer)(num);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Welcome
// ---------------------------------------------------------------------------------------------------------------
// 0 player 2, 1 15 character name, 2 16 character name (truncated), 3 63 character name, 4 empty name, 5 player 23
// of 24, 6 ids -1 (send to the joined players), 7 the send fails, 8 name followed by garbage, 9 a player past
// numPlayers
static int msvc_SendWelcome(DssvCtx* cx)
{
    DPID idPlayer = MSV_NETID(2), idTo = MSV_NETID(2);
    int num = 2;
    wchar_t awName[64];
    switch ( MSV_VAR )
    {
        case 1: msv_SetName(2, msv_awName15, 0); break;
        case 2: msv_SetName(2, msv_awName16, 0); break;
        case 3: msv_MakeName(awName, 63, L'K'); msv_SetName(2, awName, 0); break;
        case 4: msv_SetName(2, L"", 0); break;
        case 5: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); num = 23; break;
        case 6: idPlayer = 0xFFFFFFFFu; idTo = SITHMESSAGE_SENDTOJOINEDPLAYERS; num = 0; break;
        case 7: dssv_sendResult = 0; break;
        case 8: msv_SetName(2, L"Indy", 1); break;
        case 9: msv_BuildPlayers(6, 4); num = 5; break;
        default: break;
    }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_SendWelcome)(idPlayer, num, idTo);
}

static void msv_WelcomeMsg(DssvCtx* cx, int playerNum, DPID playerId, const wchar_t* pName)
{
    MsvMsgOut m = msv_MsgStart(cx, SITHDSS_WELCOME);
    msv_Put32(&m, (uint32_t)playerNum);
    msv_Put32(&m, (uint32_t)playerId);
    msv_PutWStr(&m, pName, 16);
    msv_MsgEnd(&m);
}

// 0 a new player on the host (joins, scores synced), 1 the same on a client, 2 a player that already joined,
// 3 welcome of the local client while syncing (enters the game), 4 the same when not syncing (duplicate), 5 as 3
// with tick rate 0 (assert), 6 as 0 with a new player callback, 7 name without terminator, 8 shorter name over a
// longer one, 9 player 23 of 24, 10 player 10 with 4 players in the game (no range check), 11 15 character name,
// 12 as 3 with 24 players, 13 empty name, 20 the message of SendWelcome_basic
static int msvc_ProcessWelcome(DssvCtx* cx)
{
    int num = 2;
    DPID id = MSV_NETID(2);
    const wchar_t* pName = L"Marion";
    switch ( MSV_VAR )
    {
        case 1: msv_SetClient(); break;
        case 2: break;
        case 3: case 4: case 5: case 12:
            if ( MSV_VAR == 12 ) { msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); }
            msv_SetClient();
            num = 1;
            id  = MSV_NETID(1);
            if ( MSV_VAR != 4 ) { sithMain_g_sith_mode.subModeFlags = SITH_SUBMODE_SYNC | 0x10; }
            if ( MSV_VAR == 5 ) { MSV_S->tickRate = 0; }
            break;
        case 6: MSV_S->pfNewPlayerJoinedCallback = 1; break;
        case 7: pName = msv_awName16; break;
        case 8: msv_SetName(2, L"Belloq the rival", 1); pName = L"Sallah"; break;
        case 9: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); num = 23; id = MSV_NETID(23); break;
        case 10: msv_BuildPlayers(12, 4); num = 10; id = MSV_NETID(10); break;
        case 11: pName = msv_awName15; break;
        case 13: pName = L""; break;
        default: break;
    }
    if ( MSV_VAR != 2 && MSV_VAR != 3 && MSV_VAR != 4 && MSV_VAR != 5 && MSV_VAR != 12 )
    {
        sithPlayer_g_aPlayers[num].flags &= ~SITH_PLAYER_JOINEDGAME;
    }
    if ( MSV_VAR != 20 ) { msv_WelcomeMsg(cx, num, id, pName); }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_ProcessWelcome)(MSV_MSG);
}

// 0 host, 1 client, 2 new player callback, 3 no local player thing, 4 63 character name, 5 player 23 of 24
static int msvc_ProcessPlayerJoin(DssvCtx* cx)
{
    int num = 2;
    wchar_t awName[64];
    switch ( MSV_VAR )
    {
        case 1: msv_SetClient(); break;
        case 2: MSV_S->pfNewPlayerJoinedCallback = 1; break;
        case 3: sithPlayer_g_pLocalPlayerThing = NULL; break;
        case 4: msv_MakeName(awName, 63, L'Q'); msv_SetName(2, awName, 0); break;
        case 5: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); num = 23; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_ProcessPlayerJoin)(num);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Player list sync
// ---------------------------------------------------------------------------------------------------------------
// 0 four players, 1 no players, 2 24 players, 3 mixed flags (names only with flag 4), 4 local id 0 (no model
// loads), 5 a player with net id 0, 6 the model is not found, 7 names of 15, 16, 63 and 0 characters, 8 characters
// above 0x7F, 9 to the joined players with flags 1, 10 to id 0, 11 names followed by garbage, 12 on a client
static int msvc_SyncPlayers(DssvCtx* cx)
{
    DPID idTo = MSV_NEWPLAYER;
    uint32_t flags = 0;
    wchar_t awName[64];
    switch ( MSV_VAR )
    {
        case 1: sithPlayer_g_numPlayers = 0; break;
        case 2: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); break;
        case 3:
            MSV_P(1)->flags = SITH_PLAYER_JOINEDGAME | SITH_PLAYER_PLACED;
            MSV_P(2)->flags = (SithPlayerFlag)0xFFFFFFFFu;
            MSV_P(3)->flags = (SithPlayerFlag)0;
            break;
        case 4: sithMessage_g_localPlayerId = 0; break;
        case 5: MSV_P(2)->playerNetId = 0; break;
        case 6: msv_pLoadedModel = NULL; break;
        case 7:
            msv_SetName(0, msv_awName15, 0);
            msv_SetName(1, msv_awName16, 0);
            msv_MakeName(awName, 63, L'D');
            msv_SetName(2, awName, 0);
            msv_SetName(3, L"", 0);
            break;
        case 8: msv_SetName(1, L"Caf\x00E9", 0); msv_SetName(2, L"\x00FF\x0080x", 0); break;
        case 9: idTo = SITHMESSAGE_SENDTOJOINEDPLAYERS; flags = 1; break;
        case 10: idTo = 0; break;
        case 11: msv_SetName(1, L"Short", 1); msv_SetName(3, L"", 1); break;
        case 12: msv_SetClient(); break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_SyncPlayers)(idTo, flags);
    return 0;
}

typedef struct
{
    uint32_t flags;
    DPID id;
    const char* pName; // NULL: 16 bytes without terminator
} MsvSyncEntry;

static void msv_SyncMsg(DssvCtx* cx, DPID idServer, unsigned num, const MsvSyncEntry* aEntries)
{
    MsvMsgOut m = msv_MsgStart(cx, SITHDSS_SYNCPLAYERS);
    msv_Put32(&m, (uint32_t)idServer);
    msv_Put16(&m, num);
    for ( unsigned i = 0; i < num; ++i )
    {
        msv_Put32(&m, aEntries[i].flags);
        if ( (aEntries[i].flags & SITH_PLAYER_UNKNOWN_04) == 0 ) { continue; }
        msv_Put32(&m, (uint32_t)aEntries[i].id);
        msv_PutStr(&m, aEntries[i].pName ? aEntries[i].pName : "SixteenBytesName", 16);
    }
    msv_MsgEnd(&m);
}

// 0 the message of SyncPlayers_basic, 1 player 2 joined (welcome missed), 2 the same while syncing, 3 player 2 removed
// by the server, 4 the same while syncing, 5 no players, 6 24 players, 7 entries without flag 4, 8 name without
// terminator, 9 the model is not found (stops at the first remote player), 10 local id 0, 11 characters above 0x7F,
// 12 a new server id, 13 on a client, 14 grows the list from 4 to 6 players, 15 every flag set, 16 the message of
// SyncPlayers_24 (24 players built, 4 in the game)
static int msvc_ProcessSyncPlayers(DssvCtx* cx)
{
    MsvSyncEntry aEntries[MSV_NUMPLAYERS];
    unsigned num = 4;
    DPID idServer = MSV_NETID(0);
    char aNames[MSV_NUMPLAYERS][16];
    for ( unsigned i = 0; i < MSV_NUMPLAYERS; ++i )
    {
        snprintf(aNames[i], sizeof(aNames[i]), "Remote%u", i);
        aEntries[i].flags = SITH_PLAYER_JOINEDGAME | SITH_PLAYER_PLACED | SITH_PLAYER_UNKNOWN_04;
        aEntries[i].id    = MSV_NETID(i);
        aEntries[i].pName = aNames[i];
    }

    switch ( MSV_VAR )
    {
        case 1: case 2:
            MSV_P(2)->flags &= ~SITH_PLAYER_JOINEDGAME;
            if ( MSV_VAR == 2 ) { sithMain_g_sith_mode.subModeFlags = SITH_SUBMODE_SYNC; }
            break;
        case 3: case 4:
            aEntries[2].id = 0;
            if ( MSV_VAR == 4 ) { sithMain_g_sith_mode.subModeFlags = SITH_SUBMODE_SYNC; }
            break;
        case 5: num = 0; break;
        case 6: msv_BuildPlayers(MSV_NUMPLAYERS, 4); num = MSV_NUMPLAYERS; break;
        case 7:
            aEntries[1].flags = SITH_PLAYER_JOINEDGAME | SITH_PLAYER_PLACED;
            aEntries[3].flags = 0;
            break;
        case 8: aEntries[2].pName = NULL; break;
        case 9: msv_pLoadedModel = NULL; break;
        case 10: sithMessage_g_localPlayerId = 0; break;
        case 11: aEntries[1].pName = "Caf\xE9"; aEntries[2].pName = "\xFF\x80z"; break;
        case 12: idServer = 0x3333; break;
        case 13: msv_SetClient(); break;
        case 14: msv_BuildPlayers(6, 4); num = 6; break;
        case 15: for ( unsigned i = 0; i < 4; ++i ) { aEntries[i].flags = 0xFFFFFFFFu; } break;
        case 16: msv_BuildPlayers(MSV_NUMPLAYERS, 4); break;
        default: break;
    }
    if ( MSV_VAR != 0 && MSV_VAR != 16 ) { msv_SyncMsg(cx, idServer, num, aEntries); }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_ProcessSyncPlayers)(MSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// Join request
// ---------------------------------------------------------------------------------------------------------------
static void msv_JoinMsg(DssvCtx* cx, const char* pLevel, const wchar_t* pName, uint32_t checksum)
{
    MsvMsgOut m = msv_MsgStart(cx, SITHDSS_JOIN);
    msv_PutStr(&m, pLevel, 64);
    msv_PutWStr(&m, pName, 16);
    msv_Put32(&m, checksum);
    msv_MsgEnd(&m);
}

// The sender is MSV_NEWPLAYER and player 3 is a free placed slot (net id 0). 0 client (ignored), 1 wrong level,
// 2 level name in other case, 3 level name of 64 characters without terminator (the first 63 match), 4 the sender is
// already in the game, 5 quitting, 6 busy welcoming another player, 7 already welcoming the sender, 8 no free slot,
// 9 the player list update fails, 10 the player does not verify, 11 wrong checksum, 12 joins, 13 joins with a name
// without terminator, 14 joins with a short name over a longer one, 15 the sender has net id 0 (matches the free
// slot), 16 no players, 17 24 players with the last slot free, 18 empty level names, 19 checksum INT_MIN, 20 the free
// slot is not placed, 21 two free slots
static int msvc_ProcessJoinRequest(DssvCtx* cx)
{
    const char* pLevel = "level3.ndy";
    const wchar_t* pName = L"Shorty";
    uint32_t checksum = MSV_CHECKSUM;
    char aLong[65];
    unsigned freeSlot = 3;

    dssv_senderId = MSV_NEWPLAYER;
    switch ( MSV_VAR )
    {
        case 0: msv_SetClient(); break;
        case 1: pLevel = "level4.ndy"; break;
        case 2: pLevel = "LEVEL3.NDY"; break;
        case 3:
            for ( unsigned k = 0; k < 64; ++k ) { aLong[k] = (char)('a' + k % 26); }
            aLong[64] = 0;
            pLevel = aLong;
            memcpy(MSV_A->world.aName, aLong, 63);
            MSV_A->world.aName[63] = 0;
            break;
        case 4: dssv_senderId = MSV_NETID(2); break;
        case 5: MSV_S->quitGameState = SITHMULTI_QUITSTATE_QUIT; break;
        case 6: MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = 0x2888; break;
        case 7: MSV_S->bWelcomingPlayer = 1; MSV_S->newPlayerId = MSV_NEWPLAYER; break;
        case 8: freeSlot = 99; break;
        case 9: msv_updatePlayersResult = 1; break;
        case 10: msv_verifyPlayerResult = 0x80004005; break;
        case 11: checksum = MSV_CHECKSUM ^ 1; break;
        case 13: pName = msv_awName16; break;
        case 14: msv_SetName(3, L"OldLongPlayerNm", 1); break;
        case 15: dssv_senderId = 0; break;
        case 16: msv_BuildPlayers(0, 0); freeSlot = 99; break;
        case 17: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); freeSlot = 23; break;
        case 18: pLevel = ""; MSV_A->world.aName[0] = 0; break;
        case 19: checksum = 0x80000000u; MSV_S->checksum = 0x80000000u; break;
        case 20: MSV_P(3)->flags &= ~SITH_PLAYER_PLACED; break;
        case 21: MSV_P(1)->playerNetId = 0; break;
        default: break;
    }
    if ( freeSlot < MSV_NUMPLAYERS ) { MSV_P(freeSlot)->playerNetId = 0; }
    msv_JoinMsg(cx, pLevel, pName, checksum);
    msv_Begin(cx);
    return DSSV_FN(sithMulti_ProcessJoinRequest)(MSV_MSG);
}

static const struct { int code; uint32_t progress; DPID id; } msv_aFinishJoiningArgs[] = {
    { SITHMULTI_JOIN_OK, 0x3E800000u, MSV_NEWPLAYER },             // 0.25
    { SITHMULTI_JOIN_BUSY, 0, MSV_NEWPLAYER },
    { SITHMULTI_JOIN_CANCEL, 0, MSV_NEWPLAYER },
    { SITHMULTI_JOIN_WRONGCHECKSUM, 0, MSV_NEWPLAYER },
    { SITHMULTI_JOIN_GAMEFULL, 0, MSV_NEWPLAYER },
    { SITHMULTI_JOIN_WRONGLEVEL, 0, MSV_NEWPLAYER },
    { SITHMULTI_JOIN_OK, 0x3F000000u, 0 },                        // 0.5
    { SITHMULTI_JOIN_OK, DSSV_NZERO, 0xFFFFFFFFu },
    { 0x7FFFFFFF, DSSV_FLTMAX, 0x7FFFFFFF },
    { -1, DSSV_NDENORMMAX, 0x80000000u },
    { 2, 0x3F800000u, 1 },                                          // unnamed code 2, 1.0
    { SITHMULTI_JOIN_OK, DSSV_DENORMMIN, MSV_NEWPLAYER },
};

// var: index into msv_aFinishJoiningArgs; 100: the send fails
static int msvc_FinishJoining(DssvCtx* cx)
{
    unsigned i = MSV_VAR == 100 ? 0 : (unsigned)MSV_VAR;
    if ( MSV_VAR == 100 ) { dssv_sendResult = 0; }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_FinishJoining)((SithMultiJoinStatus)msv_aFinishJoiningArgs[i].code, dssv_BitsF(msv_aFinishJoiningArgs[i].progress), (int)msv_aFinishJoiningArgs[i].id);
}

// ---------------------------------------------------------------------------------------------------------------
// Chat, ping
// ---------------------------------------------------------------------------------------------------------------
// Message: sender count, player number, length, text (numText bytes of pText, the rest of the length zero)
static void msv_ChatMsg(DssvCtx* cx, int playerNum, uint32_t len, const char* pText, size_t numText)
{
    MsvMsgOut m = msv_MsgStart(cx, SITHDSS_CHAT);
    msv_Put32(&m, 7);
    msv_Put32(&m, (uint32_t)playerNum);
    msv_Put32(&m, len);
    size_t numData = len < 200 ? len : 200;
    for ( size_t k = 0; k < numData; ++k ) { *m.p++ = k < numText ? (uint8_t)pText[k] : 0; }
    msv_MsgEnd(&m);
}

// 0 player 2 says a text, 1 player -1 (plain text), 2 player INT_MIN, 3 length 1 (empty text), 4 length 128 with 127
// characters, 5 length 200 (clamped to 128) with 150 characters, 6 length UINT_MAX (clamped) with 200 characters,
// 7 10 characters without terminator in length 10, 8 text with format characters, 9 the server with a 63 character
// name, 10 player 23 of 24, 11 player 10 with 4 players in the game (no range check), 12 length 2, 13 text with a
// terminator before the end of the length, 14 empty player name
static int msvc_ProcessChat(DssvCtx* cx)
{
    char aText[201];
    for ( unsigned k = 0; k < 200; ++k ) { aText[k] = (char)('a' + k % 26); }
    aText[200] = 0;
    wchar_t awName[64];

    switch ( MSV_VAR )
    {
        case 0: msv_ChatMsg(cx, 2, 12, "hello there", 12); break;
        case 1: msv_ChatMsg(cx, -1, 12, "hello there", 12); break;
        case 2: msv_ChatMsg(cx, (int)0x80000000u, 6, "plain", 6); break;
        case 3: msv_ChatMsg(cx, 2, 1, "", 1); break;
        case 4: msv_ChatMsg(cx, 2, 128, aText, 127); break;
        case 5: msv_ChatMsg(cx, 2, 200, aText, 150); break;
        case 6: msv_ChatMsg(cx, 2, 0xFFFFFFFFu, aText, 200); break;
        case 7: msv_ChatMsg(cx, 2, 10, "xxxxxxxxxx", 10); break;
        case 8: msv_ChatMsg(cx, 2, 14, "100% %s %d %n", 14); break;
        case 9: msv_MakeName(awName, 63, L'M'); msv_SetName(0, awName, 0); msv_ChatMsg(cx, 0, 4, "hi!", 4); break;
        case 10: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); msv_ChatMsg(cx, 23, 4, "bye", 4); break;
        case 11: msv_BuildPlayers(12, 4); msv_ChatMsg(cx, 10, 4, "who", 4); break;
        case 12: msv_ChatMsg(cx, 1, 2, "x", 2); break;
        case 13: msv_ChatMsg(cx, 1, 20, "short\0hidden text", 18); break;
        case 14: msv_SetName(2, L"", 0); msv_ChatMsg(cx, 2, 5, "anon", 5); break;
        default: break;
    }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_ProcessChat)(MSV_MSG);
}

// ProcessPing answers with the global message (not the ping it received) and changes the type of the received one.
// 0 a ping of player 3, 1 the send fails and the sender is 0
static int msvc_ProcessPing(DssvCtx* cx)
{
    MsvMsgOut m = msv_MsgStart(cx, SITHDSS_PING);
    msv_Put32(&m, 0x00C0FFEE);
    msv_MsgEnd(&m);
    if ( MSV_VAR == 1 ) { dssv_sendResult = 0; dssv_senderId = 0; }
    msv_Begin(cx);

    // The global message holds an earlier message (a deterministic one; its tail keeps the probe fill)
    sithMulti_g_message.type   = SITHDSS_QUIT;
    sithMulti_g_message.length = 4;
    memcpy(sithMulti_g_message.data, "\x01\x02\x03\x04", 4);

    int ret = DSSV_FN(sithMulti_ProcessPing)(MSV_MSG);
    MSV_EV(MSV_EV_INMSGTYPE, 2, (uint32_t)cx->inMsg.type, cx->inMsg.length);
    return ret;
}

// The ping started at MSV_GAMETIME - 250. 0 answer of player 2, 1 other start time, 2 unknown sender, 3 a start time
// after the game time (negative ping), 4 start time 0 of player 0, 5 no players, 6 63 character name, 7 two players
// with the sender's id (first one)
static int msvc_ProcessPong(DssvCtx* cx)
{
    uint32_t start = MSV_GAMETIME - 250;
    wchar_t awName[64];
    MSV_S->msecPingStartTime = MSV_GAMETIME - 250;
    dssv_senderId = MSV_NETID(2);
    switch ( MSV_VAR )
    {
        case 1: start = MSV_GAMETIME - 251; break;
        case 2: dssv_senderId = 0x9999; break;
        case 3: start = MSV_GAMETIME + 100; MSV_S->msecPingStartTime = start; break;
        case 4: start = 0; MSV_S->msecPingStartTime = 0; dssv_senderId = MSV_NETID(0); break;
        case 5: sithPlayer_g_numPlayers = 0; break;
        case 6: msv_MakeName(awName, 63, L'P'); msv_SetName(2, awName, 0); break;
        case 7: MSV_P(1)->playerNetId = MSV_NETID(2); break;
        default: break;
    }
    MsvMsgOut m = msv_MsgStart(cx, SITHDSS_PONG);
    msv_Put32(&m, start);
    msv_MsgEnd(&m);
    msv_Begin(cx);
    return DSSV_FN(sithMulti_ProcessPong)(MSV_MSG);
}

// ---------------------------------------------------------------------------------------------------------------
// Quit, eject
// ---------------------------------------------------------------------------------------------------------------
// 0 host quits player 2, 1 client (no message), 2 id 0, 3 id UINT_MAX
static int msvc_QuitPlayer(DssvCtx* cx)
{
    DPID id = MSV_NETID(2);
    switch ( MSV_VAR )
    {
        case 1: msv_SetClient(); break;
        case 2: id = 0; break;
        case 3: id = 0xFFFFFFFFu; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_QuitPlayer)(id);
    return 0;
}

// Client (player 1); the sender is the server unless noted. 0 sender is not the server, 1 the local player is
// ejected, 2 already ejected, 3 quitting (state 1) when ejected, 4 player 2 quits, 5 unknown player, 6 ejected at a
// game time that wraps the quit time, 7 server id 0 and sender 0, 8 the message of QuitPlayer_basic, 9 player 2 quits
// while ejected
static int msvc_ProcessQuit(DssvCtx* cx)
{
    DPID id = MSV_NETID(1);
    msv_SetClient();
    dssv_senderId = MSV_NETID(0);
    switch ( MSV_VAR )
    {
        case 0: dssv_senderId = MSV_NETID(3); break;
        case 2: MSV_S->quitGameState = SITHMULTI_QUITSTATE_EJECTED; MSV_S->msecQuitGameTime = 77; break;
        case 3: MSV_S->quitGameState = SITHMULTI_QUITSTATE_QUIT; MSV_S->msecQuitGameTime = MSV_GAMETIME + 100000; break;
        case 4: case 8: id = MSV_NETID(2); break;
        case 5: id = 0x9999; break;
        case 6: sithTime_g_msecGameTime = 0xFFFFF000u; break;
        case 7: sithMulti_g_serverId = 0; dssv_senderId = 0; id = MSV_NETID(2); break;
        case 9: id = MSV_NETID(2); MSV_S->quitGameState = SITHMULTI_QUITSTATE_EJECTED; break;
        default: break;
    }
    if ( MSV_VAR != 8 )
    {
        MsvMsgOut m = msv_MsgStart(cx, SITHDSS_QUIT);
        msv_Put32(&m, (uint32_t)id);
        msv_MsgEnd(&m);
    }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_ProcessQuit)(MSV_MSG);
}

static const struct { uint32_t curState, curTime, time, state; } msv_aQuitGameArgs[] = {
    { 0, 0, 5000, 1 },
    { 1, 5000, 5000, 1 },      // same state, same time: unchanged
    { 1, 5000, 6000, 1 },      // same state, later: unchanged
    { 1, 5000, 4999, 1 },      // same state, earlier: set
    { 1, 5000, 9000, 2 },      // other state: set
    { 2, 0, 0, 2 },
    { 0, 100, 0, 0 },          // state 0, earlier
    { 2, 7, 0xFFFFFFFFu, 2 },
    { 0, 0, 123, 0xFFFFFFFFu },
    { 0, 0, 1, 0x7FFFFFFF },
    { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu },
};

static int msvc_QuitGame(DssvCtx* cx)
{
    MSV_S->quitGameState    = msv_aQuitGameArgs[MSV_VAR].curState;
    MSV_S->msecQuitGameTime = msv_aQuitGameArgs[MSV_VAR].curTime;
    msv_Begin(cx);
    DSSV_FN(sithMulti_QuitGame)(msv_aQuitGameArgs[MSV_VAR].time, (int)msv_aQuitGameArgs[MSV_VAR].state);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Update and the welcome steps
// ---------------------------------------------------------------------------------------------------------------
// The welcome of player MSV_NEWPLAYER into slot 3 is in progress where noted.
static void msv_Welcoming(unsigned state)
{
    MSV_S->bWelcomingPlayer    = 1;
    MSV_S->newPlayerId         = MSV_NEWPLAYER;
    MSV_S->playerWelcomeState  = state;
    MSV_S->curWelcomePlayerNum = 3;
    MSV_P(3)->playerNetId      = 0;
}

// 0 not active, 1 client, 2 ejected and expired, 3 quit and expired, 4 ejected at exactly the quit time while
// welcoming (stops the welcome), 5 unknown quit state 3 expired, 6 scores to sync, 7 host not welcoming, 8 welcoming
// while quitting, 9 welcoming surfaces for 59 ms (no step), 10 60 ms (one step), 11 59 ms left over plus 1 ms,
// 12 600 ms, 13 6000 ms (the whole welcome and past it), 14 welcome state 0, 15 a delta that wraps with the
// left over time, 16 welcoming things, 17 welcoming removals without removals for 180 ms, 18 welcome state 5,
// 19 scores to sync while welcoming for 120 ms
static int msvc_Update(DssvCtx* cx)
{
    unsigned delta = 60;
    switch ( MSV_VAR )
    {
        case 0: dssv_bGameActive = 0; MSV_S->bSyncScores = 1; break;
        case 1: msv_SetClient(); MSV_S->bSyncScores = 1; break;
        case 2: MSV_S->quitGameState = SITHMULTI_QUITSTATE_EJECTED; MSV_S->msecQuitGameTime = MSV_GAMETIME - 1; break;
        case 3: MSV_S->quitGameState = SITHMULTI_QUITSTATE_QUIT; MSV_S->msecQuitGameTime = 0; break;
        case 4:
            MSV_S->quitGameState    = SITHMULTI_QUITSTATE_EJECTED;
            MSV_S->msecQuitGameTime = MSV_GAMETIME;
            msv_Welcoming(SITHMULTI_WELCOME_SURFACES);
            break;
        case 5: MSV_S->quitGameState = 3; MSV_S->msecQuitGameTime = 5; MSV_S->bSyncScores = 1; break;
        case 6: MSV_S->bSyncScores = 1; break;
        case 8:
            MSV_S->quitGameState    = SITHMULTI_QUITSTATE_QUIT;
            MSV_S->msecQuitGameTime = MSV_GAMETIME + 1;
            msv_Welcoming(SITHMULTI_WELCOME_SECTORS);
            break;
        case 9: msv_Welcoming(SITHMULTI_WELCOME_SURFACES); delta = 59; break;
        case 10: msv_Welcoming(SITHMULTI_WELCOME_SURFACES); break;
        case 11: msv_Welcoming(SITHMULTI_WELCOME_SURFACES); MSV_S->msecWelcomeUpdateInterval = 59; delta = 1; break;
        case 12: msv_Welcoming(SITHMULTI_WELCOME_SURFACES); delta = 600; break;
        case 13: msv_Welcoming(SITHMULTI_WELCOME_SURFACES); delta = 6000; break;
        case 14: msv_Welcoming(SITHMULTI_WELCOME_NONE); delta = 600; break;
        case 15: msv_Welcoming(SITHMULTI_WELCOME_SURFACES); MSV_S->msecWelcomeUpdateInterval = 59; delta = 0xFFFFFFFFu; break;
        case 16: msv_Welcoming(SITHMULTI_WELCOME_THINGS); MSV_S->lastUpdateIdx = 2; delta = 300; break;
        case 17: msv_Welcoming(SITHMULTI_WELCOME_REMOVALS); MSV_S->numRemovedStaticThings = 0; delta = 180; break;
        case 18: msv_Welcoming(5); delta = 600; break;
        case 19: msv_Welcoming(SITHMULTI_WELCOME_SECTORS); MSV_S->bSyncScores = 1; delta = 120; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_Update)(delta);
    return 0;
}

// 0 not set, 1 already set
static int msvc_SyncScores(DssvCtx* cx)
{
    MSV_S->bSyncScores = (uint32_t)MSV_VAR;
    msv_Begin(cx);
    DSSV_FN(sithMulti_SyncScores)();
    return 0;
}

// 0 player 0, 1 player 3, 2 unknown, 3 no players, 4 player 23 of 24, 5 two players with the id (first one),
// 6 id 0 of player 2, 7 the id belongs to a player past numPlayers
static int msvc_GetPlayerIndexByID(DssvCtx* cx)
{
    DPID id = MSV_NETID(0);
    switch ( MSV_VAR )
    {
        case 1: id = MSV_NETID(3); break;
        case 2: id = 0x9999; break;
        case 3: sithPlayer_g_numPlayers = 0; break;
        case 4: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); id = MSV_NETID(23); break;
        case 5: id = MSV_NETID(3); MSV_P(1)->playerNetId = MSV_NETID(3); break;
        case 6: id = 0; MSV_P(2)->playerNetId = 0; break;
        case 7: msv_BuildPlayers(6, 4); id = MSV_NETID(5); break;
        default: break;
    }
    msv_Begin(cx);
    return DSSV_FN(sithMulti_GetPlayerIndexByID)(id);
}

// Thing 0 is player 0. var 0 no killer, 1 an actor killer, 2 killed by player 1 (thing 1 made a player),
// 3..10 suicide with rand 0, 8191, 8192, 16383, 16384, 24575, 24576 and 32767 (the four texts at their boundaries),
// 11 suicide of a killer without player (assert), 12 killed by with two 56 character names (127 characters of text),
// 13 empty names, 14 63 character name, 15 no player thing
static int msvc_ProcessKilledPlayer(DssvCtx* cx)
{
    static const int aRand[8] = { 0, 8191, 8192, 16383, 16384, 24575, 24576, 32767 };
    SithThing* pPlayerThing = &MSV_A->things[0];
    SithThing* pKiller = NULL;
    wchar_t awName[64];
    switch ( MSV_VAR )
    {
        case 1: pKiller = &MSV_A->things[1]; break;
        case 2: case 12: case 13: case 15:
            MSV_A->things[1].type = SITH_THING_PLAYER;
            pKiller = &MSV_A->things[1];
            if ( MSV_VAR == 12 )
            {
                msv_MakeName(awName, 56, L'E');
                msv_SetName(0, awName, 0);
                msv_MakeName(awName, 56, L'S');
                msv_SetName(1, awName, 0);
            }
            if ( MSV_VAR == 13 ) { msv_SetName(0, L"", 0); msv_SetName(1, L"", 0); }
            if ( MSV_VAR == 15 ) { pPlayerThing = NULL; }
            break;
        case 11:
            MSV_A->things[0].thingInfo.actorInfo.pPlayer = NULL;
            pKiller = &MSV_A->things[0];
            msv_aRand[0] = 5;
            msv_numRand  = 1;
            break;
        case 14: msv_MakeName(awName, 63, L'Z'); msv_SetName(0, awName, 0); break;
        default:
            if ( MSV_VAR >= 3 && MSV_VAR <= 10 )
            {
                pKiller      = &MSV_A->things[0];
                msv_aRand[0] = aRand[MSV_VAR - 3];
                msv_numRand  = 1;
            }
            break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_ProcessKilledPlayer)(MSV_P(0), pPlayerThing, pKiller);
    return 0;
}

// The respawn points are the player slots whose bit is clear in the respawn mask of player 0 (thing 0). Collision
// scripts: 0 a live player, 1 an actor, 2 a dying or destroyed player, 3 a surface.
// 0 one player, 1 all masked (respawn 0), 2 slots 1 and 3 with rand 0, 3 the same with rand 32767 (clamped), 4 four
// slots, an actor and a surface in the way, 5 the first pick occupied by a player, 6 every pick occupied, 7 a dying
// player in the way, 8 a destroyed player in the way, 9..13 7, 7, 14, 21 and 7 slots with rand 4681, 9362, 4681,
// 14043 and 28086 (rand / 32767 * slots is an integer), 14 24 slots with rand 32767, 15 24 slots with rand 12345,
// 16 the thing is no player (assert), 17 24 players with only slot 0 unmasked, 18 occupied after an actor, then free
static int msvc_Respawn(DssvCtx* cx)
{
    static const struct { unsigned n; int r; } aDiv[5] = { { 7, 4681 }, { 7, 9362 }, { 14, 4681 }, { 21, 14043 }, { 7, 28086 } };
    SithThing* pThing = &MSV_A->things[0];
    switch ( MSV_VAR )
    {
        case 0: msv_BuildPlayers(1, 1); break;
        case 1: MSV_P(0)->respawnMask = 0xF; break;
        case 2: case 3:
            MSV_P(0)->respawnMask = 0x5;
            msv_aRand[0] = MSV_VAR == 2 ? 0 : 32767;
            msv_numRand  = 1;
            break;
        case 4:
            msv_aRand[0] = 16384;
            msv_numRand  = 1;
            msv_aCollScript[0][0] = 1;
            msv_aCollScript[0][1] = 3;
            break;
        case 5:
            msv_aRand[0] = 16384;
            msv_numRand  = 1;
            msv_aCollScript[0][0] = 0;
            break;
        case 6:
            msv_aRand[0] = 8000;
            msv_numRand  = 1;
            for ( unsigned k = 0; k < MSV_MAXSEARCH; ++k ) { msv_aCollScript[k][0] = 0; }
            break;
        case 7: case 8:
            MSV_A->things[5].type   = SITH_THING_PLAYER;
            MSV_A->things[5].flags |= MSV_VAR == 7 ? SITH_TF_DYING : SITH_TF_DESTROYED;
            msv_aRand[0] = 30000;
            msv_numRand  = 1;
            msv_aCollScript[0][0] = 2;
            msv_aCollScript[0][1] = 0;
            msv_aCollScript[1][0] = 2;
            break;
        case 9: case 10: case 11: case 12: case 13:
            msv_BuildPlayers(aDiv[MSV_VAR - 9].n, aDiv[MSV_VAR - 9].n);
            msv_aRand[0] = aDiv[MSV_VAR - 9].r;
            msv_numRand  = 1;
            break;
        case 14: case 15:
            msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS);
            msv_aRand[0] = MSV_VAR == 14 ? 32767 : 12345;
            msv_numRand  = 1;
            break;
        case 16: msv_BuildPlayers(2, 1); pThing = &MSV_A->things[1]; break;
        case 17: msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS); MSV_P(0)->respawnMask = 0x00FFFFFE; break;
        case 18:
            msv_aRand[0] = 0;
            msv_numRand  = 1;
            msv_aCollScript[0][0] = 1;
            msv_aCollScript[0][1] = 0;
            msv_aCollScript[1][0] = 3;
            break;
        default: break;
    }
    msv_Begin(cx);
    return (int)DSSV_FN(sithMulti_Respawn)(pThing);
}

static const struct { uint32_t num; int guid; } msv_aRemoveStaticArgs[] = {
    { 0, 5 }, { 3, -1 }, { 255, (int)0x80000000u }, { 256, 9 }, { 300, 10 }, { 100, 0x7FFFFFFF },
};

static int msvc_RemoveStaticThing(DssvCtx* cx)
{
    MSV_S->numRemovedStaticThings = msv_aRemoveStaticArgs[MSV_VAR].num;
    msv_Begin(cx);
    DSSV_FN(sithMulti_RemoveStaticThing)(msv_aRemoveStaticArgs[MSV_VAR].guid);
    return 0;
}

// 0 from surface 0, 1 from surface 2, 2 from the last surface, 3 at the end (switches to sectors), 4 past the end,
// 5 no surface to sync, 6 every surface to sync, 7 no surfaces, 8 only the last surface to sync
static int msvc_UpdateSurfaces(DssvCtx* cx)
{
    msv_Welcoming(SITHMULTI_WELCOME_SURFACES);
    for ( unsigned i = 0; i < DSSV_NUMSURFACES; ++i )
    {
        if ( MSV_VAR == 5 || MSV_VAR == 8 ) { MSV_A->surfaces[i].flags &= ~SITH_SURFACE_SYNC; }
        if ( MSV_VAR == 6 ) { MSV_A->surfaces[i].flags |= SITH_SURFACE_SYNC; }
    }
    switch ( MSV_VAR )
    {
        case 1: MSV_S->lastUpdateIdx = 2; break;
        case 2: MSV_S->lastUpdateIdx = 5; break;
        case 3: MSV_S->lastUpdateIdx = 6; break;
        case 4: MSV_S->lastUpdateIdx = 9; break;
        case 7: MSV_A->world.numSurfaces = 0; break;
        case 8: MSV_A->surfaces[5].flags |= SITH_SURFACE_SYNC; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_UpdateSurfaces)();
    return 0;
}

// 0 from sector 0, 1 nothing to sync (switches to things), 2 a sector to sync with the adjoins off (status first),
// 3 adjoins off only, 4 at the end, 5 no sectors, 6 only the last sector to sync
static int msvc_UpdateSectors(DssvCtx* cx)
{
    msv_Welcoming(SITHMULTI_WELCOME_SECTORS);
    if ( MSV_VAR != 0 && MSV_VAR != 4 )
    {
        for ( unsigned i = 0; i < DSSV_NUMSECTORS; ++i ) { MSV_A->sectors[i].flags &= ~(SITH_SECTOR_SYNC | SITH_SECTOR_ADJOINSOFF); }
    }
    switch ( MSV_VAR )
    {
        case 2: MSV_A->sectors[1].flags |= SITH_SECTOR_SYNC | SITH_SECTOR_ADJOINSOFF; break;
        case 3: MSV_A->sectors[2].flags |= SITH_SECTOR_ADJOINSOFF; break;
        case 4: MSV_S->lastUpdateIdx = DSSV_NUMSECTORS; break;
        case 5: MSV_A->world.numSectors = 0; break;
        case 6: MSV_A->sectors[3].flags |= SITH_SECTOR_SYNC; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_UpdateSectors)();
    return 0;
}

// 0 from thing 0, 1 nothing to sync (switches to removals), 2 only the weapon to sync (skipped), 3 only an explosion
// to sync (skipped), 4 a thing with a guid above 0xFFFF (full description), 5 empty thing list (last index -1),
// 6 at the last thing, 7 past the last thing, 8 a negative guid
static int msvc_UpdateThings(DssvCtx* cx)
{
    msv_Welcoming(SITHMULTI_WELCOME_THINGS);
    switch ( MSV_VAR )
    {
        case 1: msv_canSyncMask = 0; break;
        case 2: msv_canSyncMask = 1u << 3; break;
        case 3: MSV_A->things[2].type = SITH_THING_EXPLOSION; msv_canSyncMask = 1u << 2; break;
        case 4: MSV_A->things[1].guid = 0x10001; msv_canSyncMask = 1u << 1; break;
        case 5: MSV_A->world.lastThingIdx = -1; break;
        case 6: MSV_S->lastUpdateIdx = 7; break;
        case 7: MSV_S->lastUpdateIdx = 8; break;
        case 8: MSV_A->things[4].guid = (int)0xFFFF0000u; msv_canSyncMask = 1u << 4; MSV_S->lastUpdateIdx = 1; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_UpdateThings)();
    return 0;
}

// Three removals by default. 0 the first one, 1 the last one (then finishes the welcome), 2 no removals (finishes),
// 3 past the end (finishes), 4 finishes while not welcoming (assert), 5 finishes into slot 23 of 24, 6 finishes the
// welcome of net id 0, 7 removal 255 of 256
static int msvc_UpdateRemovals(DssvCtx* cx)
{
    msv_Welcoming(SITHMULTI_WELCOME_REMOVALS);
    MSV_S->numUpdatedSurfaces = 6;
    MSV_S->numUpdatedSectors  = 4;
    MSV_S->numUpdatedThings   = 7;
    switch ( MSV_VAR )
    {
        case 1: MSV_S->lastUpdateIdx = 2; break;
        case 2: MSV_S->numRemovedStaticThings = 0; break;
        case 3: MSV_S->lastUpdateIdx = 5; break;
        case 4: MSV_S->numRemovedStaticThings = 0; MSV_S->bWelcomingPlayer = 0; break;
        case 5:
            msv_BuildPlayers(MSV_NUMPLAYERS, MSV_NUMPLAYERS);
            MSV_S->numRemovedStaticThings = 0;
            MSV_S->curWelcomePlayerNum    = 23;
            MSV_P(23)->playerNetId        = 0;
            break;
        case 6: MSV_S->numRemovedStaticThings = 0; MSV_S->newPlayerId = 0; break;
        case 7: MSV_S->numRemovedStaticThings = 256; MSV_S->lastUpdateIdx = 255; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_UpdateRemovals)();
    return 0;
}

// 0 a player, 1 already welcoming (assert), 2 net id 0 (assert), 3 net id UINT_MAX, 4 resets the step counters
static int msvc_StartWelcomingPlayer(DssvCtx* cx)
{
    DPID id = MSV_NEWPLAYER;
    switch ( MSV_VAR )
    {
        case 1: msv_Welcoming(SITHMULTI_WELCOME_THINGS); MSV_S->lastUpdateIdx = 4; id = 0x2888; break;
        case 2: id = 0; break;
        case 3: id = 0xFFFFFFFFu; break;
        case 4:
            MSV_S->numUpdatedSurfaces = 6;
            MSV_S->numUpdatedSectors  = 4;
            MSV_S->numUpdatedThings   = 7;
            MSV_S->lastUpdateIdx      = 3;
            MSV_S->playerWelcomeState = SITHMULTI_WELCOME_REMOVALS;
            break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_StartWelcomingPlayer)(id);
    return 0;
}

// 0 with error, 1 without error, 2 with error for net id 0 (no message), 3 not welcoming (assert), 4 error 2,
// 5 error -1
static int msvc_StopWelcomingPlayer(DssvCtx* cx)
{
    int bError = 1;
    msv_Welcoming(SITHMULTI_WELCOME_THINGS);
    MSV_S->lastUpdateIdx = 5;
    switch ( MSV_VAR )
    {
        case 1: bError = 0; break;
        case 2: MSV_S->newPlayerId = 0; break;
        case 3: MSV_S->bWelcomingPlayer = 0; break;
        case 4: bError = 2; break;
        case 5: bError = -1; break;
        default: break;
    }
    msv_Begin(cx);
    DSSV_FN(sithMulti_StopWelcomingPlayer)(bError);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Case table
// ---------------------------------------------------------------------------------------------------------------
#define MSVC(name, fn, var) { name, fn, var, NULL, { { 0 } } }
#define MSVR(name, fn, var, src) { name, fn, var, src, { { 0 } } }

static const DssvCase msv_aCases[] = {
    MSVC("CloseGame_clientWelcoming", msvc_CloseGame, 0),
    MSVC("CloseGame_host", msvc_CloseGame, 1),
    MSVC("CloseGame_hostWelcoming", msvc_CloseGame, 2),
    MSVC("CloseGame_hostWelcomingId0", msvc_CloseGame, 3),
    MSVC("CloseGame_allStreams", msvc_CloseGame, 4),
    MSVC("CloseGame_netStreamOff", msvc_CloseGame, 5),

    MSVC("CheckPlayers_hostAllRecent", msvc_CheckPlayers, 0),
    MSVC("CheckPlayers_hostAtTimeout", msvc_CheckPlayers, 1),
    MSVC("CheckPlayers_hostPastTimeout", msvc_CheckPlayers, 2),
    MSVC("CheckPlayers_hostLocalTimedOut", msvc_CheckPlayers, 3),
    MSVC("CheckPlayers_hostNotJoinedTimedOut", msvc_CheckPlayers, 4),
    MSVC("CheckPlayers_host24ThreeTimedOut", msvc_CheckPlayers, 5),
    MSVC("CheckPlayers_hostSyncScoresPast", msvc_CheckPlayers, 6),
    MSVC("CheckPlayers_hostSyncScoresAt", msvc_CheckPlayers, 7),
    MSVC("CheckPlayers_clientServerAtTimeout", msvc_CheckPlayers, 8),
    MSVC("CheckPlayers_clientServerPastTimeout", msvc_CheckPlayers, 9),
    MSVC("CheckPlayers_noWorld", msvc_CheckPlayers, 10),
    MSVC("CheckPlayers_noLocalThing", msvc_CheckPlayers, 11),
    MSVC("CheckPlayers_syncing", msvc_CheckPlayers, 12),
    MSVC("CheckPlayers_hostNoPlayers", msvc_CheckPlayers, 13),
    MSVC("CheckPlayers_hostCommTimeWraps", msvc_CheckPlayers, 14),
    MSVC("CheckPlayers_hostSyncTimeWraps", msvc_CheckPlayers, 15),
    MSVC("CheckPlayers_clientCommTimeWraps", msvc_CheckPlayers, 16),
    MSVC("CheckPlayers_gameTime0", msvc_CheckPlayers, 17),

    MSVC("ProcessPlayerLost_player", msvc_ProcessPlayerLost, 0),
    MSVC("ProcessPlayerLost_unknown", msvc_ProcessPlayerLost, 1),
    MSVC("ProcessPlayerLost_welcomed", msvc_ProcessPlayerLost, 2),
    MSVC("ProcessPlayerLost_otherWelcomed", msvc_ProcessPlayerLost, 3),
    MSVC("ProcessPlayerLost_server", msvc_ProcessPlayerLost, 4),
    MSVC("ProcessPlayerLost_noPlayers", msvc_ProcessPlayerLost, 5),
    MSVC("ProcessPlayerLost_id0", msvc_ProcessPlayerLost, 6),
    MSVC("ProcessPlayerLost_clientServer", msvc_ProcessPlayerLost, 7),

    MSVC("RemovePlayer_host", msvc_RemovePlayer, 0),
    MSVC("RemovePlayer_client", msvc_RemovePlayer, 1),
    MSVC("RemovePlayer_server", msvc_RemovePlayer, 2),
    MSVC("RemovePlayer_atNumPlayers", msvc_RemovePlayer, 3),
    MSVC("RemovePlayer_name15", msvc_RemovePlayer, 4),
    MSVC("RemovePlayer_name63", msvc_RemovePlayer, 5),
    MSVC("RemovePlayer_emptyName", msvc_RemovePlayer, 6),
    MSVC("RemovePlayer_last24", msvc_RemovePlayer, 7),
    MSVC("RemovePlayer_serverIsPlayer2", msvc_RemovePlayer, 8),

    MSVC("SendWelcome_basic", msvc_SendWelcome, 0),
    MSVC("SendWelcome_name15", msvc_SendWelcome, 1),
    MSVC("SendWelcome_name16", msvc_SendWelcome, 2),
    MSVC("SendWelcome_name63", msvc_SendWelcome, 3),
    MSVC("SendWelcome_emptyName", msvc_SendWelcome, 4),
    MSVC("SendWelcome_last24", msvc_SendWelcome, 5),
    MSVC("SendWelcome_idsAllBits", msvc_SendWelcome, 6),
    MSVC("SendWelcome_sendFails", msvc_SendWelcome, 7),
    MSVC("SendWelcome_staleName", msvc_SendWelcome, 8),
    MSVC("SendWelcome_pastNumPlayers", msvc_SendWelcome, 9),

    MSVC("ProcessWelcome_hostNewPlayer", msvc_ProcessWelcome, 0),
    MSVC("ProcessWelcome_clientNewPlayer", msvc_ProcessWelcome, 1),
    MSVC("ProcessWelcome_alreadyJoined", msvc_ProcessWelcome, 2),
    MSVC("ProcessWelcome_localSyncing", msvc_ProcessWelcome, 3),
    MSVC("ProcessWelcome_localDuplicate", msvc_ProcessWelcome, 4),
    MSVC("ProcessWelcome_localTickRate0", msvc_ProcessWelcome, 5),
    MSVC("ProcessWelcome_callback", msvc_ProcessWelcome, 6),
    MSVC("ProcessWelcome_unterminatedName", msvc_ProcessWelcome, 7),
    MSVC("ProcessWelcome_staleName", msvc_ProcessWelcome, 8),
    MSVC("ProcessWelcome_last24", msvc_ProcessWelcome, 9),
    MSVC("ProcessWelcome_pastNumPlayers", msvc_ProcessWelcome, 10),
    MSVC("ProcessWelcome_name15", msvc_ProcessWelcome, 11),
    MSVC("ProcessWelcome_localSyncing24", msvc_ProcessWelcome, 12),
    MSVC("ProcessWelcome_emptyName", msvc_ProcessWelcome, 13),
    MSVR("ProcessWelcome_fromSendWelcome", msvc_ProcessWelcome, 20, "SendWelcome_basic"),

    MSVC("ProcessPlayerJoin_host", msvc_ProcessPlayerJoin, 0),
    MSVC("ProcessPlayerJoin_client", msvc_ProcessPlayerJoin, 1),
    MSVC("ProcessPlayerJoin_callback", msvc_ProcessPlayerJoin, 2),
    MSVC("ProcessPlayerJoin_noLocalThing", msvc_ProcessPlayerJoin, 3),
    MSVC("ProcessPlayerJoin_name63", msvc_ProcessPlayerJoin, 4),
    MSVC("ProcessPlayerJoin_last24", msvc_ProcessPlayerJoin, 5),

    MSVC("SyncPlayers_basic", msvc_SyncPlayers, 0),
    MSVC("SyncPlayers_noPlayers", msvc_SyncPlayers, 1),
    MSVC("SyncPlayers_24", msvc_SyncPlayers, 2),
    MSVC("SyncPlayers_mixedFlags", msvc_SyncPlayers, 3),
    MSVC("SyncPlayers_localId0", msvc_SyncPlayers, 4),
    MSVC("SyncPlayers_netId0", msvc_SyncPlayers, 5),
    MSVC("SyncPlayers_modelNotFound", msvc_SyncPlayers, 6),
    MSVC("SyncPlayers_nameLengths", msvc_SyncPlayers, 7),
    MSVC("SyncPlayers_highChars", msvc_SyncPlayers, 8),
    MSVC("SyncPlayers_toJoined", msvc_SyncPlayers, 9),
    MSVC("SyncPlayers_toId0", msvc_SyncPlayers, 10),
    MSVC("SyncPlayers_staleNames", msvc_SyncPlayers, 11),
    MSVC("SyncPlayers_client", msvc_SyncPlayers, 12),

    MSVR("ProcessSyncPlayers_fromSyncPlayers", msvc_ProcessSyncPlayers, 0, "SyncPlayers_basic"),
    MSVR("ProcessSyncPlayers_from24", msvc_ProcessSyncPlayers, 16, "SyncPlayers_24"),
    MSVR("ProcessSyncPlayers_fromMixedFlags", msvc_ProcessSyncPlayers, 0, "SyncPlayers_mixedFlags"),
    MSVC("ProcessSyncPlayers_joinMissed", msvc_ProcessSyncPlayers, 1),
    MSVC("ProcessSyncPlayers_joinWhileSyncing", msvc_ProcessSyncPlayers, 2),
    MSVC("ProcessSyncPlayers_removed", msvc_ProcessSyncPlayers, 3),
    MSVC("ProcessSyncPlayers_removedWhileSyncing", msvc_ProcessSyncPlayers, 4),
    MSVC("ProcessSyncPlayers_noPlayers", msvc_ProcessSyncPlayers, 5),
    MSVC("ProcessSyncPlayers_24", msvc_ProcessSyncPlayers, 6),
    MSVC("ProcessSyncPlayers_noNames", msvc_ProcessSyncPlayers, 7),
    MSVC("ProcessSyncPlayers_unterminatedName", msvc_ProcessSyncPlayers, 8),
    MSVC("ProcessSyncPlayers_modelNotFound", msvc_ProcessSyncPlayers, 9),
    MSVC("ProcessSyncPlayers_localId0", msvc_ProcessSyncPlayers, 10),
    MSVC("ProcessSyncPlayers_highChars", msvc_ProcessSyncPlayers, 11),
    MSVC("ProcessSyncPlayers_newServer", msvc_ProcessSyncPlayers, 12),
    MSVC("ProcessSyncPlayers_client", msvc_ProcessSyncPlayers, 13),
    MSVC("ProcessSyncPlayers_grow", msvc_ProcessSyncPlayers, 14),
    MSVC("ProcessSyncPlayers_allFlags", msvc_ProcessSyncPlayers, 15),

    MSVC("ProcessJoinRequest_client", msvc_ProcessJoinRequest, 0),
    MSVC("ProcessJoinRequest_wrongLevel", msvc_ProcessJoinRequest, 1),
    MSVC("ProcessJoinRequest_levelCase", msvc_ProcessJoinRequest, 2),
    MSVC("ProcessJoinRequest_levelUnterminated", msvc_ProcessJoinRequest, 3),
    MSVC("ProcessJoinRequest_alreadyIn", msvc_ProcessJoinRequest, 4),
    MSVC("ProcessJoinRequest_quitting", msvc_ProcessJoinRequest, 5),
    MSVC("ProcessJoinRequest_busy", msvc_ProcessJoinRequest, 6),
    MSVC("ProcessJoinRequest_welcomingSender", msvc_ProcessJoinRequest, 7),
    MSVC("ProcessJoinRequest_full", msvc_ProcessJoinRequest, 8),
    MSVC("ProcessJoinRequest_updatePlayersFails", msvc_ProcessJoinRequest, 9),
    MSVC("ProcessJoinRequest_verifyFails", msvc_ProcessJoinRequest, 10),
    MSVC("ProcessJoinRequest_wrongChecksum", msvc_ProcessJoinRequest, 11),
    MSVC("ProcessJoinRequest_joins", msvc_ProcessJoinRequest, 12),
    MSVC("ProcessJoinRequest_unterminatedName", msvc_ProcessJoinRequest, 13),
    MSVC("ProcessJoinRequest_staleName", msvc_ProcessJoinRequest, 14),
    MSVC("ProcessJoinRequest_senderId0", msvc_ProcessJoinRequest, 15),
    MSVC("ProcessJoinRequest_noPlayers", msvc_ProcessJoinRequest, 16),
    MSVC("ProcessJoinRequest_last24", msvc_ProcessJoinRequest, 17),
    MSVC("ProcessJoinRequest_emptyLevel", msvc_ProcessJoinRequest, 18),
    MSVC("ProcessJoinRequest_checksumIntMin", msvc_ProcessJoinRequest, 19),
    MSVC("ProcessJoinRequest_slotNotPlaced", msvc_ProcessJoinRequest, 20),
    MSVC("ProcessJoinRequest_twoFreeSlots", msvc_ProcessJoinRequest, 21),

    MSVC("FinishJoining_ok", msvc_FinishJoining, 0),
    MSVC("FinishJoining_busy", msvc_FinishJoining, 1),
    MSVC("FinishJoining_cancel", msvc_FinishJoining, 2),
    MSVC("FinishJoining_wrongChecksum", msvc_FinishJoining, 3),
    MSVC("FinishJoining_gameFull", msvc_FinishJoining, 4),
    MSVC("FinishJoining_wrongLevel", msvc_FinishJoining, 5),
    MSVC("FinishJoining_halfId0", msvc_FinishJoining, 6),
    MSVC("FinishJoining_edge_negZero", msvc_FinishJoining, 7),
    MSVC("FinishJoining_edge_fltMax", msvc_FinishJoining, 8),
    MSVC("FinishJoining_edge_negDenorm", msvc_FinishJoining, 9),
    MSVC("FinishJoining_code2", msvc_FinishJoining, 10),
    MSVC("FinishJoining_edge_denormMin", msvc_FinishJoining, 11),
    MSVC("FinishJoining_sendFails", msvc_FinishJoining, 100),

    MSVC("ProcessChat_player", msvc_ProcessChat, 0),
    MSVC("ProcessChat_noPlayer", msvc_ProcessChat, 1),
    MSVC("ProcessChat_playerIntMin", msvc_ProcessChat, 2),
    MSVC("ProcessChat_len1", msvc_ProcessChat, 3),
    MSVC("ProcessChat_len128", msvc_ProcessChat, 4),
    MSVC("ProcessChat_len200", msvc_ProcessChat, 5),
    MSVC("ProcessChat_lenUintMax", msvc_ProcessChat, 6),
    MSVC("ProcessChat_unterminated", msvc_ProcessChat, 7),
    MSVC("ProcessChat_formatChars", msvc_ProcessChat, 8),
    MSVC("ProcessChat_name63", msvc_ProcessChat, 9),
    MSVC("ProcessChat_last24", msvc_ProcessChat, 10),
    MSVC("ProcessChat_pastNumPlayers", msvc_ProcessChat, 11),
    MSVC("ProcessChat_len2", msvc_ProcessChat, 12),
    MSVC("ProcessChat_innerTerminator", msvc_ProcessChat, 13),
    MSVC("ProcessChat_emptyName", msvc_ProcessChat, 14),

    MSVC("ProcessPing_basic", msvc_ProcessPing, 0),
    MSVC("ProcessPing_sendFails", msvc_ProcessPing, 1),

    MSVC("ProcessPong_reply", msvc_ProcessPong, 0),
    MSVC("ProcessPong_otherStart", msvc_ProcessPong, 1),
    MSVC("ProcessPong_unknownSender", msvc_ProcessPong, 2),
    MSVC("ProcessPong_negative", msvc_ProcessPong, 3),
    MSVC("ProcessPong_start0", msvc_ProcessPong, 4),
    MSVC("ProcessPong_noPlayers", msvc_ProcessPong, 5),
    MSVC("ProcessPong_name63", msvc_ProcessPong, 6),
    MSVC("ProcessPong_duplicateId", msvc_ProcessPong, 7),

    MSVC("QuitPlayer_basic", msvc_QuitPlayer, 0),
    MSVC("QuitPlayer_client", msvc_QuitPlayer, 1),
    MSVC("QuitPlayer_id0", msvc_QuitPlayer, 2),
    MSVC("QuitPlayer_idUintMax", msvc_QuitPlayer, 3),

    MSVC("ProcessQuit_notServer", msvc_ProcessQuit, 0),
    MSVC("ProcessQuit_ejected", msvc_ProcessQuit, 1),
    MSVC("ProcessQuit_alreadyEjected", msvc_ProcessQuit, 2),
    MSVC("ProcessQuit_quittingEjected", msvc_ProcessQuit, 3),
    MSVC("ProcessQuit_otherPlayer", msvc_ProcessQuit, 4),
    MSVC("ProcessQuit_unknownPlayer", msvc_ProcessQuit, 5),
    MSVC("ProcessQuit_ejectTimeWraps", msvc_ProcessQuit, 6),
    MSVC("ProcessQuit_server0", msvc_ProcessQuit, 7),
    MSVR("ProcessQuit_fromQuitPlayer", msvc_ProcessQuit, 8, "QuitPlayer_basic"),
    MSVC("ProcessQuit_otherWhileEjected", msvc_ProcessQuit, 9),

    MSVC("QuitGame_set", msvc_QuitGame, 0),
    MSVC("QuitGame_sameTime", msvc_QuitGame, 1),
    MSVC("QuitGame_later", msvc_QuitGame, 2),
    MSVC("QuitGame_earlier", msvc_QuitGame, 3),
    MSVC("QuitGame_otherState", msvc_QuitGame, 4),
    MSVC("QuitGame_ejectedTime0", msvc_QuitGame, 5),
    MSVC("QuitGame_state0Earlier", msvc_QuitGame, 6),
    MSVC("QuitGame_timeUintMax", msvc_QuitGame, 7),
    MSVC("QuitGame_stateMinus1", msvc_QuitGame, 8),
    MSVC("QuitGame_stateIntMax", msvc_QuitGame, 9),
    MSVC("QuitGame_allBits", msvc_QuitGame, 10),

    MSVC("Update_notActive", msvc_Update, 0),
    MSVC("Update_client", msvc_Update, 1),
    MSVC("Update_ejectedExpired", msvc_Update, 2),
    MSVC("Update_quitExpired", msvc_Update, 3),
    MSVC("Update_ejectedAtTimeWelcoming", msvc_Update, 4),
    MSVC("Update_quitState3Expired", msvc_Update, 5),
    MSVC("Update_syncScores", msvc_Update, 6),
    MSVC("Update_hostIdle", msvc_Update, 7),
    MSVC("Update_welcomingQuitting", msvc_Update, 8),
    MSVC("Update_welcome59", msvc_Update, 9),
    MSVC("Update_welcome60", msvc_Update, 10),
    MSVC("Update_welcomeLeftOver", msvc_Update, 11),
    MSVC("Update_welcome600", msvc_Update, 12),
    MSVC("Update_welcome6000", msvc_Update, 13),
    MSVC("Update_welcomeState0", msvc_Update, 14),
    MSVC("Update_deltaWraps", msvc_Update, 15),
    MSVC("Update_welcomeThings", msvc_Update, 16),
    MSVC("Update_welcomeNoRemovals", msvc_Update, 17),
    MSVC("Update_welcomeState5", msvc_Update, 18),
    MSVC("Update_syncScoresWelcoming", msvc_Update, 19),

    MSVC("SyncScores_set", msvc_SyncScores, 0),
    MSVC("SyncScores_alreadySet", msvc_SyncScores, 1),

    MSVC("GetPlayerIndexByID_first", msvc_GetPlayerIndexByID, 0),
    MSVC("GetPlayerIndexByID_last", msvc_GetPlayerIndexByID, 1),
    MSVC("GetPlayerIndexByID_unknown", msvc_GetPlayerIndexByID, 2),
    MSVC("GetPlayerIndexByID_noPlayers", msvc_GetPlayerIndexByID, 3),
    MSVC("GetPlayerIndexByID_last24", msvc_GetPlayerIndexByID, 4),
    MSVC("GetPlayerIndexByID_duplicate", msvc_GetPlayerIndexByID, 5),
    MSVC("GetPlayerIndexByID_id0", msvc_GetPlayerIndexByID, 6),
    MSVC("GetPlayerIndexByID_pastNumPlayers", msvc_GetPlayerIndexByID, 7),

    MSVC("ProcessKilledPlayer_noKiller", msvc_ProcessKilledPlayer, 0),
    MSVC("ProcessKilledPlayer_actor", msvc_ProcessKilledPlayer, 1),
    MSVC("ProcessKilledPlayer_byPlayer", msvc_ProcessKilledPlayer, 2),
    MSVC("ProcessKilledPlayer_suicide0", msvc_ProcessKilledPlayer, 3),
    MSVC("ProcessKilledPlayer_suicide8191", msvc_ProcessKilledPlayer, 4),
    MSVC("ProcessKilledPlayer_suicide8192", msvc_ProcessKilledPlayer, 5),
    MSVC("ProcessKilledPlayer_suicide16383", msvc_ProcessKilledPlayer, 6),
    MSVC("ProcessKilledPlayer_suicide16384", msvc_ProcessKilledPlayer, 7),
    MSVC("ProcessKilledPlayer_suicide24575", msvc_ProcessKilledPlayer, 8),
    MSVC("ProcessKilledPlayer_suicide24576", msvc_ProcessKilledPlayer, 9),
    MSVC("ProcessKilledPlayer_suicide32767", msvc_ProcessKilledPlayer, 10),
    MSVC("ProcessKilledPlayer_suicideNoPlayer", msvc_ProcessKilledPlayer, 11),
    MSVC("ProcessKilledPlayer_byPlayerLongNames", msvc_ProcessKilledPlayer, 12),
    MSVC("ProcessKilledPlayer_emptyNames", msvc_ProcessKilledPlayer, 13),
    MSVC("ProcessKilledPlayer_name63", msvc_ProcessKilledPlayer, 14),
    MSVC("ProcessKilledPlayer_noPlayerThing", msvc_ProcessKilledPlayer, 15),

    MSVC("Respawn_onePlayer", msvc_Respawn, 0),
    MSVC("Respawn_allMasked", msvc_Respawn, 1),
    MSVC("Respawn_twoSlotsRand0", msvc_Respawn, 2),
    MSVC("Respawn_twoSlotsRandMax", msvc_Respawn, 3),
    MSVC("Respawn_notOccupied", msvc_Respawn, 4),
    MSVC("Respawn_occupied", msvc_Respawn, 5),
    MSVC("Respawn_allOccupied", msvc_Respawn, 6),
    MSVC("Respawn_dyingPlayer", msvc_Respawn, 7),
    MSVC("Respawn_destroyedPlayer", msvc_Respawn, 8),
    MSVC("Respawn_edge_7slots4681", msvc_Respawn, 9),
    MSVC("Respawn_edge_7slots9362", msvc_Respawn, 10),
    MSVC("Respawn_edge_14slots4681", msvc_Respawn, 11),
    MSVC("Respawn_edge_21slots14043", msvc_Respawn, 12),
    MSVC("Respawn_edge_7slots28086", msvc_Respawn, 13),
    MSVC("Respawn_24RandMax", msvc_Respawn, 14),
    MSVC("Respawn_24Rand12345", msvc_Respawn, 15),
    MSVC("Respawn_notPlayer", msvc_Respawn, 16),
    MSVC("Respawn_24OnlySlot0", msvc_Respawn, 17),
    MSVC("Respawn_occupiedAfterActor", msvc_Respawn, 18),

    MSVC("RemoveStaticThing_first", msvc_RemoveStaticThing, 0),
    MSVC("RemoveStaticThing_guidMinus1", msvc_RemoveStaticThing, 1),
    MSVC("RemoveStaticThing_last", msvc_RemoveStaticThing, 2),
    MSVC("RemoveStaticThing_full", msvc_RemoveStaticThing, 3),
    MSVC("RemoveStaticThing_pastFull", msvc_RemoveStaticThing, 4),
    MSVC("RemoveStaticThing_guidIntMax", msvc_RemoveStaticThing, 5),

    MSVC("UpdateSurfaces_first", msvc_UpdateSurfaces, 0),
    MSVC("UpdateSurfaces_from2", msvc_UpdateSurfaces, 1),
    MSVC("UpdateSurfaces_fromLast", msvc_UpdateSurfaces, 2),
    MSVC("UpdateSurfaces_atEnd", msvc_UpdateSurfaces, 3),
    MSVC("UpdateSurfaces_pastEnd", msvc_UpdateSurfaces, 4),
    MSVC("UpdateSurfaces_noneToSync", msvc_UpdateSurfaces, 5),
    MSVC("UpdateSurfaces_allToSync", msvc_UpdateSurfaces, 6),
    MSVC("UpdateSurfaces_noSurfaces", msvc_UpdateSurfaces, 7),
    MSVC("UpdateSurfaces_onlyLast", msvc_UpdateSurfaces, 8),

    MSVC("UpdateSectors_first", msvc_UpdateSectors, 0),
    MSVC("UpdateSectors_noneToSync", msvc_UpdateSectors, 1),
    MSVC("UpdateSectors_syncAndAdjoinsOff", msvc_UpdateSectors, 2),
    MSVC("UpdateSectors_adjoinsOff", msvc_UpdateSectors, 3),
    MSVC("UpdateSectors_atEnd", msvc_UpdateSectors, 4),
    MSVC("UpdateSectors_noSectors", msvc_UpdateSectors, 5),
    MSVC("UpdateSectors_onlyLast", msvc_UpdateSectors, 6),

    MSVC("UpdateThings_first", msvc_UpdateThings, 0),
    MSVC("UpdateThings_noneToSync", msvc_UpdateThings, 1),
    MSVC("UpdateThings_onlyWeapon", msvc_UpdateThings, 2),
    MSVC("UpdateThings_onlyExplosion", msvc_UpdateThings, 3),
    MSVC("UpdateThings_uniqueGuid", msvc_UpdateThings, 4),
    MSVC("UpdateThings_noThings", msvc_UpdateThings, 5),
    MSVC("UpdateThings_atLast", msvc_UpdateThings, 6),
    MSVC("UpdateThings_pastLast", msvc_UpdateThings, 7),
    MSVC("UpdateThings_negativeGuid", msvc_UpdateThings, 8),

    MSVC("UpdateRemovals_first", msvc_UpdateRemovals, 0),
    MSVC("UpdateRemovals_lastFinishes", msvc_UpdateRemovals, 1),
    MSVC("UpdateRemovals_noneFinishes", msvc_UpdateRemovals, 2),
    MSVC("UpdateRemovals_pastEnd", msvc_UpdateRemovals, 3),
    MSVC("UpdateRemovals_notWelcoming", msvc_UpdateRemovals, 4),
    MSVC("UpdateRemovals_slot23", msvc_UpdateRemovals, 5),
    MSVC("UpdateRemovals_id0", msvc_UpdateRemovals, 6),
    MSVC("UpdateRemovals_last256", msvc_UpdateRemovals, 7),

    MSVC("StartWelcomingPlayer_basic", msvc_StartWelcomingPlayer, 0),
    MSVC("StartWelcomingPlayer_alreadyWelcoming", msvc_StartWelcomingPlayer, 1),
    MSVC("StartWelcomingPlayer_id0", msvc_StartWelcomingPlayer, 2),
    MSVC("StartWelcomingPlayer_idUintMax", msvc_StartWelcomingPlayer, 3),
    MSVC("StartWelcomingPlayer_resetsCounters", msvc_StartWelcomingPlayer, 4),

    MSVC("StopWelcomingPlayer_error", msvc_StopWelcomingPlayer, 0),
    MSVC("StopWelcomingPlayer_noError", msvc_StopWelcomingPlayer, 1),
    MSVC("StopWelcomingPlayer_errorId0", msvc_StopWelcomingPlayer, 2),
    MSVC("StopWelcomingPlayer_notWelcoming", msvc_StopWelcomingPlayer, 3),
    MSVC("StopWelcomingPlayer_error2", msvc_StopWelcomingPlayer, 4),
    MSVC("StopWelcomingPlayer_errorMinus1", msvc_StopWelcomingPlayer, 5),
};

#define MSV_NUMCASES (sizeof(msv_aCases) / sizeof(msv_aCases[0]))

#endif // SITHMULTIVECCASES_H
