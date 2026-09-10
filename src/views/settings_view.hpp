#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include <functional>

namespace miqumusic {

class SettingsView {
public:
    SettingsView(std::function<void()> on_action);

    std::shared_ptr<miqu::View> get_view() { return m_root; }

private:
    std::function<void()> m_on_action;
    std::shared_ptr<miqu::LinearLayout> m_root;
    std::shared_ptr<miqu::TextView> m_status_text;
};

} // namespace miqumusic
