/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    In-DAW command console for AI-driven workflow

    Responsibilities:
    - Display command history (input + output)
    - Text input field for JSON commands or shorthand
    - Parse shorthand commands to JSON
    - Execute commands via CommandAPI
    - Show results in message history

    Shorthand examples:
    - "tracks" → list_tracks
    - "create audio Guitar" → create_track type:audio name:Guitar
    - "volume track_0 -6" → set_track_volume trackId:track_0 volumeDb:-6
    - "graph" → get_session_graph

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>

namespace zenith {
    class CommandAPI;
}

//==============================================================================
/**
    In-DAW Wingman command console component
*/
class WingmanPanel : public juce::Component,
                     private juce::TextEditor::Listener
{
public:
    //==============================================================================
    WingmanPanel(zenith::CommandAPI& api);
    ~WingmanPanel() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // Command execution
    void executeCommand(const juce::String& input);
    void clearHistory();

private:
    //==============================================================================
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;

    //==============================================================================
    // Shorthand parser
    juce::String parseShorthand(const juce::String& input);

    //==============================================================================
    // UI helpers
    void addMessage(const juce::String& message, bool isUserInput);
    void scrollHistoryToBottom();

    //==============================================================================
    // Member variables

    zenith::CommandAPI& commandAPI;

    // UI components
    std::unique_ptr<juce::TextEditor> commandInput;
    std::unique_ptr<juce::TextEditor> historyDisplay;
    std::unique_ptr<juce::TextButton> clearButton;

    // Message history
    juce::StringArray messageHistory;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};
