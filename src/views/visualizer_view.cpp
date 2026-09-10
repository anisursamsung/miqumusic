#include "visualizer_view.hpp"
#include "core/config.hpp"
#include "utils/icon_provider.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <iostream>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqumusic {

VisualizerCanvas::VisualizerCanvas() {
    m_spectrum.resize(32, 0.05f);
    m_peaks.resize(32, 0.05f);
    m_reader_thread = std::thread(&VisualizerCanvas::reader_thread_func, this);
}

VisualizerCanvas::~VisualizerCanvas() {
    m_running = false;
    if (m_reader_thread.joinable()) {
        m_reader_thread.join();
    }
}

void VisualizerCanvas::cycle_mode() {
    int next = (static_cast<int>(m_mode) + 1) % 6;
    m_mode = static_cast<VisMode>(next);
    if (m_window) m_window->schedule_redraw();
}

std::string VisualizerCanvas::get_mode_name() const {
    switch (m_mode) {
    case VisMode::RainbowSpectrum: return "Rainbow Spectrum";
    case VisMode::StereoMirrorWave: return "Stereo Mirror Wave";
    case VisMode::PeakHoldNeon: return "Peak Hold Neon";
    case VisMode::LiquidWavy: return "Liquid Wavy";
    case VisMode::DotMatrix: return "Dot Matrix";
    case VisMode::CircularRadial: return "Circular Radial";
    }
    return "Spectrum";
}

bool VisualizerCanvas::on_mouse_button(int lx, int ly, miqu::MouseButton button, bool pressed, const miqu::Rect& bounds) {
    if (button == miqu::MouseButton::Left && !pressed) {
        cycle_mode();
        return true;
    }
    return false;
}

void VisualizerCanvas::reader_thread_func() {
    const auto& cfg = MiquMusicConfig::get();
    int fd = -1;
    int16_t pcm_buf[1024];

    while (m_running) {
        if (fd < 0) {
            fd = open(cfg.fifo_path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }

        bool has_data = false;
        if (fd >= 0) {
            ssize_t n = read(fd, pcm_buf, sizeof(pcm_buf));
            if (n > 0) {
                has_data = true;
                size_t samples = n / sizeof(int16_t);

                std::lock_guard<std::mutex> lock(m_data_mutex);
                size_t bins = m_spectrum.size();
                size_t step = std::max<size_t>(1, samples / bins);

                for (size_t i = 0; i < bins; ++i) {
                    float sum = 0;
                    size_t start = i * step;
                    size_t count = 0;
                    for (size_t s = start; s < start + step && s < samples; ++s) {
                        sum += std::abs(pcm_buf[s]) / 32768.0f;
                        count++;
                    }
                    float val = count > 0 ? (sum / count) * 2.5f : 0.05f;
                    val = std::clamp(val, 0.03f, 1.0f);

                    // Smooth falloff
                    m_spectrum[i] = m_spectrum[i] * 0.7f + val * 0.3f;
                    if (m_spectrum[i] > m_peaks[i]) {
                        m_peaks[i] = m_spectrum[i];
                    } else {
                        m_peaks[i] = std::max(0.03f, m_peaks[i] - 0.015f);
                    }
                }
            } else if (n == 0) {
                close(fd);
                fd = -1;
            }
        }

        // Ambient idle animation if no audio active
        if (!has_data) {
            std::lock_guard<std::mutex> lock(m_data_mutex);
            m_phase += 0.08f;
            for (size_t i = 0; i < m_spectrum.size(); ++i) {
                float wave = std::sin(m_phase + i * 0.35f) * 0.15f + 0.20f;
                m_spectrum[i] = m_spectrum[i] * 0.85f + wave * 0.15f;
                m_peaks[i] = std::max(m_spectrum[i], m_peaks[i] - 0.01f);
            }
        }

        if (m_window) {
            m_window->schedule_redraw();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25)); // ~40 FPS
    }

    if (fd >= 0) close(fd);
}

void VisualizerCanvas::draw(cairo_t* cr, const miqu::Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    std::vector<float> spec;
    std::vector<float> peaks;
    {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        spec = m_spectrum;
        peaks = m_peaks;
    }

    int draw_x = bounds.x + m_margin.left;
    int draw_y = bounds.y + m_margin.top;
    int draw_w = std::max(0, bounds.width - m_margin.left - m_margin.right);
    int draw_h = std::max(0, bounds.height - m_margin.top - m_margin.bottom);

    if (draw_w <= 0 || draw_h <= 0) return;

    cairo_save(cr);

    if (m_mode == VisMode::RainbowSpectrum || m_mode == VisMode::PeakHoldNeon) {
        int n_bars = static_cast<int>(spec.size());
        double gap = 4.0;
        double bar_w = (draw_w - gap * (n_bars + 1)) / n_bars;
        if (bar_w < 2.0) bar_w = 2.0;

        for (int i = 0; i < n_bars; ++i) {
            double bx = draw_x + gap + i * (bar_w + gap);
            double h = draw_h * spec[i];
            double by = draw_y + draw_h - h;

            float r, g, b;
            if (m_mode == VisMode::RainbowSpectrum) {
                float freq = 0.25f;
                r = std::sin(freq * i + 0.0f) * 0.5f + 0.5f;
                g = std::sin(freq * i + 2.0f) * 0.5f + 0.5f;
                b = std::sin(freq * i + 4.0f) * 0.5f + 0.5f;
            } else {
                r = 0.2f; g = 0.8f; b = 1.0f;
            }

            cairo_set_source_rgba(cr, r, g, b, 0.85f);
            cairo_rectangle(cr, bx, by, bar_w, h);
            cairo_fill(cr);

            // Floating peak dot for PeakHoldNeon
            if (m_mode == VisMode::PeakHoldNeon) {
                double ph = draw_h * peaks[i];
                double py = draw_y + draw_h - ph - 4.0;
                cairo_set_source_rgba(cr, 1.0f, 0.9f, 0.3f, 0.95f);
                cairo_rectangle(cr, bx, py, bar_w, 3.0);
                cairo_fill(cr);
            }
        }
    } else if (m_mode == VisMode::StereoMirrorWave || m_mode == VisMode::LiquidWavy) {
        double cy = draw_y + draw_h / 2.0;
        int n_points = static_cast<int>(spec.size());
        double step_x = static_cast<double>(draw_w) / (n_points - 1);

        cairo_set_line_width(cr, 3.0);
        cairo_set_source_rgba(cr, 0.2f, 0.85f, 1.0f, 0.9f);

        cairo_move_to(cr, draw_x, cy);
        for (int i = 0; i < n_points; ++i) {
            double px = draw_x + i * step_x;
            double amp = spec[i] * (draw_h * 0.45);
            double py = (i % 2 == 0) ? cy - amp : cy + amp;
            if (m_mode == VisMode::LiquidWavy) {
                py = cy - std::sin(m_phase + i * 0.5) * amp;
            }
            cairo_line_to(cr, px, py);
        }
        cairo_stroke(cr);

        // Mirror baseline
        if (m_mode == VisMode::StereoMirrorWave) {
            cairo_set_source_rgba(cr, 1.0f, 0.4f, 0.7f, 0.7f);
            cairo_move_to(cr, draw_x, cy);
            for (int i = 0; i < n_points; ++i) {
                double px = draw_x + i * step_x;
                double amp = spec[i] * (draw_h * 0.45);
                double py = (i % 2 == 0) ? cy + amp : cy - amp;
                cairo_line_to(cr, px, py);
            }
            cairo_stroke(cr);
        }
    } else if (m_mode == VisMode::DotMatrix) {
        int n_cols = static_cast<int>(spec.size());
        int n_rows = 14;
        double dot_w = (draw_w - 4.0 * (n_cols + 1)) / n_cols;
        double dot_h = (draw_h - 4.0 * (n_rows + 1)) / n_rows;

        for (int c = 0; c < n_cols; ++c) {
            int active_dots = static_cast<int>(spec[c] * n_rows);
            for (int r = 0; r < n_rows; ++r) {
                double dx = draw_x + 4.0 + c * (dot_w + 4.0);
                double dy = draw_y + draw_h - (r + 1) * (dot_h + 4.0);
                bool on = (r <= active_dots);
                if (on) {
                    cairo_set_source_rgba(cr, 0.2f, 0.85f, 0.4f, 0.9f);
                } else {
                    cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.08f);
                }
                cairo_arc(cr, dx + dot_w / 2, dy + dot_h / 2, std::min(dot_w, dot_h) / 2, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }
    } else if (m_mode == VisMode::CircularRadial) {
        double cx = draw_x + draw_w / 2.0;
        double cy = draw_y + draw_h / 2.0;
        double base_r = std::min(draw_w, draw_h) * 0.18;
        int n_spokes = static_cast<int>(spec.size());

        cairo_set_line_width(cr, 3.5);
        for (int i = 0; i < n_spokes; ++i) {
            double angle = (2.0 * M_PI / n_spokes) * i;
            double len = spec[i] * (std::min(draw_w, draw_h) * 0.32);
            double x1 = cx + std::cos(angle) * base_r;
            double y1 = cy + std::sin(angle) * base_r;
            double x2 = cx + std::cos(angle) * (base_r + len);
            double y2 = cy + std::sin(angle) * (base_r + len);

            float freq = 0.2f;
            float r = std::sin(freq * i + 0.0f) * 0.5f + 0.5f;
            float g = std::sin(freq * i + 2.0f) * 0.5f + 0.5f;
            float b = std::sin(freq * i + 4.0f) * 0.5f + 0.5f;

            cairo_set_source_rgba(cr, r, g, b, 0.85f);
            cairo_move_to(cr, x1, y1);
            cairo_line_to(cr, x2, y2);
            cairo_stroke(cr);
        }
    }

    cairo_restore(cr);
}

VisualizerView::VisualizerView() {
    const auto& cfg = MiquMusicConfig::get();

    m_canvas = std::make_shared<VisualizerCanvas>();
    m_canvas->get_layout_params().weight = 1.0f;

    m_mode_label = miqu::TextViewBuilder::create()
        ->text("Spectrum Visualizer (Click canvas to switch modes)")
        ->fontFamily(cfg.font_family)
        ->textSize(12)
        ->textColor(miqu::Color::rgba(0.7f, 0.7f, 0.8f, 0.75f))
        ->textAlignment(miqu::TextAlignment::Center)
        ->build();
    m_mode_label->set_margin(0, 12, 0, 8);

    auto card = miqu::CardViewBuilder::create()
        ->backgroundColor(cfg.surface)
        ->stroke(cfg.border_width, cfg.border_color)
        ->cornerRadius(cfg.corner_radius)
        ->addView(m_canvas)
        ->build();
    card->get_layout_params().weight = 1.0f;
    card->set_margin(16, 8, 16, 12);

    m_root = miqu::LinearLayoutBuilder::create()
        ->orientation(miqu::Orientation::Vertical)
        ->addView(m_mode_label)
        ->addView(card)
        ->build();
    m_root->get_layout_params().weight = 1.0f;
}

} // namespace miqumusic
