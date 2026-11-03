// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/SdlPlatform.h>

#include <babylon/log/Log.h>
#include <babylon/platform/Event.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/Window.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace babylon::platform
{
namespace
{

    // SDL's video subsystem belongs to the whole process, so this backend
    // runs one platform at a time: the factory sets the flag, the destructor
    // clears it, and a second platform while one lives is refused.
    bool videoInUse = false;

    // The SDL id of the window an SDL window event refers to, which is the
    // id that window reports through Window::id().
    WindowId windowId(const SDL_Event& sdlEvent)
    {
        return static_cast<WindowId>(sdlEvent.window.windowID);
    }

    // One window SDL created. SDL_Window* is owned here and destroyed with
    // this object, which must happen before the SdlPlatform quits the video
    // subsystem.
    class SdlWindow final : public Window
    {
    public:
        SdlWindow(SDL_Window* window, WindowId id) : m_window(window), m_id(id)
        {
        }

        ~SdlWindow() override
        {
            SDL_DestroyWindow(m_window);
        }

        void setTitle(std::string_view title) override
        {
            // SDL wants a NUL-terminated string; the view is copied rather
            // than read past its end.
            const std::string text(title);
            if ( !SDL_SetWindowTitle(m_window, text.c_str()) )
            {
                log::warning("platform: setting the window title failed: {}", SDL_GetError());
            }
        }

        std::string title() const override
        {
            const char* const text = SDL_GetWindowTitle(m_window);
            if ( text == nullptr )
            {
                log::warning("platform: reading the window title failed: {}", SDL_GetError());
                return {};
            }
            return text;
        }

        Size size() const override
        {
            // SDL counts a destroyed window as no window at all, so the only
            // way this fails is a handle the video subsystem already let go;
            // there is then no size left to report.
            int width = 0;
            int height = 0;
            if ( !SDL_GetWindowSizeInPixels(m_window, &width, &height) )
            {
                log::warning("platform: reading the window size failed: {}", SDL_GetError());
            }
            return Size{width, height};
        }

        WindowId id() const override
        {
            return m_id;
        }

    private:
        // SDL applies a requested size asynchronously, so this waits for the
        // window system with SDL_SyncWindow: size() reports the new size as
        // soon as resize() returned true.
        bool doResize(Size size) override
        {
            if ( !SDL_SetWindowSize(m_window, size.width, size.height) )
            {
                log::warning("platform: resizing the window to {}x{} failed: {}",
                    size.width, size.height, SDL_GetError());
                return false;
            }
            // A failed sync means the window system never confirmed the new
            // size; the caller must not believe it was applied.
            if ( !SDL_SyncWindow(m_window) )
            {
                log::warning("platform: waiting for the resize to {}x{} failed: {}",
                    size.width, size.height, SDL_GetError());
                return false;
            }
            return true;
        }

        SDL_Window* m_window;
        WindowId m_id;
    };

    class SdlPlatform final : public Platform
    {
    public:
        ~SdlPlatform() override
        {
            // Windows come first: Platform.h says every window is destroyed
            // before its platform, and SDL frees window resources as part of
            // quitting the subsystem.
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            videoInUse = false;
        }

        std::unique_ptr<Window> createWindow(const WindowDesc& desc) override
        {
            if ( !isValidWindowSize(desc.size) )
            {
                // Checked here rather than left to SDL, so the same sizes are
                // refused in every backend with the same one warning as
                // resize().
                log::warning("platform: rejected window size {}x{}: both dimensions "
                    "must be within 1 and {}", desc.size.width, desc.size.height,
                    kMaxWindowDimension);
                return nullptr;
            }
            const SDL_WindowFlags flags = desc.resizable ? SDL_WINDOW_RESIZABLE : 0;
            SDL_Window* const window =
                SDL_CreateWindow(desc.title.c_str(), desc.size.width, desc.size.height, flags);
            if ( window == nullptr )
            {
                log::warning("platform: creating a {}x{} window failed: {}",
                    desc.size.width, desc.size.height, SDL_GetError());
                return nullptr;
            }
            // SDL's window ids are nonzero for every window it knows, and
            // 0 would collide with the "no window" id of the interface.
            const WindowId id = static_cast<WindowId>(SDL_GetWindowID(window));
            if ( id == 0 )
            {
                log::warning("platform: asking the new window for its id failed: {}",
                    SDL_GetError());
                SDL_DestroyWindow(window);
                return nullptr;
            }
            return std::make_unique<SdlWindow>(window, id);
        }

        std::optional<Event> pollEvent() override
        {
            SDL_Event sdlEvent;
            for ( ; ; )
            {
                if ( !SDL_PollEvent(&sdlEvent) )
                {
                    return std::nullopt;   // nothing of ours left in the queue
                }
                switch ( sdlEvent.type )
                {
                    case SDL_EVENT_QUIT:
                        return QuitRequested{};
                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                        return WindowCloseRequested{windowId(sdlEvent)};
                    case SDL_EVENT_WINDOW_FOCUS_GAINED:
                        return WindowFocusChanged{windowId(sdlEvent), true};
                    case SDL_EVENT_WINDOW_FOCUS_LOST:
                        return WindowFocusChanged{windowId(sdlEvent), false};
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                        return WindowResized{windowId(sdlEvent),
                            Size{sdlEvent.window.data1, sdlEvent.window.data2}};
                    default:
                        // Every other SDL event (mouse motion, key repeat,
                        // display changes, the SDL3 equivalents of the ones
                        // above) is not part of this queue yet; drop it and
                        // keep looking.
                        break;
                }
            }
        }
    };

}

    std::unique_ptr<Platform> createSdlPlatform()
    {
        if ( videoInUse )
        {
            log::warning("platform: an SDL platform already exists; there is only "
                "one video subsystem per process");
            return nullptr;
        }
        if ( !SDL_InitSubSystem(SDL_INIT_VIDEO) )
        {
            log::warning("platform: initializing the SDL video subsystem failed: {}",
                SDL_GetError());
            return nullptr;
        }
        videoInUse = true;
        return std::make_unique<SdlPlatform>();
    }

}
