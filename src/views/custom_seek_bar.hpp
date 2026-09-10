#pragma once

#include <miqutoolkit/miqutoolkit.hpp>

namespace miqumusic {

class CustomSeekBar : public miqu::SeekBar {
public:
    CustomSeekBar() {
        set_track_height(5);
        set_thumb_radius(6);
        set_thumb_visible(true);
    }
};

} // namespace miqumusic
