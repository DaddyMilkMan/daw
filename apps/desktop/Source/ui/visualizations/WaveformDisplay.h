/*
  ==============================================================================
    WaveformDisplay.h
    Real-time waveform visualization component - production ready
    Phase 4: User Interface
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

namespace zenith {
namespace ui {

// Waveform rendering modes
enum class WaveformMode {
    Normal,
    RMS,
    Peak,
    Spectrogram,
    Phase,
    Stereo
};

// Visualization colors
struct WaveformColors {
    juce::Colour background = juce::Colour(30, 30, 30);
    juce::Colour waveform = juce::Colour(100, 200, 255);
    juce::Colour rms = juce::Colour(255, 200, 100);
    juce::Colour peak = juce::Colour(255, 100, 100);
    juce::Colour grid = juce::Colour(60, 60, 60);
    juce::Colour text = juce::Colour(200, 200, 200);
    juce::Colour selection = juce::Colour(100, 255, 100, 100);
    juce::Colour clipping = juce::Colour(255, 0, 0);
};

// Waveform data point
struct WaveformData {
    float leftChannel = 0.0f;
    float rightChannel = 0.0f;
    float rms = 0.0f;
    float peak = 0.0f;
    bool isClipping = false;
    juce::Time timestamp;
};

// Real-time waveform display
class WaveformDisplay : public juce::Component,
                       public juce::Timer,
                       public juce::MouseListener {
public:
    WaveformDisplay();
    ~WaveformDisplay() override;
    
    // Audio data input
    void setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void addAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void clearAudioData();
    
    // Display configuration
    void setMode(WaveformMode mode);
    void setColors(const WaveformColors& colors);
    void setShowGrid(bool show);
    void setShowLabels(bool show);
    void setShowRMS(bool show);
    void setShowPeaks(bool show);
    
    // Zoom and pan
    void setZoomLevel(float zoom);
    void setScrollPosition(float position);
    void setViewRange(double startTime, double endTime);
    void fitToWindow();
    
    // Selection
    void setSelection(double startTime, double endTime);
    void clearSelection();
    std::pair<double, double> getSelection() const;
    bool hasSelection() const;
    
    // Playback position
    void setPlaybackPosition(double time);
    void clearPlaybackPosition();
    double getPlaybackPosition() const;
    
    // Analysis overlay
    void showGenreDetection(bool show);
    void showQualityAnalysis(bool show);
    void showSpectrumAnalysis(bool show);
    void updateAnalysisData(const juce::String& analysis);
    
    // Export
    juce::Image exportWaveform(int width, int height);
    void exportToFile(const juce::File& file, int width, int height);
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
    // Timer callback for real-time updates
    void timerCallback() override;
    
private:
    // Audio data
    std::vector<WaveformData> waveformBuffer;
    std::mutex dataMutex;
    double currentSampleRate = 44100.0;
    double audioDuration = 0.0;
    
    // Display settings
    WaveformMode currentMode = WaveformMode::Normal;
    WaveformColors colors;
    bool showGrid = true;
    bool showLabels = true;
    bool showRMS = true;
    bool showPeaks = true;
    
    // View settings
    float zoomLevel = 1.0f;
    float scrollPosition = 0.0f;
    double viewStartTime = 0.0;
    double viewEndTime = 0.0;
    double playbackPosition = -1.0;
    
    // Selection
    bool hasSelectionFlag = false;
    double selectionStart = 0.0;
    double selectionEnd = 0.0;
    bool isSelecting = false;
    double selectionAnchor = 0.0;
    
    // Analysis overlay
    bool showGenreDetectionFlag = false;
    bool showQualityAnalysisFlag = false;
    bool showSpectrumAnalysisFlag = false;
    juce::String currentAnalysis;
    
    // Rendering
    std::unique_ptr<juce::Image> waveformImage;
    std::unique_ptr<juce::Image> spectrogramImage;
    bool needsRedraw = true;
    
    // Grid settings
    int majorGridInterval = 1;  // seconds
    int minorGridInterval = 1;  // divisions per major
    float gridOpacity = 0.3f;
    
    // Performance optimization
    static constexpr int MAX_BUFFER_SIZE = 1000000;  // 1M samples
    static constexpr int REDRAW_INTERVAL_MS = 50;    // 20 FPS
    int lastRedrawTime = 0;
    
    // Coordinate conversion
    double timeToX(double time) const;
    double xToTime(double x) const;
    float amplitudeToY(float amplitude) const;
    float yToAmplitude(float y) const;
    
    // Drawing methods
    void drawBackground(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawWaveform(juce::Graphics& g);
    void drawRMS(juce::Graphics& g);
    void drawPeaks(juce::Graphics& g);
    void drawSelection(juce::Graphics& g);
    void drawPlaybackPosition(juce::Graphics& g);
    void drawLabels(juce::Graphics& g);
    void drawAnalysisOverlay(juce::Graphics& g);
    
    // Specialized drawing
    void drawNormalWaveform(juce::Graphics& g);
    void drawStereoWaveform(juce::Graphics& g);
    void drawSpectrogram(juce::Graphics& g);
    void drawPhaseDisplay(juce::Graphics& g);
    
    // Data processing
    void processAudioData(const juce::AudioBuffer<float>& buffer);
    void calculateRMSAndPeaks();
    void generateSpectrogram();
    void calculatePhase();
    
    // Optimization
    void optimizeBuffer();
    void updateWaveformImage();
    void invalidateWaveform();
    
    // Interaction helpers
    void startSelection(double time);
    void updateSelection(double time);
    void endSelection();
    bool isInSelection(double time) const;
    
    // Utility
    juce::String formatTime(double time) const;
    juce::String formatAmplitude(float amplitude) const;
    float getAmplitudeAtTime(double time, int channel = 0) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

// Waveform control panel
class WaveformControlPanel : public juce::Component,
                           public juce::Button::Listener,
                           public juce::ComboBox::Listener,
                           public juce::Slider::Listener {
public:
    WaveformControlPanel(WaveformDisplay& display);
    ~WaveformControlPanel() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Listeners
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void sliderValueChanged(juce::Slider* slider) override;
    
private:
    WaveformDisplay& waveformDisplay;
    
    // Display controls
    std::unique_ptr<juce::ComboBox> modeComboBox;
    std::unique_ptr<juce::ToggleButton> gridToggle;
    std::unique_ptr<juce::ToggleButton> labelsToggle;
    std::unique_ptr<juce::ToggleButton> rmsToggle;
    std::unique_ptr<juce::ToggleButton> peaksToggle;
    
    // Zoom controls
    std::unique_ptr<juce::Slider> zoomSlider;
    std::unique_ptr<juce::TextButton> zoomInButton;
    std::unique_ptr<juce::TextButton> zoomOutButton;
    std::unique_ptr<juce::TextButton> fitToWindowButton;
    
    // Navigation controls
    std::unique_ptr<juce::TextButton> goStartButton;
    std::unique_ptr<juce::TextButton> goEndButton;
    std::unique_ptr<juce::TextButton> playPauseButton;
    std::unique_ptr<juce::TextButton> loopButton;
    
    // Analysis controls
    std::unique_ptr<juce::ToggleButton> genreToggle;
    std::unique_ptr<juce::ToggleButton> qualityToggle;
    std::unique_ptr<juce::ToggleButton> spectrumToggle;
    
    // Export controls
    std::unique_ptr<juce::TextButton> exportButton;
    std::unique_ptr<juce::TextButton> screenshotButton;
    
    // Layout
    void createDisplayControls();
    void createZoomControls();
    void createNavigationControls();
    void createAnalysisControls();
    void createExportControls();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformControlPanel)
};

// Waveform container with controls
class WaveformContainer : public juce::Component {
public:
    WaveformContainer();
    ~WaveformContainer() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Access to components
    WaveformDisplay& getDisplay() { return *waveformDisplay; }
    WaveformControlPanel& getControlPanel() { return *controlPanel; }
    
    // Configuration
    void setColors(const WaveformColors& colors);
    void setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate);
    
private:
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<WaveformControlPanel> controlPanel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformContainer)
};

} // namespace ui
} // namespace zenith
