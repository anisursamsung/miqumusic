#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include "mpd/mpd_client.hpp"
#include <functional>
#include <vector>
#include <string>

namespace miqumusic {

class PlaylistsView {
public:
    PlaylistsView(std::function<void(const std::string&)> on_select_playlist, std::function<void()> on_action);

    std::shared_ptr<miqu::View> get_view() { return m_root; }
    void set_playlists(const std::vector<std::string>& playlists);
    void set_playlist_songs(const std::string& name, const std::vector<SongInfo>& songs);

private:
    std::function<void(const std::string&)> m_on_select_playlist;
    std::function<void()> m_on_action;

    std::shared_ptr<miqu::LinearLayout> m_root;
    std::shared_ptr<miqu::TextView> m_left_header;
    std::shared_ptr<miqu::GridView> m_left_grid;

    std::shared_ptr<miqu::TextView> m_right_header;
    std::shared_ptr<miqu::GridView> m_right_grid;

    std::string m_selected_playlist;
};

} // namespace miqumusic
