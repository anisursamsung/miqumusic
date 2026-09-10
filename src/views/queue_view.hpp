#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include "mpd/mpd_client.hpp"
#include <functional>

namespace miqumusic {

class QueueView {
public:
    QueueView(std::function<void()> on_queue_modified);

    std::shared_ptr<miqu::View> get_view() { return m_root; }
    void refresh_queue(const std::vector<SongInfo>& songs, int active_song_id);

private:
    std::function<void()> m_on_queue_modified;
    std::shared_ptr<miqu::LinearLayout> m_root;
    std::shared_ptr<miqu::TextView> m_header_text;
    std::shared_ptr<miqu::GridView> m_grid_view;
};

} // namespace miqumusic
