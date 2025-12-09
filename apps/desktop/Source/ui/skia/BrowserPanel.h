/*
  ==============================================================================

    BrowserPanel.h
    Created: 2025-11-28
    Refactored: 2025-12-05 for Universal Browser Model + Drag/Preview/Async
    Updated: 2025-12-08 for Cloud Preset Integration

    Universal Media Browser View.
    Features:
    - Tree navigation with icons
    - Async background scanning
    - Audio preview with waveform
    - Drag-and-drop to tracks
    - Cloud preset browsing and download

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"
#include "../../browser/BrowserModel.h"
#include "../../browser/BrowserScanner.h"
#include "../../browser/BrowserPreviewEngine.h"
#include "../../browser/BrowserDragSource.h"
#include "../../network/ZenithCloudClient.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkColor.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkPath.h>
#include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

// View mode for the browser (local vs cloud)
enum class BrowserViewMode
{
    Local,      // Local files, plugins, presets
    Cloud       // Cloud-shared presets
};

class BrowserPanel : public SkiaComponent, 
                     public juce::ChangeListener,
                     public juce::DragAndDropContainer
{
public:
    explicit BrowserPanel(BrowserModel& model);
    ~BrowserPanel() override;

    // SkiaComponent overrides
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    // ChangeListener override
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
    // Timer for UI updates (progress bar, waveform animation)
    void timerCallback() override;

    void setSearchText(const juce::String& text);
    
    // Preview engine access (for audio routing)
    BrowserPreviewEngine& getPreviewEngine() { return previewEngine_; }
    
    // Cloud client access
    ZenithCloudClient& getCloudClient() { return cloudClient_; }
    
    // Add custom folder to user library
    void showAddFolderDialog();
    
    // Cloud preset operations
    void refreshCloudPresets();
    void downloadCloudPreset(const juce::String& presetId);
    
    // View mode
    void setViewMode(BrowserViewMode mode);
    BrowserViewMode getViewMode() const { return viewMode_; }
    
    // Callbacks
    std::function<void(std::shared_ptr<BrowserItem>)> onItemDoubleClicked;
    std::function<void(const CloudPreset&)> onCloudPresetDoubleClicked;

private:
    BrowserModel& model_;
    BrowserScanner scanner_;
    BrowserPreviewEngine previewEngine_;
    ZenithCloudClient cloudClient_;
    
    // View State
    BrowserViewMode viewMode_ = BrowserViewMode::Local;
    std::shared_ptr<BrowserItem> currentRoot_;
    std::vector<std::shared_ptr<BrowserItem>> displayItems_;
    
    // Cloud presets
    std::vector<CloudPreset> cloudPresets_;
    bool isLoadingCloudPresets_ = false;
    juce::String cloudLoadError_;
    int cloudSelectedIndex_ = -1;
    int cloudHoverIndex_ = -1;
    
    juce::String searchText_;
    int selectedIndex_ = -1;
    int hoverIndex_ = -1;
    int scrollOffset_ = 0;
    
    // Drag state
    bool isDragging_ = false;
    juce::Point<int> dragStartPos_;
    static constexpr int dragThreshold_ = 5;
    
    // UI Layout Metrics
    juce::Rectangle<int> searchBoxBounds_;
    juce::Rectangle<int> backButtonBounds_;
    juce::Rectangle<int> addFolderButtonBounds_;
    juce::Rectangle<int> previewAreaBounds_;
    juce::Rectangle<int> progressBarBounds_;
    juce::Rectangle<int> listAreaBounds_;
    
    // Preview controls
    juce::Rectangle<int> playButtonBounds_;
    juce::Rectangle<int> stopButtonBounds_;
    juce::Rectangle<int> loopButtonBounds_;
    juce::Rectangle<int> autoPlayButtonBounds_;
    juce::Rectangle<int> volumeSliderBounds_;
    juce::Rectangle<int> waveformBounds_;
    
    static constexpr int headerHeight_ = 45;
    static constexpr int filterBarHeight_ = 28;
    static constexpr int searchBoxHeight_ = 30;
    static constexpr int itemHeight_ = 28;
    static constexpr int previewHeight_ = 80;
    static constexpr int progressBarHeight_ = 4;
    
    // Filter tabs (including Cloud)
    juce::Rectangle<int> filterAllBounds_;
    juce::Rectangle<int> filterAudioBounds_;
    juce::Rectangle<int> filterMidiBounds_;
    juce::Rectangle<int> filterPluginBounds_;
    juce::Rectangle<int> filterCloudBounds_;
    juce::Rectangle<int> filterBarBounds_;
    
    // Waveform cache
    std::vector<float> waveformData_;
    juce::File waveformFile_;
    
    // File chooser for adding folders
    std::unique_ptr<juce::FileChooser> fileChooser_;
    
    // Internal methods
    void updateDisplayItems();
    void navigateTo(std::shared_ptr<BrowserItem> folder);
    void navigateUp();
    void loadWaveform(const juce::File& file);
    void showContextMenu(int itemIndex, juce::Point<int> position);
    void toggleFavorite(std::shared_ptr<BrowserItem> item);

    // Drawing helpers
    void drawHeader(SkCanvas* canvas);
    void drawFilterBar(SkCanvas* canvas);
    void drawProgressBar(SkCanvas* canvas);
    void drawItemList(SkCanvas* canvas);
    void drawBrowserItem(SkCanvas* canvas, int index, const juce::Rectangle<int>& bounds);
    void drawCloudItem(SkCanvas* canvas, int index, const juce::Rectangle<int>& bounds);
    void drawCloudLoadingState(SkCanvas* canvas);
    void drawPreviewArea(SkCanvas* canvas);
    void drawWaveform(SkCanvas* canvas, const SkRect& bounds);
    void drawIcon(SkCanvas* canvas, BrowserItemType type, float x, float y, float size);
    void drawCloudIcon(SkCanvas* canvas, float x, float y, float size);
    void drawButton(SkCanvas* canvas, const juce::Rectangle<int>& bounds, 
                    const juce::String& icon, bool active, bool hovered);
    void drawFilterTab(SkCanvas* canvas, const juce::Rectangle<int>& bounds,
                       const juce::String& label, bool active);

    // Hit testing
    int getItemIndexAt(int y) const;
    bool isInPreviewArea(int y) const;
    
    // Drag handling
    void startItemDrag(int itemIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
