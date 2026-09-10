#include "queue_view.hpp"
#include "core/config.hpp"
#include "mpd/mpd_manager.hpp"
#include "utils/icon_provider.hpp"
#include "utils/format_utils.hpp"
#include <iostream>

namespace miqumusic {

QueueView::QueueView(std::function<void()> on_queue_modified)
    : m_on_queue_modified(std::move(on_queue_modified))
{
    const auto& cfg = MiquMusicConfig::get();
    std::string font_icon = IconProvider::get_font_family();

    m_header_text = miqu::TextViewBuilder::create()
        ->text("Playing Queue (0 tracks)")
        ->fontFamily(cfg.font_family)
        ->textSize(16)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->build();
    m_header_text->get_layout_params().weight = 1.0f;

    auto btn_clear = miqu::ButtonBuilder::create()
        ->text("Clear Queue")
        ->fontFamily(cfg.font_family)
        ->textSize(11)
        ->cornerRadius(8)
        ->onClick([this]() {
            MPDManager::clear_queue(m_on_queue_modified);
        })
        ->build();
    btn_clear->set_custom_colors(cfg.surface, miqu::Color::rgba(0.9f, 0.4f, 0.4f, 1.0f));

    auto header_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->padding(16, 12)
        ->addView(m_header_text)
        ->addView(btn_clear)
        ->build();

    m_grid_view = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->cellHeight(48)
        ->spacing(0, 4)
        ->build();
    m_grid_view->get_layout_params().weight = 1.0f;
    m_grid_view->set_margin(16, 0, 16, 8);

    m_root = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->addView(header_row)
        ->addView(m_grid_view)
        ->build();
    m_root->get_layout_params().weight = 1.0f;
}

void QueueView::refresh_queue(const std::vector<SongInfo>& songs, int active_song_id) {
    const auto& cfg = MiquMusicConfig::get();
    std::string font_icon = IconProvider::get_font_family();

    if (m_header_text) {
        m_header_text->set_text("Playing Queue (" + std::to_string(songs.size()) + " tracks)");
    }

    if (!m_grid_view) return;
    m_grid_view->clear_items();

    for (size_t i = 0; i < songs.size(); ++i) {
        const auto& song = songs[i];
        bool is_active = (song.id == active_song_id);

        auto num_text = miqu::TextViewBuilder::create()
            ->text(std::to_string(i + 1))
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(is_active ? cfg.accent_color : miqu::Color::rgba(0.6f, 0.6f, 0.7f, 0.7f))
            ->build();
        num_text->get_layout_params().width = 30;

        auto title_text = miqu::TextViewBuilder::create()
            ->text(song.display_title())
            ->fontFamily(cfg.font_family)
            ->textSize(12)
            ->bold(is_active)
            ->textColor(is_active ? cfg.accent_color : miqu::Color::rgb(1.0f, 1.0f, 1.0f))
            ->build();
        title_text->get_layout_params().weight = 1.0f;

        auto artist_text = miqu::TextViewBuilder::create()
            ->text(song.display_artist())
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.8f, 0.7f))
            ->build();
        artist_text->get_layout_params().width = 180;

        auto dur_text = miqu::TextViewBuilder::create()
            ->text(format_duration(song.duration))
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.6f, 0.7f, 0.7f))
            ->build();
        dur_text->get_layout_params().width = 50;

        int song_id = song.id;
        auto btn_remove = miqu::ImageButtonBuilder::create()
            ->icon(IconProvider::get_icon(IconType::DELETE))
            ->fontFamily(font_icon)
            ->iconSize(12)
            ->circle(true)
            ->onClick([this, song_id]() {
                MPDManager::remove_from_queue(song_id, m_on_queue_modified);
            })
            ->build();
        btn_remove->set_margin(4, 0);

        auto row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(12, 6)
            ->addView(num_text)
            ->addView(title_text)
            ->addView(artist_text)
            ->addView(dur_text)
            ->addView(btn_remove)
            ->build();

        miqu::Color bg = is_active ? cfg.accent_color.with_alpha(0.18f) : cfg.surface;
        miqu::Color border = is_active ? cfg.accent_color.with_alpha(0.4f) : cfg.border_color;

        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(bg)
            ->stroke(1, border)
            ->cornerRadius(10)
            ->addView(row)
            ->build();
        card->set_on_click_listener([this, song_id]() {
            MPDManager::play_id(song_id, m_on_queue_modified);
        });

        m_grid_view->add_item(card);
    }
}

} // namespace miqumusic
