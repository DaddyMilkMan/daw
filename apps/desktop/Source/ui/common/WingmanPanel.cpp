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
#include <core/SkCanvas.h>
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
  // Conversation Display
  conversationDisplay = std::make_unique<SkiaTextEditor>("wingman_conversation");
  conversationDisplay->setMultiLine(true);
  conversationDisplay->setReadOnly(true);
  conversationDisplay->setBackgroundColour(design::colors::BG_02);
  conversationDisplay->setTextColour(design::colors::TEXT_PRIMARY);
  addAndMakeVisible(conversationDisplay.get());

  // Welcome message
  appendToConversation(
      "Wingman",
      "**Hello!** I'm Wingman, your AI assistant.\nI can control the DAW, generate presets, and help you create music.\n\n*What would you like to do?*");

  //==========================================================================
  // Input Field
  inputField = std::make_unique<SkiaTextEditor>("wingman_input");
  inputField->setMultiLine(false);
  inputField->setBackgroundColour(design::colors::BG_02);
  inputField->setTextColour(design::colors::TEXT_PRIMARY);
  inputField->setTextToShowWhenEmpty("Ask Wingman anything...",
                                     design::withAlpha(design::colors::TEXT_SECONDARY, 0.85f));
  inputField->onReturnKey = [this]() { sendCommand(); };
  addAndMakeVisible(inputField.get());

  //==========================================================================
  // Send Button
  sendButton = std::make_unique<ZenithButton>("Send");
  sendButton->setStyle(ZenithButton::Style::Primary);
  sendButton->onClick = [this]() { sendCommand(); };
  addAndMakeVisible(sendButton.get());

  //==========================================================================
  // Vibe DAW Buttons (Accept/Deny)
  acceptButton = std::make_unique<ZenithButton>("Accept Changes");
  acceptButton->setStyle(ZenithButton::Style::Success);
  acceptButton->onClick = [this]() {
    grokController->acceptLastChanges();
    appendToConversation("Wingman", "*Changes applied and committed.*");
  };
  acceptButton->setVisible(false);
  addAndMakeVisible(acceptButton.get());

  denyButton = std::make_unique<ZenithButton>("Rollback");
  denyButton->setStyle(ZenithButton::Style::Danger);
  denyButton->onClick = [this]() {
    grokController->denyLastChanges();
    appendToConversation("Wingman", "*Changes rolled back.*");
  };
  denyButton->setVisible(false);
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
  modeLabel = std::make_unique<SkiaLabel>("mode_label", "Mode:");
  modeLabel->setTextColour(design::colors::TEXT_PRIMARY);
  modeLabel->setFont(13.0f);
  addAndMakeVisible(modeLabel.get());

  modeSelector = std::make_unique<SkiaComboBox>("mode_selector");
  modeSelector->addItem(juce::CharPointer_UTF8("\xe2\x9a\xa1 Fast (Quick responses)"), 1);
  modeSelector->addItem(juce::CharPointer_UTF8("\xf0\x9f\xa7\xa0 Thinking (Deep analysis)"), 2);
  modeSelector->setSelectedId(1); // Default to Fast
  modeSelector->onChange = [this]() { updateModeFromSelector(); };
  addAndMakeVisible(modeSelector.get());

  //==========================================================================
  // Status Label
  statusLabel = std::make_unique<SkiaLabel>("status_label", "Ready");
  statusLabel->setTextColour(design::colors::SUCCESS);
  statusLabel->setFont(12.0f);
  statusLabel->setJustification(SkiaLabel::Justification::Left);
  addAndMakeVisible(statusLabel.get());

  //==========================================================================
  // Clear Button
  clearButton = std::make_unique<ZenithButton>("Clear");
  clearButton->setStyle(ZenithButton::Style::Secondary);
  clearButton->onClick = [this]() {
    conversationDisplay->clear();
    grokController->clearHistory();
  };
  addAndMakeVisible(clearButton.get());

  //==========================================================================
  // Settings Button
  settingsButton = std::make_unique<ZenithButton>(juce::CharPointer_UTF8("\xe2\x9a\x99"));
  settingsButton->setStyle(ZenithButton::Style::Ghost);
  settingsButton->onClick = [this]() { showSettings(); };
  addAndMakeVisible(settingsButton.get());

  // Update status based on Grok readiness (Moved to END of constructor)
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Initializing Grok...");
  initializeGrok();
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Grok initialized (or failed gracefully)");
}

WingmanPanel::~WingmanPanel() = default;

//==============================================================================
void WingmanPanel::drawSkia(SkCanvas *canvas) {
  SkPaint bg;
  bg.setColor(design::colors::BG_01);
  canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), bg);

  SkPaint headerBg;
  headerBg.setColor(design::colors::BG_02);
  canvas->drawRect(SkRect::MakeXYWH(0, 0, (float)getWidth(), 40.0f), headerBg);

  SkFont titleFont = design::getSkFont(17.0f, design::FontWeight::Bold);
  SkPaint titlePaint;
  titlePaint.setAntiAlias(true);
  titlePaint.setColor(design::colors::ACCENT_PRIMARY);
  canvas->drawString("Wingman AI Assistant", 10.0f, 26.0f, titleFont, titlePaint);

  if (settingsOverlay_ && settingsOverlay_->isVisible()) {
    SkPaint scrim;
    scrim.setColor(design::withAlpha(design::colors::BG_00, 0.72f));
    canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), scrim);
  }
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

  if (settingsOverlay_ && settingsOverlay_->isVisible()) {
    const int overlayW = juce::jlimit(420, 880, getWidth() - 36);
    const int overlayH = juce::jlimit(360, 700, getHeight() - 36);
    juce::Rectangle<int> overlayBounds((getWidth() - overlayW) / 2,
                                       (getHeight() - overlayH) / 2, overlayW,
                                       overlayH);
    settingsOverlay_->setBounds(overlayBounds);
    if (closeSettingsButton_) {
      closeSettingsButton_->setBounds(overlayBounds.getRight() - 86,
                                      overlayBounds.getY() - 38, 86, 30);
    }
  }
}

//==============================================================================
bool WingmanPanel::initializeGrok(const juce::String &apiKey) {
  bool success = grokController->initialize(apiKey);

  if (success) {
    setStatus("Grok Ready", design::colors::SUCCESS);
  } else {
    setStatus("Grok initialization failed", design::colors::DANGER);
  }

  return success;
}

bool WingmanPanel::isGrokReady() const { return grokController->isReady(); }

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
  setStatus("Processing...", design::colors::WARNING);
  sendButton->setEnabled(false);

  // Send to Grok
  grokController->executeCommand(
      command, currentMode,
      [this](juce::String response) {
        // Success
        juce::MessageManager::callAsync([this, response]() {
          appendToConversation("Wingman", response);
          setStatus("Ready", design::colors::SUCCESS);
          isProcessing = false;
          sendButton->setEnabled(true);
        });
      },
      [this](juce::String error) {
        // Error
        juce::MessageManager::callAsync([this, error]() {
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

          appendToConversation("Error", friendlyError);
          if (suggestedAction.isNotEmpty()) {
             appendToConversation("Wingman", "**" + suggestedAction + "**");
          }
          
          setStatus("Error", design::colors::DANGER);
          isProcessing = false;
          sendButton->setEnabled(true);
        });
      },
      [this](juce::String status) {
        // Progress
        juce::MessageManager::callAsync(
            [this, status]() { setStatus(status, design::colors::WARNING); });
      });
}

void WingmanPanel::appendToConversation(const juce::String &speaker,
                                        const juce::String &message) {
  const juce::String prefix = "[" + speaker + "] ";
  juce::String existing = conversationDisplay->getText();
  if (existing.isNotEmpty()) {
    existing << "\n\n";
  }
  existing << prefix << message;
  conversationDisplay->setText(existing);
}

void WingmanPanel::setStatus(const juce::String &status, SkColor colour) {
  statusLabel->setText(status, juce::dontSendNotification);
  statusLabel->setTextColour(colour);
}

void WingmanPanel::updateModeFromSelector() {
  int selectedId = modeSelector->getSelectedId();
  currentMode = (selectedId == 2) ? GrokMode::Thinking : GrokMode::Fast;

  juce::String modeName = (currentMode == GrokMode::Fast) ? "Fast" : "Thinking";
  setStatus("Mode: " + modeName, design::colors::ACCENT_PRIMARY);
}

void WingmanPanel::showSettings() {
  if (!settingsOverlay_) {
    settingsOverlay_ = std::make_unique<SettingsComponent>(engine_);
    addAndMakeVisible(settingsOverlay_.get());
  }

  if (!closeSettingsButton_) {
    closeSettingsButton_ = std::make_unique<ZenithButton>("Close");
    closeSettingsButton_->setStyle(ZenithButton::Style::Secondary);
    closeSettingsButton_->onClick = [this]() {
      if (settingsOverlay_) {
        settingsOverlay_->setVisible(false);
      }
      if (closeSettingsButton_) {
        closeSettingsButton_->setVisible(false);
      }
      repaint();
    };
    addAndMakeVisible(closeSettingsButton_.get());
  }

  settingsOverlay_->setVisible(true);
  closeSettingsButton_->setVisible(true);
  settingsOverlay_->toFront(false);
  closeSettingsButton_->toFront(false);
  resized();
}

//==============================================================================
//==============================================================================
// SampleHunterAgent::Listener interface
//==============================================================================

void WingmanPanel::sampleDownloaded(const zenith::ai::FoundSample &sample) {
  juce::MessageManager::callAsync([this, sample]() {
    appendToConversation("Wingman", "Downloaded sample: " +
                                        sample.localFile.getFileName());
  });
}

void WingmanPanel::sampleAnalyzed(const zenith::ai::FoundSample &sample) {
  juce::MessageManager::callAsync([this, sample]() {
    // Optional: Show analysis details
  });
}

void WingmanPanel::sampleImported(const juce::File &file) {
  juce::MessageManager::callAsync([this, file]() {
    appendToConversation("Wingman",
                         "Imported sample to project: " + file.getFileName());
  });
}

void WingmanPanel::huntingProgressChanged(float progress,
                                          const juce::String &status) {
  juce::MessageManager::callAsync([this, progress, status]() {
    setStatus(status, design::colors::INFO);
  });
}

void WingmanPanel::huntingComplete(const zenith::ai::HuntingStats &stats,
                                   bool success) {
  juce::MessageManager::callAsync([this, stats, success]() {
    if (success) {
      appendToConversation("Wingman", "Sample hunting complete! Found " +
                                          juce::String(stats.samplesFound) +
                                          " samples.");
      setStatus("Ready", design::colors::SUCCESS);
    } else {
      appendToConversation("Wingman",
                           "Sample hunting failed or was cancelled.");
      setStatus("Failed", design::colors::DANGER);
    }
  });
}

} // namespace zenith
