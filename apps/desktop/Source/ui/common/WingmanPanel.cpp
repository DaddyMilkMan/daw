/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Updated for Grok Integration)
    Author:  Marcus Williams (UX Team)

    Complete Wingman Panel Implementation

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "../engine/ZenithLogger.h"
#include "../network/SecureKeyStore.h"
#include "SettingsComponent.h"
#include "ZenithLookAndFeel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
// #include "ZenithTheme.h" // Deprecated

namespace zenith {

//==============================================================================
//==============================================================================
WingmanPanel::WingmanPanel(CommandAPI &api, Engine &engine)
    : commandAPI(api), engine_(engine) {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Constructor started");
  // Create Grok controller
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Creating GrokDAWController...");
  grokController = std::make_unique<GrokDAWController>(commandAPI);

  //==========================================================================
  // Conversation Display (Markdown Enabled)
  conversationDisplay = std::make_unique<widgets::MarkdownComponent>();
  // conversationDisplay->setColour(...) - MarkdownComponent handles its own colors via Skia
  addAndMakeVisible(conversationDisplay.get());

  // Welcome message
  appendToConversation(
      "Wingman",
      "**Hello!** I'm Wingman, your AI assistant.\nI can control the DAW, generate presets, and help you create music.\n\n*What would you like to do?*");

  //==========================================================================
  // Input Field
  inputField = std::make_unique<juce::TextEditor>("Input");
  inputField->setMultiLine(false);
  inputField->setReturnKeyStartsNewLine(false);
  inputField->setPopupMenuEnabled(true);
  inputField->setColour(juce::TextEditor::backgroundColourId,
                        design::toJuceColour(design::colors::BG_02));
  inputField->setColour(juce::TextEditor::textColourId,
                        design::toJuceColour(design::colors::TEXT_PRIMARY));
  inputField->setColour(juce::TextEditor::outlineColourId,
                        design::toJuceColour(design::colors::ACCENT_PRIMARY));
  inputField->setFont(design::typography::getJuceFont(14.0f));
  inputField->setTextToShowWhenEmpty("Ask Wingman anything...",
                                     design::toJuceColour(design::colors::TEXT_SECONDARY));
  inputField->addListener(this);
  addAndMakeVisible(inputField.get());

  //==========================================================================
  // Send Button
  sendButton = std::make_unique<juce::TextButton>("Send");
  sendButton->setButtonText("Send");
  sendButton->setColour(juce::TextButton::buttonColourId,
                        design::toJuceColour(design::colors::ACCENT_PRIMARY));
  sendButton->setColour(juce::TextButton::textColourOffId,
                        design::toJuceColour(design::colors::TEXT_PRIMARY));
  sendButton->addListener(this);
  addAndMakeVisible(sendButton.get());

  //==========================================================================
  // Vibe DAW Buttons (Accept/Deny)
  acceptButton = std::make_unique<juce::TextButton>("Accept");
  acceptButton->setButtonText("Accept Changes");
  acceptButton->setColour(juce::TextButton::buttonColourId,
                          design::toJuceColour(design::colors::SUCCESS));
  acceptButton->setVisible(false);
  acceptButton->addListener(this);
  addAndMakeVisible(acceptButton.get());

  denyButton = std::make_unique<juce::TextButton>("Deny");
  denyButton->setButtonText("Rollback");
  denyButton->setColour(juce::TextButton::buttonColourId,
                        design::toJuceColour(design::colors::DANGER));
  denyButton->setVisible(false);
  denyButton->addListener(this);
  addAndMakeVisible(denyButton.get());

  grokController->setOnChangesPending([this](bool pending) {
      juce::MessageManager::callAsync([this, pending]() {
          acceptButton->setVisible(pending);
          denyButton->setVisible(pending);
          resized();
      });
  });

  //==========================================================================
  // Mode Selector
  modeLabel = std::make_unique<juce::Label>("ModeLabel", "Mode:");
  modeLabel->setColour(juce::Label::textColourId,
                       design::toJuceColour(design::colors::TEXT_PRIMARY));
  modeLabel->setFont(design::typography::getJuceFont(14.0f));
  addAndMakeVisible(modeLabel.get());

  modeSelector = std::make_unique<juce::ComboBox>("Mode");
  modeSelector->addItem(juce::CharPointer_UTF8("\xe2\x9a\xa1 Fast (Quick responses)"), 1);
  modeSelector->addItem(juce::CharPointer_UTF8("\xf0\x9f\xa7\xa0 Thinking (Deep analysis)"), 2);
  modeSelector->setSelectedId(1); // Default to Fast
  modeSelector->setColour(juce::ComboBox::backgroundColourId,
                          design::toJuceColour(design::colors::BG_02));
  modeSelector->setColour(juce::ComboBox::textColourId,
                          design::toJuceColour(design::colors::TEXT_PRIMARY));
  modeSelector->setColour(juce::ComboBox::outlineColourId,
                          design::toJuceColour(design::colors::ACCENT_PRIMARY));
  modeSelector->onChange = [this]() { updateModeFromSelector(); };
  addAndMakeVisible(modeSelector.get());

  //==========================================================================
  // Status Label
  statusLabel = std::make_unique<juce::Label>("Status", "Ready");
  statusLabel->setColour(juce::Label::textColourId,
                         design::toJuceColour(design::colors::SUCCESS)); // Keep green for status
  statusLabel->setFont(design::typography::getJuceFont(12.0f));
  statusLabel->setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(statusLabel.get());

  //==========================================================================
  // Clear Button
  clearButton = std::make_unique<juce::TextButton>("Clear");
  clearButton->setButtonText("Clear");
  clearButton->setColour(juce::TextButton::buttonColourId,
                         design::toJuceColour(design::colors::BG_02));
  clearButton->setColour(juce::TextButton::textColourOffId,
                         design::toJuceColour(design::colors::TEXT_SECONDARY));
  clearButton->addListener(this);
  addAndMakeVisible(clearButton.get());

  //==========================================================================
  // Settings Button
  settingsButton = std::make_unique<juce::TextButton>("Settings");
  settingsButton->setButtonText(juce::CharPointer_UTF8("\xe2\x9a\x99"));
  settingsButton->setColour(juce::TextButton::buttonColourId,
                            design::toJuceColour(design::colors::BG_02));
  settingsButton->setColour(juce::TextButton::textColourOffId,
                            design::toJuceColour(design::colors::TEXT_SECONDARY));
  settingsButton->addListener(this);
  addAndMakeVisible(settingsButton.get());

  // Update status based on Grok readiness (Moved to END of constructor)
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Initializing Grok...");
  initializeGrok();
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Grok initialized (or failed gracefully)");
}

WingmanPanel::~WingmanPanel() { inputField->removeListener(this); }

//==============================================================================
void WingmanPanel::paint(juce::Graphics &g) {
  // Background
  g.fillAll(design::toJuceColour(design::colors::BG_01));
 
  // Header
  g.setColour(design::toJuceColour(design::colors::BG_02));
  g.fillRect(0, 0, getWidth(), 40);
 
  // Title
  g.setColour(design::toJuceColour(design::colors::ACCENT_PRIMARY));
  g.setFont(design::typography::getJuceFont(18.0f, design::FontWeight::Bold));
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

  // Input area (dynamic height)
  int inputHeight = 80;
  if (acceptButton->isVisible()) inputHeight += 40;
  
  auto inputArea = bounds.removeFromBottom(inputHeight);
  inputArea.reduce(10, 10);

  // Vibe Row (Accept/Deny)
  if (acceptButton->isVisible()) {
      auto vibeRow = inputArea.removeFromTop(30);
      acceptButton->setBounds(vibeRow.removeFromLeft(vibeRow.getWidth() / 2).reduced(2, 0));
      denyButton->setBounds(vibeRow.reduced(2, 0));
      inputArea.removeFromTop(10);
  }

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
    setStatus("Grok Ready", design::toJuceColour(design::colors::SUCCESS));
  } else {
    setStatus("Grok initialization failed", design::toJuceColour(design::colors::DANGER));
  }

  return success;
}

bool WingmanPanel::isGrokReady() const { return grokController->isReady(); }

//==============================================================================
void WingmanPanel::buttonClicked(juce::Button *button) {
  if (button == sendButton.get()) {
    sendCommand();
  } else if (button == acceptButton.get()) {
    grokController->acceptLastChanges();
    appendToConversation("Wingman", "*Changes applied and committed.*");
  } else if (button == denyButton.get()) {
    grokController->denyLastChanges();
    appendToConversation("Wingman", "*Changes rolled back.*");
  } else if (button == clearButton.get()) {
    conversationDisplay->clear();
    grokController->clearHistory();
  } else if (button == settingsButton.get()) {
    showSettings();
  }
}

//==============================================================================
void WingmanPanel::textEditorReturnKeyPressed(juce::TextEditor &editor) {
  if (&editor == inputField.get()) {
    sendCommand();
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
                         "Grok API key not configured. Click Settings to set it up.");
    return;
  }

  // Clear input
  inputField->clear();

  // Add to conversation
  appendToConversation("You", command);

  // Set processing state
  isProcessing = true;
  setStatus("Processing...", design::toJuceColour(design::colors::WARNING));
  sendButton->setEnabled(false);

  // Send to Grok
  grokController->executeCommand(
      command, currentMode,
      [this](juce::String response) {
        // Success
        juce::Component::SafePointer<WingmanPanel> safeThis(this);
        juce::MessageManager::callAsync([safeThis, response]() {
          if (auto* self = safeThis.getComponent()) {
              self->appendToConversation("Wingman", response);
              self->setStatus("Ready", design::toJuceColour(design::colors::SUCCESS));
              self->isProcessing = false;
              self->sendButton->setEnabled(true);
          }
        });
      },
      [this](juce::String error) {
        // Error
        juce::Component::SafePointer<WingmanPanel> safeThis(this);
        juce::MessageManager::callAsync([safeThis, error]() {
          if (auto* self = safeThis.getComponent()) {
              juce::String friendlyError = error;
              juce::String suggestedAction = "";
              
              if (error.contains("401")) {
                friendlyError = "Authentication failed.";
                suggestedAction = "Please check your API key in Settings";
              } else if (error.contains("429")) {
                friendlyError = "Rate limit exceeded.";
                suggestedAction = "Please wait a moment before trying again.";
              } else if (error.contains("timed out") || error.contains("connection")) {
                friendlyError = "Connection issue.";
                suggestedAction = "Please check your internet connection.";
              } else if (error.contains("quota")) {
                 friendlyError = "API Quota Exceeded.";
                 suggestedAction = "Please check your usage limits at console.x.ai";
              }

              self->appendToConversation("Error", friendlyError);
              if (suggestedAction.isNotEmpty()) {
                 self->appendToConversation("Wingman", "**" + suggestedAction + "**");
              }
              
              self->setStatus("Error", design::toJuceColour(design::colors::DANGER));
              self->isProcessing = false;
              self->sendButton->setEnabled(true);
          }
        });
      },
      [this](juce::String status) {
        // Progress
        juce::Component::SafePointer<WingmanPanel> safeThis(this);
        juce::MessageManager::callAsync(
            [safeThis, status]() { 
                if (auto* self = safeThis.getComponent()) {
                    self->setStatus(status, design::toJuceColour(design::colors::WARNING)); 
                }
            });
      });
}

void WingmanPanel::appendToConversation(const juce::String &speaker,
                                        const juce::String &message) {
  conversationDisplay->appendMessage(speaker, message);
}

void WingmanPanel::setStatus(const juce::String &status, juce::Colour colour) {
  statusLabel->setText(status, juce::dontSendNotification);
  statusLabel->setColour(juce::Label::textColourId, colour);
}

void WingmanPanel::updateModeFromSelector() {
  int selectedId = modeSelector->getSelectedId();
  currentMode = (selectedId == 2) ? GrokMode::Thinking : GrokMode::Fast;

  juce::String modeName = (currentMode == GrokMode::Fast) ? "Fast" : "Thinking";
  setStatus("Mode: " + modeName, design::toJuceColour(design::colors::ACCENT_PRIMARY));
}

void WingmanPanel::showSettings() {
  juce::DialogWindow::LaunchOptions options;
  options.content.setOwned(new SettingsComponent(engine_));
  options.content->setSize(600, 500);
  options.dialogTitle = "Zenith DAW Settings";
  options.dialogBackgroundColour = design::toJuceColour(design::colors::BG_01);
  options.escapeKeyTriggersCloseButton = true;
  options.useNativeTitleBar = true;
  options.resizable = true;

  options.launchAsync();
}

//==============================================================================
//==============================================================================
// SampleHunterAgent::Listener interface
//==============================================================================

void WingmanPanel::sampleDownloaded(const zenith::ai::FoundSample &sample) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);
  juce::MessageManager::callAsync([safeThis, sample]() {
    if (auto* self = safeThis.getComponent()) {
        self->appendToConversation("Wingman", "Downloaded sample: " +
                                            sample.localFile.getFileName());
    }
  });
}

void WingmanPanel::sampleAnalyzed(const zenith::ai::FoundSample &sample) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);
  juce::MessageManager::callAsync([safeThis, sample]() {
    if (auto* self = safeThis.getComponent()) {
        // self->showAnalysis(sample);
    }
  });
}

void WingmanPanel::sampleImported(const juce::File &file) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);
  juce::MessageManager::callAsync([safeThis, file]() {
    if (auto* self = safeThis.getComponent()) {
        self->appendToConversation("Wingman",
                            "Imported sample to project: " + file.getFileName());
    }
  });
}

void WingmanPanel::huntingProgressChanged(float progress,
                                          const juce::String &status) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);
  juce::MessageManager::callAsync([safeThis, progress, status]() {
    if (auto* self = safeThis.getComponent()) {
        self->setStatus(status, design::toJuceColour(design::colors::INFO));
    }
  });
}

void WingmanPanel::huntingComplete(const zenith::ai::HuntingStats &stats,
                                   bool success) {
  juce::Component::SafePointer<WingmanPanel> safeThis(this);
  juce::MessageManager::callAsync([safeThis, stats, success]() {
    if (auto* self = safeThis.getComponent()) {
        if (success) {
        self->appendToConversation("Wingman", "Sample hunting complete! Found " +
                                            juce::String(stats.samplesFound) +
                                            " samples.");
        self->setStatus("Ready", design::toJuceColour(design::colors::SUCCESS));
        } else {
        self->appendToConversation("Wingman",
                            "Sample hunting failed or was cancelled.");
        self->setStatus("Failed", design::toJuceColour(design::colors::DANGER));
        }
    }
  });
}

} // namespace zenith
