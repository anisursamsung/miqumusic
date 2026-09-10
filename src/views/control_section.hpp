#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include <memory>

namespace miqumusic {

class ControlSection {
public:
    ControlSection();

    std::shared_ptr<miqu::View> get_view() const { return m_root; }
    std::shared_ptr<miqu::View> get_left_square() const { return m_left_square; }
    std::shared_ptr<miqu::View> get_right_section() const { return m_right_section; }

    std::shared_ptr<miqu::View> get_top_row() const { return m_row_top; }
    std::shared_ptr<miqu::View> get_mid_row() const { return m_row_mid; }
    std::shared_ptr<miqu::View> get_bot_row() const { return m_row_bot; }

    static constexpr int DEFAULT_HEIGHT = 128;

private:
    std::shared_ptr<miqu::CardView> m_root;
    std::shared_ptr<miqu::CardView> m_left_square;
    std::shared_ptr<miqu::View> m_right_section;

    std::shared_ptr<miqu::View> m_row_top;
    std::shared_ptr<miqu::View> m_row_mid;
    std::shared_ptr<miqu::View> m_row_bot;
};

} // namespace miqumusic
