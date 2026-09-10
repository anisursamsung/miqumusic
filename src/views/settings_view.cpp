#include "settings_view.hpp"
#include "core/config.hpp"
#include "mpd/mpd_manager.hpp"
#include "utils/icon_provider.hpp"
#include <iostream>

namespace miqumusic {

SettingsView::SettingsView(std::function<void()> on_action)
    : m_on_action(std::move(on_action))
{
    const auto& cfg = MiquMusicConfig::get();

    auto header = miqu::TextViewBuilder::create()
        ->text("Settings & MPD Configuration")
        ->fontFamily(cfg.font_family)
        ->textSize(18)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->build();
    header->set_margin(0, 0, 0, 16);

    auto make_info_row = [&](const std::string& label, const std::string& value) {
        auto lbl = miqu::TextViewBuilder::create()
            ->text(label)
            ->fontFamily(cfg.font_family)
            ->textSize(12)
            ->bold(true)
            ->textColor(cfg.accent_color)
            ->build();
        lbl->get_layout_params().width = 160;

        auto val = miqu::TextViewBuilder::create()
            ->text(value)
            ->fontFamily(cfg.font_family)
            ->textSize(12)
            ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
            ->build();
        val->get_layout_params().weight = 1.0f;

        return miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(8, 4)
            ->addView(lbl)
            ->addView(val)
            ->build();
    };

    auto row_host = make_info_row("MPD Host:", cfg.mpd_host);
    auto row_port = make_info_row("MPD Port:", std::to_string(cfg.mpd_port));
    auto row_dir = make_info_row("Music Directory:", cfg.music_directory);
    auto row_fifo = make_info_row("Visualizer FIFO:", cfg.fifo_path);

    auto btn_rescan = miqu::ButtonBuilder::create()
        ->text("Update Music Database")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->cornerRadius(10)
        ->onClick([this]() {
            MPDManager::update_database(m_on_action);
            if (m_status_text) m_status_text->set_text("Requested MPD database update.");
        })
        ->build();
    btn_rescan->set_custom_colors(cfg.accent_color, miqu::Color::rgb(0.0f, 0.0f, 0.0f));
    btn_rescan->set_margin(0, 0, 12, 0);

    auto btn_restart = miqu::ButtonBuilder::create()
        ->text("Ensure / Restart MPD")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->cornerRadius(10)
        ->onClick([this]() {
            MPDManager::ensure_mpd_running();
            if (m_status_text) m_status_text->set_text("Ensured MPD is running and configured.");
            if (m_on_action) m_on_action();
        })
        ->build();
    btn_restart->set_custom_colors(cfg.surface, miqu::Color::rgb(1.0f, 1.0f, 1.0f));

    auto btns_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::Center)
        ->addView(btn_rescan)
        ->addView(btn_restart)
        ->build();
    btns_row->set_margin(0, 20, 0, 16);

    m_status_text = miqu::TextViewBuilder::create()
        ->text("")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->textColor(cfg.accent_color)
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();

    auto about = miqu::TextViewBuilder::create()
        ->text("MiquMusic v1.0 • Native Wayland MPD client for Miquland\nBuilt with miqutoolkit")
        ->fontFamily(cfg.font_family)
        ->textSize(11)
        ->textColor(miqu::Color::rgba(0.6f, 0.6f, 0.7f, 0.6f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();
    about->set_margin(0, 24, 0, 0);

    auto card = miqu::CardViewBuilder::create()
        ->backgroundColor(cfg.surface)
        ->stroke(cfg.border_width, cfg.border_color)
        ->cornerRadius(cfg.corner_radius)
        ->padding(32, 28)
        ->addView(miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Vertical)
            ->addView(header)
            ->addView(row_host)
            ->addView(row_port)
            ->addView(row_dir)
            ->addView(row_fifo)
            ->addView(btns_row)
            ->addView(m_status_text)
            ->addView(about)
            ->build())
        ->build();
    card->get_layout_params().width = 620;
    card->set_margin(0, 30, 0, 0);

    m_root = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::CenterHorizontal)
        ->addView(card)
        ->build();
    m_root->get_layout_params().weight = 1.0f;
}

} // namespace miqumusic
