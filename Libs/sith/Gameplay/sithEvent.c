#include "sithEvent.h"
#include <j3dcore/j3dhook.h>

#include <sith/Gameplay/sithTime.h>
#include <sith/RTI/symbols.h>

#include <std/General/stdUtil.h>

bool bEventStartup = false; // Added: Init to false 
bool bOpen         = false; // Added: Init to false 

SithEvent aEventBuffer[256];
int aFreeEventBufferIdxs[256];
SithEventTask aTasks[5];
size_t numFreeEventBuffers;

void sithEvent_ResetFreeBufferTable(void);
SithEvent* sithEvent_Create(void);
void J3DAPI sithEvent_AddEvent(SithEvent* pEvent);
void sithEvent_InstallHooks(void)
{
    J3D_HOOKFUNC(sithEvent_Startup);
    J3D_HOOKFUNC(sithEvent_Shutdown);
    J3D_HOOKFUNC(sithEvent_Open);
    J3D_HOOKFUNC(sithEvent_Close);
    J3D_HOOKFUNC(sithEvent_Reset);
    J3D_HOOKFUNC(sithEvent_CreateEvent);
    J3D_HOOKFUNC(sithEvent_FreeEvent);
    J3D_HOOKFUNC(sithEvent_RegisterTask);
    J3D_HOOKFUNC(sithEvent_Process);
    J3D_HOOKFUNC(sithEvent_ResetFreeBufferTable);
    J3D_HOOKFUNC(sithEvent_Create);
    J3D_HOOKFUNC(sithEvent_AddEvent);
}

void sithEvent_ResetGlobals(void)
{
    memset(&sithEvent_g_pFirstQueuedEvent, 0, sizeof(sithEvent_g_pFirstQueuedEvent));
}

int sithEvent_Startup(void)
{
    if ( bEventStartup )
    {
        SITHLOG_ERROR("Warning: System already initialized!\n");
        return 0;
    }

    memset(aTasks, 0, sizeof(aTasks));
    sithEvent_Reset();

    bEventStartup = true;
    return 1;
}

void sithEvent_Shutdown(void)
{
    if ( !bEventStartup )
    {
        SITHLOG_ERROR("Warning: System already shutdown!\n");
        return;
    }

    bEventStartup = false;
}

void sithEvent_Open(void)
{
    if ( !bOpen )
    {
        bOpen = true;
    }
}

void sithEvent_Close(void)
{
    if ( bOpen )
    {
        sithEvent_Reset();
        bOpen = false;
    }
}

void sithEvent_Reset(void)
{
    memset(aEventBuffer, 0, sizeof(aEventBuffer));
    sithEvent_ResetFreeBufferTable();
    sithEvent_g_pFirstQueuedEvent = NULL;
}

int J3DAPI sithEvent_CreateEvent(size_t taskId, SithEventParams* params, uint32_t when)
{
    SITH_ASSERTREL((taskId > 0) && (taskId < STD_ARRAYLEN(aTasks)) && (when >= 0));

    SithEvent* pEvent = sithEvent_Create();
    if ( !pEvent )
    {
        SITHLOG_ERROR("Out of space in event buffer array.  Event lost.\n");
        return 0;
    }

    pEvent->taskNum       = taskId;
    pEvent->msecEventTime = when + sithTime_g_msecGameTime;
    memcpy(&pEvent->params, params, sizeof(pEvent->params));
    sithEvent_AddEvent(pEvent);
    return 1;
}

void J3DAPI sithEvent_FreeEvent(SithEvent* pEvent)
{
    SITH_ASSERTREL((numFreeEventBuffers < (STD_ARRAYLEN(aEventBuffer))));
    SITH_ASSERTREL(pEvent != NULL);

    size_t index = pEvent - aEventBuffer;
    SITH_ASSERTREL((index >= 0) && (index < STD_ARRAYLEN(aEventBuffer)));

    memset(pEvent, 0, sizeof(SithEvent));
    aFreeEventBufferIdxs[numFreeEventBuffers++] = index;
}

int J3DAPI sithEvent_RegisterTask(size_t taskId, SithEventProcess pfProcess, uint32_t frequency, SithEventTaskMode startMode)
{
    SITH_ASSERTREL(bEventStartup);
    SITH_ASSERTREL((taskId > 0) && (taskId < STD_ARRAYLEN(aTasks)) && (frequency >= 0));
    SITH_ASSERTREL((startMode >= SITHEVENT_TASKDISABLED) && (startMode <= SITHEVENT_TASKONDEMAND));

    aTasks[taskId].pfProcess            = pfProcess;
    aTasks[taskId].msecLastIntervalTime = sithTime_g_msecGameTime;
    aTasks[taskId].unknown10            = 0;
    aTasks[taskId].msecFrequency        = frequency;
    aTasks[taskId].startMode            = startMode;
    return 1;
}

void sithEvent_Process(void)
{
    // Process tasks
    size_t numProcessed = 0; // Useless
    for ( size_t i = 1; i < STD_ARRAYLEN(aTasks); ++i )
    {
        if ( aTasks[i].startMode == SITHEVENT_TASKINTERVAL
            && sithTime_g_msecGameTime - aTasks[i].msecLastIntervalTime > aTasks[i].msecFrequency )
        {
            if ( aTasks[i].pfProcess(sithTime_g_msecGameTime - aTasks[i].msecLastIntervalTime, NULL) )
            {
                ++numProcessed;
                aTasks[i].msecLastIntervalTime = sithTime_g_msecGameTime;
            }
        }
    }

    // Process queued events
    while ( 1 )
    {
        SithEvent* pEvent = sithEvent_g_pFirstQueuedEvent;
        if ( !sithEvent_g_pFirstQueuedEvent || sithEvent_g_pFirstQueuedEvent->msecEventTime >= sithTime_g_msecGameTime )
        {
            break;
        }

        size_t taskNum = sithEvent_g_pFirstQueuedEvent->taskNum;
        SITH_ASSERTREL(aTasks[taskNum].pfProcess); // TODO: Would make much more sense to make this check when event is created
        aTasks[taskNum].pfProcess(0, &pEvent->params);

        sithEvent_g_pFirstQueuedEvent = pEvent->pNextEvent;
        sithEvent_FreeEvent(pEvent);
    }
}

void sithEvent_ResetFreeBufferTable(void)
{
    // Fixed: i > 0 instead of i >= 0. The last iteration (i == 0) wrote -1 to aFreeEventBufferIdxs[256], one past the end
    for ( int i = STD_ARRAYLEN(aFreeEventBufferIdxs); i > 0; --i )
    {
        aFreeEventBufferIdxs[STD_ARRAYLEN(aFreeEventBufferIdxs) - i] = i - 1;
    }

    numFreeEventBuffers = STD_ARRAYLEN(aFreeEventBufferIdxs);
}

SithEvent* sithEvent_Create(void)
{
    if ( numFreeEventBuffers == 0 )
    {
        SITHLOG_ERROR("Out of event buffer space.\n");
        return NULL;
    }

    --numFreeEventBuffers;
    return &aEventBuffer[aFreeEventBufferIdxs[numFreeEventBuffers]];
}

void J3DAPI sithEvent_AddEvent(SithEvent* pEvent)
{
    SITH_ASSERTREL(pEvent != NULL);

    SithEvent* pNextEvent = sithEvent_g_pFirstQueuedEvent;
    SithEvent* pPrevEvent = NULL;
    while ( pNextEvent && pNextEvent->msecEventTime <= pEvent->msecEventTime )
    {
        pPrevEvent = pNextEvent;
        pNextEvent = pNextEvent->pNextEvent;
    }

    if ( pPrevEvent )
    {
        pPrevEvent->pNextEvent = pEvent;
    }
    else
    {
        sithEvent_g_pFirstQueuedEvent = pEvent;
    }

    pEvent->pNextEvent = pNextEvent;
}
