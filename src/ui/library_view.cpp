#include "library_view.hpp"

using namespace miqu;

namespace miqumusic {

LibraryView::LibraryView() : LinearLayout(Orientation::Vertical) {
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

void LibraryView::load_songs(const std::vector<Song>& songs) {
    m_items_layout->clear_views();

    for (const auto& s : songs) {
        auto row = std::make_shared<LinearLayout>(Orientation::Horizontal);
        row->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));
        row->set_padding(8, 6);
        row->set_margin(0, 1, 0, 1);

        auto icon_lbl = TextViewBuilder::create()
            ->text("🎵")
            ->caption(true)
            ->muted(true)
            ->build();
        icon_lbl->set_layout_params(LayoutParams(
            24,
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
            ->bold(false)
            ->ellipsize(true)
            ->build();
        t_lbl->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        auto a_lbl = TextViewBuilder::create()
            ->text(s.display_artist() + " • " + s.display_album())
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
            ->muted(true)
            ->textAlignment(TextAlignment::Right)
            ->build();
        dur_lbl->set_layout_params(LayoutParams(
            44,
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        row->add_view(icon_lbl);
        row->add_view(text_col);
        row->add_view(dur_lbl);

        std::string uri = s.uri;
        row->set_on_click_listener([this, uri]() {
            if (m_on_song_selected) m_on_song_selected(uri);
        });

        m_items_layout->add_view(row);
    }

    m_loaded = true;
}

} // namespace miqumusic
