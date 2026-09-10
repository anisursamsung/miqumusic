#pragma once

#include "mpd_client.hpp"
#include <functional>
#include <mutex>
#include <memory>

namespace miqumusic {

class MPDManager {
public:
    static void ensure_mpd_running();
    static void run_command(const std::function<void(struct mpd_connection*)>& cmd);

    static void toggle_play_pause(std::function<void()> on_done = nullptr);
    static void prev_track(std::function<void()> on_done = nullptr);
    static void next_track(std::function<void()> on_done = nullptr);
    static void play_id(int song_id, std::function<void()> on_done = nullptr);
    static void seek(float pct, std::function<void()> on_done = nullptr);
    static void set_volume(int vol, std::function<void()> on_done = nullptr);

    static void toggle_random(std::function<void()> on_done = nullptr);
    static void toggle_repeat(std::function<void()> on_done = nullptr);
    static void toggle_consume(std::function<void()> on_done = nullptr);

    static void add_to_queue(const std::string& uri, std::function<void()> on_done = nullptr);
    static void play_uri(const std::string& uri, std::function<void()> on_done = nullptr);
    static void remove_from_queue(int song_id, std::function<void()> on_done = nullptr);
    static void clear_queue(std::function<void()> on_done = nullptr);

    static void load_playlist(const std::string& name, std::function<void()> on_done = nullptr);
    static void delete_playlist(const std::string& name, std::function<void()> on_done = nullptr);
    static void update_database(std::function<void()> on_done = nullptr);
};

} // namespace miqumusic
