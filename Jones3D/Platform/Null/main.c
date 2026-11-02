// Added: Null-platform headless entry point. It runs the real game loop the way
// WinMain, Indy3D_WinMain and wkernel_Run do on Windows, in the same order:
// global reset, Startup (JonesMain_Startup failing means JonesMain_Shutdown and a
// nonzero exit), then JonesMain_Process() until the engine quits or the frame limit is
// reached, then JonesMain_Shutdown. The Windows-only parts of that boot chain stay
// behind: the retail mutex, the Win32 signal handler, window creation and the
// stdWin95 window/instance/Guid registration.
//
// Usage: Jones3D-headless [--load <save.nds>] [--frames <n>] [--step <msec>] [--state <file>]
//   --load   answers the load-game dialog once with the given path; needs the
//            "startMode = 1" (load game) entry in Jones.cfg, like on Windows.
//   --frames frame limit of the engine's own game-state frame counter (default 1000).
//   --step   milliseconds of virtual time each JonesMain_Process call advances the clock by
//            (default 33). 0 keeps the real clock and makes the run nondeterministic again.
//   --state  writes the final state block, plus one line per active thing, to a file.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Jones3D/Main/JonesGlobals.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Platform/Null/JonesPlatformNull.h>

#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithWorld.h>

#include <std/Null/stdPlatformNull.h>

#define HEADLESS_DEFAULT_FRAMES    1000
#define HEADLESS_DEFAULT_STEP      33
#define HEADLESS_MAX_STEP          1000
#define HEADLESS_PRINT_INTERVAL    100

static void Headless_PrintUsage(const char* pProgramName)
{
    fprintf(stderr, "usage: %s [--load <save.nds>] [--frames <n, default %d>] [--step <msec, default %d>] [--state <file>]\n",
        pProgramName ? pProgramName : "Jones3D-headless", HEADLESS_DEFAULT_FRAMES, HEADLESS_DEFAULT_STEP);
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
    size_t frameLimit        = HEADLESS_DEFAULT_FRAMES;
    unsigned long step       = HEADLESS_DEFAULT_STEP;

    for ( int i = 1; i < argc; ++i )
    {
        if ( !strcmp(argv[i], "--load") || !strcmp(argv[i], "--frames") || !strcmp(argv[i], "--step") || !strcmp(argv[i], "--state") )
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

    // wkernel_Run: the Startup callback (the dllmain Startup wrapper's logic).
    if ( JonesMain_Startup("") )
    {
        JonesMain_Shutdown();
        fprintf(stderr, "ERROR: Jones3D-headless: JonesMain_Startup failed, exiting nonzero!\n");
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

    JonesMain_Shutdown();

    printf("stopped: %s (frames=%zu)\n", pStopReason ? pStopReason : "unknown reason", frameCount);
    return exitCode;
}
