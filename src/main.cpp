#include "music_app.hpp"
#include <miqutoolkit/miqutoolkit.hpp>
#include <iostream>
#include <cstdlib>

using namespace miqu;
using namespace miqumusic;

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    auto engine = AppEngine::create();
    if (!engine) {
        std::cerr << "[miqumusic] Failed to initialize AppEngine. Is Wayland running?\n";
        return 1;
    }

    std::string host = "";
    unsigned int port = 0;

    const char* env_host = getenv("MPD_HOST");
    if (env_host) host = env_host;
    const char* env_port = getenv("MPD_PORT");
    if (env_port) port = static_cast<unsigned int>(std::atoi(env_port));

    MusicApp app(engine, host, port);
    if (!app.init()) {
        std::cerr << "[miqumusic] Failed to start MusicApp.\n";
        return 1;
    }

    return app.run();
}
