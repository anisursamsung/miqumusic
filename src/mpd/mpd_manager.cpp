#include "mpd_manager.hpp"
#include "core/config.hpp"
#include "utils/notification_mgr.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace miqumusic {

static std::mutex s_mpd_mutex;

void MPDManager::ensure_mpd_running() {
    const auto& cfg = MiquMusicConfig::get();
    struct mpd_connection* conn = mpd_connection_new(cfg.mpd_host.c_str(), cfg.mpd_port, 3000);
    bool connected = conn && (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS);
    if (conn) mpd_connection_free(conn);

    if (connected) return;

    // Try starting MPD if local
    std::string home = getenv("HOME") ? getenv("HOME") : "";
    std::string conf_path = home + "/.config/mpd/mpd.conf";
    if (!std::filesystem::exists(conf_path)) {
        std::filesystem::create_directories(home + "/.config/mpd/playlists");
        std::filesystem::create_directories(home + "/Music");

        std::ofstream file(conf_path);
        if (file.is_open()) {
            file << "music_directory     \"" << home << "/Music\"\n"
                 << "playlist_directory  \"" << home << "/.config/mpd/playlists\"\n"
                 << "db_file             \"" << home << "/.config/mpd/database\"\n"
                 << "log_file            \"" << home << "/.config/mpd/log\"\n"
                 << "pid_file            \"" << home << "/.config/mpd/pid\"\n"
                 << "state_file          \"" << home << "/.config/mpd/state\"\n"
                 << "sticker_file        \"" << home << "/.config/mpd/sticker.sql\"\n\n"
                 << "bind_to_address     \"127.0.0.1\"\n"
                 << "port                \"6600\"\n"
                 << "restore_paused      \"yes\"\n"
                 << "auto_update         \"yes\"\n\n"
                 << "audio_output {\n"
                 << "    type            \"pulse\"\n"
                 << "    name            \"PipeWire Sound Server\"\n"
                 << "}\n\n"
                 << "audio_output {\n"
                 << "    type            \"fifo\"\n"
                 << "    name            \"my_fifo\"\n"
                 << "    path            \"/tmp/mpd.fifo\"\n"
                 << "    format          \"44100:16:2\"\n"
                 << "}\n";
            file.close();
        }
    }

    std::string cmd = "mpd " + conf_path + " >/dev/null 2>&1 &";
    (void)::system(cmd.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

void MPDManager::run_command(const std::function<void(struct mpd_connection*)>& cmd) {
    std::lock_guard<std::mutex> lock(s_mpd_mutex);
    const auto& cfg = MiquMusicConfig::get();

    struct mpd_connection* conn = mpd_connection_new(cfg.mpd_host.c_str(), cfg.mpd_port, 4000);
    if (!conn) {
        std::cerr << "[miqumusic] Failed to allocate MPD connection\n";
        return;
    }

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        std::cerr << "[miqumusic] MPD Error: " << mpd_connection_get_error_message(conn) << "\n";
        mpd_connection_free(conn);
        return;
    }

    cmd(conn);
    mpd_connection_free(conn);
}

void MPDManager::toggle_play_pause(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        struct mpd_status* status = mpd_run_status(conn);
        if (status) {
            enum mpd_state state = mpd_status_get_state(status);
            if (state == MPD_STATE_PLAY) {
                mpd_run_pause(conn, true);
            } else {
                mpd_run_play(conn);
            }
            mpd_status_free(status);
        }
    });
    if (on_done) on_done();
}

void MPDManager::prev_track(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        mpd_run_previous(conn);
    });
    if (on_done) on_done();
}

void MPDManager::next_track(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        mpd_run_next(conn);
    });
    if (on_done) on_done();
}

void MPDManager::play_id(int song_id, std::function<void()> on_done) {
    run_command([song_id](struct mpd_connection* conn) {
        mpd_run_play_id(conn, song_id);
    });
    if (on_done) on_done();
}

void MPDManager::seek(float pct, std::function<void()> on_done) {
    run_command([pct](struct mpd_connection* conn) {
        struct mpd_status* status = mpd_run_status(conn);
        if (status) {
            unsigned int total = mpd_status_get_total_time(status);
            int song_pos = mpd_status_get_song_pos(status);
            if (total > 0 && song_pos >= 0) {
                unsigned int target_sec = static_cast<unsigned int>(total * pct);
                mpd_run_seek_pos(conn, song_pos, target_sec);
            }
            mpd_status_free(status);
        }
    });
    if (on_done) on_done();
}

void MPDManager::set_volume(int vol, std::function<void()> on_done) {
    int v = std::clamp(vol, 0, 100);
    run_command([v](struct mpd_connection* conn) {
        mpd_run_set_volume(conn, v);
    });
    if (on_done) on_done();
}

void MPDManager::toggle_random(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        struct mpd_status* s = mpd_run_status(conn);
        if (s) {
            bool r = mpd_status_get_random(s);
            mpd_run_random(conn, !r);
            mpd_status_free(s);
        }
    });
    if (on_done) on_done();
}

void MPDManager::toggle_repeat(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        struct mpd_status* s = mpd_run_status(conn);
        if (s) {
            bool rep = mpd_status_get_repeat(s);
            mpd_run_repeat(conn, !rep);
            mpd_status_free(s);
        }
    });
    if (on_done) on_done();
}

void MPDManager::toggle_consume(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        struct mpd_status* s = mpd_run_status(conn);
        if (s) {
            bool c = mpd_status_get_consume(s);
            mpd_run_consume(conn, !c);
            mpd_status_free(s);
        }
    });
    if (on_done) on_done();
}

void MPDManager::add_to_queue(const std::string& uri, std::function<void()> on_done) {
    run_command([uri](struct mpd_connection* conn) {
        mpd_run_add(conn, uri.c_str());
    });
    NotificationManager::notify("Queue Updated", "Added track to queue");
    if (on_done) on_done();
}

void MPDManager::play_uri(const std::string& uri, std::function<void()> on_done) {
    run_command([uri](struct mpd_connection* conn) {
        int id = mpd_run_add_id(conn, uri.c_str());
        if (id >= 0) {
            mpd_run_play_id(conn, id);
        }
    });
    if (on_done) on_done();
}

void MPDManager::remove_from_queue(int song_id, std::function<void()> on_done) {
    run_command([song_id](struct mpd_connection* conn) {
        mpd_run_delete_id(conn, song_id);
    });
    if (on_done) on_done();
}

void MPDManager::clear_queue(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        mpd_run_clear(conn);
    });
    if (on_done) on_done();
}

void MPDManager::load_playlist(const std::string& name, std::function<void()> on_done) {
    run_command([name](struct mpd_connection* conn) {
        mpd_run_load(conn, name.c_str());
    });
    NotificationManager::notify("Playlist Loaded", name);
    if (on_done) on_done();
}

void MPDManager::delete_playlist(const std::string& name, std::function<void()> on_done) {
    run_command([name](struct mpd_connection* conn) {
        mpd_run_rm(conn, name.c_str());
    });
    if (on_done) on_done();
}

void MPDManager::update_database(std::function<void()> on_done) {
    run_command([](struct mpd_connection* conn) {
        mpd_run_update(conn, "");
    });
    NotificationManager::notify("Database", "Updating MPD database...");
    if (on_done) on_done();
}

} // namespace miqumusic
