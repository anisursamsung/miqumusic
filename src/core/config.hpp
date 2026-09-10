#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include <string>
#include <unordered_map>

namespace miqumusic {

struct MiquMusicConfig {
    int window_width = 960;
    int window_height = 680;

    std::string font_family = "Sans";
    int font_size = 11;

    miqu::Color background = miqu::Color::rgba(0.07f, 0.08f, 0.11f, 0.94f);
    miqu::Color surface = miqu::Color::rgba(0.10f, 0.12f, 0.16f, 0.88f);
    miqu::Color border_color = miqu::Color::rgba(1.0f, 1.0f, 1.0f, 0.12f);
    int border_width = 1;
    int corner_radius = 16;
    miqu::Color accent_color = miqu::Color::rgba(0.20f, 0.71f, 0.90f, 1.0f);

    std::string mpd_host = "127.0.0.1";
    int mpd_port = 6600;
    std::string music_directory = "~/Music";
    std::string fifo_path = "/tmp/mpd.fifo";

    std::unordered_map<std::string, std::string> variables;

    static MiquMusicConfig& get();
    void load();
    void load_file(const std::string& filepath, int depth = 0);
    std::string resolve_vars(const std::string& raw) const;
};

} // namespace miqumusic
