#include "player_view.hpp"
#include "core/config.hpp"
#include "utils/icon_provider.hpp"
#include <iostream>

namespace miqumusic {

PlayerView::PlayerView() {
    const auto& cfg = MiquMusicConfig::get();

    // 1. Large Album Artwork Card
    m_artwork_view = miqu::ImageViewBuilder::create()
        ->targetSize(240)
        ->cornerRadius(20)
        ->fitMode(miqu::FitMode::Cover)
        ->build();

    auto art_card = miqu::CardViewBuilder::create()
        ->backgroundColor(cfg.surface)
        ->stroke(cfg.border_width, cfg.border_color)
        ->cornerRadius(22)
        ->addView(m_artwork_view, miqu::LayoutParams(240, 240))
        ->build();
    art_card->set_margin(0, 20, 0, 16);

    // 2. Track Title
    m_title_text = miqu::TextViewBuilder::create()
        ->text("No Track Selected")
        ->fontFamily(cfg.font_family)
        ->textSize(20)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();

    // 3. Artist
    m_artist_text = miqu::TextViewBuilder::create()
        ->text("Unknown Artist")
        ->fontFamily(cfg.font_family)
        ->textSize(14)
        ->textColor(cfg.accent_color)
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();
    m_artist_text->set_margin(0, 4, 0, 2);

    // 4. Album
    m_album_text = miqu::TextViewBuilder::create()
        ->text("")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.8f, 0.7f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();

    // 5. Status Pill Badge (e.g. "PLAYING • 320 KBPS")
    m_badge_text = miqu::TextViewBuilder::create()
        ->text("STOPPED")
        ->fontFamily(cfg.font_family)
        ->textSize(10)
        ->bold(true)
        ->textColor(cfg.accent_color)
        ->textAlignment(miqu::TextAlignment::Center)
        ->padding(12, 4)
        ->build();

    m_badge_card = miqu::CardViewBuilder::create()
        ->backgroundColor(cfg.accent_color.with_alpha(0.15f))
        ->stroke(1, cfg.accent_color.with_alpha(0.3f))
        ->cornerRadius(12)
        ->addView(m_badge_text)
        ->build();
    m_badge_card->set_margin(0, 12, 0, 0);

    // Root Layout
    m_root = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::Center)
        ->spacing(4)
        ->addView(art_card)
        ->addView(m_title_text)
        ->addView(m_artist_text)
        ->addView(m_album_text)
        ->addView(m_badge_card)
        ->build();
    m_root->get_layout_params().weight = 1.0f;
}

void PlayerView::update_state(const SongInfo& song, const MPDStatus& status, const std::string& art_path) {
    if (m_artwork_view && !art_path.empty()) {
        m_artwork_view->set_image_resource(art_path);
    }
    if (m_title_text) m_title_text->set_text(song.display_title());
    if (m_artist_text) m_artist_text->set_text(song.display_artist());
    if (m_album_text) m_album_text->set_text(song.album);

    if (m_badge_text) {
        std::string badge;
        if (status.state == MPD_STATE_PLAY) badge = "PLAYING";
        else if (status.state == MPD_STATE_PAUSE) badge = "PAUSED";
        else badge = "STOPPED";

        if (status.kbit_rate > 0) {
            badge += " • " + std::to_string(status.kbit_rate) + " KBPS";
        }
        m_badge_text->set_text(badge);
    }
}

} // namespace miqumusic
