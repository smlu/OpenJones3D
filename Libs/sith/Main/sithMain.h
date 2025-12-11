#ifndef SITH_SITHMAIN_H
#define SITH_SITHMAIN_H
#include <j3dcore/j3d.h>
#include <j3dcore/j3dhook.h>

#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/RTI/addresses.h>

#include <std/types.h>
#include <std/General/std.h>

J3D_EXTERN_C_START

#define SITH_RAND() ((double)rand() * (1.0 / RAND_MAX))
#define SITH_RANDF() ((float)SITH_RAND())

#define SITHLOG_DEBUG(format, ...) \
    J3DLOG_DEBUG(sith_g_pHS, format, ##__VA_ARGS__)

#define SITHLOG_STATUS(format, ...) \
    J3DLOG_STATUS(sith_g_pHS, format, ##__VA_ARGS__)

#define SITHLOG_MESSAGE(format, ...) \
    J3DLOG_MESSAGE(sith_g_pHS, format, ##__VA_ARGS__)

#define SITHLOG_WARNING(format, ...) \
    J3DLOG_WARNING(sith_g_pHS, format, ##__VA_ARGS__)

#define SITHLOG_ERROR(format, ...) \
    J3DLOG_ERROR(sith_g_pHS, format, ##__VA_ARGS__)

#define SITHLOG_FATAL(message) \
    J3DLOG_FATAL(sith_g_pHS, message)

#define SITH_ASSERT(condition) \
    J3D_ASSERT(condition, sith_g_pHS )

#define SITH_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, sith_g_pHS )

/**
 * Checks if current game frame number + offset is N-th frame (like every 4th, 8th, or 16th frame).
 * Uses the global frame counter sithMain_g_frameNumber with an optional offset.
 *
 * @param offset Shifts current game frame for which frame is considered the "N-th" one
 * @param n      N-th frame, i.e.: on every n-th frame (e.g.: every 2nd, 4th, 8th, 16th frame, etc.)
 * @return       True on every N-th frame
 */
#define SITH_ISFRAMECYCLE(offset, n) sithIsFrameCycleRange(offset, n)

#define SITH_INTENDED_FPS 30.0f

#define SITH_CFG_INSTALLPATH          "installPath"
#define SITH_CFG_GAMEPLAY_DIFFICULTY  "gameplay.difficulty"


#define sith_g_pHS J3D_DECL_FAR_VAR(sith_g_pHS, tHostServices*)
// extern tHostServices *sith_g_pHS;

// TODO: Rename all to be prefixed only by 'sith'

#define sithMain_g_frameNumber J3D_DECL_FAR_VAR(sithMain_g_frameNumber, size_t)
// extern int sithMain_g_frameNumber;

#define sithMain_g_sith_mode J3D_DECL_FAR_VAR(sithMain_g_sith_mode, SithMode)
// extern SithMode sithMain_g_sith_mode;

#define sithMain_g_curRenderTick J3D_DECL_FAR_VAR(sithMain_g_curRenderTick, size_t)
// extern unsigned int sithMain_g_curRenderTick;

extern size_t sithMain_g_currentIntendedFrameNumber;
extern size_t sithMain_g_lastIntendedFrameNumber;

void J3DAPI sithSetServices(tHostServices* pHS);
void sithClearServices(void);

int sithStartup(void);
void sithShutdown(void);

void J3DAPI sithSetPerformanceLevel(size_t level);
int sithGetPerformanceLevel(void);

int J3DAPI sithOpenStatic(const char* pFilename);
void sithCloseStatic(void);

int J3DAPI sithOpenNormal(const char* pWorldName, const wchar_t* wszPlayerName);
void sithOpenPostProcess(void);
int J3DAPI sithOpenMulti(const char* pFilename, const wchar_t* pwPlayerName); // Added

int J3DAPI sithOpen(const wchar_t* wszPlayerName);
void sithClose(void);

void sithUpdate(void);
void sithDrawScene(void);
void sithAdvanceRenderTick(void);
bool sithToggleDrawPlayerRadius(void); // Added

void J3DAPI sithSetGameDifficulty(int difficulty);
int sithGetGameDifficulty(void);

float sithGetHitAccuarancyScalar(void);
float sithGetCombatDamageScalar(void);
float sithGetIMPDamageScalar(void);

void sithMakeDirs(void);

const char* sithGetAutoSaveFilePrefix(void);
const char* sithGetSaveGamesDir(void);
const char* sithGetQuickSaveFilePrefix(void);
const char* sithGetScreenShotsDir(void);
const char* sithGetScreenShotFileTemplate(void);
void J3DAPI sithSetOpenCallback(SithOpenCallback pfCallback);
const char* sithGetCurrentWorldSaveName(void);
const char* J3DAPI sithGetLevelSaveFilename(size_t levelNum);

// Helper hooking functions
void sithMain_InstallHooks(void);
void sithMain_ResetGlobals(void);

static inline int sithIsFrameCycleRange(size_t offset, size_t n)
{
    // no advance -> nothing new
    if ( sithMain_g_currentIntendedFrameNumber <= sithMain_g_lastIntendedFrameNumber ) return 0;

    // range start and length
    size_t start = sithMain_g_lastIntendedFrameNumber + 1;
    size_t end   = sithMain_g_currentIntendedFrameNumber;

    // quick path: if range length >= n, there must be a hit
    if ( (end - start + 1) >= n ) return 1;

    // compute offset-adjusted residue of start
    size_t startAdj = (start + offset) % n;

    // distance from start to the next k with (k+offset) % n == 0
    size_t distToNext = (n - startAdj) % n; // in [0, n-1]

    // if distToNext steps from start fits into the range [start..end], there is a hit
    return distToNext <= (end - start) ? 1 : 0;
}

J3D_EXTERN_C_END
#endif // SITH_SITHMAIN_H
