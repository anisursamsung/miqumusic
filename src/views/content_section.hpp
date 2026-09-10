#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include "mpd/mpd_client.hpp"
#include <memory>
#include <vector>
#include <string>

namespace miqumusic {

struct PlaylistRowItem {
    bool is_playlist_header = false;
    std::string playlist_name;
    SongInfo song;
};

class ContentSection {
public:
    ContentSection();

    std::shared_ptr<miqu::View> get_view() const { return m_root; }
    void select_tab(int index);
    int get_selected_tab() const { return m_selected_tab; }

private:
    std::shared_ptr<miqu::View> create_queue_view();
    std::shared_ptr<miqu::View> create_database_view();
    std::shared_ptr<miqu::View> create_playlists_view();
    std::shared_ptr<miqu::View> create_ytdlp_view();
    std::shared_ptr<miqu::View> create_settings_view();
    std::shared_ptr<miqu::View> create_placeholder_view(const std::string& name);

    void repopulate_playlists_grid(std::shared_ptr<miqu::GridView> grid, const std::vector<std::string>& playlists);
    std::vector<SongInfo> get_playlist_songs(const std::string& name);

    std::shared_ptr<miqu::CardView> m_root;
    std::shared_ptr<miqu::LinearLayout> m_tab_bar;
    std::shared_ptr<miqu::FrameLayout> m_content_area;

    std::vector<std::shared_ptr<miqu::Button>> m_tab_buttons;
    std::vector<std::string> m_tab_names{"Que", "Db", "PList", "YDlp", "Settings"};
    int m_selected_tab = 0;

    std::string m_expanded_playlist = "";
    std::vector<PlaylistRowItem> m_playlist_row_items;
};

} // namespace miqumusic
