#include "queue_view.hpp"

using namespace miqu;

namespace miqumusic {

QueueView::QueueView() : LinearLayout(Orientation::Vertical) {
    set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));

    m_items_layout = std::make_shared<LinearLayout>(Orientation::Vertical);
    m_items_layout->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    m_items_layout->set_padding(0, 4, 0, 20);

    auto scroll = ScrollViewBuilder::create()
        ->contentView(m_items_layout)
        ->scrollbar(true)
        ->build();
    scroll->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    add_view(scroll);
}

void QueueView::update_queue(const std::vector<Song>& queue, int current_pos) {
    m_items_layout->clear_views();
    auto current_cfg = Config::get();

    for (size_t i = 0; i < queue.size(); ++i) {
        const auto& s = queue[i];
        bool is_active = (static_cast<int>(s.pos) == current_pos);

        auto row = std::make_shared<LinearLayout>(Orientation::Horizontal);
        row->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));
        row->set_padding(8, 6);
        row->set_margin(0, 1, 0, 1);

        if (is_active) {
            row->set_background_color(current_cfg->colors.primary_container.with_alpha(0.80f));
            row->set_corner_radius(8);
        }

        auto num_lbl = TextViewBuilder::create()
            ->text("#" + std::to_string(i + 1))
            ->caption(true)
            ->muted(!is_active)
            ->bold(is_active)
            ->build();
        num_lbl->set_layout_params(LayoutParams(
            28,
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        auto text_col = std::make_shared<LinearLayout>(Orientation::Vertical);
        text_col->set_layout_params(LayoutParams(
            0,
            static_cast<int>(LayoutDimension::WrapContent),
            1.0f
        ));
        text_col->set_margin(4, 0, 6, 0);

        auto t_lbl = TextViewBuilder::create()
            ->text(s.display_title())
            ->bold(is_active)
            ->ellipsize(true)
            ->build();
        t_lbl->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        auto a_lbl = TextViewBuilder::create()
            ->text(s.display_artist())
            ->caption(true)
            ->muted(true)
            ->ellipsize(true)
            ->build();
        a_lbl->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        text_col->add_view(t_lbl);
        text_col->add_view(a_lbl);

        auto dur_lbl = TextViewBuilder::create()
            ->text(s.formatted_duration())
            ->caption(true)
            ->muted(!is_active)
            ->textAlignment(TextAlignment::Right)
            ->build();
        dur_lbl->set_layout_params(LayoutParams(
            44,
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        row->add_view(num_lbl);
        row->add_view(text_col);
        row->add_view(dur_lbl);

        unsigned int pos = s.pos;
        row->set_on_click_listener([this, pos]() {
            if (m_on_song_clicked) m_on_song_clicked(pos);
        });

        m_items_layout->add_view(row);
    }
}

} // namespace miqumusic
