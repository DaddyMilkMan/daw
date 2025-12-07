/*
  ==============================================================================

    MainLayoutComponent.cpp
    Created: 2025-11-28
    Author:  Dr. Aris Vokos + Leo Rossi + Isabella Moretti

  ==============================================================================
*/

#include "MainLayoutComponent.h"
#include "skia/BrowserPanel.h"
#include "skia/views/SessionViewComponent.h"
#include "skia/SkiaMainWindowIntegration.h"
#include "../../include/Engine.h"
#include "../instruments/InstrumentRegistry.h"
#include "../engine/PluginHost.h"
// Browser model included in header

namespace zenith {

MainLayoutComponent::MainLayoutComponent(Engine& engine, ProjectState& state)
    : engine_(engine), projectState_(state)
{
    // Create Arranger
    arrangerComponent_ = std::make_unique<ArrangerComponent>(engine_, projectState_);
    addAndMakeVisible(arrangerComponent_.get()); 

    // Create Session View
    sessionViewComponent_ = std::make_unique<SessionViewComponent>(projectState_);
    addAndMakeVisible(sessionViewComponent_.get());
    sessionViewComponent_->setVisible(false); // Default to Arranger

    // Create Browser Model (requires InstrumentRegistry and PluginHost)
    browserModel_ = std::make_unique<BrowserModel>(
        engine_.getInstrumentRegistry(),
        engine_.getPluginHost()
    );

    // Create Browser (Skia-based)
    browserPanel_ = std::make_unique<BrowserPanel>(*browserModel_);
    
    // Setup callback for browser item double-click
    browserPanel_->onItemDoubleClicked = [this](std::shared_ptr<BrowserItem> item) {
        if (item && !item->isDirectory)
        {
            DBG("MainLayout: Browser item activated: " + item->name);
        }
    };
    
    addAndMakeVisible(browserPanel_.get());
    
    // Create Sample Editor
    sampleEditorComponent_ = std::make_unique<SampleEditorComponent>(engine_, projectState_);
    addAndMakeVisible(sampleEditorComponent_.get());
    sampleEditorComponent_->setVisible(false);

    // Connect Arranger callbacks
    arrangerComponent_->onClipDoubleClicked = [this](const juce::String& trackId, const juce::String& clipId) {
        // Check clip type
        auto track = projectState_.getTrack(trackId);
        if (track.isValid()) {
            auto clips = track.getChildWithName("CLIPS");
            auto clip = clips.getChildWithProperty("id", clipId);
            if (clip.isValid()) {
                juce::String type = clip.getProperty("type").toString();
                if (type == "audio") {
                    // Open Sample Editor
                    if (sampleEditorComponent_) {
                        sampleEditorComponent_->setClipToEdit(trackId, clipId);
                        if (!sampleEditorVisible_) {
                            toggleSampleEditor();
                        }
                    }
                } 
                // Note: MIDI clips would open Piano Roll here
            }
        }
    };

    // Ensure visibility
    arrangerComponent_->setVisible(true);
    browserPanel_->setVisible(browserVisible_);
}

void MainLayoutComponent::paint(juce::Graphics& g)
{
    // Background handled by SkiaMainWindowIntegration or children
    // No JUCE painting needed
}

void MainLayoutComponent::drawSkia(SkCanvas* canvas)
{
    // Background
    canvas->clear(SkColorSetRGB(30, 30, 30));
    
    // Recursively draw children (Arranger, Browser, Session, etc.)
    drawChildren(canvas);
}

void MainLayoutComponent::resized()
{
    auto area = getLocalBounds();

    // 1. Browser Panel (Left)
    if (browserVisible_ && browserPanel_) {
        auto browserArea = area.removeFromLeft(browserWidth_);
        browserPanel_->setBounds(browserArea);
        browserPanel_->setVisible(true);
    } else if (browserPanel_) {
        browserPanel_->setVisible(false);
    }

    // 2. Sample Editor (Bottom)
    if (sampleEditorVisible_ && sampleEditorComponent_) {
        auto editorArea = area.removeFromBottom(sampleEditorHeight_);
        sampleEditorComponent_->setBounds(editorArea);
        sampleEditorComponent_->setVisible(true);
    } else if (sampleEditorComponent_) {
        sampleEditorComponent_->setVisible(false);
    }

    // 2. Center Area (Session or Arranger)
    if (showSessionView_) {
        if (sessionViewComponent_) {
            sessionViewComponent_->setBounds(area);
            sessionViewComponent_->setVisible(true);
        }
        if (arrangerComponent_) arrangerComponent_->setVisible(false);
    } else {
        if (arrangerComponent_) {
            arrangerComponent_->setBounds(area);
            arrangerComponent_->setVisible(true);
        }
        if (sessionViewComponent_) sessionViewComponent_->setVisible(false);
    }
}

void MainLayoutComponent::toggleView()
{
    showSessionView_ = !showSessionView_;
    resized();
}

void MainLayoutComponent::toggleBrowser()
{
    browserVisible_ = !browserVisible_;
    resized();
}

void MainLayoutComponent::toggleSampleEditor()
{
    sampleEditorVisible_ = !sampleEditorVisible_;
    resized();
}

} // namespace zenith
