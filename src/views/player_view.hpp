#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include "mpd/mpd_client.hpp"

namespace miqumusic {

class PlayerView {
public:
    PlayerView();

    std::shared_ptr<miqu::View> get_view() { return m_root; }
    void update_state(const SongInfo& song, const MPDStatus& status, const std::string& art_path);

private:
    std::shared_ptr<miqu::LinearLayout> m_root;
    std::shared_ptr<miqu::ImageView> m_artwork_view;
    std::shared_ptr<miqu::TextView> m_title_text;
    std::shared_ptr<miqu::TextView> m_artist_text;
    std::shared_ptr<miqu::TextView> m_album_text;
    std::shared_ptr<miqu::TextView> m_badge_text;
    std::shared_ptr<miqu::CardView> m_badge_card;
};

} // namespace miqumusic
