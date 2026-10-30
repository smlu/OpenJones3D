#include "jonesInventory.h"
#include <j3dcore/j3dhook.h>
#include <Jones3D/RTI/symbols.h>

#include <sith/Cog/sithCog.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithWeapon.h>

#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdUtil.h>

#include <stdio.h>

static SithGameStatistics* jonesInventory_pGameStatistics;
static uint32_t jonesInventory_msecStatisticsGameTime;

void jonesInventory_InstallHooks(void)
{
    J3D_HOOKFUNC(jonesInventory_Open);
    J3D_HOOKFUNC(jonesInventory_Load);
    J3D_HOOKFUNC(jonesInventory_Close);
    J3D_HOOKFUNC(jonesInventory_ActivateItem);
    J3D_HOOKFUNC(jonesInventory_UseItem);
    J3D_HOOKFUNC(jonesInventory_ResetGameStatistics);
    J3D_HOOKFUNC(jonesInventory_GetMaxDifficultyLevel);
    J3D_HOOKFUNC(jonesInventory_UpdateSolvedHintsStatistics);
    J3D_HOOKFUNC(jonesInventory_UpdateIQPoints);
    J3D_HOOKFUNC(jonesInventory_GetTotalIQPoints);
    J3D_HOOKFUNC(jonesInventory_AdvanceStatistics);
    J3D_HOOKFUNC(jonesInventory_AdvanceFoundTreasuresStatistics);
    J3D_HOOKFUNC(jonesInventory_UpdateGameTimeStatistics);
    J3D_HOOKFUNC(jonesInventory_UpdateLevelStatisticTime);
    J3D_HOOKFUNC(jonesInventory_ResetStatisticsGameTime);
}

void jonesInventory_ResetGlobals(void)
{}

int jonesInventory_Open(void)
{
    STD_FORMAT(std_g_genBuffer, "misc\\%s", "items.dat");
    return jonesInventory_Load(std_g_genBuffer);
}

int J3DAPI jonesInventory_Load(const char* pFilename)
{
    SithCog* pItemCog;
    rdModel3* pInvModel;
    rdModel3* pItemModel;
    SithInventoryTypeFlag flags;

    STD_ASSERTREL(pFilename);
    if ( !stdConffile_Open(pFilename) )
    {
        return 1;
    }

    while ( 1 )
    {
        flags = 0, pItemCog = NULL, pItemModel = NULL, pInvModel = NULL;
        if ( !stdConffile_ReadArgs() || !strcmp(stdConffile_g_entry.aArgs[0].argValue, "end") )
        {
            // Success
            stdConffile_Close();
            return 0;
        }

        if ( stdConffile_g_entry.numArgs < 4u )
        {
            break;
        }

        int itemID = atoi(stdConffile_g_entry.aArgs[1].argValue);
        if ( itemID >= 200 )
        {
            break;
        }

        float min = strtof(stdConffile_g_entry.aArgs[2].argValue, NULL);
        float max = strtof(stdConffile_g_entry.aArgs[3].argValue, NULL);

        sscanf_s(stdConffile_g_entry.aArgs[4].argValue, "%x", &flags);

        for ( size_t argPos = 5; argPos < stdConffile_g_entry.numArgs; ++argPos )
        {
            if ( !strcmp(stdConffile_g_entry.aArgs[argPos].argName, "cog") )
            {
                pItemCog = sithCog_GetCog(stdConffile_g_entry.aArgs[argPos].argValue);
            }
            else if ( !strcmp(stdConffile_g_entry.aArgs[argPos].argName, "invmodel") )
            {
                pInvModel = sithModel_GetModel(stdConffile_g_entry.aArgs[argPos].argValue);
            }
            else if ( !strcmp(stdConffile_g_entry.aArgs[argPos].argName, "model") )
            {
                pItemModel = sithModel_GetModel(stdConffile_g_entry.aArgs[argPos].argValue);
            }
        }

        sithInventory_RegisterType(
            itemID,
            stdConffile_g_entry.aArgs[0].argValue,
            min,
            max,
            flags,
            pItemCog,
            pInvModel,
            pItemModel
        );
    }

    STDLOG_ERROR("Parse error reading item definition file %s.\n", pFilename);
    return 1;
}

void J3DAPI jonesInventory_Close()
{}

int J3DAPI jonesInventory_ActivateItem(SithThing* pThing, int typeId)
{
    J3D_UNUSED(typeId);
    return sithPlayerActions_Activate(pThing) != 0;
}

int J3DAPI jonesInventory_UseItem(SithThing* pPlayer, unsigned int typeId)
{
    sithInventory_GetCurrentItem(pPlayer);      // TODO:??? maybe just an expensive sanity check that we have player thing and pPlayer is not null
    if ( sithInventory_GetInventory(pPlayer, typeId) == 0.0f || !sithInventory_IsInventoryAvailable(pPlayer, typeId) )
    {
        return 0;
    }

    SithInventoryType* pItem = sithInventory_GetType(typeId);
    if ( pItem->pCog )
    {
        sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_INT, typeId, SITHCOG_SYM_REF_THING, pPlayer->idx, 0);
    }

    return 1;
}

int J3DAPI jonesInventory_ResetGameStatistics()
{
    jonesInventory_msecStatisticsGameTime = 0;

    jonesInventory_pGameStatistics = sithGamesave_GetGameStatistics();
    if ( !jonesInventory_pGameStatistics || !sithGamesave_LockGameStatistics() )
    {
        return 1;
    }

    STD_ZEROMEM(jonesInventory_pGameStatistics, sizeof(SithGameStatistics));

    jonesInventory_pGameStatistics->aLevelStatistic[0].levelStartIQPoints = 150;
    jonesInventory_pGameStatistics->totalIQPoints = 150;
    jonesInventory_pGameStatistics->curLevelNum   = 0;

    sithGamesave_UnlockGameStatistics();
    sithWeapon_SetActorKilledCallback(jonesInventory_UpdateIQPoints);

    return 0;
}

size_t jonesInventory_GetMaxDifficultyLevel(void)
{
    return 5;
}

void jonesInventory_UpdateSolvedHintsStatistics(void)
{
    if ( sithGamesave_LockGameStatistics() )
    {
        int levelNum = jonesInventory_pGameStatistics->curLevelNum;

        size_t numSeen, numHints;
        sithOverlayMap_GetNumSeenHints(&numSeen, &numHints);

        size_t numSeenHintsLvl = jonesInventory_pGameStatistics->aLevelStatistic[levelNum].numSeenHints;
        if ( numSeenHintsLvl != numSeen )
        {
            jonesInventory_pGameStatistics->aLevelStatistic[levelNum].numSeenHints = numSeen;
            jonesInventory_pGameStatistics->totalLevelStats.numSeenHints += numSeen - numSeenHintsLvl; // TODO: Make sure numSeen > numSeenHintsLvl
        }

        sithGamesave_UnlockGameStatistics();
    }
}

void J3DAPI jonesInventory_UpdateIQPoints(size_t difficulty)
{
    if ( sithGamesave_LockGameStatistics() )
    {
        if ( jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates < 20 )
        {
            if ( difficulty > 5 )
            {
                difficulty = 5;
            }

            int penalty = 5 - difficulty;
            if ( (int)(5 - difficulty + jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates) <= 20 )
            {
                jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates += penalty;
            }
            else
            {
                penalty = 20 - jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates;
                jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates = 20;
            }

            jonesInventory_pGameStatistics->totalIQPoints -= penalty;
            jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].difficultyPenalty += penalty;
            jonesInventory_pGameStatistics->totalLevelStats.difficultyPenalty += penalty;
        }

        if ( jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].difficultyPenalty < 0 )
        {
            jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].difficultyPenalty = 0;
        }

        sithGamesave_UnlockGameStatistics();
    }
}

int jonesInventory_GetTotalIQPoints(void)
{
    size_t numHints, numSeen;
    sithOverlayMap_GetNumSeenHints(&numSeen, &numHints);

    size_t numFoundTreasures = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numFoundTreasures;
    if ( numHints <= 0 )
    {
        return jonesInventory_pGameStatistics->totalIQPoints + 10 * numFoundTreasures / 10;// TODO: each found item is expected to have 1 IQ point and there is expected at max 10 found items per level. Remove this max 10 items limit
    }

    size_t hintPenalty = 10 * numSeen / numHints;
    if ( hintPenalty == 0 )
    {
        hintPenalty = numSeen > 0;
    }

    return jonesInventory_pGameStatistics->totalIQPoints + 10 * numFoundTreasures / 10 - hintPenalty;// TODO: same here
}

void jonesInventory_AdvanceStatistics(void)
{
    jonesInventory_UpdateGameTimeStatistics(sithTime_g_msecGameTime);
    if ( sithGamesave_LockGameStatistics() )
    {
        size_t numSeenHints = 0, numTotalHints = 0;
        sithOverlayMap_GetNumSeenHints(&numSeenHints, &numTotalHints);

        size_t numSeenHintsLvl   = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numSeenHints;
        size_t numFoundTreasures = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numFoundTreasures;

        // Note, the max treasure items to be found in level is capped to 10
        if ( numTotalHints <= 0 )
        {
            jonesInventory_pGameStatistics->totalIQPoints += 10 * numFoundTreasures / 10 + 40;
        }
        else
        {
            size_t hintPoints = 10 * numSeenHints / numTotalHints;
            if ( hintPoints == 0 )
            {
                hintPoints = numSeenHints > 0;
            }

            jonesInventory_pGameStatistics->totalIQPoints += 10 * numFoundTreasures / 10 + 40 - hintPoints; // TODO: Make sure this is can't be negative
        }

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].iqPoints = jonesInventory_pGameStatistics->totalIQPoints
            - jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].levelStartIQPoints;

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum + 1].levelStartIQPoints = jonesInventory_pGameStatistics->totalIQPoints;

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numSeenHints = numSeenHints;
        jonesInventory_pGameStatistics->totalLevelStats.numSeenHints += numSeenHints - numSeenHintsLvl; // TODO: Make sure this is can't be negative

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum + 1].curElapsedSec = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].curElapsedSec;

        ++jonesInventory_pGameStatistics->curLevelNum;
        sithGamesave_UnlockGameStatistics();
    }

    jonesInventory_msecStatisticsGameTime = 0;
}

void jonesInventory_AdvanceFoundTreasuresStatistics()
{
    if ( sithGamesave_LockGameStatistics() )
    {
        ++jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numFoundTreasures;
        ++jonesInventory_pGameStatistics->totalLevelStats.numFoundTreasures;
        sithGamesave_UnlockGameStatistics();
    }
}

void J3DAPI jonesInventory_UpdateGameTimeStatistics(uint32_t msecTime)
{
    if ( sithGamesave_LockGameStatistics() )
    {
        jonesInventory_UpdateLevelStatisticTime(
            jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].curElapsedSec,
            &jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum],
            msecTime,
            jonesInventory_msecStatisticsGameTime
        );

        jonesInventory_UpdateLevelStatisticTime(
            jonesInventory_pGameStatistics->totalLevelStats.curElapsedSec,
            &jonesInventory_pGameStatistics->totalLevelStats,
            msecTime,
            jonesInventory_msecStatisticsGameTime
        );

        jonesInventory_msecStatisticsGameTime = msecTime;
        sithGamesave_UnlockGameStatistics();
    }
}

void J3DAPI jonesInventory_UpdateLevelStatisticTime(uint32_t elapsedSec, SithLevelStatistic* pLevelStats, uint32_t msecNewTime, uint32_t msecPreviousTime)
{
    uint32_t msecNewElapsed;
    if ( msecNewTime >= jonesInventory_msecStatisticsGameTime )
    {
        msecNewElapsed = (int)msecNewTime + 1000 * (int)elapsedSec - (int)msecPreviousTime;
    }
    else
    {
        msecNewElapsed = msecNewTime + 1000 * elapsedSec - msecPreviousTime - 1;
    }

    //pLevelStats->elapsedTime = ((int)(msecNewElapsed / 60000 % 60// minutes
    //    + (pLevelStats->elapsedTime ^ (pLevelStats->elapsedTime >> 8 << 8))) % 60) | ((msecNewElapsed / 3600000// hours
    //        + (pLevelStats->elapsedTime >> 8)
    //        + (int)(msecNewElapsed / 60000 % 60// minutes
    //            + (pLevelStats->elapsedTime ^ (pLevelStats->elapsedTime >> 8 << 8)))
    //        / 60) << 8);

    // Extract old hours and minutes from elapsedTime.
    uint8_t oldMinutes = pLevelStats->elapsedTime & 0xFF; // lower 8 bits
    uint32_t oldHours  = pLevelStats->elapsedTime >> 8;   // upper bits.

    // Calculate new time components from msecNewElapsed.
    uint8_t newMinutes = (msecNewElapsed / 60000) % 60;  // minutes portion
    uint32_t newHours  = msecNewElapsed / 3600000;       // hours portion

    // Sum the minutes and compute carry for hours.
    uint16_t totalMinutes = oldMinutes + newMinutes;
    uint32_t carryHours = totalMinutes / 60;
    totalMinutes %= 60;

    // Calculate total hours.
    uint32_t totalHours = oldHours + newHours + carryHours;

    // Pack the hours and minutes back into a single integer.
    pLevelStats->elapsedTime = (totalHours << 8) | totalMinutes;

    pLevelStats->curElapsedSec = msecNewElapsed / 1000 % 60;
}

void jonesInventory_ResetStatisticsGameTime(void)
{
    jonesInventory_msecStatisticsGameTime = sithTime_g_msecGameTime;
}

