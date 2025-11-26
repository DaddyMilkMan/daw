#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class DebugLogOverlay : public juce::Component {
public:
  DebugLogOverlay() {
    addAndMakeVisible(logEditor);
    logEditor.setMultiLine(true);
    logEditor.setReadOnly(true);
    logEditor.setScrollbarsShown(true);
    logEditor.setCaretVisible(false);
    logEditor.setColour(juce::TextEditor::backgroundColourId,
                        juce::Colours::black.withAlpha(0.9f));
    logEditor.setColour(juce::TextEditor::textColourId,
                        juce::Colours::lightgreen);
    logEditor.setFont(juce::Font("Consolas", 14.0f, juce::Font::plain));

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("Close");
    closeButton.onClick = [this] { setVisible(false); };

    setSize(800, 600);
  }

  void resized() override {
    auto area = getLocalBounds();
    auto header = area.removeFromTop(30);
    closeButton.setBounds(header.removeFromRight(80).reduced(2));
    logEditor.setBounds(area);
  }

  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colours::black.withAlpha(0.8f));
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);
    g.drawText("Application Logs", 10, 0, 200, 30,
               juce::Justification::centredLeft, true);
  }

  void log(const juce::String &msg) {
    // Ensure we are on the message thread for UI updates
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
      appendLog(msg);
    } else {
      juce::MessageManager::callAsync([this, msg] { appendLog(msg); });
    }
  }

  static DebugLogOverlay &getInstance() {
    static DebugLogOverlay instance;
    return instance;
  }

private:
  void appendLog(const juce::String &msg) {
    logEditor.moveCaretToEnd();
    logEditor.insertTextAtCaret(msg + "\n");
  }

  juce::TextEditor logEditor;
  juce::TextButton closeButton;
};

} // namespace zenith
