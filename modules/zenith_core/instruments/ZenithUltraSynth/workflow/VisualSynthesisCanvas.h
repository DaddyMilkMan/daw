/*
  ==============================================================================

    VisualSynthesisCanvas.h
    Created: [Date] Author: Claude AI
    Real-time visual feedback of synthesis with multiple visualization modes

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class VisualSynthesisCanvas : public juce::Component,
                            public juce::Timer
{
public:
    // Visualization modes
    enum class ViewMode
    {
        Waveform,    // Oscilloscope view
        Spectrum,    // FFT spectrum analyzer
        Spectrogram, // Time-frequency waterfall
        Phase,       // Phase scope
        Vectorscope, // Stereo image
        Envelope,    // ADSR envelope visualization
        Modulation,  // Modulation matrix flow
        Partials,    // Partial analysis
        Formants,    // Formant analysis
        Sonogram,    // Sonogram display
        Tuning,      // Tuning meter
        Loudness     // Loudness meter
    };

    // Analysis parameters
    struct AnalysisParameters
    {
        int fftSize = 2048;                        // FFT analysis size
        int overlapFactor = 4;                     // Overlap factor for spectrogram
        float windowType = 0.5f;                   // 0=rect, 0.5=hann, 1=blackman
        float sampleRate = 44100.0f;              // Sample rate
        float updateRate = 60.0f;                 // Update rate (Hz)
        bool usePeakHold = true;                   // Peak hold functionality
        float peakHoldTime = 2.0f;                 // Peak hold time (seconds)
        bool useSmoothing = true;                  // Use smoothing for display
        float smoothingTime = 0.1f;                // Smoothing time (seconds)
        bool showGrid = true;                      // Show grid
        bool showLabels = true;                    // Show labels
        bool showControls = true;                  // Show mode controls
        bool showStatistics = true;                 // Show statistics
        bool useHistory = true;                     // Use history for some modes
        int historySize = 100;                     // History buffer size
    };

    // Color schemes
    enum class ColorScheme
    {
        Default,     // Default color scheme
        Dark,        // Dark theme
        Light,       // Light theme
        Monochrome,  // Monochrome
        Rainbow,     // Rainbow colors
        Spectrum,    // Spectrum colors
        Custom       // Custom colors
    };

    // Grid styles
    enum class GridStyle
    {
        Lines,       // Line grid
        Dots,        // Dot grid
        Cross,       // Cross grid
        None         // No grid
    };

    // Analysis data structure
    struct AnalysisData
    {
        // Time domain data
        std::vector<float> waveform;
        std::vector<float> envelope;
        float rmsLevel;
        float peakLevel;
        float zeroCrossingRate;
        float crestFactor;

        // Frequency domain data
        std::vector<float> spectrum;
        std::vector<float> phase;
        std::vector<float> magnitude;
        std::vector<float> real;
        std::vector<float> imag;
        float spectralCentroid;
        float spectralSpread;
        float spectralSkewness;
        float spectralKurtosis;
        float spectralRolloff;

        // Partial analysis
        std::vector<float> partials;
        std::vector<float> partialAmplitudes;
        std::vector<float> partialPhases;
        float fundamental;
        float inharmonicity;

        // Formant analysis
        std::vector<float> formants;
        std::vector<float> formantAmplitudes;
        std::vector<float> formantBandwidths;
        int numFormants;

        // Loudness analysis
        float integratedLoudness;
        float momentaryLoudness;
        float shortTermLoudness;
        float loudnessRange;

        // History data
        std::vector<std::vector<float>> timeHistory;
        std::vector<std::vector<float>> freqHistory;
        std::vector<float> levelHistory;
    };

    VisualSynthesisCanvas();
    ~VisualSynthesisCanvas();

    // Component lifecycle
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Visualization control
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const;
    void setDisplayMode(int displayMode); // Alternative for multiple views
    int getDisplayMode() const;

    // Analysis parameters
    void setAnalysisParameters(const AnalysisParameters& params);
    AnalysisParameters getAnalysisParameters() const;
    void updateAnalysisParameters();

    // Audio input
    void setAudioSource(juce::AudioBuffer<float>* source);
    void setAudioSource(const juce::AudioBuffer<float>& source);
    void clearAudioSource();

    // Real-time analysis
    void updateAnalysis();
    void analyzeAudio(const juce::AudioBuffer<float>& buffer);
    void updateDisplay();

    // Display options
    void setColorScheme(ColorScheme scheme);
    ColorScheme getColorScheme() const;
    void setGridStyle(GridStyle style);
    GridStyle getGridStyle() const;
    void setShowGrid(bool show);
    bool isGridVisible() const;
    void setShowLabels(bool show);
    bool areLabelsVisible() const;
    void setShowStatistics(bool show);
    bool areStatisticsVisible() const;

    // Zoom and pan
    void setZoomLevel(float level);
    float getZoomLevel() const;
    void setCenterPosition(float position);
    float getCenterPosition() const;
    void setZoomRange(juce::Range<float> range);
    juce::Range<float> getZoomRange() const;

    // Overlay options
    void setOverlayEnabled(bool enabled);
    bool isOverlayEnabled() const;
    void setOverlayOpacity(float opacity);
    float getOverlayOpacity() const;
    void setOverlayMode(ViewMode mode);
    ViewMode getOverlayMode() const;

    // Performance optimization
    void setUpdateRate(float rate);
    float getUpdateRate() const;
    void setQualityMode(bool highQuality);
    bool isHighQuality() const;
    void setUseAntiAliasing(bool use);
    bool isAntiAliasingEnabled() const;

    // Statistics display
    void setStatisticsVisible(bool visible);
    bool areStatisticsVisible() const;
    void setStatisticsFormat(int format);
    int getStatisticsFormat() const;

    // Export/Import
    bool exportAsImage(const juce::File& file);
    bool exportAsData(const juce::File& file);
    bool importColorScheme(const juce::File& file);
    bool exportColorScheme(const juce::File& file);

    // Customization
    void setCustomColors(const std::map<juce::String, juce::Colour>& colors);
    std::map<juce::String, juce::Colour> getCustomColors() const;
    void setBackgroundColor(juce::Colour color);
    juce::Colour getBackgroundColor() const;
    void setGridColor(juce::Colour color);
    juce::Colour getGridColor() const;
    void setTextColor(juce::Colour color);
    juce::Colour getTextColor() const;

    // Callbacks
    void setAnalysisCompleteListener(std::function<void()> callback);
    void setModeChangeListener(std::function<void(ViewMode)> callback);
    void setParameterChangeListener(std::function<void()> callback);

    // State management
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);

private:
    // Visualization state
    ViewMode viewMode_;
    int displayMode_;
    AnalysisParameters analysisParams_;
    juce::AudioBuffer<float>* audioSource_;
    bool needsUpdate_;

    // Display options
    ColorScheme colorScheme_;
    GridStyle gridStyle_;
    bool showGrid_;
    bool showLabels_;
    bool showStatistics_;
    bool overlayEnabled_;
    float overlayOpacity_;
    ViewMode overlayMode_;
    bool highQuality_;
    bool useAntiAliasing_;

    // Zoom and pan
    float zoomLevel_;
    float centerPosition_;
    juce::Range<float> zoomRange_;

    // Analysis data
    AnalysisData analysisData_;
    std::vector<float> smoothedData_;
    std::vector<std::vector<float>> timeHistory_;
    std::vector<std::vector<float>> freqHistory_;
    std::vector<float> levelHistory_;

    // Timing
    juce::uint64 lastAnalysisTime_;
    juce::uint64 lastUpdateTime_;
    float updateInterval_;

    // Custom colors
    std::map<juce::String, juce::Colour> customColors_;
    juce::Colour backgroundColor_;
    juce::Colour gridColor_;
    juce::Colour textColor_;

    // Performance optimization
    juce::Image renderBuffer_;
    bool renderBufferValid_;
    juce::Rectangle<float> lastViewport_;

    // Callbacks
    std::function<void()> analysisCompleteListener_;
    std::function<void(ViewMode)> modeChangeListener_;
    std::function<void()> parameterChangeListener_;

    // UI components
    std::unique_ptr<juce::TooltipWindow> tooltipWindow_;
    std::unique_ptr<juce::Component> modeSelector_;
    std::unique_ptr<juce::Component> zoomControls_;
    std::unique_ptr<juce::Component> statisticsDisplay_;

    // Private helper methods
    void initializeCanvas();
    void initializeColors();
    void initializeUI();

    // Visualization methods
    void drawWaveform(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g);
    void drawSpectrogram(juce::Graphics& g);
    void drawPhase(juce::Graphics& g);
    void drawVectorscope(juce::Graphics& g);
    void drawEnvelope(juce::Graphics& g);
    void drawModulation(juce::Graphics& g);
    void drawPartials(juce::Graphics& g);
    void drawFormants(juce::Graphics& g);
    void drawSonogram(juce::Graphics& g);
    void drawTuning(juce::Graphics& g);
    void drawLoudness(juce::Graphics& g);

    // Grid drawing
    void drawGrid(juce::Graphics& g, const juce::Rectangle<float>& area);
    void drawGridLines(juce::Graphics& g, const juce::Rectangle<float>& area);
    void drawGridDots(juce::Graphics& g, const juce::Rectangle<float>& area);
    void drawGridCross(juce::Graphics& g, const juce::Rectangle<float>& area);

    // Overlay drawing
    void drawOverlay(juce::Graphics& g);
    void drawComparisonOverlay(juce::Graphics& g);

    // Data processing
    void performFFT(const std::vector<float>& input, std::vector<float>& spectrum);
    void performWindowing(std::vector<float>& data);
    void calculateEnvelope(const std::vector<float>& data);
    void calculatePartials(const std::vector<float>& spectrum);
    void calculateFormants(const std::vector<float>& spectrum);
    void calculateLoudness(const std::vector<float>& data);
    void updateHistory();

    // Data smoothing
    void smoothData(std::vector<float>& data);
    void smoothData(std::vector<std::vector<float>>& data);

    // Coordinate transformation
    juce::Point<float> dataToScreen(const juce::Point<float>& data) const;
    juce::Point<float> screenToData(const juce::Point<float>& screen) const;
    float dataToScreenY(float value) const;
    float screenToDataY(float screenY) const;
    float dataToScreenX(float value) const;
    float screenToDataX(float screenX) const;

    // Utility methods
    juce::Colour getDisplayColor(const juce::String& name) const;
    juce::Font getDisplayFont() const;
    juce::String formatFrequency(float freq) const;
    juce::String formatLevel(float level) const;
    juce::String formatTime(float time) const;
    juce::String formatPhase(float phase) const;

    // Statistics display
    void drawStatistics(juce::Graphics& g);
    void drawStatisticsPanel(juce::Graphics& g);
    juce::StringArray getStatisticsText() const;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Keyboard interaction
    void keyPressed(const juce::KeyPress& key) override;

    // File operations
    bool saveImage(const juce::File& file);
    bool saveData(const juce::File& file);
    bool loadColors(const juce::File& file);
    bool saveColors(const juce::File& file);

    // State management
    juce::ValueTree createValueTreeFromState() const;
    void createStateFromValueTree(const juce::ValueTree& state);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualSynthesisCanvas)
};

} // namespace Zenith