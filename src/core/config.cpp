#include "config.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace miqumusic {

namespace fs = std::filesystem;

static MiquMusicConfig s_config;

MiquMusicConfig& MiquMusicConfig::get() {
    return s_config;
}

static std::string trim_str(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(first, (last - first + 1));
}

static std::string expand_home(const std::string& path) {
    if (path.empty()) return path;
    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            return std::string(home) + path.substr(1);
        }
    }
    return path;
}

static miqu::Color parse_color(const std::string& input, const miqu::Color& fallback) {
    return miqu::Color::from_hex(trim_str(input), fallback);
}

std::string MiquMusicConfig::resolve_vars(const std::string& raw_val) const {
    std::string val = trim_str(raw_val);
    if (!val.empty() && (val[0] == '@' || val[0] == '$')) {
        auto it = variables.find(val.substr(1));
        if (it != variables.end()) return it->second;
    }
    auto it2 = variables.find(val);
    if (it2 != variables.end()) return it2->second;
    return val;
}

void MiquMusicConfig::load() {
    std::string user_conf = expand_home("~/.config/miqumusic/miqumusic.conf");
    if (fs::exists(user_conf)) {
        load_file(user_conf);
        return;
    }
    std::string sys_conf = "/usr/share/miqumusic/miqumusic.conf";
    if (fs::exists(sys_conf)) {
        load_file(sys_conf);
        return;
    }
    std::string local_conf = "assets/miqumusic.conf";
    if (fs::exists(local_conf)) {
        load_file(local_conf);
    }
}

void MiquMusicConfig::load_file(const std::string& filepath, int depth) {
    if (depth > 5) return;
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    std::string line;
    std::string current_section = "";

    while (std::getline(file, line)) {
        line = trim_str(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.rfind("include", 0) == 0) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                std::string inc = trim_str(line.substr(eq + 1));
                load_file(expand_home(inc), depth + 1);
            }
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            current_section = trim_str(line.substr(1, line.size() - 2));
            std::transform(current_section.begin(), current_section.end(), current_section.begin(), ::tolower);
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim_str(line.substr(0, eq));
        std::string val = trim_str(line.substr(eq + 1));

        if (!key.empty() && (key[0] == '@' || key[0] == '$')) {
            variables[key.substr(1)] = val;
            continue;
        }

        std::string lower_key = key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
        std::string resolved = resolve_vars(val);

        if (lower_key == "window_width" || lower_key == "width") {
            try { window_width = std::max(400, std::stoi(resolved)); } catch (...) {}
        } else if (lower_key == "window_height" || lower_key == "height") {
            try { window_height = std::max(300, std::stoi(resolved)); } catch (...) {}
        } else if (lower_key == "font_family" || lower_key == "font") {
            font_family = resolved;
        } else if (lower_key == "font_size") {
            try { font_size = std::max(6, std::stoi(resolved)); } catch (...) {}
        } else if (lower_key == "background" || lower_key == "bg") {
            background = parse_color(resolved, background);
        } else if (lower_key == "surface") {
            surface = parse_color(resolved, surface);
        } else if (lower_key == "border_color" || lower_key == "border") {
            border_color = parse_color(resolved, border_color);
        } else if (lower_key == "accent_color" || lower_key == "accent") {
            accent_color = parse_color(resolved, accent_color);
        } else if (lower_key == "corner_radius" || lower_key == "rounding") {
            try { corner_radius = std::max(0, std::stoi(resolved)); } catch (...) {}
        } else if (lower_key == "host") {
            mpd_host = resolved;
        } else if (lower_key == "port") {
            try { mpd_port = std::stoi(resolved); } catch (...) {}
        } else if (lower_key == "music_directory") {
            music_directory = expand_home(resolved);
        } else if (lower_key == "fifo_path") {
            fifo_path = resolved;
        }
    }
}

} // namespace miqumusic
