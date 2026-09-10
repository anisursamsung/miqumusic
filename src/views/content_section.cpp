#include "content_section.hpp"
#include "core/config.hpp"
#include "mpd/mpd_manager.hpp"
#include "mpd/mpd_client.hpp"
#include "utils/notification_mgr.hpp"
#include "utils/format_utils.hpp"
#include <unistd.h>
#include <mpd/database.h>
#include <mpd/stats.h>
#include <iostream>

namespace miqumusic {

ContentSection::ContentSection() {
    const auto& cfg = MiquMusicConfig::get();

    // 1. Content Area (FrameLayout that swaps tab views)
    m_content_area = std::make_shared<miqu::FrameLayout>();
    m_content_area->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    m_content_area->get_layout_params().weight = 1.0f;

    // 2. Tab Bar with 5 tabs (placed below content container, horizontally centered)
    auto tab_bar_builder = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::Center)
        ->spacing(8)
        ->padding(0, 0);

    for (size_t i = 0; i < m_tab_names.size(); ++i) {
        int idx = static_cast<int>(i);
        auto btn = miqu::ButtonBuilder::create()
            ->text(m_tab_names[i])
            ->fontFamily(cfg.font_family)
            ->textSize(12)
            ->bold(true)
            ->cornerRadius(8)
            ->onClick([this, idx]() {
                select_tab(idx);
            })
            ->build();

        btn->set_padding(20, 8);
        m_tab_buttons.push_back(btn);
        tab_bar_builder->addView(btn);
    }

    m_tab_bar = tab_bar_builder->build();
    m_tab_bar->set_margin(0, 0, 0, 14);

    miqu::LayoutParams tab_params(
        static_cast<int>(miqu::LayoutDimension::WrapContent),
        static_cast<int>(miqu::LayoutDimension::WrapContent));
    tab_params.gravity = miqu::Gravity::CenterHorizontal;

    // 3. Main vertical layout inside ContentSection:
    // Content area on top, Centered Tab Bar at the bottom
    auto v_layout = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->addView(m_content_area, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            0,
            1.0f))
        ->addView(m_tab_bar, tab_params)
        ->build();

    // Distinct background color for ContentSection (Deep Slate Blue)
    miqu::Color content_bg = miqu::Color::rgba(0.14f, 0.16f, 0.24f, 1.0f);

    m_root = miqu::CardViewBuilder::create()
        ->backgroundColor(content_bg)
        ->cornerRadius(0)
        ->addView(v_layout, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    m_root->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    m_root->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);
    m_root->get_layout_params().weight = 1.0f;

    // Start with Queue tab (index 0)
    select_tab(0);
}

std::shared_ptr<miqu::View> ContentSection::create_placeholder_view(const std::string& name) {
    const auto& cfg = MiquMusicConfig::get();

    auto text = miqu::TextViewBuilder::create()
        ->text("Active Tab: " + name)
        ->fontFamily(cfg.font_family)
        ->textSize(18)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();

    auto inner = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::Center)
        ->addView(text)
        ->build();

    auto card = miqu::CardViewBuilder::create()
        ->backgroundColor(miqu::Color::rgba(0.11f, 0.13f, 0.19f, 0.9f))
        ->stroke(1, miqu::Color::rgba(0.30f, 0.35f, 0.45f, 0.3f))
        ->cornerRadius(10)
        ->padding(16)
        ->addView(inner, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    card->set_margin(16, 16, 16, 10);
    card->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    card->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);
    return card;
}

std::shared_ptr<miqu::View> ContentSection::create_queue_view() {
    const auto& cfg = MiquMusicConfig::get();

    // 1. Fetch current MPD queue and status
    std::vector<SongInfo> songs;
    int current_song_id = -1;

    MPDManager::run_command([&songs, &current_song_id](struct mpd_connection* conn) {
        songs = MPDClient::fetch_queue(conn);
        auto status = MPDClient::fetch_status(conn);
        current_song_id = status.song_id;
    });

    // 2. Build single-column GridView (ListView)
    auto grid = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->stretchMode(miqu::StretchMode::ColumnWidth)
        ->cellHeight(54)
        ->spacing(0, 6)
        ->build();

    grid->set_padding(12, 10);
    grid->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    grid->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);

    if (songs.empty()) {
        auto empty_lbl = miqu::TextViewBuilder::create()
            ->text("Queue is empty. Select songs from the Db tab!")
            ->fontFamily(cfg.font_family)
            ->textSize(13)
            ->textColor(miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.6f))
            ->textAlignment(miqu::TextAlignment::Center)
            ->build();

        auto empty_row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Vertical)
            ->gravity(miqu::Gravity::Center)
            ->padding(28)
            ->addView(empty_lbl)
            ->build();

        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(miqu::Color::rgba(0.12f, 0.14f, 0.20f, 0.5f))
            ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.2f))
            ->cornerRadius(8)
            ->addView(empty_row, miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)))
            ->build();

        grid->add_item(card);
    } else {
        // 3. Populate queue rows
        for (size_t i = 0; i < songs.size(); ++i) {
            const auto& song = songs[i];
            bool is_active = (song.id >= 0 && song.id == current_song_id);

            std::string title = song.display_title();
            std::string artist = song.display_artist();
            std::string album = song.album.empty() ? "Unknown Album" : song.album;
            std::string num_str = is_active ? "▶" : ((i + 1 < 10 ? "0" : "") + std::to_string(i + 1));

            auto num_lbl = miqu::TextViewBuilder::create()
                ->text(num_str)
                ->fontFamily(cfg.font_family)
                ->textSize(is_active ? 13 : 11)
                ->bold(is_active)
                ->textColor(is_active ? cfg.accent_color : miqu::Color::rgba(0.6f, 0.65f, 0.75f, 0.7f))
                ->build();
            num_lbl->get_layout_params().width = 30;

            auto title_lbl = miqu::TextViewBuilder::create()
                ->text(title)
                ->fontFamily(cfg.font_family)
                ->textSize(13)
                ->bold(true)
                ->textColor(is_active ? cfg.accent_color : miqu::Color::rgb(1.0f, 1.0f, 1.0f))
                ->build();

            auto meta_lbl = miqu::TextViewBuilder::create()
                ->text(artist + " • " + album)
                ->fontFamily(cfg.font_family)
                ->textSize(11)
                ->textColor(is_active ? cfg.accent_color.with_alpha(0.85f) : miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.75f))
                ->build();

            auto text_col = miqu::LinearLayoutBuilder::create()
                ->orientation(miqu::Orientation::Vertical)
                ->addView(title_lbl)
                ->addView(meta_lbl)
                ->build();
            text_col->get_layout_params().weight = 1.0f;

            auto dur_lbl = miqu::TextViewBuilder::create()
                ->text(format_duration(song.duration))
                ->fontFamily(cfg.font_family)
                ->textSize(11)
                ->textColor(is_active ? cfg.accent_color : miqu::Color::rgba(0.6f, 0.65f, 0.75f, 0.7f))
                ->textAlignment(miqu::TextAlignment::Right)
                ->build();
            dur_lbl->get_layout_params().width = 50;

            auto row = miqu::LinearLayoutBuilder::create()
                ->orientation(miqu::Orientation::Horizontal)
                ->gravity(miqu::Gravity::CenterVertical)
                ->padding(14, 6)
                ->addView(num_lbl)
                ->addView(text_col)
                ->addView(dur_lbl)
                ->build();

            miqu::Color bg = is_active ? cfg.accent_color.with_alpha(0.18f) : miqu::Color::rgba(0.12f, 0.14f, 0.20f, 0.95f);
            miqu::Color stroke = is_active ? cfg.accent_color.with_alpha(0.6f) : miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.4f);

            auto card = miqu::CardViewBuilder::create()
                ->backgroundColor(bg)
                ->stroke(1, stroke)
                ->cornerRadius(8)
                ->addView(row, miqu::LayoutParams(
                    static_cast<int>(miqu::LayoutDimension::MatchParent),
                    static_cast<int>(miqu::LayoutDimension::MatchParent)))
                ->build();

            grid->add_item(card);
        }

        // Clicking a queue item directly plays it
        grid->set_on_item_click_listener([songs, this](size_t idx, std::shared_ptr<miqu::View>) {
            if (idx < songs.size()) {
                std::string title = songs[idx].display_title();
                int song_id = songs[idx].id;
                std::cout << "[miqumusic] Directly playing queue item #" << idx << " (id: " << song_id << "): " << title << "\n";
                NotificationManager::notify("MiquMusic", "Playing: " + title);
                if (song_id >= 0) {
                    MPDManager::play_id(song_id, [this]() {
                        // Refresh queue view so active indicator updates
                        select_tab(0);
                    });
                }
            }
        });
    }

    // Wrap grid inside outer container card
    auto container_card = miqu::CardViewBuilder::create()
        ->backgroundColor(miqu::Color::rgba(0.10f, 0.12f, 0.18f, 0.95f))
        ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.35f))
        ->cornerRadius(10)
        ->addView(grid, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    container_card->set_margin(16, 16, 16, 10);
    container_card->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    container_card->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);
    return container_card;
}

std::shared_ptr<miqu::View> ContentSection::create_database_view() {
    const auto& cfg = MiquMusicConfig::get();

    // 1. Fetch songs from MPD database
    std::vector<SongInfo> songs;
    MPDManager::run_command([&songs](struct mpd_connection* conn) {
        songs = MPDClient::fetch_database(conn);
    });

    // Fallback sample items if database is empty so list is always testable
    if (songs.empty()) {
        SongInfo s1;
        s1.title = "Mashooqa (Cocktail 2)";
        s1.artist = "Shahid, Kriti, Rashmika";
        s1.album = "Cocktail 2";
        s1.duration = 176;
        s1.uri = "Mashooqa (Video) Shahid, Kriti, Rashmika Pritam, Mahmood, Amitabh B, Raghav C, Ruaa K Cocktail 2.mp3";
        songs.push_back(s1);

        SongInfo s2;
        s2.title = "Midnight Horizon";
        s2.artist = "Synthwave Collective";
        s2.album = "Neon Dreams";
        s2.duration = 240;
        s2.uri = "local/midnight_horizon.flac";
        songs.push_back(s2);

        SongInfo s3;
        s3.title = "Starlight Serenade";
        s3.artist = "Acoustic Echoes";
        s3.album = "Golden Hour";
        s3.duration = 195;
        s3.uri = "local/starlight.mp3";
        songs.push_back(s3);
    }

    // 2. Build single-column GridView (ListView)
    auto grid = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->stretchMode(miqu::StretchMode::ColumnWidth)
        ->cellHeight(54)
        ->spacing(0, 6)
        ->build();

    grid->set_padding(12, 10);
    grid->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    grid->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);

    // 3. Populate database rows
    for (size_t i = 0; i < songs.size(); ++i) {
        const auto& song = songs[i];
        std::string title = song.display_title();
        std::string artist = song.display_artist();
        std::string album = song.album.empty() ? "Unknown Album" : song.album;
        std::string uri = song.uri;
        std::string num_str = (i + 1 < 10 ? "0" : "") + std::to_string(i + 1);

        auto num_lbl = miqu::TextViewBuilder::create()
            ->text(num_str)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.65f, 0.75f, 0.7f))
            ->build();
        num_lbl->get_layout_params().width = 30;

        auto title_lbl = miqu::TextViewBuilder::create()
            ->text(title)
            ->fontFamily(cfg.font_family)
            ->textSize(13)
            ->bold(true)
            ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
            ->build();

        auto meta_lbl = miqu::TextViewBuilder::create()
            ->text(artist + " • " + album)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.75f))
            ->build();

        auto text_col = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Vertical)
            ->addView(title_lbl)
            ->addView(meta_lbl)
            ->build();
        text_col->get_layout_params().weight = 1.0f;

        auto dur_lbl = miqu::TextViewBuilder::create()
            ->text(format_duration(song.duration))
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.65f, 0.75f, 0.7f))
            ->textAlignment(miqu::TextAlignment::Right)
            ->build();
        dur_lbl->get_layout_params().width = 50;

        auto row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(14, 6)
            ->addView(num_lbl)
            ->addView(text_col)
            ->addView(dur_lbl)
            ->build();

        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(miqu::Color::rgba(0.12f, 0.14f, 0.20f, 0.95f))
            ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.4f))
            ->cornerRadius(8)
            ->addView(row, miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)))
            ->build();

        grid->add_item(card);
    }

    // Clicking a database item: adds to queue and plays
    grid->set_on_item_click_listener([songs](size_t idx, std::shared_ptr<miqu::View>) {
        if (idx < songs.size()) {
            std::string title = songs[idx].display_title();
            std::string uri = songs[idx].uri;
            std::cout << "[miqumusic] Database click -> adding to queue & playing: " << title << "\n";
            NotificationManager::notify("MiquMusic", "Added to Queue & Playing: " + title);
            MPDManager::play_uri(uri);
        }
    });

    // Wrap grid inside outer container card
    auto container_card = miqu::CardViewBuilder::create()
        ->backgroundColor(miqu::Color::rgba(0.10f, 0.12f, 0.18f, 0.95f))
        ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.35f))
        ->cornerRadius(10)
        ->addView(grid, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    container_card->set_margin(16, 16, 16, 10);
    container_card->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    container_card->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);
    return container_card;
}

std::vector<SongInfo> ContentSection::get_playlist_songs(const std::string& name) {
    std::vector<SongInfo> songs;
    MPDManager::run_command([&songs, &name](struct mpd_connection* conn) {
        songs = MPDClient::fetch_playlist_songs(conn, name);
    });

    if (!songs.empty()) return songs;

    // Fallback sample tracks for demonstration
    if (name == "Favorites") {
        SongInfo s1; s1.title = "Mashooqa (Cocktail 2)"; s1.artist = "Shahid, Kriti, Rashmika"; s1.album = "Cocktail 2"; s1.duration = 176;
        SongInfo s2; s2.title = "Midnight Horizon"; s2.artist = "Synthwave Collective"; s2.album = "Neon Dreams"; s2.duration = 240;
        SongInfo s3; s3.title = "Starlight Serenade"; s3.artist = "Acoustic Echoes"; s3.album = "Golden Hour"; s3.duration = 195;
        return {s1, s2, s3};
    } else if (name == "Chill Beats") {
        SongInfo s1; s1.title = "Lofi Sunset"; s1.artist = "Chillhop Beats"; s1.album = "Cafe Sessions"; s1.duration = 150;
        SongInfo s2; s2.title = "Coffee & Rain"; s2.artist = "Tokyo Vibes"; s2.album = "Midnight Rain"; s2.duration = 190;
        return {s1, s2};
    } else if (name == "Workout Mix") {
        SongInfo s1; s1.title = "Adrenaline Rush"; s1.artist = "Electronic Pulse"; s1.album = "High Energy"; s1.duration = 225;
        SongInfo s2; s2.title = "Power Surge"; s2.artist = "Bass Heavy"; s2.album = "Overdrive"; s2.duration = 260;
        return {s1, s2};
    } else {
        SongInfo s1; s1.title = name + " Track 1"; s1.artist = "Various Artists"; s1.album = name; s1.duration = 210;
        SongInfo s2; s2.title = name + " Track 2"; s2.artist = "Various Artists"; s2.album = name; s2.duration = 185;
        return {s1, s2};
    }
}

void ContentSection::repopulate_playlists_grid(std::shared_ptr<miqu::GridView> grid, const std::vector<std::string>& playlists) {
    if (!grid) return;
    const auto& cfg = MiquMusicConfig::get();

    grid->clear_items();
    m_playlist_row_items.clear();

    for (size_t i = 0; i < playlists.size(); ++i) {
        const auto& name = playlists[i];
        bool is_expanded = (m_expanded_playlist == name);
        auto songs = get_playlist_songs(name);
        std::string num_str = (i + 1 < 10 ? "0" : "") + std::to_string(i + 1);

        // --- A. Playlist Header Card ---
        auto arrow_lbl = miqu::TextViewBuilder::create()
            ->text(is_expanded ? "▼" : "▶")
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(is_expanded ? cfg.accent_color : miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.7f))
            ->build();
        arrow_lbl->get_layout_params().width = 24;

        auto num_lbl = miqu::TextViewBuilder::create()
            ->text(num_str)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.6f, 0.65f, 0.75f, 0.7f))
            ->build();
        num_lbl->get_layout_params().width = 28;

        auto title_lbl = miqu::TextViewBuilder::create()
            ->text(name)
            ->fontFamily(cfg.font_family)
            ->textSize(13)
            ->bold(true)
            ->textColor(is_expanded ? cfg.accent_color : miqu::Color::rgb(1.0f, 1.0f, 1.0f))
            ->build();

        auto count_lbl = miqu::TextViewBuilder::create()
            ->text(std::to_string(songs.size()) + " tracks")
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.7f))
            ->textAlignment(miqu::TextAlignment::Right)
            ->build();
        count_lbl->get_layout_params().width = 80;

        auto header_row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(14, 6)
            ->addView(arrow_lbl)
            ->addView(num_lbl)
            ->addView(title_lbl)
            ->addView(count_lbl)
            ->build();
        title_lbl->get_layout_params().weight = 1.0f;

        miqu::Color header_bg = is_expanded ? cfg.accent_color.with_alpha(0.18f) : miqu::Color::rgba(0.12f, 0.14f, 0.20f, 0.95f);
        miqu::Color header_stroke = is_expanded ? cfg.accent_color.with_alpha(0.5f) : miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.4f);

        auto header_card = miqu::CardViewBuilder::create()
            ->backgroundColor(header_bg)
            ->stroke(1, header_stroke)
            ->cornerRadius(8)
            ->addView(header_row, miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)))
            ->build();

        grid->add_item(header_card);
        m_playlist_row_items.push_back({true, name, {}});

        // --- B. Indented Track Cards (if expanded) ---
        if (is_expanded) {
            for (size_t j = 0; j < songs.size(); ++j) {
                const auto& song = songs[j];
                std::string t_num = std::to_string(j + 1);

                auto icon_lbl = miqu::TextViewBuilder::create()
                    ->text("  • " + t_num)
                    ->fontFamily(cfg.font_family)
                    ->textSize(10)
                    ->textColor(miqu::Color::rgba(0.55f, 0.60f, 0.70f, 0.7f))
                    ->build();
                icon_lbl->get_layout_params().width = 44;

                auto s_title = miqu::TextViewBuilder::create()
                    ->text(song.display_title())
                    ->fontFamily(cfg.font_family)
                    ->textSize(12)
                    ->bold(true)
                    ->textColor(miqu::Color::rgba(0.95f, 0.95f, 1.0f, 0.9f))
                    ->build();

                auto s_meta = miqu::TextViewBuilder::create()
                    ->text(song.display_artist() + " • " + song.album)
                    ->fontFamily(cfg.font_family)
                    ->textSize(10)
                    ->textColor(miqu::Color::rgba(0.65f, 0.70f, 0.80f, 0.7f))
                    ->build();

                auto s_text_col = miqu::LinearLayoutBuilder::create()
                    ->orientation(miqu::Orientation::Vertical)
                    ->addView(s_title)
                    ->addView(s_meta)
                    ->build();
                s_text_col->get_layout_params().weight = 1.0f;

                auto s_dur = miqu::TextViewBuilder::create()
                    ->text(format_duration(song.duration))
                    ->fontFamily(cfg.font_family)
                    ->textSize(11)
                    ->textColor(miqu::Color::rgba(0.6f, 0.65f, 0.75f, 0.7f))
                    ->textAlignment(miqu::TextAlignment::Right)
                    ->build();
                s_dur->get_layout_params().width = 50;

                auto song_row = miqu::LinearLayoutBuilder::create()
                    ->orientation(miqu::Orientation::Horizontal)
                    ->gravity(miqu::Gravity::CenterVertical)
                    ->padding(18, 5)
                    ->addView(icon_lbl)
                    ->addView(s_text_col)
                    ->addView(s_dur)
                    ->build();

                auto song_card = miqu::CardViewBuilder::create()
                    ->backgroundColor(miqu::Color::rgba(0.08f, 0.10f, 0.15f, 0.95f))
                    ->stroke(1, miqu::Color::rgba(0.22f, 0.25f, 0.35f, 0.3f))
                    ->cornerRadius(6)
                    ->addView(song_row, miqu::LayoutParams(
                        static_cast<int>(miqu::LayoutDimension::MatchParent),
                        static_cast<int>(miqu::LayoutDimension::MatchParent)))
                    ->build();

                grid->add_item(song_card);
                m_playlist_row_items.push_back({false, name, song});
            }
        }
    }
}

std::shared_ptr<miqu::View> ContentSection::create_playlists_view() {
    // 1. Fetch saved playlists from MPD
    std::vector<std::string> playlists;
    MPDManager::run_command([&playlists](struct mpd_connection* conn) {
        playlists = MPDClient::fetch_playlists(conn);
    });

    if (playlists.empty()) {
        playlists.push_back("Favorites");
        playlists.push_back("Chill Beats");
        playlists.push_back("Workout Mix");
        playlists.push_back("Late Night Vibes");
        playlists.push_back("Acoustic Sessions");
    }

    // 2. Build single-column GridView (ListView)
    auto grid = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->stretchMode(miqu::StretchMode::ColumnWidth)
        ->cellHeight(52)
        ->spacing(0, 6)
        ->build();

    grid->set_padding(12, 10);
    grid->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    grid->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);

    // 3. Populate rows
    repopulate_playlists_grid(grid, playlists);

    // 4. On click: expand / collapse playlist headers!
    grid->set_on_item_click_listener([this, grid, playlists](size_t idx, std::shared_ptr<miqu::View>) {
        if (idx < m_playlist_row_items.size()) {
            const auto& item = m_playlist_row_items[idx];
            if (item.is_playlist_header) {
                // Toggle expand / collapse
                if (m_expanded_playlist == item.playlist_name) {
                    m_expanded_playlist = ""; // Collapse
                } else {
                    m_expanded_playlist = item.playlist_name; // Expand
                }
                repopulate_playlists_grid(grid, playlists);
            } else {
                // Clicked on a child track inside playlist - do nothing yet per user instructions
                std::cout << "[miqumusic] Playlist song clicked: " << item.song.display_title() << " (playback not implemented yet)\n";
            }
        }
    });

    // Wrap grid inside outer container card
    auto container_card = miqu::CardViewBuilder::create()
        ->backgroundColor(miqu::Color::rgba(0.10f, 0.12f, 0.18f, 0.95f))
        ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.35f))
        ->cornerRadius(10)
        ->addView(grid, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    container_card->set_margin(16, 16, 16, 10);
    container_card->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    container_card->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);
    return container_card;
}

std::shared_ptr<miqu::View> ContentSection::create_ytdlp_view() {
    const auto& cfg = MiquMusicConfig::get();

    // 1. Header & Description
    auto header = miqu::TextViewBuilder::create()
        ->text("Online Audio & YouTube (yt-dlp)")
        ->fontFamily(cfg.font_family)
        ->textSize(18)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();
    header->set_margin(0, 0, 0, 8);

    auto subtitle = miqu::TextViewBuilder::create()
        ->text("Stream or download audio from YouTube, SoundCloud, or direct URLs into your MPD player.")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->textColor(miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.75f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();
    subtitle->set_margin(0, 0, 0, 24);

    // 2. URL Input Field - MatchParent width with 52px height and comfortable padding
    auto url_edit = miqu::EditTextBuilder::create()
        ->hint("Paste video or audio URL here (https://...)")
        ->padding(20, 15)
        ->build();
    url_edit->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    url_edit->get_layout_params().height = 52;
    url_edit->set_margin(48, 0, 48, 22);

    // 3. Action Buttons
    auto btn_stream = miqu::ButtonBuilder::create()
        ->text("▶ Stream Now")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->bold(true)
        ->cornerRadius(8)
        ->build();
    btn_stream->set_custom_colors(cfg.accent_color, miqu::Color::rgb(0.0f, 0.0f, 0.0f));
    btn_stream->set_padding(24, 10);
    btn_stream->set_margin(0, 0, 16, 0);

    auto btn_download = miqu::ButtonBuilder::create()
        ->text("⬇ Download to Library")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->bold(true)
        ->cornerRadius(8)
        ->build();
    btn_download->set_custom_colors(miqu::Color::rgba(0.18f, 0.22f, 0.32f, 0.9f), miqu::Color::rgb(1.0f, 1.0f, 1.0f));
    btn_download->set_padding(24, 10);

    auto buttons_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::Center)
        ->addView(btn_stream)
        ->addView(btn_download)
        ->build();
    buttons_row->set_margin(0, 0, 0, 24);

    // 4. Feature Badges Row
    auto make_badge = [&](const std::string& label) {
        auto text = miqu::TextViewBuilder::create()
            ->text(label)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.8f))
            ->build();

        auto badge = miqu::CardViewBuilder::create()
            ->backgroundColor(miqu::Color::rgba(0.14f, 0.16f, 0.24f, 0.8f))
            ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.3f))
            ->cornerRadius(6)
            ->padding(12, 6)
            ->addView(text)
            ->build();
        badge->set_margin(6, 0, 6, 0);
        return badge;
    };

    auto badge1 = make_badge("Format: Opus / Best Audio");
    auto badge2 = make_badge("Target: ~/Music/Downloads");
    auto badge3 = make_badge("Backend: yt-dlp");

    auto badges_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::Center)
        ->addView(badge1)
        ->addView(badge2)
        ->addView(badge3)
        ->build();

    // 5. Vertical Content Form - Fills container card and centers children
    auto form_layout = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::Center)
        ->addView(header)
        ->addView(subtitle)
        ->addView(url_edit, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            52))
        ->addView(buttons_row)
        ->addView(badges_row)
        ->build();

    form_layout->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);

    // 6. Outer Container Card matching Que, Db, and PList styling
    auto container_card = miqu::CardViewBuilder::create()
        ->backgroundColor(miqu::Color::rgba(0.10f, 0.12f, 0.18f, 0.95f))
        ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.35f))
        ->cornerRadius(10)
        ->padding(24, 28)
        ->addView(form_layout, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    container_card->set_margin(16, 16, 16, 10);
    container_card->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    container_card->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);
    return container_card;
}

std::shared_ptr<miqu::View> ContentSection::create_settings_view() {
    const auto& cfg = MiquMusicConfig::get();

    struct SettingRowData {
        std::string tag;
        std::string title;
        std::string subtitle;
        std::string action_label;
        std::function<void()> action;
    };

    std::vector<SettingRowData> settings = {
        {
            "MPD",
            "MPD Server Connection",
            cfg.mpd_host + ":" + std::to_string(cfg.mpd_port) + " • Music Player Daemon host & port",
            "Ping Server",
            [host = cfg.mpd_host, port = cfg.mpd_port]() {
                MPDManager::run_command([host, port](struct mpd_connection* conn) {
                    if (conn && mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS) {
                        NotificationManager::notify("MPD Connection", "Online at " + host + ":" + std::to_string(port));
                    } else {
                        NotificationManager::notify("MPD Connection", "Failed to connect to " + host + ":" + std::to_string(port));
                    }
                });
            }
        },
        {
            "LIB",
            "Music Library Directory",
            cfg.music_directory + " • Click to rescan audio files",
            "Rescan DB",
            []() {
                MPDManager::run_command([](struct mpd_connection* conn) {
                    if (conn) {
                        mpd_run_update(conn, nullptr);
                        NotificationManager::notify("Music Library", "Rescan triggered for ~/Music");
                    }
                });
            }
        },
        {
            "STAT",
            "MPD Server Statistics",
            "View active track, album, and artist counts",
            "Fetch Stats",
            []() {
                MPDManager::run_command([](struct mpd_connection* conn) {
                    if (conn) {
                        struct mpd_stats* stats = mpd_run_stats(conn);
                        if (stats) {
                            unsigned int songs = mpd_stats_get_number_of_songs(stats);
                            unsigned int albums = mpd_stats_get_number_of_albums(stats);
                            unsigned int artists = mpd_stats_get_number_of_artists(stats);
                            mpd_stats_free(stats);
                            NotificationManager::notify("MPD Stats", std::to_string(songs) + " songs • " + std::to_string(albums) + " albums • " + std::to_string(artists) + " artists");
                        } else {
                            NotificationManager::notify("MPD Stats", "Could not retrieve stats from MPD");
                        }
                    }
                });
            }
        },
        {
            "FIFO",
            "Spectrum Visualizer Pipe",
            cfg.fifo_path + " • 44.1kHz 16-bit audio feed",
            "Check FIFO",
            [fifo = cfg.fifo_path]() {
                bool exists = (access(fifo.c_str(), F_OK) == 0);
                if (exists) {
                    NotificationManager::notify("Visualizer FIFO", "FIFO pipe active: " + fifo);
                } else {
                    NotificationManager::notify("Visualizer FIFO", "Pipe not found: " + fifo + " (enable in mpd.conf)");
                }
            }
        },
        {
            "BELL",
            "Desktop Notifications",
            "notify-send integration for playback & status alerts",
            "Test Alert",
            []() {
                NotificationManager::notify("MiquMusic", "Desktop notifications are working! 🎵");
            }
        },
        {
            "THEME",
            "Appearance & Styling",
            "Font: " + cfg.font_family + " (" + std::to_string(cfg.font_size) + "pt) • Accent: #33b5e5",
            "Theme Info",
            [font = cfg.font_family, size = cfg.font_size]() {
                NotificationManager::notify("Appearance", "Font: " + font + " (" + std::to_string(size) + "pt) • Accent: #33b5e5");
            }
        },
        {
            "INFO",
            "About MiquMusic",
            "Wayland native music player for Miquland desktop",
            "v0.1.0",
            []() {
                NotificationManager::notify("About MiquMusic", "MiquMusic v0.1.0\nDesktop audio player built with miqutoolkit");
            }
        }
    };

    // Build single-column GridView (ListView)
    auto grid = miqu::GridViewBuilder::create()
        ->numColumns(1)
        ->stretchMode(miqu::StretchMode::ColumnWidth)
        ->cellHeight(54)
        ->spacing(0, 6)
        ->build();

    grid->set_padding(12, 10);
    grid->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    grid->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);

    for (size_t i = 0; i < settings.size(); ++i) {
        const auto& item = settings[i];

        // 1. Tag pill on the left
        auto tag_lbl = miqu::TextViewBuilder::create()
            ->text(item.tag)
            ->fontFamily(cfg.font_family)
            ->textSize(10)
            ->bold(true)
            ->textColor(cfg.accent_color)
            ->textAlignment(miqu::TextAlignment::Center)
            ->build();

        auto tag_card = miqu::CardViewBuilder::create()
            ->backgroundColor(miqu::Color::rgba(0.18f, 0.22f, 0.32f, 0.6f))
            ->stroke(1, miqu::Color::rgba(0.28f, 0.35f, 0.50f, 0.4f))
            ->cornerRadius(6)
            ->padding(6, 4)
            ->addView(tag_lbl)
            ->build();
        tag_card->get_layout_params().width = 54;
        tag_card->set_margin(0, 0, 12, 0);

        // 2. Title & Subtitle in center column
        auto title_lbl = miqu::TextViewBuilder::create()
            ->text(item.title)
            ->fontFamily(cfg.font_family)
            ->textSize(13)
            ->bold(true)
            ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
            ->build();

        auto meta_lbl = miqu::TextViewBuilder::create()
            ->text(item.subtitle)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->textColor(miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.75f))
            ->build();

        auto text_col = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Vertical)
            ->addView(title_lbl)
            ->addView(meta_lbl)
            ->build();
        text_col->get_layout_params().weight = 1.0f;

        // 3. Action button pill on the right
        auto action_lbl = miqu::TextViewBuilder::create()
            ->text(item.action_label)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->bold(true)
            ->textColor(cfg.accent_color)
            ->textAlignment(miqu::TextAlignment::Center)
            ->build();

        auto action_pill = miqu::CardViewBuilder::create()
            ->backgroundColor(miqu::Color::rgba(0.16f, 0.20f, 0.30f, 0.7f))
            ->stroke(1, miqu::Color::rgba(0.20f, 0.71f, 0.90f, 0.4f))
            ->cornerRadius(6)
            ->padding(10, 5)
            ->addView(action_lbl)
            ->build();

        // 4. Horizontal row
        auto row = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Horizontal)
            ->gravity(miqu::Gravity::CenterVertical)
            ->padding(14, 6)
            ->addView(tag_card)
            ->addView(text_col)
            ->addView(action_pill)
            ->build();

        // 5. Outer row card
        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(miqu::Color::rgba(0.12f, 0.14f, 0.20f, 0.95f))
            ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.4f))
            ->cornerRadius(8)
            ->addView(row, miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)))
            ->build();

        grid->add_item(card);
    }

    // Row click triggers setting action
    grid->set_on_item_click_listener([settings](size_t idx, std::shared_ptr<miqu::View>) {
        if (idx < settings.size()) {
            std::cout << "[miqumusic] Setting clicked: " << settings[idx].title << "\n";
            if (settings[idx].action) {
                settings[idx].action();
            }
        }
    });

    // Wrap grid inside outer container card matching Que, Db, PList, and YDlp styling
    auto container_card = miqu::CardViewBuilder::create()
        ->backgroundColor(miqu::Color::rgba(0.10f, 0.12f, 0.18f, 0.95f))
        ->stroke(1, miqu::Color::rgba(0.28f, 0.32f, 0.45f, 0.35f))
        ->cornerRadius(10)
        ->addView(grid, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    container_card->set_margin(16, 16, 16, 10);
    container_card->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    container_card->get_layout_params().height = static_cast<int>(miqu::LayoutDimension::MatchParent);
    return container_card;
}

void ContentSection::select_tab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tab_buttons.size())) return;
    m_selected_tab = index;

    const auto& cfg = MiquMusicConfig::get();

    for (size_t i = 0; i < m_tab_buttons.size(); ++i) {
        bool active = (static_cast<int>(i) == index);
        m_tab_buttons[i]->set_selected(active);

        if (active) {
            m_tab_buttons[i]->set_custom_colors(cfg.accent_color, miqu::Color::rgb(0.0f, 0.0f, 0.0f));
        } else {
            m_tab_buttons[i]->set_custom_colors(
                miqu::Color::rgba(0.18f, 0.21f, 0.30f, 0.8f),
                miqu::Color::rgba(0.75f, 0.80f, 0.90f, 0.85f));
        }
    }

    // Switch view in content area
    if (m_content_area) {
        m_content_area->clear_views();
        if (index == 0) { // "Que" tab
            m_content_area->add_view(create_queue_view(), miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)));
        } else if (index == 1) { // "Db" tab
            m_content_area->add_view(create_database_view(), miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)));
        } else if (index == 2) { // "PList" tab
            m_content_area->add_view(create_playlists_view(), miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)));
        } else if (index == 3) { // "YDlp" tab
            m_content_area->add_view(create_ytdlp_view(), miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)));
        } else if (index == 4) { // "Settings" tab
            m_content_area->add_view(create_settings_view(), miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)));
        } else {
            m_content_area->add_view(create_placeholder_view(m_tab_names[index]), miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)));
        }
    }
}

} // namespace miqumusic
