/*
  ==============================================================================

    MainWindow.h
    Created: 2025-11-10

    Main application window for Zenith DAW.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Engine.h"

//==============================================================================
/**
    The main application window.

    Contains the timeline, mixer, piano roll, and Wingman AI panel.
*/
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name, ZenithEngine& engineToUse)
        : DocumentWindow(name,
                        juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                    .findColour(ResizableWindow::backgroundColourId),
                        DocumentWindow::allButtons),
          engine(engineToUse)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        // TODO: Create and set the main content component
        // setContentOwned(new MainContentComponent(engine), true);

        centreWithSize(1600, 900);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    ZenithEngine& engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
