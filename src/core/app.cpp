#include "app.hpp"
#include "core/config.hpp"
#include "utils/icon_provider.hpp"
#include <iostream>

namespace miqumusic {

App::App(int argc, char* argv[]) {
    // 1. Load configuration
    MiquMusicConfig::get().load();

    // 2. Initialize embedded icon font
    IconProvider::register_font();

    // 3. Initialize engine
    m_engine = miqu::AppEngine::create();
    if (!m_engine) {
        throw std::runtime_error("Failed to initialize miqu AppEngine");
    }

    // 4. Setup UI
    setup_ui();
}

App::~App() {
    m_running = false;
}

void App::setup_ui() {
    const auto& cfg = MiquMusicConfig::get();

    // Sync toolkit global config with miqumusic dark theme
    auto toolkit_cfg = miqu::Config::get();
    toolkit_cfg->colors.background = cfg.background;
    toolkit_cfg->colors.surface = cfg.surface;
    toolkit_cfg->colors.surface_variant = miqu::Color::rgba(0.14f, 0.17f, 0.25f, 0.95f);
    toolkit_cfg->colors.on_surface_variant = miqu::Color::rgba(0.60f, 0.68f, 0.82f, 0.65f);
    toolkit_cfg->colors.on_surface = miqu::Color::rgb(0.95f, 0.97f, 1.0f);
    toolkit_cfg->colors.primary = cfg.accent_color;
    toolkit_cfg->colors.on_primary = miqu::Color::rgb(0.0f, 0.0f, 0.0f);
    toolkit_cfg->metrics.corner_radius = 10;
    toolkit_cfg->metrics.font_family = cfg.font_family;
    toolkit_cfg->metrics.font_size = cfg.font_size;

    // Top section: ContentSection
    m_content_section = std::make_shared<ContentSection>();

    // Bottom section: ControlSection
    m_control_section = std::make_shared<ControlSection>();

    // Simple vertical linear layout
    auto root_layout = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->addView(m_content_section->get_view(), miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            0,
            1.0f))
        ->addView(m_control_section->get_view(), miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            ControlSection::DEFAULT_HEIGHT))
        ->build();

    auto root_card = miqu::CardViewBuilder::create()
        ->backgroundColor(cfg.background)
        ->cornerRadius(0)
        ->addView(root_layout, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    m_window = miqu::WindowBuilder::create()
        ->role(miqu::WindowRole::Toplevel)
        ->title("MiquMusic")
        ->appId("miqumusic")
        ->preferredSize(cfg.window_width, cfg.window_height)
        ->contentView(root_card)
        ->onClose([this]() {
            m_running = false;
            if (m_engine) m_engine->quit();
        })
        ->build();
}

int App::run() {
    if (!m_engine) return 1;
    return m_engine->enter_loop();
}

} // namespace miqumusic
