#include "playlists_view.hpp"
#include "core/config.hpp"
#include "mpd/mpd_manager.hpp"
#include "utils/icon_provider.hpp"
#include "utils/format_utils.hpp"
#include <iostream>

namespace miqumusic {

PlaylistsView::PlaylistsView(std::function<void(const std::string&)> on_select_playlist, std::function<void()> on_action)
    : m_on_select_playlist(std::move(on_select_playlist)), m_on_action(std::move(on_action))
{
    const auto& cfg = MiquMusicConfig::get();

    // 1. Left Pane: Playlists
    m_left_header = miqu::TextViewBuilder::create()
        ->text("Playlists (0)")
        ->fontFamily(cfg.font_family)
        ->textSize(15)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->build();
    m_left_header->set_margin(16, 12, 16, 8);

    m_left_grid = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->cellHeight(44)
        ->spacing(0, 4)
        ->build();
    m_left_grid->get_layout_params().weight = 1.0f;
    m_left_grid->set_margin(12, 0, 12, 8);

    auto left_pane = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->addView(m_left_header)
        ->addView(m_left_grid)
        ->build();
    left_pane->get_layout_params().width = 240;

    // 2. Right Pane: Tracks
    m_right_header = miqu::TextViewBuilder::create()
        ->text("Select a playlist")
        ->fontFamily(cfg.font_family)
        ->textSize(15)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->build();
    m_right_header->set_margin(16, 12, 16, 8);

    m_right_grid = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->cellHeight(46)
        ->spacing(0, 4)
        ->build();
    m_right_grid->get_layout_params().weight = 1.0f;
    m_right_grid->set_margin(12, 0, 16, 8);

    auto right_pane = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->addView(m_right_header)
        ->addView(m_right_grid)
        ->build();
    right_pane->get_layout_params().weight = 1.0f;

    m_root = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->addView(left_pane)
        ->addView(right_pane)
        ->build();
    m_root->get_layout_params().weight = 1.0f;
}

void PlaylistsView::set_playlists(const std::vector<std::string>& playlists) {
    const auto& cfg = MiquMusicConfig::get();
    std::string font_icon = IconProvider::get_font_family();

    if (m_left_header) {
        m_left_header->set_text("Playlists (" + std::to_string(playlists.size()) + ")");
    }

    if (!m_left_grid) return;
    m_left_grid->clear_items();

    for (const auto& pl : playlists) {
        bool is_selected = (pl == m_selected_playlist);

        auto icon = miqu::TextViewBuilder::create()
            ->text(IconProvider::get_icon(IconType::NAV_PLAYLIST))
            ->fontFamily(font_icon)
            ->textSize(13)
            ->textColor(is_selected ? cfg.accent_color : miqu::Color::rgba(0.7f, 0.7f, 0.8f, 0.7f))
            ->build();
        icon->get_layout_params().width = 24;

        auto title = miqu::TextViewBuilder::create()
            ->text(pl)
            ->fontFamily(cfg.font_family)
            ->textSize(12)
            ->bold(is_selected)
            ->textColor(is_selected ? cfg.accent_color : miqu::Color::rgb(1.0f, 1.0f, 1.0f))
            ->build();
        title->get_layout_params().weight = 1.0f;

        std::string pl_name = pl;
        auto btn_load = miqu::ImageButtonBuilder::create()
            ->icon(IconProvider::get_icon(IconType::PLAY))
            ->fontFamily(font_icon)
            ->iconSize(12)
            ->circle(true)
            ->onClick([this, pl_name]() {
                MPDManager::load_playlist(pl_name, m_on_action);
            })
            ->build();

        auto btn_del = miqu::ImageButtonBuilder::create()
            ->icon(IconProvider::get_icon(IconType::DELETE))
            ->fontFamily(font_icon)
            ->iconSize(12)
            ->circle(true)
            ->onClick([this, pl_name]() {
                MPDManager::delete_playlist(pl_name, m_on_action);
            })
            ->build();

        auto row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(8, 4)
            ->addView(icon)
            ->addView(title)
            ->addView(btn_load)
            ->addView(btn_del)
            ->build();

        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(is_selected ? cfg.accent_color.with_alpha(0.2f) : cfg.surface)
            ->stroke(1, is_selected ? cfg.accent_color.with_alpha(0.5f) : cfg.border_color)
            ->cornerRadius(8)
            ->addView(row)
            ->build();
        card->set_on_click_listener([this, pl_name]() {
            m_selected_playlist = pl_name;
            if (m_on_select_playlist) m_on_select_playlist(pl_name);
        });

        m_left_grid->add_item(card);
    }
}

void PlaylistsView::set_playlist_songs(const std::string& name, const std::vector<SongInfo>& songs) {
    const auto& cfg = MiquMusicConfig::get();
    m_selected_playlist = name;

    if (m_right_header) {
        m_right_header->set_text(name + " (" + std::to_string(songs.size()) + " tracks)");
    }

    if (!m_right_grid) return;
    m_right_grid->clear_items();

    for (size_t i = 0; i < songs.size(); ++i) {
        const auto& song = songs[i];

        auto num_text = miqu::TextViewBuilder::create()
            ->text(std::to_string(i + 1))
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.6f, 0.7f, 0.7f))
            ->build();
        num_text->get_layout_params().width = 28;

        auto title_text = miqu::TextViewBuilder::create()
            ->text(song.display_title())
            ->fontFamily(cfg.font_family)
            ->textSize(12)
            ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
            ->build();
        title_text->get_layout_params().weight = 1.0f;

        auto artist_text = miqu::TextViewBuilder::create()
            ->text(song.display_artist())
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.8f, 0.7f))
            ->build();
        artist_text->get_layout_params().width = 160;

        auto dur_text = miqu::TextViewBuilder::create()
            ->text(format_duration(song.duration))
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.6f, 0.7f, 0.7f))
            ->build();
        dur_text->get_layout_params().width = 50;

        std::string uri = song.uri;
        auto row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(10, 6)
            ->addView(num_text)
            ->addView(title_text)
            ->addView(artist_text)
            ->addView(dur_text)
            ->build();

        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(cfg.surface)
            ->stroke(1, cfg.border_color)
            ->cornerRadius(8)
            ->addView(row)
            ->build();
        card->set_on_click_listener([this, uri]() {
            MPDManager::play_uri(uri, m_on_action);
        });

        m_right_grid->add_item(card);
    }
}

} // namespace miqumusic
