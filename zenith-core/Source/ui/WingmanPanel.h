/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 7: Wingman AI Integration

    In-DAW command console for AI-driven workflow

    Responsibilities:
    - Display command history (input + output)
    - Text input field for JSON commands or shorthand or natural language
    - Parse shorthand commands to JSON
    - Execute commands via CommandAPI (Command mode)
    - Send natural language to AI bridge server (AI mode)
    - Handle AI responses with yes/no confirmation workflow
    - Show results in message history

    Modes:
    - Command mode: Manual JSON or shorthand execution
    - AI mode: Natural language → AI → command batch → confirmation

    Shorthand examples:
    - "tracks" → list_tracks
    - "create audio Guitar" → create_track type:audio name:Guitar
    - "volume track_0 -6" → set_track_volume trackId:track_0 volumeDb:-6
    - "graph" → get_session_graph

    AI mode examples:
    - "make a 4 bar drum loop"
    - "sidechain the bass to the kick"
    - "create vocals track and add reverb"

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#ifdef ZENITH_USE_SKIA
#include "skia/SkiaButtonNative.h"
#include "skia/SkiaLabel.h"
#include "skia/SkiaTextInput.h"

#endif

namespace zenith {
class CommandAPI;
class AIBridgeClient;
} // namespace zenith

//==============================================================================
/**
    In-DAW Wingman command console component with AI integration
*/
#ifdef ZENITH_USE_SKIA
#include "skia/SkiaCanvasComponent.h"
#endif

//==============================================================================
/**
    In-DAW Wingman command console component with AI integration
*/
class WingmanPanel : public
#ifdef ZENITH_USE_SKIA
                     zenith::SkiaCanvasComponent,
#else
                     juce::Component,
#endif
                     public juce::Timer
#ifndef ZENITH_USE_SKIA
    ,
                     private juce::TextEditor::Listener
#endif
    ,
                     private juce::ChangeListener {
public:
  WingmanPanel(zenith::CommandAPI &api, zenith::AIBridgeClient &aiClient);
  ~WingmanPanel() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

#ifdef ZENITH_USE_SKIA
  void paintSkia(SkCanvas &canvas, const juce::Rectangle<int> &bounds) override;
#endif

private:
  // Timer callback
  void timerCallback() override;
  //==============================================================================
  // TextEditor::Listener (JUCE only)
#ifndef ZENITH_USE_SKIA
  void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
#endif

  //==============================================================================
  // ChangeListener (for AI responses)
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

  //==============================================================================
  // Mode management
  enum class Mode { Command, AI };

  void setMode(Mode newMode);
  Mode getCurrentMode() const { return currentMode; }

  //==============================================================================
  // AI mode handlers
  void executeAIRequest(const juce::String &naturalLanguage);
  void handleAIResponse();
  void showAIPlan(const juce::String &thought,
                  const juce::Array<juce::var> &commands);
  void executePendingBatch();
  void cancelPendingBatch();

  //==============================================================================
  // Shorthand parser
  juce::String parseShorthand(const juce::String &input);

  //==============================================================================
  // Command execution
  void executeCommand(const juce::String &input);
  void clearHistory();

  //==============================================================================
  // UI helpers
  void addMessage(const juce::String &message, bool isUserInput);
  void scrollHistoryToBottom();

  //==============================================================================
  // Member variables

  zenith::CommandAPI &commandAPI;
  zenith::AIBridgeClient &aiBridgeClient;

  // UI components
#ifdef ZENITH_USE_SKIA
  std::unique_ptr<zenith::SkiaTextInput> commandInput;
  std::unique_ptr<zenith::SkiaTextInput> historyDisplay;

  std::unique_ptr<zenith::SkiaButtonNative> clearButton;
  std::unique_ptr<zenith::SkiaButtonNative> commandModeButton;
  std::unique_ptr<zenith::SkiaButtonNative> aiModeButton;
#else
  std::unique_ptr<juce::TextEditor> commandInput;
  std::unique_ptr<juce::TextEditor> historyDisplay;

  std::unique_ptr<juce::TextButton> clearButton;
  std::unique_ptr<juce::TextButton> commandModeButton;
  std::unique_ptr<juce::TextButton> aiModeButton;
#endif

  // Message history
  juce::StringArray messageHistory;

  // Mode state
  Mode currentMode{Mode::Command};

  // Pending batch state
  bool hasPendingBatch{false};
  juce::Array<juce::var> pendingCommands;
  juce::String pendingRequestId;

  // Animation state
  float inputFocusAnim{0.0f};
  bool inputHasFocus{false};
  bool isWaitingForAI{false};
  float typingIndicatorPhase{0.0f};

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};
