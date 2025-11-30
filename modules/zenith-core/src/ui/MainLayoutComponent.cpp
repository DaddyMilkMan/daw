/*
  ==============================================================================

    MainLayoutComponent.cpp
    Created: 2025-11-28
    Author:  Dr. Aris Vokos + Leo Rossi + Isabella Moretti

  ==============================================================================
*/

#include "MainLayoutComponent.h"
#include "BrowserPanel.h"
#include "skia/SkiaMainWindowIntegration.h"

namespace zenith {

MainLayoutComponent::MainLayoutComponent(ProjectState& state, Engine& engine)
    : projectState_(state), engine_(engine)
{
    // Create Arranger
    arrangerComponent_ = std::make_unique<ArrangementComponent>(projectState_, engine_);
    addAndMakeVisible(arrangerComponent_.get());

    // Create Browser (Skia-based)
    // Note: In a real implementation, we might want to wrap this or have a dedicated container
    // For now, we'll assume the BrowserPanel is managed by the layout
}

void MainLayoutComponent::paint(juce::Graphics& g)
{
    // Background handled by SkiaMainWindowIntegration or children
    if (!isSessionView()) {
        g.fillAll(juce::Colour(0xff1e1e1e)); // Dark background for arranger area
    }
}

void MainLayoutComponent::resized()
{
    auto area = getLocalBounds();

    // 1. Browser Panel (Left)
    if (browserVisible_) {
        // In this architecture, the browser might be a separate component passed in, 
        // or we reserve space for it. 
        // Based on MainWindow.cpp, the BrowserPanel is actually created inside MainWindow 
        // but let's assume for this component we are managing the center area.
        
        // If this component is the "Center" of the MainWindow, it contains Arranger/Session.
        // The Browser is likely a sibling in MainWindow.cpp's layout logic, 
        // OR this component manages the whole "Content" area below the Transport.
        
        // Let's assume this component fills the center and manages the Arranger.
    }

    // 2. Arranger / Session View
    if (arrangerComponent_) {
        arrangerComponent_->setBounds(area);
    }
}

void MainLayoutComponent::toggleView()
{
    showSessionView_ = !showSessionView_;
    
    if (arrangerComponent_) {
        arrangerComponent_->setVisible(!showSessionView_);
    }
    
    // Toggle Session View visibility when implemented
    resized();
}

void MainLayoutComponent::toggleBrowser()
{
    browserVisible_ = !browserVisible_;
    resized();
}

} // namespace zenith
