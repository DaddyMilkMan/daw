/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Updated for Grok Integration)
    Author:  Marcus Williams (UX Team)

    Complete Wingman Panel Implementation

  ==============================================================================
*/

#ifndef SK_COLOR_SET_ARGB
#define SK_COLOR_SET_ARGB(a, r, g, b) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#endif

// ... imports ...
#include "WingmanPanel.h"
#include "../../commands/CommandAPI.h"
#include "../../engine/Engine.h"
#include "../../network/AIBridgeClient.h" // Replaced controller
#include "../../engine/ZenithLogger.h"
#include "../design-system/ZenithTheme.h"
#include "../framework/GlassmorphicPanel.h"
#include "SettingsComponent.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

//==============================================================================
//==============================================================================
WingmanPanel::WingmanPanel(CommandAPI &api, Engine &engine)
    : commandAPI(api), engine_(engine) {
  using namespace design;
  
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Constructor started");
  aiClient_ = std::make_unique<AIBridgeClient>(commandAPI);
  aiClient_->addListener(this);

  //==========================================================================
  // Conversation Display (Markdown Enabled)
  conversationDisplay = std::make_unique<widgets::MarkdownComponent>();
  addAndMakeVisible(conversationDisplay.get());

  // Welcome message
  appendToConversation(
      "Wingman",
      "**Hello!** I'm Wingman, your AI assistant.\nI can control the DAW, generate presets, and help you create music.\n\n*What would you like to do?*");

  //==========================================================================
  // Input Field
  inputField = std::make_unique<SkiaTextEditor>("Input");
  inputField->setMultiLine(false);
  inputField->setBackgroundColour(colors::BG_DARK);
  inputField->setTextColour(colors::TEXT_PRIMARY);
  inputField->setTextToShowWhenEmpty("Ask Wingman anything...",
                                     colors::TEXT_SECONDARY);
  inputField->onReturnKey = [this]() { sendCommand(); };
  addAndMakeVisible(inputField.get());

  //==========================================================================
  // Send Button
  sendButton = std::make_unique<SkiaButton>("Send");
  sendButton->setStyle(SkiaButton::Style::Primary);
  sendButton->onClick = [this]() { sendCommand(); };
  addAndMakeVisible(sendButton.get());

  //==========================================================================
  // Mode Selector
  modeLabel = std::make_unique<SkiaLabel>("ModeLabel", "Mode:");
  modeLabel->setTextColour(colors::TEXT_PRIMARY);
  modeLabel->setFont(typography::FONT_SM);
  addAndMakeVisible(modeLabel.get());

  modeSelector = std::make_unique<SkiaComboBox>("Mode");
  modeSelector->addItem("⚡ Fast", 1);
  modeSelector->addItem("🧠 Thinking", 2);
  modeSelector->setSelectedId(1);
  modeSelector->onChange = [this]() { updateModeFromSelector(); };
  addAndMakeVisible(modeSelector.get());

  //==========================================================================
  // Status Label
  statusLabel = std::make_unique<SkiaLabel>("Status", "Ready");
  statusLabel->setTextColour(colors::GREEN);
  statusLabel->setFont(typography::FONT_XS);
  statusLabel->setJustification(SkiaLabel::Justification::Left);
  addAndMakeVisible(statusLabel.get());

  //==========================================================================
  // Clear Button
  clearButton = std::make_unique<SkiaButton>("Clear");
  clearButton->setStyle(SkiaButton::Style::Ghost);
  clearButton->onClick = [this]() {
    conversationDisplay->clear();
    // aiClient_ has no clear yet, but UI is cleared
    appendToConversation("Wingman",
                         "Conversation cleared. How can I help you?");
  };
  addAndMakeVisible(clearButton.get());

  //==========================================================================
  // Settings Button
  settingsButton = std::make_unique<SkiaButton>("⚙");
  settingsButton->setStyle(SkiaButton::Style::Ghost);
  settingsButton->onClick = [this]() { showSettings(); };
  addAndMakeVisible(settingsButton.get());
}

WingmanPanel::~WingmanPanel() {
   if (aiClient_)
       aiClient_->removeListener(this);
  // Signal shutdown to prevent async callbacks from accessing destroyed object
  isShuttingDown_->store(true);
}


void WingmanPanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  SkRect rect = SkRect::MakeXYWH(0, 0, (float)bounds.getWidth(), (float)bounds.getHeight());
  
  using namespace design;

  // 1. REAL Glassmorphism Backdrop
  GlassmorphicPanel::draw(canvas, rect, GlassmorphicPanel::Style::Elevated);

  // 2. Header Divider
  GlassmorphicPanel::drawDivider(canvas, 0, 44.0f, rect.width());

  // 3. Title Text (Inter Bold)
  SkPaint textPaint;
  textPaint.setColor(colors::CYAN);
  textPaint.setAntiAlias(true);
  
  SkFont titleFont = getDisplayFont(18.0f);
  canvas->drawString("Wingman assistant", spacing::MD, 28, titleFont, textPaint);
}

void WingmanPanel::resized() {
  auto bounds = getLocalBounds();
  using namespace design;

  // Header area (44px for consistency with other panels)
  auto headerArea = bounds.removeFromTop(44);
  settingsButton->setBounds(headerArea.removeFromRight(40).reduced(8));

  // Status bar (24px)
  auto statusArea = bounds.removeFromBottom(24);
  statusLabel->setBounds(statusArea.removeFromLeft(200).reduced(spacing::MD, 0));

  // Input area (100px)
  auto inputArea = bounds.removeFromBottom(100);
  inputArea.reduce(spacing::MD, spacing::MD);

  // Mode selector row
  auto modeRow = inputArea.removeFromTop(dimensions::BUTTON_HEIGHT_SM);
  modeLabel->setBounds(modeRow.removeFromLeft(40));
  modeSelector->setBounds(modeRow.removeFromLeft(120).reduced(0, 2));
  modeRow.removeFromLeft(spacing::SM);
  clearButton->setBounds(modeRow.removeFromLeft(60).reduced(0, 2));

  inputArea.removeFromTop(spacing::SM);

  // Input field and send button
  auto inputRow = inputArea;
  sendButton->setBounds(inputRow.removeFromRight(80).reduced(0, 2));
  inputRow.removeFromRight(spacing::SM);
  inputField->setBounds(inputRow.reduced(0, 2));

  // Conversation display (remaining space)
  conversationDisplay->setBounds(bounds.reduced(spacing::MD));
}

//==============================================================================

void WingmanPanel::responseReceived(const juce::String& response) {
    auto shutdownFlag = isShuttingDown_;
    juce::MessageManager::callAsync([this, response, shutdownFlag]() {
        if (shutdownFlag->load()) return;
        appendToConversation("Wingman", response);
        isProcessing = false;
        sendButton->setEnabled(true);
    });
}

void WingmanPanel::errorReceived(const juce::String& error) {
    auto shutdownFlag = isShuttingDown_;
    juce::MessageManager::callAsync([this, error, shutdownFlag]() {
        if (shutdownFlag->load()) return;
        appendToConversation("Error", error);
        isProcessing = false;
        sendButton->setEnabled(true);
    });
}

void WingmanPanel::statusChanged(const juce::String& status) {
    auto shutdownFlag = isShuttingDown_;
    juce::MessageManager::callAsync([this, status, shutdownFlag]() {
        if (shutdownFlag->load()) return;
        SkColor color = (status == "Error") ? SkColorSetARGB(255, 255, 0, 0) : SkColorSetARGB(255, 0, 255, 0);
        setStatus(status, color);
    });
}

//==============================================================================

void WingmanPanel::sendCommand() {
  if (isProcessing)
    return;

  auto command = inputField->getText().trim();
  if (command.isEmpty())
    return;

  // Clear input
  inputField->clear();

  // Add to conversation
  appendToConversation("You", command);

  // Set processing state
  isProcessing = true;
  setStatus("Processing...", SkColorSetARGB(255, 255, 255, 0));
  sendButton->setEnabled(false);

  // Send to AI Bridge
  aiClient_->processNaturalLanguage(command, currentMode);
}

void WingmanPanel::appendToConversation(const juce::String &speaker,
                                        const juce::String &message) {
  conversationDisplay->appendMessage(speaker, message);
}

void WingmanPanel::setStatus(const juce::String &status, SkColor colour) {
  statusLabel->setText(status);
  statusLabel->setTextColour(colour);
}

void WingmanPanel::updateModeFromSelector() {
  int selectedId = modeSelector->getSelectedId();
  currentMode = (selectedId == 2) ? GrokMode::Thinking : GrokMode::Fast;

  juce::String modeName = (currentMode == GrokMode::Fast) ? "Fast" : "Thinking";
  setStatus("Mode: " + modeName, SkColorSetARGB(255, 0, 170, 255));
}

void WingmanPanel::showSettings() {
  juce::DialogWindow::LaunchOptions options;
  options.content.setOwned(new SettingsComponent(engine_));
  options.content->setSize(600, 500);
  options.dialogTitle = "Zenith DAW Settings";
  options.dialogBackgroundColour = zenith::ZenithTheme::Colors::bg_01;
  options.escapeKeyTriggersCloseButton = true;
  options.useNativeTitleBar = true;
  options.resizable = true;

  options.launchAsync();
}

//==============================================================================
// SampleHunterAgent::Listener interface
//==============================================================================

void WingmanPanel::sampleDownloaded(const zenith::ai::FoundSample &sample) {
  auto shutdownFlag = isShuttingDown_;
  juce::MessageManager::callAsync([this, sample, shutdownFlag]() {
    if (shutdownFlag->load()) return;
    appendToConversation("Wingman", "Downloaded sample: " +
                                        sample.localFile.getFileName());
  });
}

void WingmanPanel::sampleAnalyzed(const zenith::ai::FoundSample &sample) {
  auto shutdownFlag = isShuttingDown_;
  juce::MessageManager::callAsync([this, sample, shutdownFlag]() {
    if (shutdownFlag->load()) return;
    // Optional: Show analysis details
  });
}

void WingmanPanel::sampleImported(const juce::File &file) {
  auto shutdownFlag = isShuttingDown_;
  juce::MessageManager::callAsync([this, file, shutdownFlag]() {
    if (shutdownFlag->load()) return;
    appendToConversation("Wingman",
                         "Imported sample to project: " + file.getFileName());
  });
}

void WingmanPanel::huntingProgressChanged(float progress,
                                          const juce::String &status) {
  auto shutdownFlag = isShuttingDown_;
  juce::MessageManager::callAsync([this, progress, status, shutdownFlag]() {
    if (shutdownFlag->load()) return;
    setStatus(status, SkColorSetARGB(255, 0, 170, 255));
  });
}

void WingmanPanel::huntingComplete(const zenith::ai::HuntingStats &stats,
                                   bool success) {
  auto shutdownFlag = isShuttingDown_;
  juce::MessageManager::callAsync([this, stats, success, shutdownFlag]() {
    if (shutdownFlag->load()) return;
    if (success) {
      appendToConversation("Wingman", "Sample hunting complete! Found " +
                                          juce::String(stats.samplesFound) +
                                          " samples.");
      setStatus("Ready", SkColorSetARGB(255, 0, 255, 0));
    } else {
      appendToConversation("Wingman",
                           "Sample hunting failed or was cancelled.");
      setStatus("Failed", SkColorSetARGB(255, 255, 0, 0));
    }
  });
}

} // namespace zenith
