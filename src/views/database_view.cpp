#include "database_view.hpp"
#include "core/config.hpp"
#include "mpd/mpd_manager.hpp"
#include "utils/icon_provider.hpp"
#include "utils/format_utils.hpp"
#include <algorithm>

namespace miqumusic {

DatabaseView::DatabaseView(std::function<void()> on_action)
    : m_on_action(std::move(on_action))
{
    const auto& cfg = MiquMusicConfig::get();
    std::string font_icon = IconProvider::get_font_family();

    m_search_edit = miqu::EditTextBuilder::create()
        ->hint("Search artist, album, or song...")
        ->padding(10, 8)
        ->onTextChanged([this](std::shared_ptr<miqu::EditText>, const std::string& text) {
            m_filter_query = text;
            filter_and_render();
        })
        ->build();
    m_search_edit->get_layout_params().weight = 1.0f;
    m_search_edit->set_margin(0, 0, 12, 0);

    auto btn_rescan = miqu::ButtonBuilder::create()
        ->text("Update DB")
        ->fontFamily(cfg.font_family)
        ->textSize(11)
        ->cornerRadius(10)
        ->onClick([this]() {
            MPDManager::update_database(m_on_action);
        })
        ->build();
    btn_rescan->set_custom_colors(cfg.surface, cfg.accent_color);

    auto search_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->padding(16, 12)
        ->addView(m_search_edit)
        ->addView(btn_rescan)
        ->build();

    m_count_text = miqu::TextViewBuilder::create()
        ->text("0 tracks in library")
        ->fontFamily(cfg.font_family)
        ->textSize(11)
        ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.8f, 0.7f))
        ->build();
    m_count_text->set_margin(16, 0, 16, 8);

    m_grid_view = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->cellHeight(48)
        ->spacing(0, 4)
        ->build();
    m_grid_view->get_layout_params().weight = 1.0f;
    m_grid_view->set_margin(16, 0, 16, 8);

    m_root = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->addView(search_row)
        ->addView(m_count_text)
        ->addView(m_grid_view)
        ->build();
    m_root->get_layout_params().weight = 1.0f;
}

void DatabaseView::set_songs(std::vector<SongInfo> songs) {
    m_all_songs = std::move(songs);
    filter_and_render();
}

static bool string_contains_ci(const std::string& haystack, const std::string& needle) {
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return (it != haystack.end());
}

void DatabaseView::filter_and_render() {
    const auto& cfg = MiquMusicConfig::get();
    std::string font_icon = IconProvider::get_font_family();

    std::vector<SongInfo> matches;
    matches.reserve(m_all_songs.size());

    for (const auto& song : m_all_songs) {
        if (m_filter_query.empty() ||
            string_contains_ci(song.title, m_filter_query) ||
            string_contains_ci(song.artist, m_filter_query) ||
            string_contains_ci(song.album, m_filter_query) ||
            string_contains_ci(song.uri, m_filter_query)) {
            matches.push_back(song);
        }
    }

    if (m_count_text) {
        m_count_text->set_text(std::to_string(matches.size()) + " tracks found");
    }

    if (!m_grid_view) return;
    m_grid_view->clear_items();

    for (size_t i = 0; i < matches.size(); ++i) {
        const auto& song = matches[i];

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

        auto album_text = miqu::TextViewBuilder::create()
            ->text(song.album)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.6f, 0.7f, 0.6f))
            ->build();
        album_text->get_layout_params().width = 140;

        auto dur_text = miqu::TextViewBuilder::create()
            ->text(format_duration(song.duration))
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.6f, 0.7f, 0.7f))
            ->build();
        dur_text->get_layout_params().width = 50;

        std::string uri = song.uri;
        auto btn_add = miqu::ImageButtonBuilder::create()
            ->icon(IconProvider::get_icon(IconType::ADD_TO_QUEUE))
            ->fontFamily(font_icon)
            ->iconSize(13)
            ->circle(true)
            ->onClick([this, uri]() {
                MPDManager::add_to_queue(uri, m_on_action);
            })
            ->build();
        btn_add->set_margin(4, 0);

        auto row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(12, 6)
            ->addView(title_text)
            ->addView(artist_text)
            ->addView(album_text)
            ->addView(dur_text)
            ->addView(btn_add)
            ->build();

        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(cfg.surface)
            ->stroke(1, cfg.border_color)
            ->cornerRadius(10)
            ->addView(row)
            ->build();
        card->set_on_click_listener([this, uri]() {
            MPDManager::play_uri(uri, m_on_action);
        });

        m_grid_view->add_item(card);
    }
}

} // namespace miqumusic
