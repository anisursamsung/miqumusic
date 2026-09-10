#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include "views/content_section.hpp"
#include "views/control_section.hpp"
#include <memory>

namespace miqumusic {

class App {
public:
    App(int argc, char* argv[]);
    ~App();

    int run();

private:
    void setup_ui();

    std::shared_ptr<miqu::AppEngine> m_engine;
    std::shared_ptr<miqu::Window> m_window;

    std::shared_ptr<ContentSection> m_content_section;
    std::shared_ptr<ControlSection> m_control_section;

    bool m_running = true;
};

} // namespace miqumusic
