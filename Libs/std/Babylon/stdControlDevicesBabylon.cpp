// Added: the Babylon column's device side of std/Null/stdControlNull.h. The engine's keyboard and
// mouse buttons come from the Babylon platform event queue through the one shared
// babylon::input::InputState of stdBabylonEvents.cpp, read once per stdControl_ReadControls as a
// snapshot of every key's state, the way stdControlDX9.c polls DirectInput; the headless runner's
// scripted keys go into that same queue. Mouse axes and gamepads are not read yet.
// Adapter code: no Fixed/Added markers inside.
#include <babylon/input/InputState.h>
#include <babylon/platform/Event.h>
#include <babylon/platform/Keys.h>
#include <babylon/platform/NullPlatform.h>
#include <babylon/platform/Platform.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <std/Babylon/stdBabylonEvents.h>
#include <std/Babylon/stdControlDevicesBabylon.h>
#include <std/Null/stdControlNull.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdKeys.h>

#include <std/General/stdUtil.h>

#include "stdBabylon.h"

namespace
{
    // Every key the engine and babylon::platform::Key both name, in engine key id order. Keys only
    // one of them has stay out: the engine has no PrintScreen, ScrollLock or Pause and no keypad
    // comma or keypad equals, and Babylon has no F13 to F15. Mouse buttons stay out too, they have
    // their own engine ids and their own table below.
    constexpr StdBabylonKeyMapping aKeyMappings[] =
    {
        { STDKEY_ESCAPE,     babylon::platform::Key::Escape          },
        { STDKEY_1,          babylon::platform::Key::Num1            },
        { STDKEY_2,          babylon::platform::Key::Num2            },
        { STDKEY_3,          babylon::platform::Key::Num3            },
        { STDKEY_4,          babylon::platform::Key::Num4            },
        { STDKEY_5,          babylon::platform::Key::Num5            },
        { STDKEY_6,          babylon::platform::Key::Num6            },
        { STDKEY_7,          babylon::platform::Key::Num7            },
        { STDKEY_8,          babylon::platform::Key::Num8            },
        { STDKEY_9,          babylon::platform::Key::Num9            },
        { STDKEY_0,          babylon::platform::Key::Num0            },
        { STDKEY_MINUS,      babylon::platform::Key::Minus           },
        { STDKEY_EQUALS,     babylon::platform::Key::Equals          },
        { STDKEY_BACK,       babylon::platform::Key::Backspace       },
        { STDKEY_TAB,        babylon::platform::Key::Tab             },
        { STDKEY_Q,          babylon::platform::Key::Q               },
        { STDKEY_W,          babylon::platform::Key::W               },
        { STDKEY_E,          babylon::platform::Key::E               },
        { STDKEY_R,          babylon::platform::Key::R               },
        { STDKEY_T,          babylon::platform::Key::T               },
        { STDKEY_Y,          babylon::platform::Key::Y               },
        { STDKEY_U,          babylon::platform::Key::U               },
        { STDKEY_I,          babylon::platform::Key::I               },
        { STDKEY_O,          babylon::platform::Key::O               },
        { STDKEY_P,          babylon::platform::Key::P               },
        { STDKEY_LBRACKET,   babylon::platform::Key::LeftBracket     },
        { STDKEY_RBRACKET,   babylon::platform::Key::RightBracket    },
        { STDKEY_RETURN,     babylon::platform::Key::Enter           },
        { STDKEY_LCONTROL,   babylon::platform::Key::LeftCtrl        },
        { STDKEY_A,          babylon::platform::Key::A               },
        { STDKEY_S,          babylon::platform::Key::S               },
        { STDKEY_D,          babylon::platform::Key::D               },
        { STDKEY_F,          babylon::platform::Key::F               },
        { STDKEY_G,          babylon::platform::Key::G               },
        { STDKEY_H,          babylon::platform::Key::H               },
        { STDKEY_J,          babylon::platform::Key::J               },
        { STDKEY_K,          babylon::platform::Key::K               },
        { STDKEY_L,          babylon::platform::Key::L               },
        { STDKEY_SEMICOLON,  babylon::platform::Key::Semicolon       },
        { STDKEY_APOSTROPHE, babylon::platform::Key::Apostrophe      },
        { STDKEY_GRAVE,      babylon::platform::Key::Grave           },
        { STDKEY_LSHIFT,     babylon::platform::Key::LeftShift       },
        { STDKEY_BACKSLASH,  babylon::platform::Key::Backslash       },
        { STDKEY_Z,          babylon::platform::Key::Z               },
        { STDKEY_X,          babylon::platform::Key::X               },
        { STDKEY_C,          babylon::platform::Key::C               },
        { STDKEY_V,          babylon::platform::Key::V               },
        { STDKEY_B,          babylon::platform::Key::B               },
        { STDKEY_N,          babylon::platform::Key::N               },
        { STDKEY_M,          babylon::platform::Key::M               },
        { STDKEY_COMMA,      babylon::platform::Key::Comma           },
        { STDKEY_PERIOD,     babylon::platform::Key::Period          },
        { STDKEY_SLASH,      babylon::platform::Key::Slash           },
        { STDKEY_RSHIFT,     babylon::platform::Key::RightShift      },
        { STDKEY_MULTIPLY,   babylon::platform::Key::KeypadMultiply  },
        { STDKEY_LMENU,      babylon::platform::Key::LeftAlt         },
        { STDKEY_SPACE,      babylon::platform::Key::Space           },
        { STDKEY_CAPITAL,    babylon::platform::Key::CapsLock        },
        { STDKEY_F1,         babylon::platform::Key::F1              },
        { STDKEY_F2,         babylon::platform::Key::F2              },
        { STDKEY_F3,         babylon::platform::Key::F3              },
        { STDKEY_F4,         babylon::platform::Key::F4              },
        { STDKEY_F5,         babylon::platform::Key::F5              },
        { STDKEY_F6,         babylon::platform::Key::F6              },
        { STDKEY_F7,         babylon::platform::Key::F7              },
        { STDKEY_F8,         babylon::platform::Key::F8              },
        { STDKEY_F9,         babylon::platform::Key::F9              },
        { STDKEY_F10,        babylon::platform::Key::F10             },
        { STDKEY_NUMLOCK,    babylon::platform::Key::NumLock         },
        { STDKEY_NUMPAD7,    babylon::platform::Key::Keypad7         },
        { STDKEY_NUMPAD8,    babylon::platform::Key::Keypad8         },
        { STDKEY_NUMPAD9,    babylon::platform::Key::Keypad9         },
        { STDKEY_SUBTRACT,   babylon::platform::Key::KeypadMinus     },
        { STDKEY_NUMPAD4,    babylon::platform::Key::Keypad4         },
        { STDKEY_NUMPAD5,    babylon::platform::Key::Keypad5         },
        { STDKEY_NUMPAD6,    babylon::platform::Key::Keypad6         },
        { STDKEY_ADD,        babylon::platform::Key::KeypadPlus      },
        { STDKEY_NUMPAD1,    babylon::platform::Key::Keypad1         },
        { STDKEY_NUMPAD2,    babylon::platform::Key::Keypad2         },
        { STDKEY_NUMPAD3,    babylon::platform::Key::Keypad3         },
        { STDKEY_NUMPAD0,    babylon::platform::Key::Keypad0         },
        { STDKEY_DECIMAL,    babylon::platform::Key::KeypadPeriod    },
        { STDKEY_F11,        babylon::platform::Key::F11             },
        { STDKEY_F12,        babylon::platform::Key::F12             },
        { STDKEY_NUMPADENTER, babylon::platform::Key::KeypadEnter    },
        { STDKEY_RCONTROL,   babylon::platform::Key::RightCtrl       },
        { STDKEY_DIVIDE,     babylon::platform::Key::KeypadDivide    },
        { STDKEY_RMENU,      babylon::platform::Key::RightAlt        },
        { STDKEY_HOME,       babylon::platform::Key::Home            },
        { STDKEY_UP,         babylon::platform::Key::Up              },
        { STDKEY_PRIOR,      babylon::platform::Key::PageUp          },
        { STDKEY_LEFT,       babylon::platform::Key::Left            },
        { STDKEY_RIGHT,      babylon::platform::Key::Right           },
        { STDKEY_END,        babylon::platform::Key::End             },
        { STDKEY_DOWN,       babylon::platform::Key::Down            },
        { STDKEY_NEXT,       babylon::platform::Key::PageDown        },
        { STDKEY_INSERT,     babylon::platform::Key::Insert          },
        { STDKEY_DELETE,     babylon::platform::Key::Delete          },
        { STDKEY_LWIN,       babylon::platform::Key::LeftMeta        },
        { STDKEY_RWIN,       babylon::platform::Key::RightMeta       },
    };

    // The Babylon mouse button of each engine mouse-button id, in the order of the engine ids
    // STDCONTROL_KID_MOUSE_LBUTTON, _RBUTTON, _MBUTTON and _XBUTTON. Babylon's X2 has no engine id.
    constexpr babylon::platform::MouseButton aMouseButtons[] =
    {
        babylon::platform::MouseButton::Left,
        babylon::platform::MouseButton::Right,
        babylon::platform::MouseButton::Middle,
        babylon::platform::MouseButton::X1,
    };

    // True when no engine key id and no Babylon key appears twice in aKeyMappings, so every entry
    // pairs one engine id with one key and both directions of the lookup are unambiguous. Keys
    // either side lacks stay out of the table, so they cannot collide.
    constexpr bool stdControlDevicesBabylon_MappingsAreOneToOne()
    {
        for ( size_t first = 0; first < STD_ARRAYLEN(aKeyMappings); ++first )
        {
            for ( size_t second = first + 1; second < STD_ARRAYLEN(aKeyMappings); ++second )
            {
                if ( aKeyMappings[first].engineKeyId == aKeyMappings[second].engineKeyId
                    || aKeyMappings[first].key == aKeyMappings[second].key )
                {
                    return false;
                }
            }
        }

        return true;
    }

    static_assert(stdControlDevicesBabylon_MappingsAreOneToOne(),
        "the key mapping must give every engine key id one Babylon key and every Babylon key one engine key id");
    static_assert(STD_ARRAYLEN(aMouseButtons) == STDCONTROL_MAX_MOUSE_BUTTONS,
        "the mouse button mapping must give every engine mouse button one Babylon mouse button");

    // The Babylon mouse button of an engine key id, or nullopt when the id is not one of the four
    // engine mouse buttons.
    std::optional<babylon::platform::MouseButton> stdControlDevicesBabylon_MouseButtonFromEngineId(int engineKeyId)
    {
        if ( engineKeyId < 0 || !STDCONTROL_ISMOUSEBUTTON((size_t)engineKeyId) )
        {
            return std::nullopt;
        }

        return aMouseButtons[(size_t)engineKeyId - STDCONTROL_KID_MOUSE_LBUTTON];
    }
}

std::span<const StdBabylonKeyMapping> stdControlDevicesBabylon_KeyMappings()
{
    return std::span<const StdBabylonKeyMapping>(aKeyMappings);
}

babylon::platform::Key stdControlDevicesBabylon_KeyFromEngineId(int engineKeyId)
{
    for ( const StdBabylonKeyMapping& rMapping : aKeyMappings )
    {
        if ( rMapping.engineKeyId == engineKeyId )
        {
            return rMapping.key;
        }
    }

    return babylon::platform::Key::Unknown;
}

int stdControlDevicesBabylon_EngineIdFromKey(babylon::platform::Key key)
{
    for ( const StdBabylonKeyMapping& rMapping : aKeyMappings )
    {
        if ( rMapping.key == key )
        {
            return rMapping.engineKeyId;
        }
    }

    return -1;
}

#ifdef J3D_TEST
void stdControlDevicesBabylon_ResetForTest()
{
    stdBabylonEvents_ResetForTest();
}
#endif

extern "C" {

void stdControl_ReadDevices(uint32_t readTime, bool bReadMouse)
{
    // The frame's events: everything the platform has queued, applied in the order it made them,
    // through the column's one pump. The pump is safe to call any number of times per frame, so
    // the window adapter may have drained the queue already; the held-key state then still says
    // what this read reports.
    stdBabylonEvents_Pump();

    babylon::input::InputState& rInputState = stdBabylonEvents_InputState();

    // The keyboard snapshot, stdControlDX9.c's stdControl_ApplyKeyboardState over the device state:
    // every mapped key is reported as it is now, so a key that went down and came up between two
    // reads never shows as down, exactly like the polled device.
    for ( const StdBabylonKeyMapping& rMapping : aKeyMappings )
    {
        stdControl_UpdateKeyState(rMapping.engineKeyId, rInputState.keyDown(rMapping.key) ? 1 : 0, readTime);
    }

    // The mouse button snapshot, stdControlDX9.c's stdControl_ApplyMouseButtonState, only for the
    // four buttons the engine has ids for.
    if ( bReadMouse )
    {
        for ( size_t i = 0; i < STD_ARRAYLEN(aMouseButtons); ++i )
        {
            const int keyId = (int)(STDCONTROL_KID_MOUSE_LBUTTON + i);
            stdControl_UpdateKeyState(keyId, rInputState.mouseButtonDown(aMouseButtons[i]) ? 1 : 0, readTime);
        }
    }

    // The snapshot is taken: clear the per-frame flags, so the next pump records the next frame's
    // transitions, the way beginFrame before the apply of the frame's events did.
    rInputState.beginFrame();

    // TODO: the mouse axes (stdControl_ReadAxis on STDCONTROL_AID_MOUSE_X, _Y and _Z) and the
    // gamepad buttons and axes are not read yet; they come with the playable build.
}

bool stdControl_QueueKeyEvent(int keyId, int bPressed)
{
    StdBabylonContext& rContext = stdBabylon_RequireContext("stdControl_QueueKeyEvent");
    if ( !rContext.pScriptPlatform )
    {
        return false;
    }

    // Scripted events carry no window: 0 is the platform's "no window".
    const babylon::platform::Key key = stdControlDevicesBabylon_KeyFromEngineId(keyId);
    if ( key != babylon::platform::Key::Unknown )
    {
        rContext.pScriptPlatform->pushEvent(
            babylon::platform::KeyChanged{ 0, key, bPressed != 0, false });
        return true;
    }

    const std::optional<babylon::platform::MouseButton> button =
        stdControlDevicesBabylon_MouseButtonFromEngineId(keyId);
    if ( button )
    {
        rContext.pScriptPlatform->pushEvent(
            babylon::platform::MouseButtonChanged{ 0, *button, bPressed != 0 });
        return true;
    }

    return false;
}

} // extern "C"
