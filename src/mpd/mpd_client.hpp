#pragma once

#include <mpd/client.h>
#include <string>
#include <vector>
#include <unordered_set>

namespace miqumusic {

struct SongInfo {
    int id = -1;
    int pos = -1;
    std::string uri;
    std::string title;
    std::string artist;
    std::string album;
    unsigned int duration = 0;
    std::string date;

    std::string display_title() const {
        if (!title.empty()) return title;
        if (!uri.empty()) {
            size_t slash = uri.find_last_of("/\\");
            if (slash != std::string::npos) return uri.substr(slash + 1);
            return uri;
        }
        return "Unknown Title";
    }

    std::string display_artist() const {
        return !artist.empty() ? artist : "Unknown Artist";
    }
};

struct MPDStatus {
    enum mpd_state state = MPD_STATE_UNKNOWN;
    int volume = 50;
    bool repeat = false;
    bool random = false;
    bool single = false;
    bool consume = false;
    unsigned int queue_length = 0;
    unsigned int queue_version = 0;
    int song_id = -1;
    int song_pos = -1;
    unsigned int elapsed_time = 0;
    unsigned int total_time = 0;
    unsigned int kbit_rate = 0;
};

class MPDClient {
public:
    static SongInfo parse_song(const struct mpd_song* song);
    static SongInfo fetch_current_song(struct mpd_connection* conn);
    static MPDStatus fetch_status(struct mpd_connection* conn);
    static std::vector<SongInfo> fetch_queue(struct mpd_connection* conn);
    static std::vector<SongInfo> fetch_database(struct mpd_connection* conn, const std::string& query = "");
    static std::vector<std::string> fetch_playlists(struct mpd_connection* conn);
    static std::vector<SongInfo> fetch_playlist_songs(struct mpd_connection* conn, const std::string& playlist_name);
    static std::unordered_set<std::string> get_queue_uris(struct mpd_connection* conn);
};

} // namespace miqumusic
