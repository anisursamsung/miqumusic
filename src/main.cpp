#include "mpd_client.hpp"
#include "art_loader.hpp"
#include "mpd_types.hpp"
#include <miqutoolkit/miqutoolkit.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <cmath>
#include <map>

using namespace miqu;
using namespace miqumusic;

int main(int argc, char** argv) {
    auto engine = AppEngine::create();
    if (!engine) {
        std::cerr << "[miqumusic] Failed to initialize AppEngine.\n";
        return 1;
    }

    auto client = std::make_shared<MpdClient>();
    std::string host = "";
    unsigned int port = 0;

    const char* env_host = getenv("MPD_HOST");
    if (env_host) host = env_host;
    const char* env_port = getenv("MPD_PORT");
    if (env_port) port = static_cast<unsigned int>(std::atoi(env_port));

    bool initial_connected = client->connect(host, port);
    if (!initial_connected) {
        std::cerr << "[miqumusic] Warning: Could not connect to MPD on startup. Will keep retrying.\n";
    }

    auto config = Config::get();

    // =========================================================================
    // STATE TRACKING
    // =========================================================================
    std::atomic<bool> app_running{true};
    std::atomic<bool> is_seeking{false};
    std::string current_art_path = "";
    std::string current_art_uri = "";
    MpdStatus last_status;
    Song last_song;
    std::vector<Song> last_queue;
    bool library_loaded = false;
    bool artists_loaded = false;

    // Window forward declaration
    std::shared_ptr<Window> window;

    // =========================================================================
    // UI BUILDER - ROOT CONTAINER (Matching miqugallery)
    // =========================================================================
    auto root_container = std::make_shared<LinearLayout>(Orientation::Vertical);
    root_container->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    root_container->set_padding(0);


    // -------------------------------------------------------------------------
    // 1. TOP TOOLBAR & NAVIGATION CONTROLLER (NavigationView)
    // -------------------------------------------------------------------------
    auto nav_view = NavigationViewBuilder::create()
        ->autoBack(true)
        ->backOnEscape(true)
        ->build();
    nav_view->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        0,
        1.0f
    ));

    std::function<void()> refresh_action;
    std::shared_ptr<ImageButton> btn_settings;

    auto toolbar = ToolbarBuilder::create()
        ->title("Music")
        ->subtitle(initial_connected ? "MPD • Connected" : "MPD • Offline")
        ->titleAlignment(TitleAlignment::Center)
        ->onBack([nav_view]() {
            nav_view->pop();
        })
        ->onRefresh([&refresh_action]() {
            if (refresh_action) refresh_action();
        })
        ->onClose([&app_running, client, engine]() {
            app_running.store(false);
            client->stop_idle_listener();
            engine->quit();
        })
        ->build();
    toolbar->set_back_visible(false);
    toolbar->set_margin(14, 12, 14, 8);
    root_container->add_view(toolbar);

    // -------------------------------------------------------------------------
    // 2. MASTER BOTTOM NAVIGATION BAR (Android Material 3 Style)
    // -------------------------------------------------------------------------
    auto bottom_nav = BottomNavigationViewBuilder::create()
        ->addItem("Player", "▶")
        ->addItem("Queue", "☰")
        ->addItem("Library", "🎵")
        ->addItem("Artists", "👤")
        ->selectedIndex(0)
        ->showDivider(true)
        ->barHeight(58)
        ->build();
    bottom_nav->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        58
    ));
    bottom_nav->set_margin(0, 0, 0, 0);

    // -------------------------------------------------------------------------
    // 3. MASTER VIEW PAGER (4 Dedicated Pages)
    // -------------------------------------------------------------------------
    auto view_pager = ViewPagerBuilder::create()->build();
    view_pager->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        0,
        1.0f
    ));
    view_pager->set_margin(14, 0, 14, 0);


    // =========================================================================
    // PAGE 0: NOW PLAYING (Dedicated Full-Height Player View)
    // =========================================================================
    auto player_layout = std::make_shared<LinearLayout>(Orientation::Vertical);
    player_layout->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    player_layout->set_padding(4, 2, 4, 0);

    // 1. Hero Album Art (Spacious 280x280 Framed & Elevated)
    auto art_frame = std::make_shared<FrameLayout>();
    art_frame->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        288
    ));
    art_frame->set_margin(0, 4, 0, 10);

    auto cover_image = ImageViewBuilder::create()
        ->imageResource("audio-x-generic")
        ->targetSize(280)
        ->cornerRadius(20)
        ->fitMode(FitMode::Cover)
        ->build();
    cover_image->set_layout_params(LayoutParams(
        280,
        280,
        Gravity::Center
    ));
    art_frame->add_view(cover_image);
    player_layout->add_view(art_frame);

    // 2. Track Title
    auto track_title = TextViewBuilder::create()
        ->text("No track playing")
        ->h1()
        ->bold(true)
        ->textAlignment(TextAlignment::Center)
        ->ellipsize(true)
        ->build();
    track_title->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    track_title->set_margin(0, 4, 0, 2);
    player_layout->add_view(track_title);

    // 3. Artist & Album
    auto track_artist = TextViewBuilder::create()
        ->text("Music Player Daemon")
        ->caption(true)
        ->muted(true)
        ->textAlignment(TextAlignment::Center)
        ->ellipsize(true)
        ->build();
    track_artist->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    track_artist->set_margin(0, 0, 0, 6);
    player_layout->add_view(track_artist);

    // 4. Audiophile Stream & Format Badge Row
    auto format_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    format_row->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterHorizontal
    ));
    format_row->set_margin(0, 0, 0, 14);

    auto format_badge = TextViewBuilder::create()
        ->text("")
        ->caption(true)
        ->muted(true)
        ->textAlignment(TextAlignment::Center)
        ->build();
    format_badge->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    format_row->add_view(format_badge);
    player_layout->add_view(format_row);

    // "Up Next" Queue Peek Strip
    auto up_next_card = std::make_shared<FrameLayout>();
    up_next_card->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    up_next_card->set_background_color(config->colors.surface_variant);
    up_next_card->set_corner_radius(12);
    up_next_card->set_padding(12, 7);
    up_next_card->set_margin(4, 0, 4, 12);

    auto up_next_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    up_next_row->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));

    auto up_next_badge = TextViewBuilder::create()
        ->text("⏭ Up Next:")
        ->caption(true)
        ->bold(true)
        ->build();
    up_next_badge->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    up_next_badge->set_margin(0, 0, 8, 0);

    auto up_next_title = TextViewBuilder::create()
        ->text("Queue is empty")
        ->caption(true)
        ->ellipsize(true)
        ->build();
    up_next_title->set_layout_params(LayoutParams(
        0,
        static_cast<int>(LayoutDimension::WrapContent),
        1.0f
    ));

    auto up_next_time = TextViewBuilder::create()
        ->text("")
        ->caption(true)
        ->muted(true)
        ->textAlignment(TextAlignment::Right)
        ->build();
    up_next_time->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    up_next_row->add_view(up_next_badge);
    up_next_row->add_view(up_next_title);
    up_next_row->add_view(up_next_time);
    up_next_card->add_view(up_next_row);
    player_layout->add_view(up_next_card);

    up_next_card->set_on_click_listener([client]() {
        client->next();
    });

    // Flexible vertical spacer (pins timeline & controls to bottom against bottom_nav)
    auto flex_spacer = std::make_shared<FrameLayout>();
    flex_spacer->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        0,
        1.0f
    ));
    player_layout->add_view(flex_spacer);

    // =========================================================================
    // BOTTOM REGION: INTERACTIVE TIMELINE & TRANSPORT CONTROL DECK
    // =========================================================================
    // Seek Bar Row: Elapsed + Slider + Total (Placed above controls)
    auto seek_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    seek_row->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));
    seek_row->set_margin(0, 4, 0, 8);

    auto seek_elapsed_text = TextViewBuilder::create()
        ->text("0:00")
        ->caption(true)
        ->muted(true)
        ->textAlignment(TextAlignment::Left)
        ->build();
    seek_elapsed_text->set_layout_params(LayoutParams(
        46,
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    auto seek_slider = SliderBuilder::create()
        ->progress(0.0f)
        ->trackHeight(7)
        ->thumbRadius(9)
        ->build();
    seek_slider->set_layout_params(LayoutParams(
        0,
        static_cast<int>(LayoutDimension::WrapContent),
        1.0f
    ));
    seek_slider->set_margin(6, 0, 6, 0);

    auto seek_total_text = TextViewBuilder::create()
        ->text("0:00")
        ->caption(true)
        ->muted(true)
        ->textAlignment(TextAlignment::Right)
        ->build();
    seek_total_text->set_layout_params(LayoutParams(
        42,
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    seek_row->add_view(seek_elapsed_text);
    seek_row->add_view(seek_slider);
    seek_row->add_view(seek_total_text);
    player_layout->add_view(seek_row);

    // Transport Controls Row: 🔀  ⏮  ▶  ⏭  🔁 (Symmetric 5-Button Deck)
    auto controls_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    controls_row->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    controls_row->set_gravity(Gravity::CenterVertical);
    controls_row->set_margin(0, 4, 0, 14);

    auto spacer1 = std::make_shared<FrameLayout>();
    spacer1->set_layout_params(LayoutParams(0, 1, 0.6f));

    auto btn_shuffle = ButtonBuilder::create()
        ->text("🔀")
        ->flat(true)
        ->textSize(18)
        ->padding(10, 8)
        ->cornerRadius(10)
        ->build();
    btn_shuffle->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));

    auto spacer2 = std::make_shared<FrameLayout>();
    spacer2->set_layout_params(LayoutParams(0, 1, 0.9f));

    auto btn_prev = ButtonBuilder::create()
        ->text("⏮")
        ->flat(true)
        ->textSize(22)
        ->padding(14, 8)
        ->cornerRadius(12)
        ->build();
    btn_prev->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));

    auto spacer3 = std::make_shared<FrameLayout>();
    spacer3->set_layout_params(LayoutParams(0, 1, 1.0f));

    auto btn_play = ButtonBuilder::create()
        ->text("▶")
        ->primary(true)
        ->textSize(24)
        ->bold(true)
        ->padding(2, 2)
        ->cornerRadius(32)
        ->build();
    btn_play->set_layout_params(LayoutParams(64, 64, Gravity::CenterVertical));

    auto spacer4 = std::make_shared<FrameLayout>();
    spacer4->set_layout_params(LayoutParams(0, 1, 1.0f));

    auto btn_next = ButtonBuilder::create()
        ->text("⏭")
        ->flat(true)
        ->textSize(22)
        ->padding(14, 8)
        ->cornerRadius(12)
        ->build();
    btn_next->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));

    auto spacer5 = std::make_shared<FrameLayout>();
    spacer5->set_layout_params(LayoutParams(0, 1, 0.9f));

    auto btn_repeat = ButtonBuilder::create()
        ->text("🔁")
        ->flat(true)
        ->textSize(18)
        ->padding(10, 8)
        ->cornerRadius(10)
        ->build();
    btn_repeat->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));

    auto spacer6 = std::make_shared<FrameLayout>();
    spacer6->set_layout_params(LayoutParams(0, 1, 0.6f));

    controls_row->add_view(spacer1);
    controls_row->add_view(btn_shuffle);
    controls_row->add_view(spacer2);
    controls_row->add_view(btn_prev);
    controls_row->add_view(spacer3);
    controls_row->add_view(btn_play);
    controls_row->add_view(spacer4);
    controls_row->add_view(btn_next);
    controls_row->add_view(spacer5);
    controls_row->add_view(btn_repeat);
    controls_row->add_view(spacer6);
    player_layout->add_view(controls_row);

    // =========================================================================
    // CENTERED VOLUME ROW:  🔈  [━━━━━●━━━━━]  🔊
    // =========================================================================
    auto volume_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
    volume_row->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));
    volume_row->set_gravity(Gravity::CenterVertical);
    volume_row->set_margin(0, 2, 0, 18);

    auto vol_spacer1 = std::make_shared<FrameLayout>();
    vol_spacer1->set_layout_params(LayoutParams(0, 1, 1.0f));

    auto btn_vol_down = ButtonBuilder::create()
        ->text("🔈")
        ->flat(true)
        ->textSize(15)
        ->padding(8, 6)
        ->cornerRadius(8)
        ->build();
    btn_vol_down->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));

    auto vol_slider = SliderBuilder::create()
        ->progress(0.8f)
        ->trackHeight(5)
        ->thumbRadius(7)
        ->build();
    vol_slider->set_layout_params(LayoutParams(
        220,
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));
    vol_slider->set_margin(8, 0, 8, 0);

    auto btn_vol_up = ButtonBuilder::create()
        ->text("🔊")
        ->flat(true)
        ->textSize(15)
        ->padding(8, 6)
        ->cornerRadius(8)
        ->build();
    btn_vol_up->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::WrapContent),
        static_cast<int>(LayoutDimension::WrapContent),
        Gravity::CenterVertical
    ));

    auto vol_spacer2 = std::make_shared<FrameLayout>();
    vol_spacer2->set_layout_params(LayoutParams(0, 1, 1.0f));

    volume_row->add_view(vol_spacer1);
    volume_row->add_view(btn_vol_down);
    volume_row->add_view(vol_slider);
    volume_row->add_view(btn_vol_up);
    volume_row->add_view(vol_spacer2);
    player_layout->add_view(volume_row);

    auto player_scroll = ScrollViewBuilder::create()
        ->contentView(player_layout)
        ->scrollbar(true)
        ->build();
    player_scroll->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    view_pager->add_page(player_scroll);

    // =========================================================================
    // PAGE 1: QUEUE
    // =========================================================================
    auto queue_items_layout = std::make_shared<LinearLayout>(Orientation::Vertical);
    queue_items_layout->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    auto queue_scroll = ScrollViewBuilder::create()
        ->contentView(queue_items_layout)
        ->scrollbar(true)
        ->build();
    queue_scroll->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    view_pager->add_page(queue_scroll);

    // =========================================================================
    // PAGE 2: LIBRARY
    // =========================================================================
    auto library_items_layout = std::make_shared<LinearLayout>(Orientation::Vertical);
    library_items_layout->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    auto library_scroll = ScrollViewBuilder::create()
        ->contentView(library_items_layout)
        ->scrollbar(true)
        ->build();
    library_scroll->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    view_pager->add_page(library_scroll);

    // =========================================================================
    // PAGE 3: ARTISTS
    // =========================================================================
    auto artists_items_layout = std::make_shared<LinearLayout>(Orientation::Vertical);
    artists_items_layout->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    auto artists_scroll = ScrollViewBuilder::create()
        ->contentView(artists_items_layout)
        ->scrollbar(true)
        ->build();
    artists_scroll->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    view_pager->add_page(artists_scroll);

    auto create_settings_view = [&]() -> std::shared_ptr<View> {
        auto settings_scroll_layout = std::make_shared<LinearLayout>(Orientation::Vertical);
        settings_scroll_layout->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));
        settings_scroll_layout->set_padding(8, 6, 8, 18);

        // Helper lambda to create a card section
        auto make_section_card = [](const std::string& title_text, const std::string& icon_text) {
            auto card = std::make_shared<CardView>();
            card->set_layout_params(LayoutParams(
                static_cast<int>(LayoutDimension::MatchParent),
                static_cast<int>(LayoutDimension::WrapContent)
            ));
            card->set_corner_radius(12);
            card->set_padding(14, 12);
            card->set_margin(0, 4, 0, 10);

            auto card_col = std::make_shared<LinearLayout>(Orientation::Vertical);
            card_col->set_layout_params(LayoutParams(
                static_cast<int>(LayoutDimension::MatchParent),
                static_cast<int>(LayoutDimension::WrapContent)
            ));

            auto header_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
            header_row->set_layout_params(LayoutParams(
                static_cast<int>(LayoutDimension::MatchParent),
                static_cast<int>(LayoutDimension::WrapContent),
                Gravity::CenterVertical
            ));
            header_row->set_margin(0, 0, 0, 8);

            auto header_icon = TextViewBuilder::create()
                ->text(icon_text)
                ->textSize(16)
                ->build();
            header_icon->set_layout_params(LayoutParams(
                static_cast<int>(LayoutDimension::WrapContent),
                static_cast<int>(LayoutDimension::WrapContent)
            ));
            header_icon->set_margin(0, 0, 8, 0);

            auto header_title = TextViewBuilder::create()
                ->text(title_text)
                ->bold(true)
                ->textSize(14)
                ->build();
            header_title->set_layout_params(LayoutParams(
                0,
                static_cast<int>(LayoutDimension::WrapContent),
                1.0f
            ));

            header_row->add_view(header_icon);
            header_row->add_view(header_title);
            card_col->add_view(header_row);

            return std::make_pair(card, card_col);
        };

        // 1. Connection Card
        auto [conn_card, conn_col] = make_section_card("MPD Server Connection", "📡");

        std::string server_str = (host.empty() ? "localhost" : host) + ":" + std::to_string(port > 0 ? port : 6600);
        auto conn_status_lbl = TextViewBuilder::create()
            ->text(client->is_connected() ? ("Status: Connected to " + server_str) : ("Status: Disconnected from " + server_str))
            ->caption(true)
            ->muted(true)
            ->build();
        conn_status_lbl->set_margin(0, 0, 0, 10);
        conn_col->add_view(conn_status_lbl);

        auto conn_btn_row = std::make_shared<LinearLayout>(Orientation::Horizontal);
        conn_btn_row->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        auto btn_reconnect = ButtonBuilder::create()
            ->text("↻ Reconnect")
            ->primary(true)
            ->textSize(12)
            ->padding(12, 6)
            ->cornerRadius(8)
            ->build();
        btn_reconnect->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::WrapContent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));
        btn_reconnect->set_margin(0, 0, 8, 0);

        auto btn_update_db = ButtonBuilder::create()
            ->text("🔄 Rescan Library")
            ->flat(true)
            ->textSize(12)
            ->padding(12, 6)
            ->cornerRadius(8)
            ->build();
        btn_update_db->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::WrapContent),
            static_cast<int>(LayoutDimension::WrapContent)
        ));

        conn_btn_row->add_view(btn_reconnect);
        conn_btn_row->add_view(btn_update_db);
        conn_col->add_view(conn_btn_row);
        conn_card->add_view(conn_col);
        settings_scroll_layout->add_view(conn_card);

        // 2. Playback Modes Card
        auto [modes_card, modes_col] = make_section_card("Playback Preferences", "🎛️");

        auto make_switch_row = [](const std::string& label, const std::string& desc, bool initial_val, std::function<void(bool)> on_toggle) {
            auto row = std::make_shared<LinearLayout>(Orientation::Horizontal);
            row->set_layout_params(LayoutParams(
                static_cast<int>(LayoutDimension::MatchParent),
                static_cast<int>(LayoutDimension::WrapContent),
                Gravity::CenterVertical
            ));
            row->set_margin(0, 4, 0, 8);

            auto text_col = std::make_shared<LinearLayout>(Orientation::Vertical);
            text_col->set_layout_params(LayoutParams(
                0,
                static_cast<int>(LayoutDimension::WrapContent),
                1.0f
            ));
            text_col->set_margin(0, 0, 8, 0);

            auto l_lbl = TextViewBuilder::create()->text(label)->bold(true)->build();
            auto d_lbl = TextViewBuilder::create()->text(desc)->caption(true)->muted(true)->build();
            text_col->add_view(l_lbl);
            text_col->add_view(d_lbl);

            auto sw = SwitchBuilder::create()
                ->checked(initial_val)
                ->onCheckedChanged(std::move(on_toggle))
                ->build();
            sw->set_layout_params(LayoutParams(
                static_cast<int>(LayoutDimension::WrapContent),
                static_cast<int>(LayoutDimension::WrapContent),
                Gravity::CenterVertical
            ));

            row->add_view(text_col);
            row->add_view(sw);
            return row;
        };

        MpdStatus cur_st = client->get_status();

        modes_col->add_view(make_switch_row(
            "Consume Mode",
            "Automatically remove songs from queue after playing",
            cur_st.consume,
            [client](bool checked) {
                client->set_consume(checked);
            }
        ));

        modes_col->add_view(make_switch_row(
            "Single Track Mode",
            "Stop playback or repeat only current track",
            cur_st.single,
            [client](bool checked) {
                client->set_single(checked);
            }
        ));

        modes_col->add_view(make_switch_row(
            "Shuffle / Random",
            "Play queue in randomized order",
            cur_st.random,
            [client](bool checked) {
                client->set_random(checked);
            }
        ));

        modes_col->add_view(make_switch_row(
            "Repeat Queue",
            "Loop playback continuously when queue reaches end",
            cur_st.repeat,
            [client](bool checked) {
                client->set_repeat(checked);
            }
        ));

        modes_card->add_view(modes_col);
        settings_scroll_layout->add_view(modes_card);

        // 3. About Card
        auto [about_card, about_col] = make_section_card("About Miquland Music", "ℹ️");

        auto app_name_lbl = TextViewBuilder::create()
            ->text("miqumusic v1.0.0")
            ->bold(true)
            ->build();
        auto app_desc_lbl = TextViewBuilder::create()
            ->text("Native Wayland client for Music Player Daemon (MPD)\nBuilt with miqutoolkit & libmpdclient")
            ->caption(true)
            ->muted(true)
            ->build();
        app_desc_lbl->set_margin(0, 2, 0, 0);

        about_col->add_view(app_name_lbl);
        about_col->add_view(app_desc_lbl);
        about_card->add_view(about_col);
        settings_scroll_layout->add_view(about_card);

        // Reconnect and update db callbacks
        btn_reconnect->set_on_click_listener([client, host, port, conn_status_lbl, server_str, toolbar]() {
            bool ok = client->connect(host, port);
            conn_status_lbl->set_text(ok ? ("Status: Connected to " + server_str) : ("Status: Disconnected from " + server_str));
            toolbar->set_subtitle(ok ? "MPD • Connected" : "MPD • Disconnected");
        });

        btn_update_db->set_on_click_listener([client, btn_update_db]() {
            bool ok = client->update_database();
            btn_update_db->set_text(ok ? "✓ Scan started" : "✗ Scan failed");
        });

        auto settings_scroll = ScrollViewBuilder::create()
            ->contentView(settings_scroll_layout)
            ->scrollbar(true)
            ->build();
        settings_scroll->set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::MatchParent)
        ));

        return settings_scroll;
    };

    auto root_content_layout = std::make_shared<LinearLayout>(Orientation::Vertical);
    root_content_layout->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::MatchParent)
    ));
    root_content_layout->add_view(view_pager);
    root_content_layout->add_view(bottom_nav);

    nav_view->push(root_content_layout, "Music", initial_connected ? "MPD • Connected" : "MPD • Offline", "root");
    root_container->add_view(nav_view);

    btn_settings = toolbar->add_action(icons::SETTINGS, [nav_view, create_settings_view]() {
        nav_view->push(create_settings_view(), "Settings", "Preferences & Server", "settings");
    });

    nav_view->set_on_navigation_listener([toolbar, btn_settings](const NavigationPage& page, bool can_go_back) {
        toolbar->set_title(page.title);
        toolbar->set_subtitle(page.subtitle);
        toolbar->set_back_visible(can_go_back);
        if (btn_settings) {
            btn_settings->set_visibility(can_go_back ? Visibility::Gone : Visibility::Visible);
        }
    });

    // =========================================================================
    // UI REFRESH HELPERS
    // =========================================================================
    auto update_playback_ui = [&](const MpdStatus& st, const Song& song) {
        last_status = st;
        last_song = song;

        if (st.connected) {
            if (nav_view->get_depth() <= 1) toolbar->set_subtitle("MPD • Connected");
        } else {
            if (nav_view->get_depth() <= 1) toolbar->set_subtitle("MPD • Disconnected");
        }

        // Title & Artist
        if (!song.uri.empty()) {
            track_title->set_text(song.display_title());
            track_artist->set_text(song.display_artist() + " • " + song.display_album());
        } else {
            track_title->set_text("No track playing");
            track_artist->set_text("Music Player Daemon");
        }

        // Stream & Audiophile Quality Badge
        std::string badge_str;
        if (!st.audio_format.empty()) {
            badge_str = "🎵 " + st.audio_format;
        }
        if (st.bitrate > 0) {
            if (!badge_str.empty()) badge_str += " • ";
            badge_str += std::to_string(st.bitrate) + " kbps";
        }
        if (!song.date.empty()) {
            if (!badge_str.empty()) badge_str += " • ";
            badge_str += song.date;
        }
        if (!song.genre.empty()) {
            if (!badge_str.empty()) badge_str += " • ";
            badge_str += song.genre;
        }
        format_badge->set_text(badge_str);

        // Update "Up Next" Peek
        if (st.song_pos >= 0 && st.song_pos + 1 < static_cast<int>(last_queue.size())) {
            const auto& next_s = last_queue[st.song_pos + 1];
            up_next_title->set_text(next_s.display_title() + " — " + next_s.display_artist());
            up_next_time->set_text(next_s.formatted_duration());
        } else if (st.repeat && !last_queue.empty()) {
            const auto& first_s = last_queue[0];
            up_next_title->set_text("Loops to: " + first_s.display_title() + " — " + first_s.display_artist());
            up_next_time->set_text(first_s.formatted_duration());
        } else {
            up_next_title->set_text("End of queue");
            up_next_time->set_text("");
        }

        // Cover Art
        if (song.uri != current_art_uri) {
            current_art_uri = song.uri;
            std::string art = ArtLoader::get_art_for_song(song, *client);
            if (!art.empty()) {
                current_art_path = art;
                cover_image->set_image_resource(art);
            } else {
                current_art_path = "";
                cover_image->set_image_resource("audio-x-generic");
            }
        }

        // Seek Bar
        if (!is_seeking.load()) {
            seek_slider->set_progress(st.elapsed_fraction);
            seek_elapsed_text->set_text(st.formatted_elapsed());
        }
        seek_total_text->set_text(st.formatted_total());

        // Play/Pause button text & style
        if (st.state == PlaybackState::Playing) {
            btn_play->set_text("⏸");
        } else {
            btn_play->set_text("▶");
        }

        // Mode toggles
        btn_shuffle->set_selected(st.random);
        if (st.repeat && st.single) {
            btn_repeat->set_text("🔂");
            btn_repeat->set_selected(true);
        } else if (st.repeat) {
            btn_repeat->set_text("🔁");
            btn_repeat->set_selected(true);
        } else {
            btn_repeat->set_text("🔁");
            btn_repeat->set_selected(false);
        }

        // Volume control
        if (st.volume >= 0) {
            volume_row->set_visibility(Visibility::Visible);
            vol_slider->set_value(st.volume / 100.0f);
            btn_vol_down->set_text(st.volume == 0 ? "🔇" : "🔈");
        } else {
            volume_row->set_visibility(Visibility::Gone);
        }

        if (window) window->schedule_redraw();
    };

    auto update_queue_ui = [&](const std::vector<Song>& queue, int current_pos) {
        last_queue = queue;
        queue_items_layout->clear_views();

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

            // Track index / number
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

            // Title + Artist column
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

            // Duration
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

            unsigned int song_pos = s.pos;
            row->set_on_click_listener([client, song_pos]() {
                client->play_pos(song_pos);
            });

            queue_items_layout->add_view(row);
        }

        // Update "Up Next" Peek in Player View
        if (current_pos >= 0 && current_pos + 1 < static_cast<int>(queue.size())) {
            const auto& next_s = queue[current_pos + 1];
            up_next_title->set_text(next_s.display_title() + " — " + next_s.display_artist());
            up_next_time->set_text(next_s.formatted_duration());
        } else if (last_status.repeat && !queue.empty()) {
            const auto& first_s = queue[0];
            up_next_title->set_text("Loops to: " + first_s.display_title() + " — " + first_s.display_artist());
            up_next_time->set_text(first_s.formatted_duration());
        } else {
            up_next_title->set_text("End of queue");
            up_next_time->set_text("");
        }

        if (window) window->schedule_redraw();
    };

    auto load_library_ui = [&]() {
        library_items_layout->clear_views();
        std::vector<Song> all_songs = client->get_all_songs();

        for (const auto& s : all_songs) {
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
            row->set_on_click_listener([client, uri, &bottom_nav, &view_pager, &update_playback_ui, &update_queue_ui]() {
                client->add_and_play(uri);
                MpdStatus st = client->get_status();
                Song song = client->get_current_song();
                std::vector<Song> q = client->get_queue();
                update_playback_ui(st, song);
                update_queue_ui(q, st.song_pos);
                bottom_nav->set_selected_index(0);
                view_pager->set_current_page(0);
            });

            library_items_layout->add_view(row);
        }

        library_loaded = true;
        if (window) window->schedule_redraw();
    };

    auto load_artists_ui = [&]() {
        artists_items_layout->clear_views();
        std::vector<Song> all_songs = client->get_all_songs();

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
            auto play_artist_action = [client, first_uri, &bottom_nav, &view_pager, &update_playback_ui, &update_queue_ui]() {
                if (!first_uri.empty()) {
                    client->add_and_play(first_uri);
                    MpdStatus st = client->get_status();
                    Song song = client->get_current_song();
                    std::vector<Song> q = client->get_queue();
                    update_playback_ui(st, song);
                    update_queue_ui(q, st.song_pos);
                    bottom_nav->set_selected_index(0);
                    view_pager->set_current_page(0);
                }
            };

            row->set_on_click_listener(play_artist_action);
            play_btn->set_on_click_listener(play_artist_action);

            artists_items_layout->add_view(row);
        }

        artists_loaded = true;
        if (window) window->schedule_redraw();
    };

    // =========================================================================
    // EVENT & CONTROL LISTENERS
    // =========================================================================
    bottom_nav->set_on_item_selected_listener([&](int idx) {
        view_pager->set_current_page(idx);
        if (idx == 2 && !library_loaded) {
            load_library_ui();
        } else if (idx == 3 && !artists_loaded) {
            load_artists_ui();
        }
    });

    view_pager->set_on_page_changed_listener([&](int old_idx, int new_idx) {
        if (bottom_nav && bottom_nav->get_selected_index() != new_idx) {
            bottom_nav->set_selected_index(new_idx);
        }
        if (new_idx == 2 && !library_loaded) {
            load_library_ui();
        } else if (new_idx == 3 && !artists_loaded) {
            load_artists_ui();
        }
    });

    btn_play->set_on_click_listener([client]() {
        client->toggle_pause();
    });

    btn_prev->set_on_click_listener([client]() {
        client->previous();
    });

    btn_next->set_on_click_listener([client]() {
        client->next();
    });

    btn_shuffle->set_on_click_listener([client, &last_status]() {
        client->set_random(!last_status.random);
    });

    btn_repeat->set_on_click_listener([client, &last_status]() {
        if (!last_status.repeat && !last_status.single) {
            // Off -> Repeat All
            client->set_repeat(true);
            client->set_single(false);
        } else if (last_status.repeat && !last_status.single) {
            // Repeat All -> Repeat One
            client->set_repeat(true);
            client->set_single(true);
        } else {
            // Repeat One -> Off
            client->set_repeat(false);
            client->set_single(false);
        }
    });

    vol_slider->set_on_value_changed_listener([client, btn_vol_down](float val, bool from_user) {
        if (from_user) {
            int v = std::clamp(static_cast<int>(std::round(val * 100.0f)), 0, 100);
            client->set_volume(v);
            btn_vol_down->set_text(v == 0 ? "🔇" : "🔈");
        }
    });

    auto last_vol_before_mute = std::make_shared<int>(80);
    btn_vol_down->set_on_click_listener([client, &last_status, vol_slider, btn_vol_down, last_vol_before_mute]() {
        if (last_status.volume > 0) {
            *last_vol_before_mute = last_status.volume;
            client->set_volume(0);
            vol_slider->set_value(0.0f);
            btn_vol_down->set_text("🔇");
        } else if (last_status.volume == 0) {
            int target = *last_vol_before_mute > 0 ? *last_vol_before_mute : 75;
            client->set_volume(target);
            vol_slider->set_value(target / 100.0f);
            btn_vol_down->set_text("🔈");
        }
    });

    btn_vol_up->set_on_click_listener([client, vol_slider, btn_vol_down]() {
        client->set_volume(100);
        vol_slider->set_value(1.0f);
        btn_vol_down->set_text("🔈");
    });

    refresh_action = [client, &update_playback_ui, &update_queue_ui, &load_library_ui, &load_artists_ui, &bottom_nav]() {
        MpdStatus st = client->get_status();
        Song song = client->get_current_song();
        std::vector<Song> q = client->get_queue();
        update_playback_ui(st, song);
        update_queue_ui(q, st.song_pos);
        if (bottom_nav->get_selected_index() == 2) load_library_ui();
        if (bottom_nav->get_selected_index() == 3) load_artists_ui();
    };

    // Seek interaction
    seek_slider->set_on_value_changed_listener([client, &is_seeking, &last_status, seek_elapsed_text](float val, bool from_user) {
        if (from_user) {
            is_seeking.store(true);
            client->seek_fraction(val);
            if (last_status.total_seconds > 0) {
                unsigned int cur = static_cast<unsigned int>(val * last_status.total_seconds);
                seek_elapsed_text->set_text(format_time(cur));
            }
            std::thread([&is_seeking]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                is_seeking.store(false);
            }).detach();
        }
    });



    // Register MPD callbacks
    client->set_on_status_changed([&update_playback_ui, &update_queue_ui, client](const MpdStatus& st, const Song& song) {
        update_playback_ui(st, song);
        std::vector<Song> q = client->get_queue();
        update_queue_ui(q, st.song_pos);
    });

    client->set_on_queue_changed([&update_queue_ui, &last_status](const std::vector<Song>& q) {
        update_queue_ui(q, last_status.song_pos);
    });

    client->set_on_connection_changed([toolbar, &window](bool connected) {
        toolbar->set_subtitle(connected ? "MPD • Connected" : "MPD • Offline");
        if (window) window->schedule_redraw();
    });

    // Initial state population
    if (initial_connected) {
        MpdStatus st = client->get_status();
        Song song = client->get_current_song();
        std::vector<Song> q = client->get_queue();
        update_playback_ui(st, song);
        update_queue_ui(q, st.song_pos);
    }

    // Start background idle event monitoring
    client->start_idle_listener();

    // =========================================================================
    // PLAYBACK PROGRESS TICKER THREAD
    // =========================================================================
    std::thread ticker_thread([&]() {
        while (app_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (!app_running.load()) break;

            if (last_status.state == PlaybackState::Playing && !is_seeking.load() && last_status.total_seconds > 0) {
                if (auto eng = AppEngine::instance()) {
                    eng->post([&]() {
                        if (last_status.state != PlaybackState::Playing || is_seeking.load()) return;
                        if (last_status.elapsed_seconds < last_status.total_seconds) {
                            last_status.elapsed_seconds++;
                            float frac = static_cast<float>(last_status.elapsed_seconds) / static_cast<float>(last_status.total_seconds);
                            seek_slider->set_progress(frac);
                            seek_elapsed_text->set_text(last_status.formatted_elapsed());
                            if (window) window->schedule_redraw();
                        }
                    });
                }
            }
        }
    });

    // =========================================================================
    // WAYLAND TOPLEVEL WINDOW CREATION
    // =========================================================================
    window = WindowBuilder::create()
        ->title("miqumusic")
        ->appId("miqumusic")
        ->role(WindowRole::Toplevel)
        ->preferredSize(600, 680)
        ->closeOnEscape(false)
        ->contentView(root_container)
        ->onClose([&]() {
            app_running.store(false);
            client->stop_idle_listener();
            engine->quit();
        })
        ->onKey([&](const KeyPressEvent& ev) {
            if (!ev.pressed) return;
            switch (ev.keysym) {
                case XKB_KEY_Escape:
                    if (nav_view && nav_view->can_go_back()) {
                        nav_view->pop();
                    } else {
                        app_running.store(false);
                        client->stop_idle_listener();
                        engine->quit();
                    }
                    break;
                case XKB_KEY_space:
                    client->toggle_pause();
                    break;
                case XKB_KEY_n:
                case XKB_KEY_N:
                    client->next();
                    break;
                case XKB_KEY_p:
                case XKB_KEY_P:
                    client->previous();
                    break;
                case XKB_KEY_Right:
                    if (last_status.total_seconds > 0) {
                        unsigned int target = std::min(last_status.total_seconds, last_status.elapsed_seconds + 5);
                        client->seek(target);
                    }
                    break;
                case XKB_KEY_Left:
                    if (last_status.total_seconds > 0) {
                        unsigned int target = (last_status.elapsed_seconds > 5) ? (last_status.elapsed_seconds - 5) : 0;
                        client->seek(target);
                    }
                    break;
                case XKB_KEY_Up:
                    if (last_status.volume >= 0) {
                        client->set_volume(std::min(100, last_status.volume + 5));
                    }
                    break;
                case XKB_KEY_Down:
                    if (last_status.volume >= 0) {
                        client->set_volume(std::max(0, last_status.volume - 5));
                    }
                    break;
                case XKB_KEY_1:
                    bottom_nav->set_selected_index(0);
                    view_pager->set_current_page(0);
                    break;
                case XKB_KEY_2:
                    bottom_nav->set_selected_index(1);
                    view_pager->set_current_page(1);
                    break;
                case XKB_KEY_3:
                    bottom_nav->set_selected_index(2);
                    view_pager->set_current_page(2);
                    if (!library_loaded) load_library_ui();
                    break;
                case XKB_KEY_4:
                    bottom_nav->set_selected_index(3);
                    view_pager->set_current_page(3);
                    if (!artists_loaded) load_artists_ui();
                    break;
                default:
                    break;
            }
        })
        ->build();

    if (!window) {
        std::cerr << "[miqumusic] Failed to create Wayland window.\n";
        app_running.store(false);
        client->stop_idle_listener();
        if (ticker_thread.joinable()) ticker_thread.join();
        return 1;
    }

    window->show();
    int res = engine->enter_loop();

    app_running.store(false);
    client->stop_idle_listener();
    if (ticker_thread.joinable()) {
        ticker_thread.join();
    }

    return res;
}
