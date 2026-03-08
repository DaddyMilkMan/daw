/*
    Zenith Synth - Standalone Application
    Copyright (C)2025 Micah Cooley <micahcooley@protonmail.com>
    AGPL-3.0
*/

#include <modules/zenith_core/instruments/ZenithSynthProcessor.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

namespace zenith {

class ZenithStandalone : public juce::JUCEApplication {
public:
    ZenithStandalone() = default;

    const juce::String getApplicationName() override {
        return "Zenith Synth";
    }

    const juce::String getApplicationVersion() override {
        return "1.0.0";
    }

    void initialise(const juce::String& commandLine) override {
        // Main window setup
        mainWindow_.reset(new MainWindow("Zenith Synth",
            new ZenithSynthProcessor()));
    }

    void shutdown() override {
        mainWindow_ = nullptr;
    }

private:
    class MainWindow : public juce::DocumentWindow {
    public:
        MainWindow(juce::String name, juce::AudioProcessor* processor)
            : DocumentWindow(name,
                juce::Colours::lightgrey,
                juce::DocumentWindow::allButtons),
              processor_(processor) {

            // Create editor
            editor_.reset(processor->createEditor());

            // Setup window
            setContentOwned(editor_.get(), true);
            setResizable(true, true);
            centreWithSize(800, 600);
            setVisible(true);
        }

        void closeButtonPressed() override {
            juce::JUCEApplication::quit();
        }

    private:
        juce::AudioProcessor* processor_ = nullptr;
        std::unique_ptr<juce::AudioProcessorEditor> editor_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace zenith

START_JUCE_APPLICATION(Zenith::ZenithStandalone)
