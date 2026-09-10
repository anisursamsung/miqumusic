#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include "mpd/mpd_client.hpp"
#include <functional>
#include <vector>

namespace miqumusic {

class DatabaseView {
public:
    DatabaseView(std::function<void()> on_action);

    std::shared_ptr<miqu::View> get_view() { return m_root; }
    void set_songs(std::vector<SongInfo> songs);

private:
    void filter_and_render();

    std::function<void()> m_on_action;
    std::shared_ptr<miqu::LinearLayout> m_root;
    std::shared_ptr<miqu::EditText> m_search_edit;
    std::shared_ptr<miqu::TextView> m_count_text;
    std::shared_ptr<miqu::GridView> m_grid_view;

    std::vector<SongInfo> m_all_songs;
    std::string m_filter_query;
};

} // namespace miqumusic
