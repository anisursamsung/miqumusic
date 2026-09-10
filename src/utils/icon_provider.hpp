#pragma once

#include <string>

namespace miqumusic {

enum class IconType {
    PLAY,
    PAUSE,
    PREV_TRACK,
    NEXT_TRACK,
    SHUFFLE,
    REPEAT_OFF,
    REPEAT_ONCE,
    REPEAT_ALL,
    CONSUME,
    VOLUME_MUTED,
    VOLUME_LOW,
    VOLUME_MEDIUM,
    VOLUME_HIGH,
    NAV_PLAYER,
    NAV_QUEUE,
    NAV_DATABASE,
    NAV_PLAYLIST,
    NAV_YTDLP,
    NAV_VISUALIZER,
    NAV_SETTINGS,
    ADD,
    ADD_TO_QUEUE,
    ADD_TO_PLAYLIST,
    DELETE,
    CLEAR,
    SEARCH,
    FOLDER,
    MUSIC_NOTE,
    DOWNLOAD,
    REFRESH,
};

class IconProvider {
public:
    static void register_font();
    static std::string get_font_family();
    static std::string get_icon(IconType type);
    static std::string get_volume_icon(bool muted, int vol);
};

} // namespace miqumusic
