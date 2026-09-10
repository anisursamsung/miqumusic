#include "mpd_client.hpp"
#include <iostream>
#include <algorithm>

namespace miqumusic {

SongInfo MPDClient::parse_song(const struct mpd_song* song) {
    SongInfo info;
    if (!song) return info;

    info.id = mpd_song_get_id(song);
    info.pos = mpd_song_get_pos(song);

    const char* uri = mpd_song_get_uri(song);
    if (uri) info.uri = uri;

    const char* title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if (title) info.title = title;

    const char* artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if (artist) info.artist = artist;

    const char* album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if (album) info.album = album;

    const char* date = mpd_song_get_tag(song, MPD_TAG_DATE, 0);
    if (date) info.date = date;

    info.duration = mpd_song_get_duration(song);
    return info;
}

SongInfo MPDClient::fetch_current_song(struct mpd_connection* conn) {
    if (!conn) return {};
    struct mpd_song* song = mpd_run_current_song(conn);
    if (!song) return {};
    SongInfo info = parse_song(song);
    mpd_song_free(song);
    return info;
}

MPDStatus MPDClient::fetch_status(struct mpd_connection* conn) {
    MPDStatus s;
    if (!conn) return s;

    struct mpd_status* status = mpd_run_status(conn);
    if (!status) return s;

    s.state = mpd_status_get_state(status);
    s.volume = mpd_status_get_volume(status);
    s.repeat = mpd_status_get_repeat(status);
    s.random = mpd_status_get_random(status);
    s.single = mpd_status_get_single(status);
    s.consume = mpd_status_get_consume(status);
    s.queue_length = mpd_status_get_queue_length(status);
    s.queue_version = mpd_status_get_queue_version(status);
    s.song_id = mpd_status_get_song_id(status);
    s.song_pos = mpd_status_get_song_pos(status);
    s.elapsed_time = mpd_status_get_elapsed_time(status);
    s.total_time = mpd_status_get_total_time(status);
    s.kbit_rate = mpd_status_get_kbit_rate(status);

    mpd_status_free(status);
    return s;
}

std::vector<SongInfo> MPDClient::fetch_queue(struct mpd_connection* conn) {
    std::vector<SongInfo> list;
    if (!conn) return list;

    if (!mpd_send_list_queue_meta(conn)) return list;

    struct mpd_song* song;
    while ((song = mpd_recv_song(conn)) != nullptr) {
        list.push_back(parse_song(song));
        mpd_song_free(song);
    }
    mpd_response_finish(conn);
    return list;
}

std::vector<SongInfo> MPDClient::fetch_database(struct mpd_connection* conn, const std::string& query) {
    std::vector<SongInfo> list;
    if (!conn) return list;

    if (query.empty()) {
        if (!mpd_send_list_all_meta(conn, "")) return list;
    } else {
        if (!mpd_search_db_songs(conn, false) ||
            !mpd_search_add_any_tag_constraint(conn, MPD_OPERATOR_DEFAULT, query.c_str()) ||
            !mpd_search_commit(conn)) {
            return list;
        }
    }

    struct mpd_entity* entity;
    while ((entity = mpd_recv_entity(conn)) != nullptr) {
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            const struct mpd_song* song = mpd_entity_get_song(entity);
            list.push_back(parse_song(song));
        }
        mpd_entity_free(entity);
    }
    mpd_response_finish(conn);
    return list;
}

std::vector<std::string> MPDClient::fetch_playlists(struct mpd_connection* conn) {
    std::vector<std::string> list;
    if (!conn) return list;

    if (!mpd_send_list_playlists(conn)) return list;

    struct mpd_playlist* pl;
    while ((pl = mpd_recv_playlist(conn)) != nullptr) {
        const char* path = mpd_playlist_get_path(pl);
        if (path) list.push_back(path);
        mpd_playlist_free(pl);
    }
    mpd_response_finish(conn);
    return list;
}

std::vector<SongInfo> MPDClient::fetch_playlist_songs(struct mpd_connection* conn, const std::string& playlist_name) {
    std::vector<SongInfo> list;
    if (!conn || playlist_name.empty()) return list;

    if (!mpd_send_list_playlist_meta(conn, playlist_name.c_str())) return list;

    struct mpd_song* song;
    while ((song = mpd_recv_song(conn)) != nullptr) {
        list.push_back(parse_song(song));
        mpd_song_free(song);
    }
    mpd_response_finish(conn);
    return list;
}

std::unordered_set<std::string> MPDClient::get_queue_uris(struct mpd_connection* conn) {
    std::unordered_set<std::string> set;
    if (!conn) return set;

    if (!mpd_send_list_queue_meta(conn)) return set;

    struct mpd_song* song;
    while ((song = mpd_recv_song(conn)) != nullptr) {
        const char* uri = mpd_song_get_uri(song);
        if (uri) set.insert(uri);
        mpd_song_free(song);
    }
    mpd_response_finish(conn);
    return set;
}

} // namespace miqumusic
