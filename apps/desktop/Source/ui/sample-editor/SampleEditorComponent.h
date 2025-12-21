/*
  ==============================================================================

    SampleEditorComponent.h
    Created: 2025-12-05
    Author:  Zenith DAW

    ZENITH EDISON - Professional Sample Editor
    Designed to compete with FL Studio Edison
    
    Features:
    - Full waveform editing (cut, copy, paste, trim)
    - Audio processing (normalize, reverse, fade, gain)
    - Playhead with transport integration
    - Markers and regions
    - Spectrogram view option
    - Overview minimap
    - Undo/redo support

  ==============================================================================
*/

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include "SkiaComponent.h"
#include "../engine/AudioFilePool.h"


#include <core/SkCanvas.h>
#include <core/SkPath.h>
#include <core/SkPaint.h>

#include <vector>
#include <atomic>
#include <algorithm>
#include <memory>

namespace zenith {

//==============================================================================
// Marker/Region for sample editor
//==============================================================================
struct SampleMarker {
    juce::String id;
    juce::String name;
    double timeSeconds;
    SkColor color;
};

struct SampleRegion {
    juce::String id;
    juce::String name;
    double startTime;
    double endTime;
    SkColor color;
};

//==============================================================================
// Tool modes
//==============================================================================
enum class SampleEditorTool {
    Select,         // Default - selection tool
    Pencil,         // Draw/repair waveform
    Slice,          // Split at click points
    Zoom,           // Click to zoom
    Scrub           // Drag to audition
};

//==============================================================================
// View modes
//==============================================================================
enum class WaveformViewMode {
    Waveform,       // Standard waveform
    Spectrogram,    // Frequency view
    Combined        // Both overlaid
};

//==============================================================================
class SampleEditorComponent : public SkiaComponent,
                              public juce::ValueTree::Listener,
                              public juce::AudioIODeviceCallback
{
public:
    SampleEditorComponent(Engine& engine, ProjectState& projectState);
    ~SampleEditorComponent() override;

    //==============================================================================
    // AudioIODeviceCallback overrides
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    //==============================================================================
    // SkiaComponent overrides
    void drawSkia(SkCanvas* canvas) override;
    
    // JUCE Component overrides
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Timer for playhead updates and recording drain
    void timerCallback() override;

    //==============================================================================
    // Editor API
    void setClipToEdit(const juce::String& trackId, const juce::String& clipId);
    void clearClip();
    
    // Tool selection
    void setTool(SampleEditorTool tool) { currentTool_ = tool; repaint(); }
    SampleEditorTool getCurrentTool() const { return currentTool_; }
    
    // View mode
    void setViewMode(WaveformViewMode mode) { viewMode_ = mode; repaint(); }
    WaveformViewMode getViewMode() const { return viewMode_; }
    
    //==============================================================================
    // Zoom/Scroll API
    void zoomHorizontal(float factor, float centerX);
    void zoomVertical(float factor);
    void scrollHorizontal(float deltaPixels);
    void fitToWindow();
    void zoomToSelection();
    
    //==============================================================================
    // Selection API
    juce::Range<double> getSelection() const { return selection_; }
    void setSelection(double start, double end);
    void selectAll();
    void clearSelection();
    bool hasSelection() const { return !selection_.isEmpty(); }
    
    //==============================================================================
    // Editing Operations (all undoable)
    void cutSelection();
    void copySelection();
    void paste();
    void deleteSelection();
    void trimToSelection();
    void splitAtCursor();
    
    //==============================================================================
    // Processing Operations (all undoable)
    void normalize(float targetDb = 0.0f);
    void reverse();
    void fadeIn(double durationSeconds = 0.1);
    void fadeOut(double durationSeconds = 0.1);
    void adjustGain(float db);
    void silenceSelection();
    void removeOffset();  // DC offset removal
    
    //==============================================================================
    // Advanced Processing
    void timeStretch(float ratio);      // 0.5 = half speed, 2.0 = double speed
    void pitchShift(int semitones);     // +/- semitones
    void detectTransients(float sensitivity = 0.5f);  // Returns markers at transients
    void autoSlice(float sensitivity = 0.5f);         // Slice at transients
    void sliceToMidi();                 // Export slices to MIDI triggers
    
    //==============================================================================
    // RECORDING (Critical Feature)
    void startRecording();
    void stopRecording();
    bool isRecording() const { return isRecording_; }
    void setRecordInputChannel(int channel) { recordInputChannel_ = channel; }
    
    //==============================================================================
    // UNDO/REDO (Critical Feature)
    void undo();
    void redo();
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    void pushUndoState(const juce::String& description);
    
    //==============================================================================
    // SAVE/EXPORT (Critical Feature)
    void saveToFile();
    void saveAsNewFile(const juce::File& targetFile);
    void exportSelection(const juce::File& targetFile);
    bool hasUnsavedChanges() const { return hasUnsavedChanges_; }
    
    //==============================================================================
    // WARP MARKERS (Ableton-style)
    struct WarpMarker {
        double originalTime;  // Position in original audio
        double warpedTime;    // Position in warped audio
    };
    void addWarpMarker(double originalTime, double warpedTime);
    void removeWarpMarker(int index);
    void clearWarpMarkers();
    void quantizeToGrid(double gridSize);  // Snap transients to grid
    const std::vector<WarpMarker>& getWarpMarkers() const { return warpMarkers_; }
    
    enum class WarpMode { Beats, Tones, Texture, RePitch, Complex };
    void setWarpMode(WarpMode mode) { warpMode_ = mode; }
    WarpMode getWarpMode() const { return warpMode_; }
    
    //==============================================================================
    // PENCIL TOOL (Logic-style click/pop repair)
    void enablePencilTool(bool enable);
    void pencilDraw(float x, float y);  // Draw directly on waveform
    void smoothSelection(int windowSize = 5);  // Interpolate to fix clicks
    
    //==============================================================================
    // VOLUME/PAN ENVELOPES (Edison-style)
    struct EnvelopePoint {
        double time;
        float value;  // 0.0 to 1.0 for volume, -1.0 to 1.0 for pan
    };
    void addVolumeEnvelopePoint(double time, float volume);
    void addPanEnvelopePoint(double time, float pan);
    void applyVolumeEnvelope();
    void applyPanEnvelope();
    void clearEnvelopes();
    
    //==============================================================================
    // NOISE REDUCTION (Spectral Subtraction)
    void captureNoiseProfile();  // Capture noise from selection
    void applyNoiseReduction(float strength = 1.0f);
    bool hasNoiseProfile() const { return noiseProfile_ != nullptr; }
    
    //==============================================================================
    // EQ TOOL (Edison-style inline EQ)
    void applyEQ(const std::vector<std::pair<float, float>>& bands); // freq, gain pairs
    void applyHighPassFilter(float cutoffHz);
    void applyLowPassFilter(float cutoffHz);
    void applyBandPassFilter(float lowHz, float highHz);
    
    //==============================================================================
    // CONVOLUTION / REVERB (Edison-style)
    void applyConvolutionReverb(const juce::File& impulseResponse);
    void applySimpleReverb(float roomSize, float damping, float wetLevel);
    
    //==============================================================================
    // BLUR TOOL (Edison's unique feature)
    void applyBlur(float amount);  // FFT-based spectral blur
    
    //==============================================================================
    // STEREO TOOLS
    void convertToMono();
    void convertToStereo();
    void swapChannels();
    void adjustStereoWidth(float width);  // 0.0 = mono, 1.0 = normal, 2.0 = wide
    void extractCenter();   // Vocal isolation
    void extractSides();    // Remove center
    
    //==============================================================================
    // Analysis
    void toggleSpectrogram() { viewMode_ = (viewMode_ == WaveformViewMode::Spectrogram) ? WaveformViewMode::Waveform : WaveformViewMode::Spectrogram; repaint(); }
    void generateWaveformCache();       // Pre-compute peaks for fast rendering
    float getRMSLevel() const;          // Current RMS level
    float getPeakLevel() const;         // Current peak level
    
    //==============================================================================
    // Playback
    void play();
    void stop();
    void playSelection();
    void toggleLoop();
    bool isPlaying() const { return isPlaying_; }
    bool isLooping() const { return isLooping_; }
    double getPlayheadPosition() const { return playheadPosition_; }
    void setPlayheadPosition(double timeSeconds);
    
    //==============================================================================
    // Markers
    void addMarker(double timeSeconds, const juce::String& name = "");
    void removeMarker(const juce::String& markerId);
    void clearMarkers();
    const std::vector<SampleMarker>& getMarkers() const { return markers_; }
    
    //==============================================================================
    // Regions
    void addRegion(double start, double end, const juce::String& name = "");
    void removeRegion(const juce::String& regionId);
    void clearRegions();
    const std::vector<SampleRegion>& getRegions() const { return regions_; }
    
    //==============================================================================
    // Snap settings
    void setSnapEnabled(bool enabled) { snapEnabled_ = enabled; }
    bool isSnapEnabled() const { return snapEnabled_; }
    void setSnapToZeroCrossing(bool enabled) { snapToZeroCrossing_ = enabled; }
    bool isSnapToZeroCrossing() const { return snapToZeroCrossing_; }
    
    //==============================================================================
    // ValueTree Listener
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override {}
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override {}
    void valueTreeParentChanged(juce::ValueTree& tree) override {}
    void valueTreeRedirected(juce::ValueTree& tree) override {}

private:
    //==============================================================================
    Engine& engine_;
    ProjectState& projectState_;

    // Current state
    juce::String currentTrackId_;
    juce::String currentClipId_;
    juce::ValueTree clipNode_;
    AudioFilePool::HandlePtr audioHandle_;
    
    // Editing buffer (for non-destructive editing preview)
    std::unique_ptr<juce::AudioBuffer<float>> editBuffer_;
    bool hasUnsavedChanges_ = false;

    // View state
    double zoomLevel_ = 1.0;
    double timeOffset_ = 0.0;
    double viewWidthSeconds_ = 10.0;
    float verticalZoom_ = 1.0f;  // 1.0 = 100%, 2.0 = 200% amplitude
    
    // Selection
    juce::Range<double> selection_;
    bool isSelecting_ = false;
    double selectionAnchor_ = 0.0;  // For shift-extend
    
    // Clipboard
    static std::unique_ptr<juce::AudioBuffer<float>> clipboard_;
    static double clipboardSampleRate_;
    
    // Playback
    bool isPlaying_ = false;
    bool isLooping_ = false;
    double playheadPosition_ = 0.0;
    
    // Tool
    SampleEditorTool currentTool_ = SampleEditorTool::Select;
    WaveformViewMode viewMode_ = WaveformViewMode::Waveform;
    
    // Snap
    bool snapEnabled_ = true;
    bool snapToZeroCrossing_ = true;
    
    // Markers and regions
    std::vector<SampleMarker> markers_;
    std::vector<SampleRegion> regions_;
    
    // UI Layout
    static constexpr float toolbarHeight_ = 36.0f;
    static constexpr float rulerHeight_ = 24.0f;
    static constexpr float overviewHeight_ = 40.0f;
    static constexpr float scrollbarHeight_ = 12.0f;
    
    // Mouse state
    bool isDraggingPlayhead_ = false;
    bool isDraggingOverview_ = false;
    float lastMouseX_ = 0.0f;

    //==============================================================================
    // Drawing methods
    void drawToolbar(SkCanvas* canvas, const SkRect& bounds);
    void drawOverview(SkCanvas* canvas, const SkRect& bounds);
    void drawRuler(SkCanvas* canvas, const SkRect& bounds);
    void drawWaveform(SkCanvas* canvas, const SkRect& bounds);
    void drawSpectrogram(SkCanvas* canvas, const SkRect& bounds);
    void drawGrid(SkCanvas* canvas, const SkRect& bounds);
    void drawSelection(SkCanvas* canvas, const SkRect& bounds);
    void drawPlayhead(SkCanvas* canvas, const SkRect& bounds);
    void drawMarkers(SkCanvas* canvas, const SkRect& bounds);
    void drawRegions(SkCanvas* canvas, const SkRect& bounds);
    void drawScrollbar(SkCanvas* canvas, const SkRect& bounds);
    void drawBackground(SkCanvas* canvas, float w, float h, float radius);
    void drawEmptyState(SkCanvas* canvas, float w, float h);
    void drawBorder(SkCanvas* canvas, float w, float h, float radius);
    
    // Toolbar button drawing
    void drawToolbarButton(SkCanvas* canvas, const SkRect& bounds, 
                           const char* icon, const char* tooltip, 
                           bool isActive, bool isEnabled);
    
    //==============================================================================
    // Coordinate conversion
    float timeToPixels(double timeSeconds, float viewWidth) const;
    double pixelsToTime(float pixels, float viewWidth) const;
    juce::int64 timeToSamples(double timeSeconds) const;
    double samplesToTime(juce::int64 samples) const;
    
    // Zero-crossing snap
    double snapToNearestZeroCrossing(double timeSeconds) const;
    
    // Hit testing
    bool isInToolbar(float y) const { return y < toolbarHeight_; }
    bool isInOverview(float y) const { return y >= toolbarHeight_ && y < toolbarHeight_ + overviewHeight_; }
    bool isInRuler(float y) const { return y >= toolbarHeight_ + overviewHeight_ && y < toolbarHeight_ + overviewHeight_ + rulerHeight_; }
    bool isInWaveform(float y) const { return y >= toolbarHeight_ + overviewHeight_ + rulerHeight_ && y < getHeight() - scrollbarHeight_; }
    bool isInScrollbar(float y) const { return y >= getHeight() - scrollbarHeight_; }
    
    // Cached colors
    SkColor waveformColor_;
    SkColor selectionColor_;
    SkColor backgroundColor_;
    SkColor gridColor_;
    SkColor rulerColor_;
    SkColor playheadColor_;
    
    //==============================================================================
    // Recording state
    bool isRecording_ = false;
    int recordInputChannel_ = 0;
    std::unique_ptr<juce::AudioBuffer<float>> recordBuffer_;
    std::atomic<int> recordWritePos_{0};

    // Thread-safe FIFO for incoming audio
    std::unique_ptr<juce::AbstractFifo> incomingFifo_;
    juce::AudioBuffer<float> incomingBuffer_; // Ring buffer for thread exchange
    
    //==============================================================================
    // Undo/Redo
    struct UndoState {
        juce::String description;
        std::unique_ptr<juce::AudioBuffer<float>> buffer;
    };
    std::vector<UndoState> undoStack_;
    std::vector<UndoState> redoStack_;
    static constexpr int maxUndoLevels_ = 20;
    
    //==============================================================================
    // Warp Markers
    std::vector<WarpMarker> warpMarkers_;
    WarpMode warpMode_ = WarpMode::Beats;
    
    //==============================================================================
    // Envelopes
    std::vector<EnvelopePoint> volumeEnvelope_;
    std::vector<EnvelopePoint> panEnvelope_;
    
    //==============================================================================
    // Noise Reduction
    std::unique_ptr<std::vector<float>> noiseProfile_;
    
    //==============================================================================
    // Pencil tool state
    bool pencilToolEnabled_ = false;
    
    //==============================================================================
    // FFT for spectral processing (placeholder - would use FFTW or similar)
    static constexpr int fftSize_ = 2048;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleEditorComponent)
};

} // namespace zenith
