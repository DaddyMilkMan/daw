/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Rewritten: 2025-12-30)
    Author:  Marcus Williams (Original) / AI Assistant (Redesign)

    Modern Wingman AI Panel with pure Skia rendering.
    Sharp rectangle, hairline borders, glassmorphism.

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
#include "../engine/ZenithLogger.h"
#include "../network/SecureKeyStore.h"
#include "SettingsComponent.h"
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

WingmanPanel::WingmanPanel(CommandAPI &api, Engine &engine)
    : commandAPI_(api), engine_(engine) {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Constructor started");
  
  // Initialize Grok controller
  grokController_ = std::make_unique<GrokDAWController>(commandAPI_);
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: GrokController created");

  //==========================================================================
  // Chat Viewport
  chatViewport_ = std::make_unique<juce::Viewport>();
  chatViewport_->setScrollBarsShown(true, false);
  chatViewport_->setScrollBarThickness(4);
  chatViewport_->getVerticalScrollBar().setColour(
      juce::ScrollBar::thumbColourId,
      juce::Colour(design::withAlpha(design::colors::TEXT_TERTIARY, 0.4f)));
  addAndMakeVisible(chatViewport_.get());

  chatContainer_ = std::make_unique<juce::Component>();
  chatViewport_->setViewedComponent(chatContainer_.get(), false);
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Viewport setup complete");

  //==========================================================================
  // Input Field (Pill Editor)
  inputField_ = std::make_unique<WingmanPillEditor>();
  inputField_->onReturnKey = [this] { sendMessage(); };
  addAndMakeVisible(inputField_.get());
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: InputField created");

  //==========================================================================
  // Brain Toggle (Reasoning Mode)
  setupBrainToggle();

  //==========================================================================
  // Send Button
  setupSendButton();

  //==========================================================================
  // Settings Button (Header)
  setupSettingsButton();

  // Initialize Grok
  initializeGrok();
}

WingmanPanel::~WingmanPanel() = default;

//==============================================================================
// Button Setup
//==============================================================================

void WingmanPanel::setupBrainToggle() {
  brainToggle_ = std::make_unique<ZenithButton>();
  brainToggle_->setIconPath(icons::Brain());
  brainToggle_->setButtonStyle(ZenithButton::Style::Ghost);
  brainToggle_->setToggleable(true);
  brainToggle_->setToggleState(reasoningMode_, false);
  brainToggle_->setTooltip("Toggle Reasoning Mode (Grok 4.1)");
  
  brainToggle_->onToggle = [this](bool toggled) {
    reasoningMode_ = toggled;
    
    // Pink glow when ON
    if (toggled) {
      brainToggle_->setGlowColor(design::colors::NEON_PINK);
      brainToggle_->setGlowEnabled(true);
    } else {
      brainToggle_->setGlowEnabled(false);
    }
    
    DBG("Wingman: Reasoning mode " + juce::String(toggled ? "ON" : "OFF"));
  };
  
  addAndMakeVisible(brainToggle_.get());
}

void WingmanPanel::setupSendButton() {
  sendButton_ = std::make_unique<ZenithButton>();
  sendButton_->setIconPath(icons::Send());
  sendButton_->setButtonStyle(ZenithButton::Style::Ghost);
  sendButton_->setTooltip("Send Message");
  
  // Blue glow on click
  sendButton_->setGlowColor(design::colors::ACCENT_PRIMARY);
  
  sendButton_->onClick = [this] { 
    sendButton_->setGlowEnabled(true);
    sendMessage(); 
    
    // Disable glow after short delay
    juce::Timer::callAfterDelay(200, [this] {
      if (sendButton_)
        sendButton_->setGlowEnabled(false);
    });
  };
  
  addAndMakeVisible(sendButton_.get());
}

void WingmanPanel::setupSettingsButton() {
  settingsButton_ = std::make_unique<ZenithButton>();
  settingsButton_->setIconPath(icons::Settings());
  settingsButton_->setButtonStyle(ZenithButton::Style::Ghost);
  settingsButton_->setTooltip("Settings");
  
  settingsButton_->onClick = [this] {
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(new SettingsComponent(engine_));
    options.content->setSize(600, 500);
    options.dialogTitle = "Zenith Settings";
    options.dialogBackgroundColour = juce::Colour(design::colors::BG_01);
    options.useNativeTitleBar = true;
    options.launchAsync();
  };
  
  addAndMakeVisible(settingsButton_.get());
}

//==============================================================================
// Skia Rendering
//==============================================================================

void WingmanPanel::visibilityChanged() {
  if (isVisible()) {
    onShow();
  }
}

void WingmanPanel::onShow() {
  // Reset and start entrance animation
  stopAllAnimations();
  animateTo("entrance_progress", 1.0f, 500);
  
  // Welcome message if conversation is empty
  /*
  if (chatBubbles_.empty()) {
    appendMessage("Wingman", "Hello! I'm Wingman. Toggle the brain for reasoning mode.");
  }
  */
}

void WingmanPanel::drawSkia(SkCanvas *canvas) {
  if (canvas == nullptr) return;
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: drawSkia() called");

  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Get animation progress (0.0 to 1.0)
  float progress = getAnimatedValue("entrance_progress");
  
  // Safety: if not animating and progress is 0, default to 1.0 (prevents being invisible if onShow wasn't called)
  if (!isAnimating("entrance_progress") && progress < 0.01f) progress = 1.0f;

  // Fade and Slide-up effect
  float alpha = progress;
  float offsetY = (1.0f - progress) * 30.0f;

  canvas->save();
  if (offsetY > 0.1f) {
      canvas->translate(0, offsetY);
  }

  canvas->saveLayerAlpha(nullptr, (uint8_t)(alpha * 255));

  //==========================================================================
  // 1. Background - Sharp rectangle (NO rounded corners)
  //==========================================================================
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::withAlpha(design::colors::BG_01, 0.92f));
  canvas->drawRect(rect, bgPaint);

  // Subtle gradient overlay
  SkPoint gradPts[2] = {{0, 0}, {0, rect.height()}};
  SkColor gradColors[2] = {
      design::withAlpha(design::colors::ACCENT_PRIMARY, 0.03f),
      SkColorSetARGB(0, 0, 0, 0)
  };
  SkPaint gradPaint;
  gradPaint.setShader(SkGradientShader::MakeLinear(
      gradPts, gradColors, nullptr, 2, SkTileMode::kClamp));
  canvas->drawRect(rect, gradPaint);

  //==========================================================================
  // 2. Hairline Border (0.5px)
  //==========================================================================
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(0.5f);  // HAIRLINE
  borderPaint.setColor(design::colors::BORDER_SUBTLE);
  canvas->drawRect(rect, borderPaint);

  //==========================================================================
  // 3. Header Area
  //==========================================================================
  float headerBottom = static_cast<float>(HEADER_HEIGHT);
  
  // Header background (slightly elevated)
  SkRect headerRect = SkRect::MakeLTRB(0, 0, rect.width(), headerBottom);
  SkPaint headerPaint;
  headerPaint.setColor(design::withAlpha(design::colors::BG_02, 0.5f));
  canvas->drawRect(headerRect, headerPaint);

  // Header separator (hairline)
  borderPaint.setAlphaf(0.5f);
  canvas->drawLine(0, headerBottom, rect.width(), headerBottom, borderPaint);

  // Title
  SkFont titleFont = design::typography::getSkFont(
      design::typography::FONT_LG, design::FontWeight::Bold);
  SkPaint titlePaint;
  titlePaint.setAntiAlias(true);
  titlePaint.setColor(design::withAlpha(design::colors::TEXT_PRIMARY, 1.0f));
  canvas->drawString("WINGMAN AI", PADDING * 1.5f, headerBottom / 2.0f + 6.0f, titleFont, titlePaint);

  //==========================================================================
  // 4. Input Area Separator
  //==========================================================================
  float inputTop = bounds.getHeight() - INPUT_ROW_HEIGHT;
  canvas->drawLine(0, inputTop, rect.width(), inputTop, borderPaint);

  //==========================================================================
  // 5. Draw Children (Buttons, Editor)
  //==========================================================================
  drawChildren(canvas);

  canvas->restore();
  canvas->restore();
}

//==============================================================================
// Layout
//==============================================================================

void WingmanPanel::resized() {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: resized() called");
  auto bounds = getLocalBounds();

  // Header - settings button on right
  auto headerArea = bounds.removeFromTop(HEADER_HEIGHT);
  settingsButton_->setBounds(
      headerArea.removeFromRight(BUTTON_SIZE + PADDING).reduced(4));

  // Input row at bottom
  auto inputArea = bounds.removeFromBottom(INPUT_ROW_HEIGHT).reduced(PADDING, 8);
  
  // Brain toggle on left
  brainToggle_->setBounds(inputArea.removeFromLeft(BUTTON_SIZE).reduced(2));
  inputArea.removeFromLeft(4);  // Spacing
  
  // Send button on right
  sendButton_->setBounds(inputArea.removeFromRight(BUTTON_SIZE).reduced(2));
  inputArea.removeFromRight(4);  // Spacing
  
  // Input field fills remainder
  inputField_->setBounds(inputArea);

  // Chat viewport fills middle
  chatViewport_->setBounds(bounds);

  // Layout bubbles
  layoutChatBubbles();
}

void WingmanPanel::layoutChatBubbles() {
  if (chatContainer_ == nullptr || chatViewport_ == nullptr) return;

  int y = PADDING;
  int containerWidth = chatViewport_->getWidth() - PADDING * 2;
  if (containerWidth <= 0) containerWidth = 280;

  for (auto &bubble : chatBubbles_) {
    int bubbleHeight = bubble->getRequiredHeight(containerWidth);
    bubble->setBounds(PADDING, y, containerWidth, bubbleHeight);
    y += bubbleHeight + PADDING;
  }

  chatContainer_->setSize(chatViewport_->getWidth(), y + PADDING);
}

void WingmanPanel::scrollToBottom() {
  if (chatViewport_ && chatContainer_) {
    chatViewport_->setViewPosition(0, chatContainer_->getHeight());
  }
}

//==============================================================================
// Messaging
//==============================================================================

void WingmanPanel::sendMessage() {
  if (isProcessing_) return;

  auto query = inputField_->getText().trim();
  if (query.isEmpty()) return;

  inputField_->clear();
  appendMessage("You", query);

  isProcessing_ = true;

  // Determine Grok mode
  GrokMode mode = reasoningMode_ ? GrokMode::Thinking : GrokMode::Fast;

  grokController_->executeCommand(
      query, mode,
      // Success callback
      [this](juce::String response) {
        juce::MessageManager::callAsync([this, response] {
          appendMessage("Wingman", response);
          isProcessing_ = false;
        });
      },
      // Error callback
      [this](juce::String error) {
        juce::MessageManager::callAsync([this, error] {
          appendMessage("System", "Error: " + error);
          isProcessing_ = false;
        });
      },
      // Progress callback (optional)
      [this](juce::String status) {
        juce::MessageManager::callAsync([this, status] {
          DBG("Wingman progress: " + status);
        });
      });
}

void WingmanPanel::appendMessage(const juce::String &speaker,
                                  const juce::String &message) {
  bool isUser = (speaker == "You");
  auto bubble = std::make_unique<WingmanChatBubble>(message, isUser);
  chatContainer_->addAndMakeVisible(bubble.get());
  chatBubbles_.push_back(std::move(bubble));

  layoutChatBubbles();
  scrollToBottom();
}

//==============================================================================
// Grok Integration
//==============================================================================

bool WingmanPanel::initializeGrok(const juce::String &apiKey) {
  bool success = grokController_->initialize(apiKey);
  
  if (success) {
    DBG("Wingman: Grok initialized successfully");
  } else {
    DBG("Wingman: Grok initialization failed - check API key");
  }
  
  return success;
}

bool WingmanPanel::isGrokReady() const {
  return grokController_->isReady();
}

} // namespace zenith
