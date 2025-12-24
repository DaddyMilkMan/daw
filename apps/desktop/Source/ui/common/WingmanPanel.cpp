/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Updated for Premium AI Chat)
    Author:  Marcus Williams (UX Team)

    Wingman AI Chat Interface (Pure Skia)
    Supports Grok 4.1 Reasoning Modes (Fast/Thinking)

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "../../engine/ZenithLogger.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include "../network/SecureKeyStore.h"
#include "SettingsComponent.h"
#include "ZenithSkia.h"
#include <core/SkTypeface.h>

namespace zenith {

// Helper to convert JUCE colour to SkColor
static SkColor toSkColor(const juce::Colour &c) {
  return SkColorSetARGB(c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue());
}

//==============================================================================
WingmanPanel::WingmanPanel(CommandAPI &api, Engine &engine)
    : commandAPI_(api), engine_(engine) {

  // Create Grok controller
  grokController_ = std::make_unique<GrokDAWController>(commandAPI_);

  //==========================================================================
  // UI Components (Pure Skia)

  // Reasoning Mode Label
  reasoningLabel_ = std::make_unique<SkiaLabel>("ReasoningLabel");
  reasoningLabel_->setText("Reasoning:");
  addAndMakeVisible(reasoningLabel_.get());

  // Reasoning Buttons (Fast/Thinking)
  fastModeButton_ = std::make_unique<SkiaButton>("FastMode");
  fastModeButton_->setButtonText("Fast");
  fastModeButton_->setToggleState(true);
  fastModeButton_->onClick = [this] {
    currentMode_ = GrokMode::Fast;
    fastModeButton_->setToggleState(true);
    thinkingModeButton_->setToggleState(false);
    repaint();
  };
  addAndMakeVisible(fastModeButton_.get());

  thinkingModeButton_ = std::make_unique<SkiaButton>("ThinkingMode");
  thinkingModeButton_->setButtonText("Thinking");
  thinkingModeButton_->setToggleState(false);
  thinkingModeButton_->onClick = [this] {
    currentMode_ = GrokMode::Thinking;
    fastModeButton_->setToggleState(false);
    thinkingModeButton_->setToggleState(true);
    repaint();
  };
  addAndMakeVisible(thinkingModeButton_.get());

  // Input Field
  inputField_ = std::make_unique<SkiaTextEditor>("Input");
  inputField_->setMultiLine(false);
  inputField_->setTextToShowWhenEmpty(
      "Ask Wingman anything...", toSkColor(ZenithTheme::Colors::text_tertiary));
  inputField_->setTextColour(toSkColor(ZenithTheme::Colors::text_primary));
  inputField_->onReturnKey = [this] { sendCommand(); };
  addAndMakeVisible(inputField_.get());

  // Send Button
  sendButton_ = std::make_unique<SkiaButton>("Send");
  sendButton_->setButtonText("Send");
  sendButton_->onClick = [this] { sendCommand(); };
  addAndMakeVisible(sendButton_.get());

  // Status Label
  statusLabel_ = std::make_unique<SkiaLabel>("Status");
  statusLabel_->setText("Ready");
  addAndMakeVisible(statusLabel_.get());

  // Clear Button
  clearButton_ = std::make_unique<SkiaButton>("Clear");
  clearButton_->setButtonText("Clear");
  clearButton_->setStyle(SkiaButton::Style::Secondary);
  clearButton_->onClick = [this] {
    conversationHistory_.clear();
    conversationScrollY_ = 0;
    grokController_->clearHistory();
    appendToConversation("Wingman",
                         "Chat history cleared. How can I assist you?");
  };
  addAndMakeVisible(clearButton_.get());

  // Settings Button
  settingsButton_ = std::make_unique<SkiaButton>("Set");
  settingsButton_->setButtonText("Set");
  settingsButton_->setStyle(SkiaButton::Style::Ghost);
  settingsButton_->onClick = [this] { showSettings(); };
  addAndMakeVisible(settingsButton_.get());

  // Welcome Message
  appendToConversation(
      "Wingman",
      "Hello! I am Wingman, your AI music assistant. I use Grok 4.1 to help "
      "you find samples, control tracks, and explain synthesis.\n\n"
      "Switch to **Thinking Mode** for complex music theory or sound design "
      "advice.");

  // Initial check for API Key
  if (!isGrokReady()) {
    appendToConversation(
        "System", "Grok API Key is missing. Click 'Set' to configure it.");
  }
}

WingmanPanel::~WingmanPanel() = default;

//==============================================================================
void WingmanPanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();

  // 1. Solid Panel Background (Neon Noir Slate)
  SkPaint bgPaint;
  bgPaint.setColor(toSkColor(ZenithTheme::Colors::bg_01));
  bgPaint.setAlpha(255);
  canvas->drawRect(
      SkRect::MakeWH((float)bounds.getWidth(), (float)bounds.getHeight()),
      bgPaint);

  // 2. Header
  SkRect headerRect = SkRect::MakeXYWH(0, 0, (float)bounds.getWidth(), 40.0f);
  SkPaint headerPaint;
  headerPaint.setColor(toSkColor(ZenithTheme::Colors::bg_02));
  canvas->drawRect(headerRect, headerPaint);

  SkFont titleFont = zenith::design::getSkFont(
      zenith::design::typography::FONT_LG, zenith::design::FontWeight::Bold);
  SkPaint titlePaint;
  titlePaint.setColor(toSkColor(ZenithTheme::Colors::accent_primary));
  canvas->drawString("Wingman Chat", 12, 28, titleFont, titlePaint);

  // 3. Conversation Area
  SkRect convRect = SkRect::MakeXYWH(0, 42, (float)bounds.getWidth(),
                                     (float)bounds.getHeight() - 182);
  canvas->save();
  canvas->clipRect(convRect);
  drawConversation(canvas, convRect);
  canvas->restore();

  // 4. Children (Controls)
  drawChildren(canvas);
}

void WingmanPanel::drawConversation(SkCanvas *canvas, const SkRect &bounds) {
  SkFont bubbleFont =
      zenith::design::getSkFont(zenith::design::typography::FONT_MD);
  SkFont speakerFont = zenith::design::getSkFont(
      zenith::design::typography::FONT_XS, zenith::design::FontWeight::Medium);

  SkPaint textPaint;
  textPaint.setColor(toSkColor(ZenithTheme::Colors::text_primary));

  float y = bounds.top() + 10 - conversationScrollY_;
  float padding = 12;
  float msgGap = 16;

  for (const auto &msg : conversationHistory_) {
    // Bubble calculations (simplified layout)
    auto lines = juce::StringArray::fromLines(msg.content);
    float boxWidth = bounds.width() - 24;
    float boxHeight = 25 + (lines.size() * 18); // rough calc

    if (y + boxHeight > bounds.top() && y < bounds.bottom()) {
      SkRect bubbleRect = SkRect::MakeXYWH(12, y, boxWidth, boxHeight);

      // Bubble Background
      SkPaint bubblePaint;
      if (msg.speaker == "You") {
        bubblePaint.setColor(
            zenith::design::withAlpha(zenith::design::colors::CYAN, 0.08f));
      } else if (msg.speaker == "System" || msg.speaker == "Error") {
        bubblePaint.setColor(
            zenith::design::withAlpha(zenith::design::colors::MAGENTA, 0.12f));
      } else {
        bubblePaint.setColor(toSkColor(ZenithTheme::Colors::bg_02));
      }
      canvas->drawRoundRect(bubbleRect, 8, 8, bubblePaint);

      // Speaker Label
      SkPaint spkPaint;
      spkPaint.setColor(toSkColor(msg.speaker == "You"
                                      ? ZenithTheme::Colors::accent_primary
                                      : ZenithTheme::Colors::success));
      canvas->drawString(msg.speaker.getCharPointer(), 20, y + 16, speakerFont,
                         spkPaint);

      // Content text
      float lineY = y + 34;
      for (const auto &line : lines) {
        canvas->drawString(line.toStdString().c_str(), 20, lineY, bubbleFont,
                           textPaint);
        lineY += 18;
      }
    }

    y += boxHeight + msgGap;
  }
}

void WingmanPanel::resized() {
  auto bounds = getLocalBounds();

  // Header controls
  settingsButton_->setBounds(bounds.getWidth() - 45, 6, 40, 28);

  // Bottom Section
  auto bottomArea = bounds.removeFromBottom(140).reduced(10, 0);

  // 1. Reasoning mode toggle row
  auto modeRow = bottomArea.removeFromTop(40);
  reasoningLabel_->setBounds(modeRow.removeFromLeft(70));
  fastModeButton_->setBounds(modeRow.removeFromLeft(60).reduced(2));
  thinkingModeButton_->setBounds(modeRow.removeFromLeft(80).reduced(2));
  statusLabel_->setBounds(modeRow.removeFromRight(80));

  // 2. Input Row
  auto inputRow = bottomArea.removeFromTop(50);
  sendButton_->setBounds(inputRow.removeFromRight(70).reduced(0, 5));
  inputField_->setBounds(inputRow.reduced(0, 5));

  // 3. Clear button
  auto clearRow = bottomArea;
  clearButton_->setBounds(clearRow.removeFromLeft(60).reduced(0, 8));
}

//==============================================================================
void WingmanPanel::sendCommand() {
  if (isProcessing_)
    return;

  juce::String command = inputField_->getText().trim();
  if (command.isEmpty())
    return;

  inputField_->clear();
  appendToConversation("You", command);

  if (!isGrokReady()) {
    appendToConversation(
        "System", "Grok API not configured. Use 'Set' to configure key.");
    return;
  }

  isProcessing_ = true;
  setStatus("Thinking...", toSkColor(ZenithTheme::Colors::accent_primary));

  grokController_->executeCommand(
      command, currentMode_,
      [this](juce::String response) {
        juce::MessageManager::callAsync([this, response]() {
          appendToConversation("Wingman", response);
          isProcessing_ = false;
          setStatus("Ready", SK_ColorGREEN);
        });
      },
      [this](juce::String error) {
        juce::MessageManager::callAsync([this, error]() {
          appendToConversation("Error", error);
          isProcessing_ = false;
          setStatus("Error", SK_ColorRED);
        });
      },
      [this](juce::String status) {
        juce::MessageManager::callAsync([this, status]() {
          setStatus(status, toSkColor(ZenithTheme::Colors::accent_primary));
        });
      });
}

void WingmanPanel::appendToConversation(const juce::String &speaker,
                                        const juce::String &content) {
  ChatMessage msg;
  msg.speaker = speaker;
  msg.content = content;

  // Auto color based on speaker
  if (speaker == "You")
    msg.speakerColor = toSkColor(ZenithTheme::Colors::accent_primary);
  else if (speaker == "Wingman")
    msg.speakerColor = toSkColor(ZenithTheme::Colors::success);
  else
    msg.speakerColor = SK_ColorWHITE;

  conversationHistory_.push_back(msg);

  // Simple auto-scroll
  if (conversationHistory_.size() > 5) {
    conversationScrollY_ =
        std::max(0.0f, (float)(conversationHistory_.size() * 60) - 300);
  }

  repaint();
}

void WingmanPanel::showSettings() {
  juce::DialogWindow::LaunchOptions options;
  options.content.setOwned(new SettingsComponent(engine_));
  options.content->setSize(600, 500);
  options.dialogTitle = "Zenith Settings";
  options.launchAsync();
}

void WingmanPanel::mouseWheelMove(const juce::MouseEvent &,
                                  const juce::MouseWheelDetails &wheel) {
  conversationScrollY_ -= wheel.deltaY * 60.0f;
  if (conversationScrollY_ < 0)
    conversationScrollY_ = 0;
  repaint();
}

void WingmanPanel::setStatus(const juce::String &text, SkColor colour) {
  if (statusLabel_) {
    statusLabel_->setText(text);
    statusLabel_->setTextColour(colour);
  }
}

bool WingmanPanel::initializeGrok(const juce::String &apiKey) {
  return grokController_->initialize(apiKey);
}

bool WingmanPanel::isGrokReady() const {
  return grokController_ && grokController_->isReady();
}

// Stubs for SampleHunterListener
void WingmanPanel::sampleDownloaded(const zenith::ai::FoundSample &) {}
void WingmanPanel::sampleAnalyzed(const zenith::ai::FoundSample &) {}
void WingmanPanel::sampleImported(const juce::File &) {}
void WingmanPanel::huntingProgressChanged(float, const juce::String &) {}
void WingmanPanel::huntingComplete(const zenith::ai::HuntingStats &, bool) {}
bool WingmanPanel::detectSampleSearchIntent(const juce::String &,
                                            juce::String &) {
  return false;
}
juce::String WingmanPanel::cleanQueryFiller(const juce::String &q) { return q; }
bool WingmanPanel::handleImportCommand(const juce::String &) { return false; }
void WingmanPanel::displaySearchResults(
    const std::vector<zenith::ai::FoundSample> &) {}

} // namespace zenith
