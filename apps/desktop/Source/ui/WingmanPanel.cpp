/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Updated for Grok Integration)
    Author:  Marcus Williams (UX Team)

    Complete Wingman Panel Implementation

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "../SimpleLogger.h"
#include "../network/SecureKeyStore.h"
#include "SettingsComponent.h"
#include "ZenithLookAndFeel.h"

namespace zenith {

//==============================================================================
WingmanPanel::WingmanPanel(CommandAPI &api, AIBridgeClient &client,
                           Engine &engine)
    : commandAPI(api), aiBridgeClient(client), engine_(engine) {
  logToFile("WingmanPanel: Constructor started");
  // Create Grok controller
  logToFile("WingmanPanel: Creating GrokDAWController...");
  grokController = std::make_unique<GrokDAWController>(commandAPI);

  //==========================================================================
  // Conversation Display
  conversationDisplay = std::make_unique<juce::TextEditor>("Conversation");
  conversationDisplay->setMultiLine(true);
  conversationDisplay->setReadOnly(true);
  conversationDisplay->setScrollbarsShown(true);
  conversationDisplay->setCaretVisible(false);
  conversationDisplay->setPopupMenuEnabled(true);
  conversationDisplay->setColour(juce::TextEditor::backgroundColourId,
                                 ZenithLookAndFeel::Colors::panel);
  conversationDisplay->setColour(juce::TextEditor::textColourId,
                                 ZenithLookAndFeel::Colors::textPrimary);
  conversationDisplay->setColour(juce::TextEditor::outlineColourId,
                                 ZenithLookAndFeel::Colors::border);
  conversationDisplay->setFont(ZenithLookAndFeel::getFontMedium());
  addAndMakeVisible(conversationDisplay.get());

  // Welcome message
  appendToConversation(
      "Wingman",
      "Hello! I'm Wingman, your AI assistant. I can control the DAW, generate "
      "presets, and help you create music. What would you like to do?");

  //==========================================================================
  // Input Field
  inputField = std::make_unique<juce::TextEditor>("Input");
  inputField->setMultiLine(false);
  inputField->setReturnKeyStartsNewLine(false);
  inputField->setPopupMenuEnabled(true);
  inputField->setColour(juce::TextEditor::backgroundColourId,
                        ZenithLookAndFeel::Colors::backgroundPanel);
  inputField->setColour(juce::TextEditor::textColourId,
                        ZenithLookAndFeel::Colors::textPrimary);
  inputField->setColour(juce::TextEditor::outlineColourId,
                        ZenithLookAndFeel::Colors::accent);
  inputField->setFont(ZenithLookAndFeel::getFontMedium());
  inputField->setTextToShowWhenEmpty("Ask Wingman anything...",
                                     ZenithLookAndFeel::Colors::textSecondary);
  inputField->addListener(this);
  addAndMakeVisible(inputField.get());

  //==========================================================================
  // Send Button
  sendButton = std::make_unique<juce::TextButton>("Send");
  sendButton->setButtonText("Send");
  sendButton->setColour(juce::TextButton::buttonColourId,
                        ZenithLookAndFeel::Colors::accent);
  sendButton->setColour(juce::TextButton::textColourOffId,
                        ZenithLookAndFeel::Colors::textPrimary);
  sendButton->addListener(this);
  addAndMakeVisible(sendButton.get());

  //==========================================================================
  // Mode Selector
  modeLabel = std::make_unique<juce::Label>("ModeLabel", "Mode:");
  modeLabel->setColour(juce::Label::textColourId,
                       ZenithLookAndFeel::Colors::textPrimary);
  modeLabel->setFont(ZenithLookAndFeel::getFontMedium());
  addAndMakeVisible(modeLabel.get());

  modeSelector = std::make_unique<juce::ComboBox>("Mode");
  modeSelector->addItem("⚡ Fast (Quick responses)", 1);
  modeSelector->addItem("🧠 Thinking (Deep analysis)", 2);
  modeSelector->setSelectedId(1); // Default to Fast
  modeSelector->setColour(juce::ComboBox::backgroundColourId,
                          ZenithLookAndFeel::Colors::backgroundPanel);
  modeSelector->setColour(juce::ComboBox::textColourId,
                          ZenithLookAndFeel::Colors::textPrimary);
  modeSelector->setColour(juce::ComboBox::outlineColourId,
                          ZenithLookAndFeel::Colors::accent);
  modeSelector->onChange = [this]() { updateModeFromSelector(); };
  addAndMakeVisible(modeSelector.get());

  //==========================================================================
  // Status Label
  statusLabel = std::make_unique<juce::Label>("Status", "Ready");
  statusLabel->setColour(juce::Label::textColourId,
                         juce::Colours::green); // Keep green for status
  statusLabel->setFont(ZenithLookAndFeel::getFontSmall());
  statusLabel->setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(statusLabel.get());

  //==========================================================================
  // Clear Button
  clearButton = std::make_unique<juce::TextButton>("Clear");
  clearButton->setButtonText("Clear");
  clearButton->setColour(juce::TextButton::buttonColourId,
                         ZenithLookAndFeel::Colors::panel);
  clearButton->setColour(juce::TextButton::textColourOffId,
                         ZenithLookAndFeel::Colors::textSecondary);
  clearButton->addListener(this);
  addAndMakeVisible(clearButton.get());

  //==========================================================================
  // Settings Button
  settingsButton = std::make_unique<juce::TextButton>("Settings");
  settingsButton->setButtonText("⚙");
  settingsButton->setColour(juce::TextButton::buttonColourId,
                            ZenithLookAndFeel::Colors::panel);
  settingsButton->setColour(juce::TextButton::textColourOffId,
                            ZenithLookAndFeel::Colors::textSecondary);
  settingsButton->addListener(this);
  addAndMakeVisible(settingsButton.get());

  // Update status based on Grok readiness (Moved to END of constructor)
  logToFile("WingmanPanel: Initializing Grok...");
  initializeGrok();
  logToFile("WingmanPanel: Grok initialized (or failed gracefully)");
}

WingmanPanel::~WingmanPanel() { inputField->removeListener(this); }

//==============================================================================
void WingmanPanel::paint(juce::Graphics &g) {
  // Background
  g.fillAll(ZenithLookAndFeel::Colors::background);

  // Header
  g.setColour(ZenithLookAndFeel::Colors::panel);
  g.fillRect(0, 0, getWidth(), 40);

  // Title
  g.setColour(ZenithLookAndFeel::Colors::accent);
  g.setFont(ZenithLookAndFeel::getFontLarge().boldened());
  g.drawText("Wingman AI Assistant", 10, 0, getWidth() - 20, 40,
             juce::Justification::centredLeft);
}

void WingmanPanel::resized() {
  auto bounds = getLocalBounds();

  // Header area (40px)
  auto headerArea = bounds.removeFromTop(40);
  settingsButton->setBounds(headerArea.removeFromRight(40).reduced(5));

  // Status bar (30px)
  auto statusArea = bounds.removeFromBottom(30);
  statusLabel->setBounds(statusArea.reduced(5));

  // Input area (80px)
  auto inputArea = bounds.removeFromBottom(80);
  inputArea.reduce(10, 10);

  // Mode selector row
  auto modeRow = inputArea.removeFromTop(30);
  modeLabel->setBounds(modeRow.removeFromLeft(50));
  modeSelector->setBounds(modeRow.removeFromLeft(200).reduced(0, 2));
  modeRow.removeFromLeft(10);
  clearButton->setBounds(modeRow.removeFromLeft(80).reduced(0, 2));

  inputArea.removeFromTop(10);

  // Input field and send button
  auto inputRow = inputArea;
  sendButton->setBounds(inputRow.removeFromRight(80).reduced(0, 2));
  inputRow.removeFromRight(10);
  inputField->setBounds(inputRow.reduced(0, 2));

  // Conversation display (remaining space)
  conversationDisplay->setBounds(bounds.reduced(10));
}

//==============================================================================
bool WingmanPanel::initializeGrok(const juce::String &apiKey) {
  bool success = grokController->initialize(apiKey);

  if (success) {
    setStatus("Grok Ready", juce::Colour(0xff00ff00));
  } else {
    setStatus("Grok initialization failed", juce::Colour(0xffff0000));
  }

  return success;
}

bool WingmanPanel::isGrokReady() const { return grokController->isReady(); }

//==============================================================================
void WingmanPanel::textEditorReturnKeyPressed(juce::TextEditor &editor) {
  if (&editor == inputField.get()) {
    sendCommand();
  }
}

void WingmanPanel::buttonClicked(juce::Button *button) {
  if (button == sendButton.get()) {
    sendCommand();
  } else if (button == clearButton.get()) {
    conversationDisplay->clear();
    grokController->clearHistory();
    appendToConversation("Wingman",
                         "Conversation cleared. How can I help you?");
  } else if (button == settingsButton.get()) {
    showSettings();
  }
}

//==============================================================================
void WingmanPanel::sendCommand() {
  if (isProcessing)
    return;

  auto command = inputField->getText().trim();
  if (command.isEmpty())
    return;

  if (!isGrokReady()) {
    appendToConversation("System",
                         "Grok API key not configured. Click ⚙ to set it up.");
    return;
  }

  // Clear input
  inputField->clear();

  // Add to conversation
  appendToConversation("You", command);

  // Set processing state
  isProcessing = true;
  setStatus("Processing...", juce::Colour(0xffffff00));
  sendButton->setEnabled(false);

  // Send to Grok
  grokController->executeCommand(
      command, currentMode,
      [this](juce::String response) {
        // Success
        juce::MessageManager::callAsync([this, response]() {
          appendToConversation("Wingman", response);
          setStatus("Ready", juce::Colour(0xff00ff00));
          isProcessing = false;
          sendButton->setEnabled(true);
        });
      },
      [this](juce::String error) {
        // Error
        juce::MessageManager::callAsync([this, error]() {
          appendToConversation("Error", error);
          setStatus("Error", juce::Colour(0xffff0000));
          isProcessing = false;
          sendButton->setEnabled(true);
        });
      },
      [this](juce::String status) {
        // Progress
        juce::MessageManager::callAsync(
            [this, status]() { setStatus(status, juce::Colour(0xffffff00)); });
      });
}

void WingmanPanel::appendToConversation(const juce::String &speaker,
                                        const juce::String &message) {
  juce::String timestamp =
      juce::Time::getCurrentTime().toString(false, true, false, true);
  juce::String entry =
      "[" + timestamp + "] " + speaker + ": " + message + "\n\n";

  conversationDisplay->moveCaretToEnd();
  conversationDisplay->insertTextAtCaret(entry);
  conversationDisplay->moveCaretToEnd();
}

void WingmanPanel::setStatus(const juce::String &status, juce::Colour colour) {
  statusLabel->setText(status, juce::dontSendNotification);
  statusLabel->setColour(juce::Label::textColourId, colour);
}

void WingmanPanel::updateModeFromSelector() {
  int selectedId = modeSelector->getSelectedId();
  currentMode = (selectedId == 2) ? GrokMode::Thinking : GrokMode::Fast;

  juce::String modeName = (currentMode == GrokMode::Fast) ? "Fast" : "Thinking";
  setStatus("Mode: " + modeName, juce::Colour(0xff00aaff));
}

void WingmanPanel::showSettings() {
  juce::DialogWindow::LaunchOptions options;
  options.content.setOwned(new SettingsComponent(engine_));
  options.content->setSize(600, 500);
  options.dialogTitle = "Zenith DAW Settings";
  options.dialogBackgroundColour = ZenithLookAndFeel::Colors::background;
  options.escapeKeyTriggersCloseButton = true;
  options.useNativeTitleBar = true;
  options.resizable = true;

  options.launchAsync();
}

//==============================================================================
// SampleHunterAgent::Listener
//==============================================================================

void WingmanPanel::sampleDownloaded(
    const struct zenith::ai::FoundSample &sample) {
  juce::ignoreUnused(sample);
  appendToConversation("Wingman", "Downloaded: " + sample.getSafeFilename());
}

void WingmanPanel::sampleAnalyzed(
    const struct zenith::ai::FoundSample &sample) {
  juce::ignoreUnused(sample);
}

void WingmanPanel::sampleImported(const juce::File &file) {
  appendToConversation("Wingman", "Imported: " + file.getFileName());
}

void WingmanPanel::huntingProgressChanged(float progress,
                                          const juce::String &status) {
  setStatus(status, juce::Colour(0xff00aaff));
}

void WingmanPanel::huntingComplete(const struct zenith::ai::HuntingStats &stats,
                                   bool success) {
  juce::ignoreUnused(stats);
  if (success) {
    statusLabel->setText("Hunting Complete", juce::dontSendNotification);
  } else {
    statusLabel->setText("Hunting Failed", juce::dontSendNotification);
  }
}

} // namespace zenith
