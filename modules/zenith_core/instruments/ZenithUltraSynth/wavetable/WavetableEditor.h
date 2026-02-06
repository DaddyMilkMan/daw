/*
  ==============================================================================

    WavetableEditor.h
    Created: [Date] Author: Claude AI
    Visual waveform editing with advanced tools

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"
#include "AdvancedWavetableEngine.h"

namespace Zenith
{

class WavetableEditor : public juce::Component,
                       public juce::FileDragAndDropTarget,
                       public juce::DragAndDropTarget
{
public:
    // Editing tools
    enum class Tool
    {
        Draw,      // Freehand drawing
        Smooth,    // Smooth tool
        Mutate,    // Genetic mutation
        Morph,     // Morph between frames
        Select,    // Selection tool
        Erase,     // Erase tool
        Sample,    // Sample tool
        Zoom,      // Zoom tool
        Pan        // Pan tool
    };

    // Editing modes
    enum class EditMode
    {
        Frame,     // Edit individual frames
        Morph,      // Edit morphing paths
        Global,    // Edit all frames
        Template   // Edit from template
    };

    // Interpolation types
    enum class InterpolationType
    {
        Linear,     // Linear interpolation
        Cosine,     // Cosine interpolation
        Cubic,      // Cubic spline
        Sinc,       // Sinc interpolation
        Step        // Step interpolation
    };

    // Visualization options
    enum class DisplayMode
    {
        Waveform,   // Single waveform view
        Morphing,   // Morphing visualization
        Frequency,  // Frequency domain view
        Phase,      // Phase view
        Comparison  // Side-by-side comparison
    };

    WavetableEditor();
    ~WavetableEditor();

    // Component lifecycle
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // File operations
    bool loadWavetable(const juce::File& file);
    bool saveWavetable(const juce::File& file);
    bool exportAsAudio(const juce::File& file);
    bool importFromAudio(const juce::File& file);

    // Wavetable management
    void setWavetable(const AdvancedWavetableEngine::WavetableData& table);
    AdvancedWavetableEngine::WavetableData getWavetable() const;
    void clearWavetable();
    void resetToDefaults();

    // Editing tools
    void setTool(Tool tool);
    Tool getCurrentTool() const;
    void setEditMode(EditMode mode);
    EditMode getEditMode() const;
    void setInterpolationType(InterpolationType type);
    InterpolationType getInterpolationType() const;

    // Frame operations
    void addFrame();
    void removeFrame();
    void duplicateFrame();
    void moveFrame(int fromIndex, int toIndex);
    void setNumFrames(int count);
    int getCurrentFrame() const;
    void setCurrentFrame(int index);

    // Editing operations
    void drawPoint(const juce::Point<float>& position, float value);
    void smoothSelection(int startIndex, int endIndex);
    void mutateFrame(int frameIndex, float mutationAmount);
    void interpolateFrames(int startFrame, int endFrame, InterpolationType type);
    void morphFrames(int frame1, int frame2, float morphAmount);

    // Selection management
    void selectFrame(int index);
    void selectFrames(juce::Range<int> range);
    void clearSelection();
    juce::Range<int> getSelection() const;
    bool isFrameSelected(int index) const;

    // Visualization
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const;
    void setZoomLevel(float level);
    float getZoomLevel() const;
    void setCenterPosition(float position);
    float getCenterPosition() const;

    // Grid and snapping
    void setGridEnabled(bool enabled);
    bool isGridEnabled() const;
    void setSnapToGrid(bool enabled);
    bool isSnapToGrid() const;
    void setGridSize(float size);
    float getGridSize() const;

    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    void clearHistory();

    // Copy/Paste
    void copySelection();
    void pasteSelection();
    void cutSelection();
    void deleteSelection();

    // Analysis tools
    void showFrequencyAnalysis();
    void showPhaseAnalysis();
    void showHarmonicContent();
    void showAutoCorrelation();

    // Export/Import
    bool exportAsImage(const juce::File& file);
    bool exportAsJSON(const juce::File& file);
    bool importFromJSON(const juce::File& file);

    // Callbacks
    void setWavetableChangeListener(std::function<void()> callback);
    void setUndoRedoListener(std::function<void(bool, bool)> callback);

    // State management
    void saveState();
    void restoreState();
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);

    // Mouse cursor
    juce::MouseCursor getMouseCursorForPosition(const juce::Point<int>& position) override;

private:
    // Editor state
    Tool currentTool_;
    EditMode editMode_;
    InterpolationType interpolationType_;
    DisplayMode displayMode_;
    int currentFrame_;
    juce::Range<int> selection_;
    float zoomLevel_;
    float centerPosition_;
    bool gridEnabled_;
    bool snapToGrid_;
    float gridSize_;

    // Wavetable data
    AdvancedWavetableEngine::WavetableData wavetable_;
    std::vector<std::vector<float>> originalFrames_;
    bool hasChanges_;

    // Visualization
    juce::Image backgroundImage_;
    juce::Path waveformPath_;
    juce::Path morphPath_;
    juce::Rectangle<float> viewport_;
    juce::Point<float> lastMousePosition_;

    // Editing state
    bool isDrawing_;
    bool isPanning_;
    bool isSelecting_;
    juce::Point<float> dragStart_;
    juce::Point<float> dragEnd_;
    std::vector<float> clipboardData_;

    // Undo/Redo system
    struct EditAction
    {
        juce::String actionName;
        juce::ValueTree beforeState;
        juce::ValueTree afterState;
        juce::uint64 timestamp;
    };
    std::vector<EditAction> undoStack_;
    std::vector<EditAction> redoStack_;
    int maxHistorySize_;

    // Analysis data
    struct AnalysisData
    {
        std::vector<float> spectrum;
        std::vector<float> phase;
        std::vector<float> harmonicContent;
        std::vector<float> autoCorrelation;
        float rms;
        float peak;
        float fundamental;
        float brightness;
    } analysisData_;

    // Callbacks
    std::function<void()> wavetableChangeListener_;
    std::function<void(bool, bool)> undoRedoListener_;

    // UI components
    std::unique_ptr<juce::TooltipWindow> tooltipWindow_;
    std::unique_ptr<juce::FileChooser> fileChooser_;

    // Private helper methods
    void initializeUI();
    void updateVisualization();
    void drawGrid(juce::Graphics& g);
    void drawWaveform(juce::Graphics& g);
    void drawMorphing(juce::Graphics& g);
    void drawFrequencySpectrum(juce::Graphics& g);
    drawPhase(juce::Graphics& g);
    drawSelection(juce::Graphics& g);
    drawTools(juce::Graphics& g);

    // Mouse event handling
    void handleDrawing(const juce::MouseEvent& e);
    void handleSmoothing(const juce::MouseEvent& e);
    void handleMutating(const juce::MouseEvent& e);
    void handleMorphing(const juce::MouseEvent& e);
    void handleSelection(const juce::MouseEvent& e);
    void handleErasing(const juce::MouseEvent& e);
    void handleSampling(const juce::MouseEvent& e);
    void handleZooming(const juce::MouseEvent& e);
    void handlePanning(const juce::MouseEvent& e);

    // Coordinate conversion
    juce::Point<float> screenToSample(const juce::Point<float>& screen) const;
    juce::Point<float> sampleToScreen(const juce::Point<float>& sample) const;
    float screenToX(float screenX) const;
    float screenToY(float screenY) const;
    float xToScreen(float x) const;
    float yToScreen(float y) const;

    // Edit operations
    void applySmoothing(std::vector<float>& frame, int start, int end);
    void applyMutation(std::vector<float>& frame, float amount);
    void applyInterpolation(std::vector<float>& frame1, std::vector<float>& frame2, float t, InterpolationType type);
    void applyMorph(std::vector<float>& frame1, std::vector<float>& frame2, float amount);

    // Analysis operations
    void updateAnalysisData();
    void performFFTAnalysis();
    void performPhaseAnalysis();
    void performHarmonicAnalysis();
    void performAutoCorrelation();

    // File operations
    bool loadFromBinary(const juce::File& file);
    bool saveToBinary(const juce::File& file);
    bool loadFromJSON(const juce::File& file);
    bool saveToJSON(const juce::File& file);
    bool exportAsWAV(const juce::File& file);
    bool importFromWAV(const juce::File& file);

    // Undo/Redo operations
    void pushUndoState(const juce::String& actionName);
    void popUndoState();
    void popRedoState();
    void clearUndoRedoStacks();

    // State management
    juce::ValueTree createValueTreeFromWavetable() const;
    void createWavetableFromValueTree(const juce::ValueTree& state);

    // UI utilities
    void showTooltip(const juce::String& text, const juce::Point<int>& position);
    void updateCursor();
    void updateTitle();
    void showPopupMenu();

    // Validation
    bool validateWavetable(const AdvancedWavetableEngine::WavetableData& table) const;
    bool validateFrame(const std::vector<float>& frame) const;
    void normalizeFrame(std::vector<float>& frame) const;

    // Event handling
    void handleFileDrag(const juce::StringArray& files, const juce::MouseDragTargetDetails& dragDetails) override;
    bool isFileDragAccepted(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, const juce::MouseDragTargetDetails& dragDetails) override;
    void fileDragMove(const juce::StringArray& files, const juce::MouseDragTargetDetails& dragDetails) override;
    void fileDragExit(const juce::StringArray& files) override;
    void fileDropped(const juce::StringArray& files, const juce::MouseDragTargetDetails& dragDetails) override;

    // Keyboard shortcuts
    void handleKeyPress(const juce::KeyPress& key) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableEditor)
};

} // namespace Zenith