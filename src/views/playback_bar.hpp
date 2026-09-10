#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include "core/view_mode.hpp"
#include "mpd/mpd_client.hpp"
#include "custom_seek_bar.hpp"
#include <functional>

namespace miqumusic {

class PlaybackBar {
public:
    PlaybackBar(std::function<void(ViewMode)> on_change_tab, std::function<void()> on_update_needed);

    std::shared_ptr<miqu::View> get_view() { return m_root_card; }

    void update_state(const SongInfo& song, const MPDStatus& status, const std::string& art_path);
    void set_active_tab(ViewMode mode);

private:
    std::function<void(ViewMode)> m_on_change_tab;
    std::function<void()> m_on_update_needed;

    std::shared_ptr<miqu::CardView> m_root_card;

    // Left
    std::shared_ptr<miqu::CircleImageView> m_mini_art;
    std::shared_ptr<miqu::TextView> m_title_text;
    std::shared_ptr<miqu::TextView> m_artist_text;

    // Center
    std::shared_ptr<miqu::ImageButton> m_btn_shuffle;
    std::shared_ptr<miqu::ImageButton> m_btn_prev;
    std::shared_ptr<miqu::ImageButton> m_btn_play_pause;
    std::shared_ptr<miqu::ImageButton> m_btn_next;
    std::shared_ptr<miqu::ImageButton> m_btn_repeat;
    std::shared_ptr<miqu::ImageButton> m_btn_consume;

    std::shared_ptr<miqu::TextView> m_elapsed_text;
    std::shared_ptr<CustomSeekBar> m_seek_bar;
    std::shared_ptr<miqu::TextView> m_total_text;

    // Right
    std::shared_ptr<miqu::ImageButton> m_btn_volume;
    std::shared_ptr<miqu::SeekBar> m_volume_bar;

    std::shared_ptr<miqu::ImageButton> m_tab_player;
    std::shared_ptr<miqu::ImageButton> m_tab_queue;
    std::shared_ptr<miqu::ImageButton> m_tab_db;
    std::shared_ptr<miqu::ImageButton> m_tab_playlists;
    std::shared_ptr<miqu::ImageButton> m_tab_visualizer;
    std::shared_ptr<miqu::ImageButton> m_tab_ytdlp;
    std::shared_ptr<miqu::ImageButton> m_tab_settings;

    bool m_is_user_seeking = false;
};

} // namespace miqumusic
