// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/NullPlatform.h>

#include <babylon/log/Log.h>
#include <babylon/platform/Event.h>
#include <babylon/platform/Platform.h>
#include <babylon/platform/Window.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace babylon::platform
{
namespace
{

    // One Null window: the title and size it was made with, kept in memory.
    class NullWindow final : public Window
    {
    public:
        NullWindow(std::string title, Size size, WindowId id) :
            m_title(std::move(title)), m_size(size), m_id(id)
        {
        }

        void setTitle(std::string_view title) override
        {
            m_title.assign(title);
        }

        std::string title() const override
        {
            return m_title;
        }

        Size size() const override
        {
            return m_size;
        }

        WindowId id() const override
        {
            return m_id;
        }

        // A Null window has no cursor to hide, so nothing can refuse: the
        // mode is only the state the interface promises to report back.
        bool setRelativeMouse(bool enabled) override
        {
            m_relativeMouse = enabled;
            return true;
        }

        bool relativeMouse() const override
        {
            return m_relativeMouse;
        }

    private:
        // Called only with a size that passed the shared check in resize(). Returns
        // false and logs one warning if the backend refused it.
        bool doResize(Size size) override
        {
            m_size = size;
            return true;
        }

        std::string m_title;
        Size m_size;
        WindowId m_id;
        bool m_relativeMouse = false;
    };

}

    std::unique_ptr<Window> NullPlatform::createWindow(const WindowDesc& desc)
    {
        if ( !isValidWindowSize(desc.size) )
        {
            // The same check and the same one warning as Window::resize(),
            // so a size is accepted at creation exactly when it is later.
            log::warning("platform: rejected window size {}x{}: both dimensions "
                "must be within 1 and {}", desc.size.width, desc.size.height,
                kMaxWindowDimension);
            return nullptr;
        }
        return std::make_unique<NullWindow>(desc.title, desc.size, m_nextId++);
    }

    std::optional<Event> NullPlatform::pollEvent()
    {
        if ( m_events.empty() )
        {
            return std::nullopt;   // nothing was ever pushed: never blocks
        }
        const Event event = m_events.front();
        m_events.pop_front();
        return event;
    }

    void NullPlatform::pushEvent(const Event& event)
    {
        m_events.push_back(event);
    }

}
