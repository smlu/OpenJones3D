# 6. Runtime view

## 6.1 One frame

```mermaid
sequenceDiagram
    participant H as Host loop (wkernel adapter)
    participant J as Jones3D
    participant S as sith
    participant A as Adapter (std, sound)
    participant B as Babylon Engine
    H->>B: poll events
    B-->>H: window and input events
    H->>J: process frame
    J->>S: update world (fixed-step physics, AI, COG, events)
    S->>A: stdControl_* reads input
    A->>B: input state
    S->>A: Sound_* plays and positions sounds
    A->>B: audio voices
    J->>S: render
    S->>A: std3D_* draws through rdroid
    A->>B: render submission
    H->>B: present
```

The simulation step reads only input and game time. Rendering and audio only read simulation state. That
split is what makes headless traces possible: replace the window, input, audio and render backends with
Null and Script backends, and the simulation produces the same state.

## 6.2 Scenarios

| Page | Covers |
|---|---|
| [Boot-And-Lifecycle.md](Boot-And-Lifecycle.md) | Process startup, subsystem startup, opening a world, shutdown |
| [Frame-Loop-And-Simulation.md](Frame-Loop-And-Simulation.md) | Update order, timing, events, things, physics, collision within a frame |
| [Event-And-Message-Systems.md](Event-And-Message-Systems.md) | Deferred events, COG message dispatch, the multiplayer and file synchronization bus |

## 6.3 A headless trace run

```mermaid
sequenceDiagram
    participant R as Headless runner
    participant G as Game
    participant N as Null and Script backends
    R->>G: load savegame
    loop for each frame up to --frames
        R->>N: feed scripted input for this frame
        R->>G: process frame with fixed game time
        G->>N: input, sound and draw calls (discarded)
        R->>R: append simulation state to the trace
    end
    R->>R: write the state file
```

Today: `Jones3D-headless --load <save> --frames <n> --state <file>`, built with the Null platform. Two runs
with the same inputs give identical state files on 32-bit, 64-bit and sanitizer builds.
