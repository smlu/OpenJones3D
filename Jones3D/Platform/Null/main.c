// Added: Null-platform headless entry point. It runs the real game loop the way
// WinMain, Indy3D_WinMain and wkernel_Run do on Windows, in the same order:
// global reset, Startup (JonesMain_Startup failing means JonesMain_Shutdown and a
// nonzero exit), then JonesMain_Process() until the engine quits or the frame limit is
// reached, then JonesMain_Shutdown. The Windows-only parts of that boot chain stay
// behind: the retail mutex, the Win32 signal handler, window creation and the
// stdWin95 window/instance/Guid registration.
//
// Usage: Jones3D-headless [--load <save.nds>] [--frames <n>] [--step <msec>] [--state <file>] [--input <file>]
//   --load   answers the load-game dialog once with the given path; needs the
//            "startMode = 1" (load game) entry in Jones.cfg, like on Windows.
//   --frames frame limit of the engine's own game-state frame counter (default 1000).
//   --step   milliseconds of virtual time each JonesMain_Process call advances the clock by
//            (default 33). 0 keeps the real clock and makes the run nondeterministic again.
//   --state  writes the final state block, plus one line per active thing, to a file.
//   --input  runs a scripted-input file: "frame <n>: cmd <command>" lines run console commands
//            and "frame <n>: key <name|number> down|up" lines press keys at engine frame <n>.
//            A malformed script aborts the run before startup, so the same script keeps a run
//            deterministic like every other aspect of the run.
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Jones3D/Main/JonesGlobals.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Platform/Null/JonesPlatformNull.h>

#include <sith/Devices/sithConsole.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdPlatform.h>
#include <std/Null/stdPlatformNull.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdKeys.h>

#define HEADLESS_DEFAULT_FRAMES    1000
#define HEADLESS_DEFAULT_STEP      33
#define HEADLESS_MAX_STEP          1000
#define HEADLESS_PRINT_INTERVAL    100

// Added: Scripted input (--input) constants: one parsed event per "frame <n>: <event>" line.
#define HEADLESS_INPUT_MAX_EVENTS         100000
#define HEADLESS_INPUT_MAX_LINE           512
#define HEADLESS_INPUT_MAX_TEXT           (HEADLESS_INPUT_MAX_LINE + 1)
#define HEADLESS_INPUT_INITIAL_CAPACITY   1024

// Added: --input event types.
typedef enum HeadlessInputEventType
{
    HEADLESS_INPUT_EVENT_CMD = 0,
    HEADLESS_INPUT_EVENT_KEY,
} HeadlessInputEventType;

// Added: One parsed "frame <n>: <event>" line. aText keeps the trimmed original text for
// the "input frame=..." print; the rest is the already validated event payload.
typedef struct HeadlessInputEvent
{
    size_t                 frame;    // JonesMain_GetFrameCount() value the event fires on
    HeadlessInputEventType type;
    char                   aText[HEADLESS_INPUT_MAX_TEXT]; // trimmed original event text
    const char*            pCmdText; // HEADLESS_INPUT_EVENT_CMD: points into aText, the command payload
    int                    keyId;    // HEADLESS_INPUT_EVENT_KEY: raw key id, see stdControl_UpdateKeyState
    int                    bPressed; // HEADLESS_INPUT_EVENT_KEY: 1 = down, 0 = up
} HeadlessInputEvent;

static void Headless_PrintUsage(const char* pProgramName)
{
    fprintf(stderr, "usage: %s [--load <save.nds>] [--frames <n, default %d>] [--step <msec, default %d>] [--state <file>] [--input <file>]\n",
        pProgramName ? pProgramName : "Jones3D-headless", HEADLESS_DEFAULT_FRAMES, HEADLESS_DEFAULT_STEP);
}

// Added: --input helpers. The script is parsed once, completely, before JonesMain_Startup, so a
// bad script aborts the run with exit code 1 before any engine code runs and a good script feeds
// identical events to the engine every run.
static HeadlessInputEvent* apInputEvents = NULL; // parsed events, malloc'd, freed in main
static size_t numInputEvents   = 0;
static size_t maxInputEvents   = 0;
static size_t nextInputEvent   = 0;    // index of the next event to fire in the main loop

// Added: Key id for every named key the script accepts, case-insensitively. The ids are the
// engine key numbers (DirectInput scan codes, see std/Win95/stdKeys.h) plus the mouse button
// ids from stdControl.
typedef struct HeadlessInputKeyName
{
    const char* pName;
    unsigned int keyId;
} HeadlessInputKeyName;

static const HeadlessInputKeyName aInputKeyNames[] =
{
    { "up",     STDKEY_UP },
    { "down",   STDKEY_DOWN },
    { "left",   STDKEY_LEFT },
    { "right",  STDKEY_RIGHT },
    { "space",  STDKEY_SPACE },
    { "ctrl",   STDKEY_LCONTROL },
    { "alt",    STDKEY_LMENU },
    { "shift",  STDKEY_LSHIFT },
    { "enter",  STDKEY_RETURN },
    { "mouse1", STDCONTROL_KID_MOUSE_LBUTTON },
    { "mouse2", STDCONTROL_KID_MOUSE_RBUTTON },
};

// True when the token names this key, ignoring case.
static bool Headless_InputIsKeyName(const char* pToken, const char* pName)
{
    do
    {
        if ( tolower((unsigned char)*pToken) != tolower((unsigned char)*pName) )
        {
            return false;
        }

        ++pToken;
        ++pName;
    } while ( *pToken && *pName );

    return *pToken == *pName;
}

// Trims whitespace in place: returns the first non-space character and shortens the string
// with a '\0' behind the last non-space one.
static char* Headless_InputTrim(char* pLine)
{
    while ( *pLine != '\0' && isspace((unsigned char)*pLine) )
    {
        ++pLine;
    }

    size_t len = strlen(pLine);
    while ( len > 0 && isspace((unsigned char)pLine[len - 1]) )
    {
        pLine[--len] = '\0';
    }

    return pLine;
}

// Returns the next whitespace-separated token from *ppCursor as '\0'-terminated text and
// advances the cursor behind it, or NULL when the text is spent.
static char* Headless_InputNextToken(char** ppCursor)
{
    char* pCursor = *ppCursor;
    while ( *pCursor != '\0' && isspace((unsigned char)*pCursor) )
    {
        ++pCursor;
    }

    if ( *pCursor == '\0' )
    {
        *ppCursor = pCursor;
        return NULL;
    }

    char* pToken = pCursor;
    while ( *pCursor != '\0' && !isspace((unsigned char)*pCursor) )
    {
        ++pCursor;
    }

    if ( *pCursor != '\0' )
    {
        *pCursor++ = '\0';
    }

    *ppCursor = pCursor;
    return pToken;
}

// Returns the key id for a script key name/number (STDCONTROL_MAX_KEYID-sized unsigned decimal),
// or ((unsigned int)-1) when the name is unknown or the number is out of range.
static unsigned int Headless_InputParseKeyId(const char* pToken)
{
    if ( isdigit((unsigned char)pToken[0]) )
    {
        char* pEnd = NULL;
        unsigned long keyId = strtoul(pToken, &pEnd, 10);
        if ( !pEnd || *pEnd || keyId >= STDCONTROL_MAX_KEYID )
        {
            return (unsigned int)-1;
        }

        return (unsigned int)keyId;
    }

    for ( size_t i = 0; i < sizeof(aInputKeyNames) / sizeof(aInputKeyNames[0]); ++i )
    {
        if ( Headless_InputIsKeyName(pToken, aInputKeyNames[i].pName) )
        {
            return aInputKeyNames[i].keyId;
        }
    }

    return (unsigned int)-1;
}

// Parses one "frame <n>: <event>" line into the apInputEvents slot at numInputEvents. On any
// problem it prints "ERROR: Jones3D-headless: <file>:<line>: <reason>" to stderr and returns false.
static bool Headless_InputParseLine(const char* pPath, size_t lineNum, char* aLine)
{
    char* pText = Headless_InputTrim(aLine);
    if ( *pText == '\0' || *pText == '#' )
    {
        return true; // ignore blank lines and comments
    }

    // Prefix "frame <n>:" — the number is read in place, so the ':' may abut it ("frame 10: cmd").
    if ( strncmp(pText, "frame", 5) != 0
        || (pText[5] != '\0' && !isspace((unsigned char)pText[5])) )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: missing 'frame <n>:' prefix!\n", pPath, lineNum);
        return false;
    }

    char* pCursor = pText + 5;
    while ( *pCursor != '\0' && isspace((unsigned char)*pCursor) )
    {
        ++pCursor;
    }

    if ( !isdigit((unsigned char)*pCursor) )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: missing 'frame <n>:' prefix!\n", pPath, lineNum);
        return false;
    }

    char* pEnd = NULL;
    unsigned long frame = strtoul(pCursor, &pEnd, 10);
    while ( *pEnd != '\0' && isspace((unsigned char)*pEnd) )
    {
        ++pEnd;
    }

    if ( *pEnd != ':' )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: missing 'frame <n>:' prefix!\n", pPath, lineNum);
        return false;
    }

    pEnd++;
    char* pEventText = Headless_InputTrim(pEnd);
    if ( *pEventText == '\0' )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: missing event text after 'frame %lu:'!\n", pPath, lineNum, frame);
        return false;
    }

    if ( numInputEvents > 0 && (size_t)frame < apInputEvents[numInputEvents - 1].frame )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: events must be in non-decreasing frame order!\n", pPath, lineNum);
        return false;
    }

    if ( numInputEvents == HEADLESS_INPUT_MAX_EVENTS )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: input script has more than %d events!\n", pPath, lineNum, HEADLESS_INPUT_MAX_EVENTS);
        return false;
    }

    if ( numInputEvents == maxInputEvents )
    {
        size_t newMax = maxInputEvents ? maxInputEvents * 2 : HEADLESS_INPUT_INITIAL_CAPACITY;
        HeadlessInputEvent* pEvents = realloc(apInputEvents, newMax * sizeof(HeadlessInputEvent));
        if ( !pEvents )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: out of memory reading input script!\n", pPath, lineNum);
            return false;
        }

        apInputEvents = pEvents;
        maxInputEvents = newMax;
    }

    // From here on the new event is built in its final slot; pEvent keeps all of it.
    HeadlessInputEvent* pEvent = &apInputEvents[numInputEvents];
    memset(pEvent, 0, sizeof(*pEvent));
    strcpy(pEvent->aText, pText);
    pEvent->frame = (size_t)frame;

    // The verb decides the event kind; both commands and keys are fully validated here so
    // firing cannot fail at run time.
    char* pEventCursor = pEventText;
    char* pVerb = Headless_InputNextToken(&pEventCursor);
    if ( strcmp(pVerb, "cmd") == 0 )
    {
        pEvent->type     = HEADLESS_INPUT_EVENT_CMD;
        pEvent->pCmdText = Headless_InputTrim(pEventCursor);
        if ( *pEvent->pCmdText == '\0' )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: missing command text after 'cmd'!\n", pPath, lineNum);
            return false;
        }

        // sithConsole_ExeCommand copies the command into a 128 char buffer, like the
        // interactive Jones console line, so longer commands would not run as written.
        if ( strlen(pEvent->pCmdText) >= 127 )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: command text longer than 126 characters!\n", pPath, lineNum);
            return false;
        }

        // pCmdText pointed into the parse line so far; re-base it into the stored text copy.
        pEvent->pCmdText = pEvent->aText + (pEvent->pCmdText - pText);
    }
    else if ( strcmp(pVerb, "key") == 0 )
    {
        char* pKeyToken = Headless_InputNextToken(&pEventCursor);
        if ( !pKeyToken )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: missing key name after 'key'!\n", pPath, lineNum);
            return false;
        }

        char* pStateToken = Headless_InputNextToken(&pEventCursor);
        if ( !pStateToken )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: missing 'down' or 'up' after key '%s'!\n", pPath, lineNum, pKeyToken);
            return false;
        }

        unsigned int keyId = Headless_InputParseKeyId(pKeyToken);
        if ( keyId == (unsigned int)-1 )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: unknown key '%s'!\n", pPath, lineNum, pKeyToken);
            return false;
        }

        if ( strcmp(pStateToken, "down") != 0 && strcmp(pStateToken, "up") != 0 )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: expected 'down' or 'up', got '%s'!\n", pPath, lineNum, pStateToken);
            return false;
        }

        if ( Headless_InputNextToken(&pEventCursor) )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: unexpected text after key event!\n", pPath, lineNum);
            return false;
        }

        pEvent->type     = HEADLESS_INPUT_EVENT_KEY;
        pEvent->keyId    = (int)keyId;
        pEvent->bPressed = strcmp(pStateToken, "down") == 0;
    }
    else
    {
        fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: unknown event '%s'!\n", pPath, lineNum, pVerb);
        return false;
    }

    ++numInputEvents;

    return true;
}

// Reads and parses a whole --input script file. The script is fully validated before startup,
// so any parse error prints one "ERROR: Jones3D-headless: <file>:<line>: <reason>" and returns
// false without leaving a half-parsed array behind.
static bool Headless_InputLoadFile(const char* pPath)
{
    FILE* pFile = fopen(pPath, "r");
    if ( !pFile )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: cannot open input script '%s'!\n", pPath);
        return false;
    }

    // fgets stops at 512 bytes of read data, so a longer line ends without a newline in the
    // buffer while the file continues below. Measure each line's content without its trailing
    // newline/CR and reject every line that is actually longer than the cap, so splitting
    // cannot smuggle an overlong line (or its remainder) through, EOF or not.
    char aLine[HEADLESS_INPUT_MAX_LINE + 2];
    size_t lineNum = 0;
    while ( fgets(aLine, sizeof(aLine), pFile) )
    {
        ++lineNum;
        size_t lineLen = strlen(aLine); // measured before trimming removes the '\n'
        if ( lineLen > 0 && aLine[lineLen - 1] == '\n' )
        {
            --lineLen;
            if ( lineLen > 0 && aLine[lineLen - 1] == '\r' )
            {
                --lineLen;
            }
        }

        if ( lineLen > HEADLESS_INPUT_MAX_LINE )
        {
            fprintf(stderr, "ERROR: Jones3D-headless: %s:%zu: line longer than %d characters!\n", pPath, lineNum, HEADLESS_INPUT_MAX_LINE);
            fclose(pFile);
            return false;
        }

        if ( !Headless_InputParseLine(pPath, lineNum, aLine) )
        {
            fclose(pFile);
            return false;
        }
    }

    bool bFailed = ferror(pFile) != 0;
    fclose(pFile);
    if ( bFailed )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: read error in input script '%s'!\n", pPath);
        return false;
    }

    return true;
}

// Fires every pending input event whose frame is due at the given engine frame count, in file
// order. Commands go through sithConsole_ExeCommand, keys through stdControl_UpdateKeyState with
// the same clock the Null input backend stamps its reads with.
static void Headless_InputFireDue(size_t frameCount)
{
    while ( nextInputEvent < numInputEvents && apInputEvents[nextInputEvent].frame <= frameCount )
    {
        HeadlessInputEvent* pEvent = &apInputEvents[nextInputEvent];
        printf("input frame=%zu %s\n", pEvent->frame, pEvent->aText);

        if ( pEvent->type == HEADLESS_INPUT_EVENT_CMD )
        {
            sithConsole_ExeCommand(pEvent->pCmdText);
        }
        else
        {
            stdControl_UpdateKeyState(pEvent->keyId, pEvent->bPressed, stdPlatform_GetTimeMsec());
        }

        ++nextInputEvent;
    }
}

// Releases the parsed input script array. Safe to call at any point, even when the
// load failed midway, since every error path above leaves the state consistent.
static void Headless_InputFree(void)
{
    free(apInputEvents);
    apInputEvents = NULL;
    numInputEvents = 0;
    maxInputEvents = 0;
    nextInputEvent = 0;
}

static size_t Headless_CountActiveThings(const SithWorld* pWorld)
{
    size_t numActive = 0;
    if ( pWorld && pWorld->aThings )
    {
        for ( size_t i = 0; i < pWorld->numThings; ++i )
        {
            if ( pWorld->aThings[i].type != SITH_THING_FREE )
            {
                ++numActive;
            }
        }
    }

    return numActive;
}

static int Headless_GetPlayerSectorIndex(const SithThing* pPlayerThing)
{
    if ( !sithWorld_g_pCurrentWorld || !pPlayerThing || !pPlayerThing->pInSector )
    {
        return -1;
    }

    return sithSector_GetSectorIndex(pPlayerThing->pInSector);
}

// Prints one run-state line: frame count, game time, player position, sector index,
// health and thing counts, with %.6f values so two runs can be diffed.
static void Headless_PrintRunState(size_t frameCount)
{
    const SithWorld* pWorld  = sithWorld_g_pCurrentWorld;
    const SithThing* pPlayer = sithPlayer_g_pLocalPlayerThing;

    float gameTime = sithTime_g_secGameTime;
    rdVector3 playerPos = { 0.0f, 0.0f, 0.0f };
    float playerHealth   = 0.0f;
    if ( pPlayer )
    {
        playerPos     = pPlayer->pos;
        playerHealth  = pPlayer->thingInfo.actorInfo.health;
    }

    size_t numThings = pWorld ? pWorld->numThings : 0;

    printf("state frame=%zu gameTime=%.6f playerPos=%.6f %.6f %.6f playerSector=%d playerHealth=%.6f things=%zu activeThings=%zu\n",
        frameCount,
        (double)gameTime,
        (double)playerPos.x, (double)playerPos.y, (double)playerPos.z,
        Headless_GetPlayerSectorIndex(pPlayer),
        (double)playerHealth,
        numThings,
        Headless_CountActiveThings(pWorld));
}

// Writes the final comparison block: the state line values plus every active thing's
// index, type and position, one per line.
static bool Headless_WriteStateFile(const char* pPath, size_t frameCount)
{
    FILE* pFile = fopen(pPath, "w");
    if ( !pFile )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: cannot open state file '%s' for writing!\n", pPath);
        return false;
    }

    const SithWorld* pWorld  = sithWorld_g_pCurrentWorld;
    const SithThing* pPlayer = sithPlayer_g_pLocalPlayerThing;

    float gameTime = sithTime_g_secGameTime;
    rdVector3 playerPos = { 0.0f, 0.0f, 0.0f };
    float playerHealth   = 0.0f;
    if ( pPlayer )
    {
        playerPos     = pPlayer->pos;
        playerHealth  = pPlayer->thingInfo.actorInfo.health;
    }

    fprintf(pFile, "frame %zu\n", frameCount);
    fprintf(pFile, "gameTime %.6f\n", (double)gameTime);
    fprintf(pFile, "playerPos %.6f %.6f %.6f\n",
        (double)playerPos.x, (double)playerPos.y, (double)playerPos.z);
    fprintf(pFile, "playerSector %d\n", Headless_GetPlayerSectorIndex(pPlayer));
    fprintf(pFile, "playerHealth %.6f\n", (double)playerHealth);
    fprintf(pFile, "things %zu\n", pWorld ? pWorld->numThings : (size_t)0);
    fprintf(pFile, "activeThings %zu\n", Headless_CountActiveThings(pWorld));

    if ( pWorld && pWorld->aThings )
    {
        for ( size_t i = 0; i < pWorld->numThings; ++i )
        {
            const SithThing* pThing = &pWorld->aThings[i];
            if ( pThing->type == SITH_THING_FREE )
            {
                continue;
            }

            fprintf(pFile, "thing %zu %d %.6f %.6f %.6f\n",
                i, (int)pThing->type,
                (double)pThing->pos.x, (double)pThing->pos.y, (double)pThing->pos.z);
        }
    }

    int bClosed = fclose(pFile) == 0;
    if ( !bClosed )
    {
        fprintf(stderr, "ERROR: Jones3D-headless: failed to flush state file '%s'!\n", pPath);
    }

    return bClosed != 0;
}

int main(int argc, char** argv)
{
    const char* pLoadPath    = NULL;
    const char* pStatePath   = NULL;
    const char* pInputPath   = NULL;
    size_t frameLimit        = HEADLESS_DEFAULT_FRAMES;
    unsigned long step       = HEADLESS_DEFAULT_STEP;

    for ( int i = 1; i < argc; ++i )
    {
        if ( !strcmp(argv[i], "--load") || !strcmp(argv[i], "--frames") || !strcmp(argv[i], "--step")
            || !strcmp(argv[i], "--state") || !strcmp(argv[i], "--input") )
        {
            if ( i + 1 >= argc )
            {
                fprintf(stderr, "ERROR: Jones3D-headless: %s needs a value!\n", argv[i]);
                Headless_PrintUsage(argv[0]);
                return 1;
            }

            if ( !strcmp(argv[i], "--load") )
            {
                pLoadPath = argv[++i];
            }
            else if ( !strcmp(argv[i], "--frames") )
            {
                char* pEnd = NULL;
                unsigned long value = strtoul(argv[++i], &pEnd, 10);
                if ( !pEnd || *pEnd || value == 0 || value > 0xFFFFFFFFUL )
                {
                    fprintf(stderr, "ERROR: Jones3D-headless: --frames needs a positive frame count!\n");
                    Headless_PrintUsage(argv[0]);
                    return 1;
                }

                frameLimit = (size_t)value;
            }
            else if ( !strcmp(argv[i], "--step") )
            {
                char* pEnd = NULL;
                unsigned long value = strtoul(argv[++i], &pEnd, 10);
                if ( !pEnd || *pEnd || value > HEADLESS_MAX_STEP )
                {
                    fprintf(stderr, "ERROR: Jones3D-headless: --step needs a millisecond count from 0 to %d!\n", HEADLESS_MAX_STEP);
                    Headless_PrintUsage(argv[0]);
                    return 1;
                }

                step = value;
            }
            else if ( !strcmp(argv[i], "--input") )
            {
                pInputPath = argv[++i];
            }
            else
            {
                pStatePath = argv[++i];
            }
        }
        else
        {
            fprintf(stderr, "ERROR: Jones3D-headless: unknown option '%s'!\n", argv[i]);
            Headless_PrintUsage(argv[0]);
            return 1;
        }
    }

    // Added: --input: parse the whole script up front. A malformed script exits 1 before any
    // engine code runs, so a run never starts on a script it cannot honor.
    if ( pInputPath && !Headless_InputLoadFile(pInputPath) )
    {
        Headless_InputFree();
        return 1;
    }

    printf("clock: %s, step %lu ms\n", step ? "virtual" : "real", step);

    // Added: The virtual clock must be latched on before any code reads the time, startup included.
    if ( step )
    {
        stdPlatformNull_UseVirtualClock(0x00100000);
    }

    // WinMain/DllMain: the global reset before anything else runs.
    Jones3D_ResetAllGlobals();

    // Indy3D_WinMain: hand the --load path to the Null load-game dialog, which
    // JonesMain_Startup asks when Jones.cfg is in the load-game start mode.
    if ( pLoadPath )
    {
        JonesPlatformNull_SetLoadPath(pLoadPath);
    }

    // wkernel_Run: the Startup callback (the WinMain.c Startup wrapper's logic).
    if ( JonesMain_Startup("") )
    {
        JonesMain_Shutdown();
        fprintf(stderr, "ERROR: Jones3D-headless: JonesMain_Startup failed, exiting nonzero!\n");
        Headless_InputFree();
        return 1;
    }

    // wkernel_Run: the main loop until JonesMain_Process() returns -1 (or 1 to finish),
    // stopped by the frame limit. Frames are counted by the engine's own counter, which
    // only advances once the game state is reached.
    int exitCode    = 0;
    const char* pStopReason = NULL;
    size_t frameCount       = 0;
    size_t nextMilestone    = HEADLESS_PRINT_INTERVAL;
    while ( 1 )
    {
        // Added: --input: fire every pending scripted event whose frame is due at the current
        // engine frame count, in file order, just before the engine processes the frame.
        Headless_InputFireDue(JonesMain_GetFrameCount());

        int result = JonesMain_Process();

        // Added: fixed time step: the engine clock moves exactly one frame's worth of time per
        // processed frame, so a run does not depend on the wall clock anymore.
        if ( step )
        {
            stdPlatformNull_AdvanceClock((uint32_t)step);
        }

        if ( result == -1 )
        {
            exitCode    = 1;
            pStopReason = "JonesMain_Process() returned -1 (engine quit)";
            break;
        }

        if ( result == 1 )
        {
            exitCode    = 1;
            pStopReason = "JonesMain_Process() returned 1 (finish requested)";
            break;
        }

        if ( result )
        {
            // Added: Same answer as wkernel_Run for results it does not handle.
            fprintf(stdout, "WARNING: Jones3D-headless: Unhandled result code: %d, skipping...\n", result);
        }

        frameCount = JonesMain_GetFrameCount();
        while ( frameCount >= nextMilestone )
        {
            Headless_PrintRunState(nextMilestone);
            nextMilestone += HEADLESS_PRINT_INTERVAL;
        }

        if ( frameCount >= frameLimit )
        {
            exitCode    = 0;
            pStopReason = "frame limit reached";
            break;
        }
    }

    // Capture the state before shutdown frees the world.
    Headless_PrintRunState(frameCount);
    if ( pStatePath && !Headless_WriteStateFile(pStatePath, frameCount) )
    {
        if ( !exitCode )
        {
            exitCode = 2;
        }
    }

    // Added: --input: report scripted events that outlived the run, then release the script.
    if ( nextInputEvent < numInputEvents )
    {
        printf("WARNING: Jones3D-headless: %zu input events never fired\n", numInputEvents - nextInputEvent);
    }
    Headless_InputFree();

    JonesMain_Shutdown();

    printf("stopped: %s (frames=%zu)\n", pStopReason ? pStopReason : "unknown reason", frameCount);
    return exitCode;
}
