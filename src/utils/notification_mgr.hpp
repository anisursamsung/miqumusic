#pragma once

#include <string>
#include <cstdlib>

namespace miqumusic {

class NotificationManager {
public:
    static void notify(const std::string& title, const std::string& body) {
        if (title.empty() && body.empty()) return;

        std::string safe_title;
        for (char c : title) {
            if (c == '\'') safe_title += "'\"'\"'";
            else safe_title += c;
        }

        std::string safe_body;
        for (char c : body) {
            if (c == '\'') safe_body += "'\"'\"'";
            else safe_body += c;
        }

        std::string cmd = "notify-send -a \"miqumusic\" -i \"multimedia-audio-player\" '"
                        + safe_title + "' '" + safe_body + "' &";
        (void)::system(cmd.c_str());
    }
};

} // namespace miqumusic
