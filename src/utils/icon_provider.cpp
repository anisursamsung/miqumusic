#include "icon_provider.hpp"
#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace UI::Components {
    extern const unsigned char g_miqumusicFontData[];
    extern const size_t g_miqumusicFontDataLen;
}

namespace miqumusic {

static bool s_font_registered = false;

void IconProvider::register_font() {
    if (s_font_registered) return;

    FcConfig* config = FcConfigGetCurrent();
    if (!config) {
        config = FcInitLoadConfigAndFonts();
        FcConfigSetCurrent(config);
    }

    std::filesystem::path font_path = std::filesystem::temp_directory_path() / "miqumusic_embedded_font.ttf";

    std::ofstream ofs(font_path, std::ios::binary);
    if (ofs) {
        ofs.write(reinterpret_cast<const char*>(UI::Components::g_miqumusicFontData),
                  UI::Components::g_miqumusicFontDataLen);
        ofs.close();

        std::string abs_path = std::filesystem::absolute(font_path).string();
        if (FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(abs_path.c_str())) == FcTrue) {
            FcConfigBuildFonts(config);
            pango_cairo_font_map_set_default(NULL);
            s_font_registered = true;
        }
    }
}

std::string IconProvider::get_font_family() {
    register_font();
    return "hyprmusic"; // Internal name defined in font file metadata
}

std::string IconProvider::get_icon(IconType type) {
    switch (type) {
    case IconType::PLAY:            return "\uE90C";
    case IconType::PAUSE:           return "\uE910";
    case IconType::PREV_TRACK:      return "\uE91A";
    case IconType::NEXT_TRACK:      return "\uE90D";
    case IconType::SHUFFLE:         return "\uE914";
    case IconType::REPEAT_OFF:      return "\uE91D";
    case IconType::REPEAT_ONCE:     return "\uE91E";
    case IconType::REPEAT_ALL:      return "\uE91F";
    case IconType::CONSUME:         return "\uE921";
    case IconType::VOLUME_MUTED:    return "\uE917";
    case IconType::VOLUME_LOW:      return "\uE916";
    case IconType::VOLUME_MEDIUM:   return "\uE916";
    case IconType::VOLUME_HIGH:     return "\uE918";
    case IconType::NAV_PLAYER:      return "\uE909";
    case IconType::NAV_QUEUE:       return "\uE911";
    case IconType::NAV_DATABASE:    return "\uE904";
    case IconType::NAV_PLAYLIST:    return "\uE90F";
    case IconType::NAV_YTDLP:       return "\uE919";
    case IconType::NAV_VISUALIZER:  return "\uE920";
    case IconType::NAV_SETTINGS:    return "\uE913";
    case IconType::ADD:             return "\uE900";
    case IconType::ADD_TO_QUEUE:    return "\uE901";
    case IconType::ADD_TO_PLAYLIST: return "\uE90E";
    case IconType::DELETE:          return "\uE905";
    case IconType::CLEAR:           return "\uE902";
    case IconType::SEARCH:          return "\uE904";
    case IconType::FOLDER:          return "\uE908";
    case IconType::MUSIC_NOTE:      return "\uE909";
    case IconType::DOWNLOAD:        return "\uE91C";
    case IconType::REFRESH:         return "\uE904";
    }
    return "▪";
}

std::string IconProvider::get_volume_icon(bool muted, int vol) {
    if (muted || vol == 0) return get_icon(IconType::VOLUME_MUTED);
    if (vol <= 50) return get_icon(IconType::VOLUME_LOW);
    return get_icon(IconType::VOLUME_HIGH);
}

} // namespace miqumusic
