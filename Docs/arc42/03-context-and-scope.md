# 3. Context and scope

## 3.1 Business context

```mermaid
flowchart LR
    player([Player])
    modder([Modder])
    data[(Original game data<br/>GOB archives, CD data)]
    saves[(Savegames<br/>.nds)]
    cfg[(Jones.cfg)]
    mods[(Mod folders<br/>loose files)]
    ex[Jones3D EX]

    player -- input --> ex
    ex -- picture, sound --> player
    data --> ex
    mods --> ex
    modder -- writes --> mods
    ex <--> saves
    ex <--> cfg
```

| Partner | Input | Output |
|---|---|---|
| Player | Keyboard, mouse, gamepad | Picture, sound, savegames |
| Original game data | Levels, models, materials, sounds, COG scripts from the player's copy of the game | |
| Savegames | Original-format savegames, also from the original game | Savegames in the same format |
| `Jones.cfg` | Settings | Changed settings |
| Mod folders | Files that override or add game data | |

Out of scope: online services, store integrations (Steam, GOG), achievements. Multiplayer code keeps
compiling but has no supported network backend.

## 3.2 Technical context

```mermaid
flowchart TB
    subgraph ex [Jones3D EX process]
        game[Game: Jones3D, sith, rdroid<br/>C11, AGPL]
        adapter[Adapter: std, wkernel, sound<br/>AGPL]
        babylon[Babylon Engine<br/>C++, MPL-2.0]
        game --> adapter --> babylon
    end
    babylon --> sdl[SDL3<br/>window, input, timer, files]
    babylon --> gpu[SDL3 GPU API<br/>Vulkan, Metal, D3D12]
    babylon --> ma[miniaudio<br/>audio devices]
    sdl & gpu & ma --> os[Operating system and drivers]
```

| Channel | Technology | Owned by |
|---|---|---|
| Window, input, timers, file system | SDL3 | Babylon backend |
| Graphics | SDL3 GPU API, which maps to Vulkan, Metal or Direct3D 12 | Babylon backend |
| Audio output | miniaudio | Babylon backend |
| Game files | Original GOB archives and loose files, read through the game's resource code | Game |
| Headless runs | No window, no device: null backends, scripted input, state trace output | Babylon null backends plus the game's headless runner |

Until the new renderer reaches parity (milestone M6), the legacy Win32 and DirectX 9 path still exists on
Windows, outside Babylon. It is the visual reference for the new renderer, and is removed after that.
