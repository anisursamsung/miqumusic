#include "artists_view.hpp"
#include <map>

using namespace miqu;

namespace miqumusic {

ArtistsView::ArtistsView() : LinearLayout(Orientation::Vertical) {
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

void ArtistsView::load_artists(const std::vector<Song>& all_songs) {
    m_items_layout->clear_views();

    std::map<std::string, std::vector<Song>> artists_map;
    for (const auto& s : all_songs) {
        artists_map[s.display_artist()].push_back(s);
    }

    for (const auto& pair : artists_map) {
        const std::string& artist_name = pair.first;
        const auto& songs = pair.second;

        auto row = std::make_shared<LinearLayout>(Orientation::Horizontal);
        row->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));
        row->set_padding(10, 8);
        row->set_margin(0, 1, 0, 1);

        auto icon_lbl = TextViewBuilder::create()
            ->text("👤")
            ->caption(true)
            ->muted(true)
            ->build();
        icon_lbl->set_layout_params(LayoutParams(
            26,
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        auto text_col = std::make_shared<LinearLayout>(Orientation::Vertical);
        text_col->set_layout_params(LayoutParams(
            0,
            static_cast<int>(LayoutDimension::WrapContent),
            1.0f
        ));
        text_col->set_margin(6, 0, 6, 0);

        auto name_lbl = TextViewBuilder::create()
            ->text(artist_name)
            ->bold(true)
            ->ellipsize(true)
            ->build();
        name_lbl->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        auto count_lbl = TextViewBuilder::create()
            ->text(std::to_string(songs.size()) + (songs.size() == 1 ? " track" : " tracks"))
            ->caption(true)
            ->muted(true)
            ->build();
        count_lbl->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        text_col->add_view(name_lbl);
        text_col->add_view(count_lbl);

        auto play_btn = ButtonBuilder::create()
            ->text("▶")
            ->flat(true)
            ->padding(6, 4)
            ->build();

        row->add_view(icon_lbl);
        row->add_view(text_col);
        row->add_view(play_btn);

        std::string first_uri = songs.empty() ? "" : songs[0].uri;
        auto play_action = [this, first_uri]() {
            if (m_on_artist_play && !first_uri.empty()) {
                m_on_artist_play(first_uri);
            }
        };

        row->set_on_click_listener(play_action);
        play_btn->set_on_click_listener(play_action);

        m_items_layout->add_view(row);
    }

    m_loaded = true;
}

} // namespace miqumusic
