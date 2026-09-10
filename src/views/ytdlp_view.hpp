#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include <functional>
#include <string>

namespace miqumusic {

class YtDlpView {
public:
    YtDlpView(std::function<void()> on_action);

    std::shared_ptr<miqu::View> get_view() { return m_root; }

private:
    void stream_url(const std::string& url);
    void download_url(const std::string& url);

    std::function<void()> m_on_action;
    std::shared_ptr<miqu::LinearLayout> m_root;
    std::shared_ptr<miqu::EditText> m_url_edit;
    std::shared_ptr<miqu::TextView> m_status_text;
    std::shared_ptr<miqu::Button> m_btn_stream;
    std::shared_ptr<miqu::Button> m_btn_download;
};

} // namespace miqumusic
