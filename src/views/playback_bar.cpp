#include "playback_bar.hpp"
#include "core/config.hpp"
#include "mpd/mpd_manager.hpp"
#include "utils/icon_provider.hpp"
#include "utils/format_utils.hpp"
#include <iostream>

namespace miqumusic {

PlaybackBar::PlaybackBar(std::function<void(ViewMode)> on_change_tab, std::function<void()> on_update_needed)
    : m_on_change_tab(std::move(on_change_tab)), m_on_update_needed(std::move(on_update_needed))
{
    const auto& cfg = MiquMusicConfig::get();
    std::string font_icon = IconProvider::get_font_family();

    // 1. Left Section: Mini Album Art + Track Info
    m_mini_art = miqu::CircleImageViewBuilder::create()
        ->targetSize(46)
        ->border(1, cfg.border_color)
        ->build();
    m_mini_art->set_margin(4, 4, 10, 4);

    m_title_text = miqu::TextViewBuilder::create()
        ->text("No track playing")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->build();

    m_artist_text = miqu::TextViewBuilder::create()
        ->text("MPD Idle")
        ->fontFamily(cfg.font_family)
        ->textSize(10)
        ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.75f, 0.8f))
        ->build();

    auto track_info_layout = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::CenterVertical)
        ->spacing(2)
        ->addView(m_title_text)
        ->addView(m_artist_text)
        ->build();

    auto left_section = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->addView(m_mini_art)
        ->addView(track_info_layout)
        ->build();
    left_section->get_layout_params().width = 240;

    // 2. Center Section: Transport Controls + SeekBar
    m_btn_shuffle = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::SHUFFLE))
        ->fontFamily(font_icon)
        ->iconSize(14)
        ->circle(true)
        ->onClick([this]() {
            MPDManager::toggle_random(m_on_update_needed);
        })
        ->build();

    m_btn_prev = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::PREV_TRACK))
        ->fontFamily(font_icon)
        ->iconSize(16)
        ->circle(true)
        ->onClick([this]() {
            MPDManager::prev_track(m_on_update_needed);
        })
        ->build();

    m_btn_play_pause = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::PLAY))
        ->fontFamily(font_icon)
        ->iconSize(20)
        ->circle(true)
        ->backgroundColor(cfg.accent_color)
        ->iconColor(miqu::Color::rgb(0.0f, 0.0f, 0.0f))
        ->onClick([this]() {
            MPDManager::toggle_play_pause(m_on_update_needed);
        })
        ->build();
    m_btn_play_pause->set_margin(4, 0);

    m_btn_next = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NEXT_TRACK))
        ->fontFamily(font_icon)
        ->iconSize(16)
        ->circle(true)
        ->onClick([this]() {
            MPDManager::next_track(m_on_update_needed);
        })
        ->build();

    m_btn_repeat = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::REPEAT_OFF))
        ->fontFamily(font_icon)
        ->iconSize(14)
        ->circle(true)
        ->onClick([this]() {
            MPDManager::toggle_repeat(m_on_update_needed);
        })
        ->build();

    m_btn_consume = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::CONSUME))
        ->fontFamily(font_icon)
        ->iconSize(14)
        ->circle(true)
        ->onClick([this]() {
            MPDManager::toggle_consume(m_on_update_needed);
        })
        ->build();

    auto transport_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::Center)
        ->spacing(8)
        ->addView(m_btn_shuffle)
        ->addView(m_btn_prev)
        ->addView(m_btn_play_pause)
        ->addView(m_btn_next)
        ->addView(m_btn_repeat)
        ->addView(m_btn_consume)
        ->build();

    m_elapsed_text = miqu::TextViewBuilder::create()
        ->text("00:00")
        ->fontFamily(cfg.font_family)
        ->textSize(10)
        ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.75f, 0.8f))
        ->build();
    m_elapsed_text->set_margin(0, 0, 8, 0);

    m_seek_bar = std::make_shared<CustomSeekBar>();
    m_seek_bar->set_progress_color(cfg.accent_color);
    m_seek_bar->get_layout_params().weight = 1.0f;
    m_seek_bar->set_on_seek_listener([this](float progress, bool from_user) {
        if (from_user) {
            m_is_user_seeking = true;
            MPDManager::seek(progress, [this]() {
                m_is_user_seeking = false;
                if (m_on_update_needed) m_on_update_needed();
            });
        }
    });

    m_total_text = miqu::TextViewBuilder::create()
        ->text("00:00")
        ->fontFamily(cfg.font_family)
        ->textSize(10)
        ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.75f, 0.8f))
        ->build();
    m_total_text->set_margin(8, 0, 0, 0);

    auto progress_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->addView(m_elapsed_text)
        ->addView(m_seek_bar)
        ->addView(m_total_text)
        ->build();

    auto center_section = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::Center)
        ->spacing(4)
        ->addView(transport_row)
        ->addView(progress_row)
        ->build();
    center_section->get_layout_params().weight = 1.0f;

    // 3. Right Section: Volume Slider & Tab Switcher
    m_btn_volume = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::VOLUME_HIGH))
        ->fontFamily(font_icon)
        ->iconSize(14)
        ->circle(true)
        ->onClick([this]() {
            // Mute / unmute toggle
            MPDManager::set_volume(0, m_on_update_needed);
        })
        ->build();

    m_volume_bar = miqu::SeekBarBuilder::create()
        ->trackHeight(4)
        ->thumbRadius(5)
        ->progress(0.75f)
        ->progressColor(cfg.accent_color)
        ->onSeek([this](float p, bool from_user) {
            if (from_user) {
                int v = static_cast<int>(p * 100);
                MPDManager::set_volume(v, m_on_update_needed);
            }
        })
        ->build();
    m_volume_bar->get_layout_params().width = 75;

    auto vol_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->spacing(4)
        ->addView(m_btn_volume)
        ->addView(m_volume_bar)
        ->build();

    // Tabs
    m_tab_player = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NAV_PLAYER))
        ->fontFamily(font_icon)->iconSize(14)->circle(true)
        ->onClick([this]() { if (m_on_change_tab) m_on_change_tab(ViewMode::Player); })
        ->build();

    m_tab_queue = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NAV_QUEUE))
        ->fontFamily(font_icon)->iconSize(14)->circle(true)
        ->onClick([this]() { if (m_on_change_tab) m_on_change_tab(ViewMode::Queue); })
        ->build();

    m_tab_db = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NAV_DATABASE))
        ->fontFamily(font_icon)->iconSize(14)->circle(true)
        ->onClick([this]() { if (m_on_change_tab) m_on_change_tab(ViewMode::Database); })
        ->build();

    m_tab_playlists = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NAV_PLAYLIST))
        ->fontFamily(font_icon)->iconSize(14)->circle(true)
        ->onClick([this]() { if (m_on_change_tab) m_on_change_tab(ViewMode::Playlists); })
        ->build();

    m_tab_visualizer = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NAV_VISUALIZER))
        ->fontFamily(font_icon)->iconSize(14)->circle(true)
        ->onClick([this]() { if (m_on_change_tab) m_on_change_tab(ViewMode::Visualizer); })
        ->build();

    m_tab_ytdlp = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NAV_YTDLP))
        ->fontFamily(font_icon)->iconSize(14)->circle(true)
        ->onClick([this]() { if (m_on_change_tab) m_on_change_tab(ViewMode::YtDlp); })
        ->build();

    m_tab_settings = miqu::ImageButtonBuilder::create()
        ->icon(IconProvider::get_icon(IconType::NAV_SETTINGS))
        ->fontFamily(font_icon)->iconSize(14)->circle(true)
        ->onClick([this]() { if (m_on_change_tab) m_on_change_tab(ViewMode::Settings); })
        ->build();

    auto tabs_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->spacing(4)
        ->addView(m_tab_player)
        ->addView(m_tab_queue)
        ->addView(m_tab_db)
        ->addView(m_tab_playlists)
        ->addView(m_tab_visualizer)
        ->addView(m_tab_ytdlp)
        ->addView(m_tab_settings)
        ->build();
    tabs_row->set_margin(12, 0, 0, 0);

    auto right_section = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->addView(vol_row)
        ->addView(tabs_row)
        ->build();
    right_section->get_layout_params().width = 330;

    // Full Root Bar Layout
    auto content = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->spacing(12)
        ->padding(16, 8)
        ->addView(left_section)
        ->addView(center_section)
        ->addView(right_section)
        ->build();

    m_root_card = miqu::CardViewBuilder::create()
        ->backgroundColor(cfg.surface)
        ->stroke(cfg.border_width, cfg.border_color)
        ->cornerRadius(cfg.corner_radius)
        ->addView(content, miqu::LayoutParams(static_cast<int>(miqu::LayoutDimension::MatchParent), 76))
        ->build();
    m_root_card->set_margin(12, 6, 12, 12);
}

void PlaybackBar::set_active_tab(ViewMode mode) {
    const auto& cfg = MiquMusicConfig::get();
    auto reset_btn = [](const std::shared_ptr<miqu::ImageButton>& btn) {
        if (btn) btn->set_background_color(miqu::Color::transparent());
    };
    reset_btn(m_tab_player);
    reset_btn(m_tab_queue);
    reset_btn(m_tab_db);
    reset_btn(m_tab_playlists);
    reset_btn(m_tab_visualizer);
    reset_btn(m_tab_ytdlp);
    reset_btn(m_tab_settings);

    std::shared_ptr<miqu::ImageButton> active;
    switch (mode) {
    case ViewMode::Player: active = m_tab_player; break;
    case ViewMode::Queue: active = m_tab_queue; break;
    case ViewMode::Database: active = m_tab_db; break;
    case ViewMode::Playlists: active = m_tab_playlists; break;
    case ViewMode::Visualizer: active = m_tab_visualizer; break;
    case ViewMode::YtDlp: active = m_tab_ytdlp; break;
    case ViewMode::Settings: active = m_tab_settings; break;
    }
    if (active) {
        active->set_background_color(cfg.accent_color.with_alpha(0.35f));
    }
}

void PlaybackBar::update_state(const SongInfo& song, const MPDStatus& status, const std::string& art_path) {
    const auto& cfg = MiquMusicConfig::get();

    // Track Info
    if (m_title_text) m_title_text->set_text(song.display_title());
    if (m_artist_text) {
        std::string meta = song.display_artist();
        if (!song.album.empty()) meta += " • " + song.album;
        m_artist_text->set_text(meta);
    }

    if (m_mini_art && !art_path.empty()) {
        m_mini_art->set_image_resource(art_path);
    }

    // Play / Pause Icon
    if (m_btn_play_pause) {
        if (status.state == MPD_STATE_PLAY) {
            m_btn_play_pause->set_icon(IconProvider::get_icon(IconType::PAUSE));
        } else {
            m_btn_play_pause->set_icon(IconProvider::get_icon(IconType::PLAY));
        }
    }

    // Shuffle state
    if (m_btn_shuffle) {
        m_btn_shuffle->set_background_color(status.random ? cfg.accent_color.with_alpha(0.4f) : miqu::Color::transparent());
    }

    // Repeat state
    if (m_btn_repeat) {
        m_btn_repeat->set_background_color(status.repeat ? cfg.accent_color.with_alpha(0.4f) : miqu::Color::transparent());
    }

    // Consume state
    if (m_btn_consume) {
        m_btn_consume->set_background_color(status.consume ? cfg.accent_color.with_alpha(0.4f) : miqu::Color::transparent());
    }

    // Progress & Seek
    if (!m_is_user_seeking && m_seek_bar) {
        if (status.total_time > 0) {
            float p = static_cast<float>(status.elapsed_time) / static_cast<float>(status.total_time);
            m_seek_bar->set_progress(p);
        } else {
            m_seek_bar->set_progress(0.0f);
        }
    }

    if (m_elapsed_text) m_elapsed_text->set_text(format_duration(status.elapsed_time));
    if (m_total_text) m_total_text->set_text(format_duration(status.total_time));

    // Volume
    if (m_volume_bar) {
        m_volume_bar->set_progress(std::clamp(status.volume / 100.0f, 0.0f, 1.0f));
    }
    if (m_btn_volume) {
        m_btn_volume->set_icon(IconProvider::get_volume_icon(status.volume <= 0, status.volume));
    }
}

} // namespace miqumusic
