#pragma once

#include <string>
#include <mpd/client.h>

namespace miqumusic {

class ArtworkUtils {
public:
    static std::string get_default_artwork_path();
    static std::string resolve_track_artwork(struct mpd_connection* conn, const std::string& song_uri, const std::string& music_dir);
    static void clear_cache();
};

} // namespace miqumusic
