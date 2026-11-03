# Babylon design rules

Naming, interface and pattern rules for code in `Babylon/`. Game and adapter code follow
[CONTRIBUTING.md](../../../CONTRIBUTING.md) instead. When `Babylon/` exists, its style guide refers here.

## Principles

The rules below follow from SOLID and information hiding:

| Principle | Where it shows up |
|---|---|
| Single responsibility | Modules split by role ([ADR-0015](../09-decisions/ADR-0015-babylon-modules-as-cmake-targets.md)); one class per role, such as `Window` and `Clock`, not one large `Platform` object |
| Open/closed | A new backend is a new class; callers and the interface don't change |
| Liskov substitution | Every backend, Null included, honors the full contract. Interface tests run against every backend, so a violation fails CI |
| Interface segregation | Small interfaces per role. Watch render, which tends to grow into one huge interface |
| Dependency inversion | Callers depend on the interface; the composition root injects the backend |

### Information hiding

Each module hides one design decision that is likely to change, so changing it touches only that module:

| Module | Decision it hides | Where a change lands |
|---|---|---|
| Platform | SDL3 as the OS layer, window and event details | `backends/sdl/` |
| Render | SDL3 GPU, the shader format, how draws are batched | The render backend |
| Audio | miniaudio, sample formats, the mixer | The audio backend |
| Files | Path separators, case sensitivity, where user directories live | The files backend |
| Time | Clock source, frame pacing | The time backend |

One level up, the adapter hides Babylon from the game
([ADR-0007](../09-decisions/ADR-0007-adapter-between-game-and-babylon.md)): C game code calls `stdFile_Open`
and never learns that Babylon or SDL3 exists.

What a caller can observe is not hidden: call order, return values, timing and limits are part of the
contract. A backend may change how it works, never what the caller sees.

In the code:

1. **Modules:** only `include/babylon/<module>/` is public; `src/` and `backends/` are private, enforced by
   CMake.
2. **Backends:** callers see `Window`, never `SdlWindow`. Backend headers stay inside `backends/`.
3. **Resources:** typed handles instead of pointers. No caller may depend on a backend's buffer layout,
   coordinate convention, error codes or batching.
4. **Headers:** interfaces have no data members, and public headers include no SDL3 or miniaudio headers.
   CI checks this like the include boundary.
5. **No escape hatches:** `src/` helpers sit in an anonymous namespace, no `friend` across modules, and no
   getter hands out a backend object such as `SDL_Window*`.

Limit: no abstraction without a second user. Open/closed and dependency inversion apply where a second
backend really exists, such as Null, not to every helper class.

## Naming

| Thing | Rule | Example |
|---|---|---|
| Namespace | `babylon::<module>` | `babylon::input`, `babylon::files` |
| Interface | Abstract class named by role, no `I` prefix | `Window`, `Clock`, `FileSystem`, `InputDevice` |
| Backend | Backend name, then the role | `SdlWindow`, `NullWindow`, `ScriptInputDevice` |
| Types | `PascalCase` | `DisplayMode`, `KeyEvent` |
| Functions | `camelCase`, verb first | `open()`, `pollEvents()`, `setTitle()` |
| Enums | `enum class`, `PascalCase` values | `KeyCode::Escape` |
| Members | `m_` prefix | `m_width` |
| Constants | `k` prefix | `kMaxGamepads` |
| Macros | `BABYLON_` prefix | `BABYLON_ASSERT` |
| Files | One class per file, named after the class | `include/babylon/platform/Window.h`, `backends/sdl/SdlWindow.cpp` |

Names describe the role on any platform, never a Jones3D or Sith concept
([ADR-0006](../09-decisions/ADR-0006-babylon-engine-in-repo.md)).

## Lifecycle and errors

- `startup` and `shutdown` for a whole module, `open` and `close` for one resource. Closing something that
  isn't open is a safe no-op.
- Functions that can fail return `[[nodiscard]] bool` and log the failure. No exceptions cross an interface.

## Interfaces

Interfaces are pure abstract classes, and backends are chosen once at startup
([ADR-0016](../09-decisions/ADR-0016-virtual-interfaces-chosen-at-startup.md)).

```cpp
namespace babylon::platform {

// Window: one OS window. Created by a Platform backend, not directly.
class Window
{
public:
    virtual ~Window() = default;

    // Validates once for every backend, then hands over.
    [[nodiscard]] bool resize(int width, int height)
    {
        if ( width <= 0 || height <= 0 ) { log::error(...); return false; }
        return doResize(width, height);
    }

    virtual void setTitle(std::string_view title) = 0;
    virtual Size size() const = 0;

private:
    virtual bool doResize(int width, int height) = 0;
};

}
```

- The non-virtual interface pattern (as in `resize` above) only for contract checks every backend shares.
- Backends inherit one level. Shared logic goes into free functions or a member helper.

## Patterns

Use:

1. **One interface, several backends:** SDL3, Null, sometimes Script.
2. **One composition root, no singletons:** `main` builds the backends and passes them on, including cvars,
   clock, files and audio. A singleton's global access hides dependencies, leaks state between tests and
   blocks replacing an object at runtime. Two exceptions:
   - **Log** is a global free-function API (`babylon::log::error(...)`). The composition root attaches the
     sinks at startup, and tests can replace them.
   - The **C adapter edge** holds the pointers the recovered `std*` functions use, because those functions
     have no parameter to pass a backend through.
3. **Polled event queues:** platform and input push events into a queue the game drains once per frame, so
   event order is deterministic and replays are exact.
4. **Typed handles** (index plus generation) for render and audio resources. A stale handle is caught, and no
   caller sees backend objects.
5. **RAII ownership:** factories return `std::unique_ptr`; resources are freed by their owner.
6. **Time as a dependency:** all time comes from a `Clock` interface; the Null clock steps a fixed amount per
   frame ([8.1](README.md#81-fidelity-and-traces)).
7. **Record, then submit** for rendering: a frame's draws go into a command list, submitted once (M6).

Avoid:

- Singletons and static self-registration ([ADR-0015](../09-decisions/ADR-0015-babylon-modules-as-cmake-targets.md))
- Inheritance deeper than interface to backend
- Observers beyond cvar change callbacks
- ECS or a job system in the foundation (threads come in M9)
- Shared ownership through `shared_ptr`
- Heavy template metaprogramming in public headers
