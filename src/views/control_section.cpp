#include "control_section.hpp"
#include "core/config.hpp"

namespace miqumusic {

ControlSection::ControlSection() {
    const auto& cfg = MiquMusicConfig::get();

    int total_h = ControlSection::DEFAULT_HEIGHT; // 128px
    int pad_v = 14;
    int pad_h = 16;
    int square_size = total_h - (pad_v * 2); // 100x100 square

    // 1. Left small square shaped container (100x100)
    auto square_text = miqu::TextViewBuilder::create()
        ->text("Square")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->bold(true)
        ->textColor(miqu::Color::rgba(1.0f, 1.0f, 1.0f, 0.7f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();

    auto square_inner = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::Center)
        ->addView(square_text)
        ->build();

    m_left_square = miqu::CardViewBuilder::create()
        ->backgroundColor(miqu::Color::rgba(0.16f, 0.18f, 0.26f, 1.0f))
        ->stroke(1, miqu::Color::rgba(0.35f, 0.40f, 0.55f, 0.5f))
        ->cornerRadius(8)
        ->addView(square_inner, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    // 2. Right section: 3 vertically stacked full width sections
    auto make_row = [&](const std::string& label, const miqu::Color& text_col, const miqu::Color& bg_col) {
        auto lbl = miqu::TextViewBuilder::create()
            ->text(label)
            ->fontFamily(cfg.font_family)
            ->textSize(11)
            ->bold(true)
            ->textColor(text_col)
            ->textAlignment(miqu::TextAlignment::Center)
            ->build();

        auto inner = miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Vertical)
            ->gravity(miqu::Gravity::Center)
            ->addView(lbl)
            ->build();

        auto card = miqu::CardViewBuilder::create()
            ->backgroundColor(bg_col)
            ->stroke(1, miqu::Color::rgba(0.30f, 0.35f, 0.45f, 0.35f))
            ->cornerRadius(6)
            ->addView(inner, miqu::LayoutParams(
                static_cast<int>(miqu::LayoutDimension::MatchParent),
                static_cast<int>(miqu::LayoutDimension::MatchParent)))
            ->build();

        card->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
        card->get_layout_params().weight = 1.0f;
        return card;
    };

    m_row_top = make_row("Top Section (Full Width)", miqu::Color::rgb(1.0f, 1.0f, 1.0f), miqu::Color::rgba(0.14f, 0.16f, 0.24f, 0.9f));
    m_row_mid = make_row("Middle Section (Full Width)", cfg.accent_color, miqu::Color::rgba(0.12f, 0.14f, 0.21f, 0.9f));
    m_row_bot = make_row("Bottom Section (Full Width)", miqu::Color::rgba(0.7f, 0.75f, 0.85f, 0.9f), miqu::Color::rgba(0.10f, 0.12f, 0.18f, 0.9f));

    auto right_v_layout = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->spacing(6)
        ->addView(m_row_top, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            0,
            1.0f))
        ->addView(m_row_mid, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            0,
            1.0f))
        ->addView(m_row_bot, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            0,
            1.0f))
        ->build();

    right_v_layout->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    right_v_layout->get_layout_params().height = square_size;
    right_v_layout->get_layout_params().weight = 1.0f;

    m_right_section = right_v_layout;

    // 3. Horizontal LinearLayout dividing ControlSection cleanly
    auto h_layout = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::CenterVertical)
        ->padding(pad_h, pad_v)
        ->spacing(14)
        ->addView(m_left_square, miqu::LayoutParams(
            square_size,
            square_size))
        ->addView(m_right_section, miqu::LayoutParams(
            0,
            square_size,
            1.0f))
        ->build();

    // 4. Outer ControlSection Card
    miqu::Color control_bg = miqu::Color::rgba(0.08f, 0.09f, 0.13f, 1.0f);
    miqu::Color stroke_col = miqu::Color::rgba(0.35f, 0.40f, 0.55f, 0.5f);

    m_root = miqu::CardViewBuilder::create()
        ->backgroundColor(control_bg)
        ->stroke(1, stroke_col)
        ->cornerRadius(0)
        ->addView(h_layout, miqu::LayoutParams(
            static_cast<int>(miqu::LayoutDimension::MatchParent),
            static_cast<int>(miqu::LayoutDimension::MatchParent)))
        ->build();

    m_root->get_layout_params().width = static_cast<int>(miqu::LayoutDimension::MatchParent);
    m_root->get_layout_params().height = total_h;
}

} // namespace miqumusic
