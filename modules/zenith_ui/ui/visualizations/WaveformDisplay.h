/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    WaveformDisplay.h
    Real-time waveform visualization component - Skia GPU-accelerated
    Phase 4: User Interface
  ==============================================================================
*/


#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkFont.h>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

#include "../framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {
namespace ui {

enum class WaveformMode {
    Normal,
    RMS,
    Peak,
    Spectrogram,
    Phase,
    Stereo
};

struct WaveformColors {
    SkColor background = design::colors::BG_01;
    SkColor waveform = design::colors::WAVEFORM_AUDIO;
    SkColor rms = design::colors::ORANGE;
    SkColor peak = design::colors::RED;
    SkColor grid = design::colors::BG_03;
    SkColor text = design::colors::TEXT_PRIMARY;
    SkColor selection = SkColorSetARGB(100, 100, 255, 100);
    SkColor clipping = design::colors::DANGER;
};

struct WaveformData {
    float leftChannel = 0.0f;
    float rightChannel = 0.0f;
    float rms = 0.0f;
    float peak = 0.0f;
    bool isClipping = false;
    double timeSeconds = 0.0;
    juce::Time timestamp;
};

class WaveformDisplay : public zenith::SkiaComponent,
                       public juce::MouseListener {
public:
    WaveformDisplay();
    ~WaveformDisplay() override;
    
    void setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void addAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void clearAudioData();
    
    void setMode(WaveformMode mode);
    void setColors(const WaveformColors& colors);
    void setShowGrid(bool show);
    void setShowLabels(bool show);
    void setShowRMS(bool show);
    void setShowPeaks(bool show);
    
    void setZoomLevel(float zoom);
    float getZoomLevel() const { return zoomLevel_; }
    void setScrollPosition(float position);
    void setViewRange(double startTime, double endTime);
    void fitToWindow();
    
    void setSelection(double startTime, double endTime);
    void clearSelection();
    std::pair<double, double> getSelection() const;
    bool hasSelection() const;
    
    void setPlaybackPosition(double time);
    void clearPlaybackPosition();
    double getPlaybackPosition() const;
    
    void showGenreDetection(bool show);
    void showQualityAnalysis(bool show);
    void showSpectrumAnalysis(bool show);
    void updateAnalysisData(const juce::String& analysis);
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
    void timerCallback() override;
    
private:
    std::vector<WaveformData> waveformBuffer_;
    std::mutex dataMutex_;
    double currentSampleRate_ = 44100.0;
    double audioDuration_ = 0.0;
    int64_t totalSamplesProcessed_ = 0;
    
    WaveformMode currentMode_ = WaveformMode::Normal;
    WaveformColors colors_;
    bool showGrid_ = true;
    bool showLabels_ = true;
    bool showRMS_ = true;
    bool showPeaks_ = true;
    
    float zoomLevel_ = 1.0f;
    float scrollPosition_ = 0.0f;
    double viewStartTime_ = 0.0;
    double viewEndTime_ = 0.0;
    double playbackPosition_ = -1.0;
    
    bool hasSelectionFlag_ = false;
    double selectionStart_ = 0.0;
    double selectionEnd_ = 0.0;
    bool isSelecting_ = false;
    double selectionAnchor_ = 0.0;
    
    bool showGenreDetectionFlag_ = false;
    bool showQualityAnalysisFlag_ = false;
    bool showSpectrumAnalysisFlag_ = false;
    juce::String currentAnalysis_;
    
    bool needsRedraw_ = true;
    
    int majorGridInterval_ = 1;
    int minorGridInterval_ = 1;
    float gridOpacity_ = 0.3f;
    
    static constexpr int MAX_BUFFER_SIZE = 1000000;
    static constexpr int REDRAW_INTERVAL_MS = 50;
    
    double timeToX(double time) const;
    double xToTime(double x) const;
    float amplitudeToY(float amplitude) const;
    float yToAmplitude(float y) const;
    
    void drawBackground(SkCanvas* canvas);
    void drawGrid(SkCanvas* canvas);
    void drawWaveform(SkCanvas* canvas);
    void drawRMS(SkCanvas* canvas);
    void drawPeaks(SkCanvas* canvas);
    void drawSelection(SkCanvas* canvas);
    void drawPlaybackPosition(SkCanvas* canvas);
    void drawLabels(SkCanvas* canvas);
    void drawAnalysisOverlay(SkCanvas* canvas);
    
    void drawNormalWaveform(SkCanvas* canvas);
    void drawStereoWaveform(SkCanvas* canvas);
    void drawSpectrogram(SkCanvas* canvas);
    void drawPhaseDisplay(SkCanvas* canvas);
    
    void processAudioData(const juce::AudioBuffer<float>& buffer);
    void updateViewRange();
    void optimizeBuffer();
    
    void startSelection(double time);
    void updateSelection(double time);
    void endSelection();
    
    juce::String formatTime(double time) const;
    juce::String formatAmplitude(float amplitude) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

class WaveformControlPanel : public zenith::SkiaComponent,
                           public juce::Button::Listener,
                           public juce::ComboBox::Listener,
                           public juce::Slider::Listener {
public:
    WaveformControlPanel(WaveformDisplay& display);
    ~WaveformControlPanel() override;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void sliderValueChanged(juce::Slider* slider) override;
    
private:
    WaveformDisplay& waveformDisplay_;
    
    std::unique_ptr<juce::ComboBox> modeComboBox_;
    std::unique_ptr<juce::ToggleButton> gridToggle_;
    std::unique_ptr<juce::ToggleButton> labelsToggle_;
    std::unique_ptr<juce::ToggleButton> rmsToggle_;
    std::unique_ptr<juce::ToggleButton> peaksToggle_;
    
    std::unique_ptr<juce::Slider> zoomSlider_;
    std::unique_ptr<juce::TextButton> zoomInButton_;
    std::unique_ptr<juce::TextButton> zoomOutButton_;
    std::unique_ptr<juce::TextButton> fitToWindowButton_;
    
    std::unique_ptr<juce::TextButton> goStartButton_;
    std::unique_ptr<juce::TextButton> goEndButton_;
    std::unique_ptr<juce::TextButton> playPauseButton_;
    std::unique_ptr<juce::TextButton> loopButton_;
    
    std::unique_ptr<juce::ToggleButton> genreToggle_;
    std::unique_ptr<juce::ToggleButton> qualityToggle_;
    std::unique_ptr<juce::ToggleButton> spectrumToggle_;
    
    std::unique_ptr<juce::TextButton> exportButton_;
    std::unique_ptr<juce::TextButton> screenshotButton_;
    
    void createDisplayControls();
    void createZoomControls();
    void createNavigationControls();
    void createAnalysisControls();
    void createExportControls();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformControlPanel)
};

class WaveformContainer : public zenith::SkiaComponent {
public:
    WaveformContainer();
    ~WaveformContainer() override;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    WaveformDisplay& getDisplay() { return *waveformDisplay_; }
    WaveformControlPanel& getControlPanel() { return *controlPanel_; }
    
    void setColors(const WaveformColors& colors);
    void setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate);
    
private:
    std::unique_ptr<WaveformDisplay> waveformDisplay_;
    std::unique_ptr<WaveformControlPanel> controlPanel_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformContainer)
};

} // namespace ui
} // namespace zenith
