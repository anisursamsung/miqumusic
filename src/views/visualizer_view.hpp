#pragma once

#include <miqutoolkit/miqutoolkit.hpp>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

namespace miqumusic {

enum class VisMode {
    RainbowSpectrum,
    StereoMirrorWave,
    PeakHoldNeon,
    LiquidWavy,
    DotMatrix,
    CircularRadial
};

class VisualizerCanvas : public miqu::View {
public:
    VisualizerCanvas();
    ~VisualizerCanvas() override;

    void draw(cairo_t* cr, const miqu::Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, miqu::MouseButton button, bool pressed, const miqu::Rect& bounds) override;

    void cycle_mode();
    VisMode get_mode() const { return m_mode; }
    std::string get_mode_name() const;

private:
    void reader_thread_func();

    VisMode m_mode = VisMode::RainbowSpectrum;
    std::atomic<bool> m_running{true};
    std::thread m_reader_thread;

    std::mutex m_data_mutex;
    std::vector<float> m_spectrum;
    std::vector<float> m_peaks;
    float m_phase = 0.0f;
};

class VisualizerView {
public:
    VisualizerView();

    std::shared_ptr<miqu::View> get_view() { return m_root; }

private:
    std::shared_ptr<miqu::LinearLayout> m_root;
    std::shared_ptr<miqu::TextView> m_mode_label;
    std::shared_ptr<VisualizerCanvas> m_canvas;
};

} // namespace miqumusic
