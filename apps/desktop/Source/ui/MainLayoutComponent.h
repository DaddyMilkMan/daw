/*
  ==============================================================================

    MainLayoutComponent.h
    Created: 2025-11-28
    Author:  Dr. Aris Vokos + Leo Rossi + Isabella Moretti

    Tri-pane layout manager for Browser, Session View, and Arranger View.
    The heart of the "Perfect DAW" layout.

  ==============================================================================
*/

#pragma once

#include "../../Source/ui/skia/SkiaComponent.h"
#include "../../Source/ui/skia/BrowserPanel.h"
#include "../../include/ui/SessionViewComponent.h"
#include "../../include/ui/ArrangerComponent.h"
#include "../../include/ProjectState.h"

#include "../browser/BrowserModel.h"
#include "SampleEditorComponent.h"

class Engine; // Forward declaration

namespace zenith {

/**
 * @brief Main layout component managing Browser, Session, and Arranger views
 * 
 * Layout structure:
 * [Browser (collapsible)] [Session/Arranger (toggleable)] 
 */
class MainLayoutComponent : public SkiaComponent {
public:
    explicit MainLayoutComponent(Engine& engine, ProjectState& state);
    ~MainLayoutComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Skia rendering
    void drawSkia(SkCanvas* canvas) override;

    // View management
    void toggleView();  // Toggle between Session and Arranger
    void toggleBrowser();  // Show/hide browser panel
    
    bool isSessionView() const { return showSessionView_; }
    bool isBrowserVisible() const { return browserVisible_; }

private:
    Engine& engine_;
    ProjectState& projectState_;
    
    // View state
    bool showSessionView_ = false;  // false = Arranger, true = Session
    bool browserVisible_ = true;
    
    // Layout constants
    static constexpr int browserWidth_ = 300;
    static constexpr int minCenterWidth_ = 400;
    
    // Components
    std::unique_ptr<ArrangerComponent> arrangerComponent_;
    std::unique_ptr<SessionViewComponent> sessionViewComponent_;
    std::unique_ptr<BrowserPanel> browserPanel_;
    std::unique_ptr<BrowserModel> browserModel_;
    std::unique_ptr<SampleEditorComponent> sampleEditorComponent_;

public:
    void toggleSampleEditor();
    bool isSampleEditorVisible() const { return sampleEditorVisible_; }
    SampleEditorComponent* getSampleEditor() { return sampleEditorComponent_.get(); }

private:
    bool sampleEditorVisible_ = false;    
    static constexpr int sampleEditorHeight_ = 250;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLayoutComponent)
};

} // namespace zenith
