#include "music_app.hpp"
#include <miqutoolkit/miqutoolkit.hpp>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;
using namespace miqu;
using namespace miqumusic;

static std::string trim_str(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(first, (last - first + 1));
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: miqumusic\n\n"
                      << "Modern native MPD music player built with miqutoolkit.\n";
            return 0;
        }
    }

    auto engine = AppEngine::create();
    if (!engine) {
        std::cerr << "[miqumusic] Failed to initialize AppEngine. Is Wayland running?\n";
        return 1;
    }

    // 1. Resolve and bootstrap configuration file
    std::string user_conf = Config::ensure_user_config("miqumusic", "miqumusic.conf");
    std::string target_conf;
    if (!user_conf.empty() && fs::exists(user_conf)) {
        target_conf = user_conf;
    } else if (fs::exists("/usr/share/miqumusic/miqumusic.conf")) {
        target_conf = "/usr/share/miqumusic/miqumusic.conf";
    }

    std::string host = "";
    unsigned int port = 0;

    if (!target_conf.empty()) {
        // Overlay any toolkit appearance overrides (colors, fonts, metrics, icon_theme)
        Config::get()->load_from_file(target_conf);

        // Read MPD connection options
        std::ifstream file(target_conf);
        std::string line;
        while (std::getline(file, line)) {
            line = trim_str(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string k = trim_str(line.substr(0, eq));
            std::string v = trim_str(line.substr(eq + 1));
            size_t cp = v.find('#');
            if (cp != std::string::npos) v = trim_str(v.substr(0, cp));

            if (k == "mpd_host" && !v.empty()) {
                host = v;
            } else if (k == "mpd_port" && !v.empty()) {
                try { port = static_cast<unsigned int>(std::stoul(v)); } catch (...) {}
            }
        }
    }

    // Environment variables override config file
    const char* env_host = getenv("MPD_HOST");
    if (env_host && *env_host) host = env_host;
    const char* env_port = getenv("MPD_PORT");
    if (env_port && *env_port) port = static_cast<unsigned int>(std::atoi(env_port));

    MusicApp app(engine, host, port);
    if (!app.init()) {
        std::cerr << "[miqumusic] Failed to start MusicApp.\n";
        return 1;
    }

    return app.run();
}
