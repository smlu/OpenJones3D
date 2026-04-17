# Event And Message Systems

This page documents the engine's three main dispatch paths:

1. the local deferred event scheduler
2. the gameplay-side COG message system
3. the multiplayer/file synchronization message bus

Jones3D does not use one universal event bus for everything. Different problems use different delivery mechanisms, and understanding the engine means understanding where those mechanisms meet.

Primary source files:

- [`Libs/sith/Gameplay/sithEvent.c`](../../Libs/sith/Gameplay/sithEvent.c)
- [`Libs/sith/Cog/sithCog.c`](../../Libs/sith/Cog/sithCog.c)
- [`Libs/sith/Cog/sithCogExec.c`](../../Libs/sith/Cog/sithCogExec.c)
- [`Libs/sith/Devices/sithComm.c`](../../Libs/sith/Devices/sithComm.c)
- [`Libs/sith/Dss/sithDSSCog.c`](../../Libs/sith/Dss/sithDSSCog.c)
- [`Libs/sith/AI/sithAIAwareness.c`](../../Libs/sith/AI/sithAIAwareness.c)

## Three Different Kinds Of "Event"

The word "event" means different things in different parts of the engine.

- `sithEvent` handles deferred local work keyed to game time.
- COG messages are gameplay/script dispatches such as `TOUCHED`, `ACTIVATE`, `TIMER`, or `DAMAGED`.
- `sithMessage` handles serialized DSS/network/file messages such as sync updates, replicated COG messages, and multiplayer control state.

Those systems interact, but they are not interchangeable.

## Local Deferred Scheduler: `sithEvent`

[`sithEvent.c`](../../Libs/sith/Gameplay/sithEvent.c#L1) implements a very small fixed scheduler.

### Fixed Storage Model

The scheduler owns:

- a fixed event pool of `256` records
- a free-list array with `256` indices
- a task table with `5` task slots
- one sorted linked list of queued events

Those structures live directly in [`sithEvent.c`](../../Libs/sith/Gameplay/sithEvent.c#L12). There is no heap allocation in the normal runtime path.

### Startup And Registration

The key entry points are:

- [`sithEvent_Startup()`](../../Libs/sith/Gameplay/sithEvent.c#L41)
- [`sithEvent_Open()`](../../Libs/sith/Gameplay/sithEvent.c#L67)
- [`sithEvent_Reset()`](../../Libs/sith/Gameplay/sithEvent.c#L84)
- [`sithEvent_RegisterTask()`](../../Libs/sith/Gameplay/sithEvent.c#L121)

Tasks can be interval-driven or on-demand. The important registrations in the current runtime are:

- the COG timer task, installed by [`sithCog_Startup()`](../../Libs/sith/Cog/sithCog.c#L132) through [`sithEvent_RegisterTask()`](../../Libs/sith/Cog/sithCog.c#L156)
- the AI awareness propagation task, installed by [`sithAIAwareness_Startup()`](../../Libs/sith/AI/sithAIAwareness.c#L56) through [`sithEvent_RegisterTask()`](../../Libs/sith/AI/sithAIAwareness.c#L64)

### Queued Events

One-shot events are created through [`sithEvent_CreateEvent()`](../../Libs/sith/Gameplay/sithEvent.c#L91).

That function:

1. allocates a slot through [`sithEvent_Create()`](../../Libs/sith/Gameplay/sithEvent.c#L180)
2. converts the caller's relative `when` into an absolute `msecEventTime` using current game time
3. copies the parameter block
4. inserts the event into the sorted linked list through [`sithEvent_AddEvent()`](../../Libs/sith/Gameplay/sithEvent.c#L192)

Because insertion is sorted, [`sithEvent_Process()`](../../Libs/sith/Gameplay/sithEvent.c#L135) only needs to pop from the list head while the first event is due.

### Process Algorithm

[`sithEvent_Process()`](../../Libs/sith/Gameplay/sithEvent.c#L135) runs in two phases:

1. interval tasks whose frequency has elapsed are called with the elapsed time
2. queued events whose fire time is earlier than current game time are dispatched and returned to the free list through [`sithEvent_FreeEvent()`](../../Libs/sith/Gameplay/sithEvent.c#L108)

The scheduler is deliberately tiny, but it is architecturally important because it gives the engine a unified deferred-work mechanism without requiring every subsystem to maintain its own timer queue.

## COG Message Dispatch

The COG system is the engine's gameplay message layer. It is how most scripted reactions happen.

### Direct Delivery To A Cog

The direct entry points are:

- [`sithCog_SendMessage()`](../../Libs/sith/Cog/sithCog.c#L384)
- [`sithCog_SendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L477)

Their algorithm is:

1. reject null, disabled, paused, or handler-less cases
2. find the message handler inside the cog script's handler table
3. if this is a multiplayer client and the cog is not local, replicate the message over DSS instead of executing immediately
4. otherwise write sender/source/link/param context into the cog instance
5. execute the message entry point through [`sithCogExec_ExecuteMessage()`](../../Libs/sith/Cog/sithCogExec.c#L148)

The `Ex` form differs mainly by carrying four integer parameters and returning a script-visible return value.

### VM Entry And Resume

[`sithCogExec_ExecuteMessage()`](../../Libs/sith/Cog/sithCogExec.c#L148) prepares the cog for execution:

- reset or preserve stack state depending on whether the cog was already active
- push suspended execution onto the call stack if necessary
- move `execPos` to the chosen handler entry
- run the bytecode loop through [`sithCogExec_Execute()`](../../Libs/sith/Cog/sithCogExec.c#L70)

This is why COG messages are cheap to deliver. The engine is not parsing source or looking up symbol names in the hot path. It is dispatching directly into a bytecode program.

### Message Fan-Out By World Object Type

The higher-level helpers are:

- [`sithCog_ThingSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1425)
- [`sithCog_SectorSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1523)
- [`sithCog_SurfaceSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1577)
- [`sithCog_BroadcastMessageEx()`](../../Libs/sith/Cog/sithCog.c#L363)

These functions do not broadcast blindly. They:

- derive a source-type mask from the originating thing, or from the synthetic "system/no source thing" case
- send first to the primary cog owned by the object where relevant
- send to special attached cogs such as capture cogs
- fan out to linked cogs whose link masks accept the source thing type

That last step is what makes COG links useful. The same sector or surface can have multiple linked cogs, each filtering which source thing types should trigger it.

### Global Broadcast Path

[`sithCog_BroadcastMessage()`](../../Libs/sith/Cog/sithCog.c#L358) and [`sithCog_BroadcastMessageEx()`](../../Libs/sith/Cog/sithCog.c#L363) are the true broadcast helpers.

They differ from the thing/sector/surface helpers in an important way:

- they do not target one world object
- they do not use link masks
- they do not walk thing, sector, or surface link tables
- they simply iterate every cog instance in the static world and then every cog instance in the current world

For each cog they call [`sithCog_SendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L477) directly with:

- the same message type
- the same sender/source references
- `linkId = 0`
- the same four integer parameters in the `Ex` form

This is the path used for truly global script events such as startup/shutdown-style notifications where the engine wants every cog instance to get a chance to react, regardless of object ownership or link masks.

### Damaged Message Semantics

`DAMAGED` is handled specially inside [`sithCog_ThingSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1425), [`sithCog_SectorSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1523), and [`sithCog_SurfaceSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1577).

Instead of simply summing return values, the return value is fed back into `param0` and becomes the running damage value seen by the next recipient. That is how chains of linked cogs can modify damage as it propagates.

## Timers, Pulses, And Suspended Cogs

Even after direct message delivery, some cogs remain alive in suspended states.

That work is resumed by:

- [`sithCog_ProcessCog()`](../../Libs/sith/Cog/sithCog.c#L1765)
- [`sithCog_ProcessCogs()`](../../Libs/sith/Cog/sithCog.c#L2024)

[`sithCog_ProcessCog()`](../../Libs/sith/Cog/sithCog.c#L1765) is the runtime bridge between deferred state and active execution. It:

- fires `PULSE` messages for pulse-enabled cogs
- fires direct `TIMER` messages for cogs using the simple per-cog timer flags
- wakes sleeping or waiting cogs when their wait condition is satisfied

The supported wait conditions include:

- sleeping until a target game-time
- waiting for a moving thing to stop
- waiting for a keyframe or animation to finish
- waiting for a sound to stop
- waiting for AI motion or head tracking to stop
- waiting for player or weapon-selection state changes

So the COG runtime has two separate deferred-execution models:

- `sithEvent` for generic scheduled tasks and timer verbs
- per-cog suspended statuses resumed during [`sithCog_ProcessCogs()`](../../Libs/sith/Cog/sithCog.c#L2024)

### On-Demand Timer Events

The explicit timer-task bridge is [`sithCog_TimerEventTask()`](../../Libs/sith/Cog/sithCog.c#L2114).

When a timer-related verb schedules a delayed event through [`sithEvent_CreateEvent()`](../../Libs/sith/Gameplay/sithEvent.c#L91), the scheduler later invokes [`sithCog_TimerEventTask()`](../../Libs/sith/Cog/sithCog.c#L2114), which converts the deferred event back into a `TIMER` message on the target cog.

This is the cleanest example of the scheduler and COG message systems joining together.

## Multiplayer And File Message Bus

The third dispatch path is [`sithMessage`](../../Libs/sith/Devices/sithComm.c#L59). This is not the local gameplay event queue. It is the serialized transport used for:

- multiplayer replication
- saved-game/file stream messages
- some engine synchronization traffic

### Handler Table

[`sithMessage_Startup()`](../../Libs/sith/Devices/sithComm.c#L59) initializes a handler table of `65` message types and fills it through [`sithMessage_InitMessageHandlers()`](../../Libs/sith/Devices/sithComm.c#L433).

Each message type is a small vtable entry registered with [`sithMessage_RegisterFunction()`](../../Libs/sith/Devices/sithComm.c#L407).

This gives the transport layer the same architectural shape as several other Jones3D systems:

- one compact numeric type id
- one central dispatch table
- one function pointer per message kind

### Sending

[`sithMessage_SendMessage()`](../../Libs/sith/Devices/sithComm.c#L94) chooses the output stream:

- network
- file
- or none

It stamps the outgoing message with current game time and then forwards to:

- [`sithMessage_NetWrite()`](../../Libs/sith/Devices/sithComm.c#L167)
- or the file-stream path

`sithMessage` therefore does not know gameplay semantics. It knows transport, stream selection, and handler dispatch.

### Receiving And Dispatch

[`sithMessage_ProcessMessages()`](../../Libs/sith/Devices/sithComm.c#L120) repeatedly calls [`sithMessage_ReceiveMessage()`](../../Libs/sith/Devices/sithComm.c#L200), validates sender state, and then dispatches to [`sithMessage_Process()`](../../Libs/sith/Devices/sithComm.c#L413).

That means the transport layer enforces some membership and sender sanity before the subsystem-specific message handlers ever see the payload.

### Replicated COG Messages

COG replication goes through [`sithDSSCog_SendMessage()`](../../Libs/sith/Dss/sithDSSCog.c#L22) and [`sithDSSCog_ProcessMessage()`](../../Libs/sith/Dss/sithDSSCog.c#L97).

This path serializes:

- cog index
- sender and source types
- sender and source identifiers
- message type
- four integer parameters

For thing references it sends GUIDs rather than transient indices, then resolves them back to live indices on the receiving side. That is crucial because thing indices are only stable within a local world instance.

## How The Systems Fit Together

The simplest way to think about the overall event architecture is:

- `sithEvent` schedules deferred local work
- `sithCog` turns gameplay occurrences into script execution
- `sithMessage` transports serialized state and replicated events across network/file boundaries

These systems meet at several important seams:

- timer verbs schedule [`sithEvent_CreateEvent()`](../../Libs/sith/Gameplay/sithEvent.c#L91), which later re-enters COG through [`sithCog_TimerEventTask()`](../../Libs/sith/Cog/sithCog.c#L2114)
- gameplay systems send direct object messages through [`sithCog_ThingSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1425), [`sithCog_SectorSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1523), or [`sithCog_SurfaceSendMessageEx()`](../../Libs/sith/Cog/sithCog.c#L1577)
- multiplayer clients forward non-local COG messages through [`sithDSSCog_SendMessage()`](../../Libs/sith/Dss/sithDSSCog.c#L22)
- interval event tasks drive subsystems such as AI awareness through [`sithAIAwareness_Update()`](../../Libs/sith/AI/sithAIAwareness.c#L107)

So while there is no single grand unified event bus, the engine does have a coherent pattern:

- small numeric ids
- fixed tables
- explicit dispatch helpers
- deferred work resumed in the frame loop
