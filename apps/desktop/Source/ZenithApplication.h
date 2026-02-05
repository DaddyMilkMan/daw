#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_events/juce_events.h"

class MainWindow : public juce::DocumentWindow {
public:
    MainWindow(const juce::String& name);
    ~MainWindow() override;

    void closeButtonPressed() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::unique_ptr<juce::Component> contentComponent;
};

class ZenithApplication : public juce::JUCEApplication {
public:
    ZenithApplication();
    ~ZenithApplication() override;

    void initialise(const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

private:
    std::unique_ptr<MainWindow> mainWindow;
};