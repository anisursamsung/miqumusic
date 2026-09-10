#include "ytdlp_view.hpp"
#include "core/config.hpp"
#include "mpd/mpd_manager.hpp"
#include "utils/notification_mgr.hpp"
#include "utils/format_utils.hpp"
#include <thread>
#include <array>
#include <memory>
#include <iostream>

namespace miqumusic {

YtDlpView::YtDlpView(std::function<void()> on_action)
    : m_on_action(std::move(on_action))
{
    const auto& cfg = MiquMusicConfig::get();

    auto header = miqu::TextViewBuilder::create()
        ->text("Online Audio & YouTube")
        ->fontFamily(cfg.font_family)
        ->textSize(18)
        ->bold(true)
        ->textColor(miqu::Color::rgb(1.0f, 1.0f, 1.0f))
        ->build();
    header->set_margin(0, 0, 0, 6);

    auto subtitle = miqu::TextViewBuilder::create()
        ->text("Enter a YouTube, SoundCloud, or direct audio link to stream or download into your library.")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.8f, 0.75f))
        ->build();
    subtitle->set_margin(0, 0, 0, 20);

    m_url_edit = miqu::EditTextBuilder::create()
        ->hint("Paste video or audio URL here (https://...)")
        ->padding(12, 10)
        ->build();
    m_url_edit->get_layout_params().width = 540;
    m_url_edit->set_margin(0, 0, 0, 16);

    m_btn_stream = miqu::ButtonBuilder::create()
        ->text("Stream Now")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->bold(true)
        ->cornerRadius(10)
        ->onClick([this]() {
            if (m_url_edit) stream_url(trim(m_url_edit->get_text()));
        })
        ->build();
    m_btn_stream->set_custom_colors(cfg.accent_color, miqu::Color::rgb(0.0f, 0.0f, 0.0f));
    m_btn_stream->set_margin(0, 0, 12, 0);

    m_btn_download = miqu::ButtonBuilder::create()
        ->text("Download to Library")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->cornerRadius(10)
        ->onClick([this]() {
            if (m_url_edit) download_url(trim(m_url_edit->get_text()));
        })
        ->build();
    m_btn_download->set_custom_colors(cfg.surface, miqu::Color::rgb(1.0f, 1.0f, 1.0f));

    auto buttons_row = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Horizontal)
        ->gravity(miqu::Gravity::Center)
        ->addView(m_btn_stream)
        ->addView(m_btn_download)
        ->build();
    buttons_row->set_margin(0, 0, 0, 16);

    m_status_text = miqu::TextViewBuilder::create()
        ->text("")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->textColor(cfg.accent_color)
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();

    auto card = miqu::CardViewBuilder::create()
        ->backgroundColor(cfg.surface)
        ->stroke(cfg.border_width, cfg.border_color)
        ->cornerRadius(cfg.corner_radius)
        ->padding(32, 28)
        ->addView(miqu::LinearLayoutBuilder::create()
            ->orientation(miqu::Orientation::Vertical)
            ->gravity(miqu::Gravity::Center)
            ->addView(header)
            ->addView(subtitle)
            ->addView(m_url_edit)
            ->addView(buttons_row)
            ->addView(m_status_text)
            ->build())
        ->build();
    card->get_layout_params().width = 620;
    card->set_margin(0, 40, 0, 0);

    m_root = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->gravity(miqu::Gravity::CenterHorizontal)
        ->addView(card)
        ->build();
    m_root->get_layout_params().weight = 1.0f;
}

void YtDlpView::stream_url(const std::string& url) {
    if (url.empty()) return;

    if (m_status_text) m_status_text->set_text("Resolving audio stream URL with yt-dlp...");

    std::thread([this, url]() {
        std::string cmd = "yt-dlp -g -f bestaudio \"" + url + "\" 2>/dev/null";
        std::array<char, 512> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (pipe) {
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                result += buffer.data();
            }
        }
        result = trim(result);

        if (!result.empty()) {
            // Found audio stream URI
            MPDManager::play_uri(result, [this]() {
                if (m_on_action) m_on_action();
            });
            NotificationManager::notify("Stream Started", "Audio streaming via MPD");
        } else {
            // Direct playback attempt
            MPDManager::play_uri(url, [this]() {
                if (m_on_action) m_on_action();
            });
        }
    }).detach();
}

void YtDlpView::download_url(const std::string& url) {
    if (url.empty()) return;

    const auto& cfg = MiquMusicConfig::get();
    std::string out_dir = cfg.music_directory;

    if (m_status_text) m_status_text->set_text("Downloading audio track in background...");
    NotificationManager::notify("Download", "Downloading audio track...");

    std::thread([this, url, out_dir]() {
        std::string cmd = "yt-dlp -x --audio-format mp3 -o \"" + out_dir + "/%(title)s.%(ext)s\" \"" + url + "\" 2>&1";
        int ret = ::system(cmd.c_str());
        if (ret == 0) {
            MPDManager::update_database([this]() {
                if (m_on_action) m_on_action();
            });
            NotificationManager::notify("Download Complete", "Saved to " + out_dir);
        } else {
            NotificationManager::notify("Download Failed", "yt-dlp encountered an error");
        }
    }).detach();
}

} // namespace miqumusic
