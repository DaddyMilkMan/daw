/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Rewritten: 2026-01-10)
    Author:  Marcus Williams (Original) / AI Assistant (Pure Skia Redesign)

    Modern Wingman AI Panel with pure Skia rendering.
    Sharp rectangle, hairline borders, glassmorphism.
    No JUCE Viewport - custom scroll handling.

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
  // Input Field (Pure Skia Text Input)
  inputField_ = std::make_unique<SkiaTextInput>();
  inputField_->setPlaceholder("Ask Wingman...");
  inputField_->setPillShape(true);
  inputField_->setFontSize(14.0f);
  inputField_->onReturnKey = [this] { sendMessage(); };
  addAndMakeVisible(inputField_.get());
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: SkiaTextInput created");

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
  
  // Add welcome message
  appendMessage("Wingman", "Hello! I'm Wingman, your AI assistant. Toggle the brain icon for deep reasoning mode.");
}

WingmanPanel::~WingmanPanel() = default;

//==============================================================================
// Button Setup
//==============================================================================

void WingmanPanel::setupBrainToggle() {
  brainToggle_ = std::make_unique<ZenithButton>();
  // FIX: Use Sparkles icon for "Reasoning Mode" instead of the old Brain icon
  brainToggle_->setIconPath(icons::Sparkles());
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
  sendButton_->setIconPath(icons::SendArrow());
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
  recalculateLayout();
}

void WingmanPanel::drawSkia(SkCanvas *canvas) {
  if (canvas == nullptr) return;

  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Get animation progress (0.0 to 1.0)
  float progress = getAnimatedValue("entrance_progress");
  
  // Safety: if not animating and progress is 0, default to 1.0
  if (!isAnimating("entrance_progress") && progress < 0.01f) progress = 1.0f;

  // Fade and Slide-in effect (FROM LEFT)
  float alpha = progress;
  // FIX: Animate X from -50 to 0 (Left to Right) instead of Y
  float offsetX = (1.0f - progress) * -50.0f;

  canvas->save();
  if (std::abs(offsetX) > 0.1f) {
      canvas->translate(offsetX, 0); // Slide horizontally
  }

  // PERFORMANCE: Only use saveLayerAlpha when actually fading.
  // saveLayerAlpha triggers an offscreen buffer allocation which is expensive.
  // When fully visible, skip it entirely to stay on the fast GPU path.
  bool needsAlphaLayer = (alpha < 0.999f);
  if (needsAlphaLayer) {
      canvas->saveLayerAlpha(nullptr, (uint8_t)(alpha * 255));
  }

  //==========================================================================
  // 1. Background - Sharp rectangle (NO rounded corners)
  //==========================================================================
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::withAlpha(design::colors::BG_01, 0.95f));
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
  // 4. Chat Area (Pure Skia)
  //==========================================================================
  drawChatArea(canvas);

  //==========================================================================
  // 5. Input Area Separator
  //==========================================================================
  float inputTop = bounds.getHeight() - INPUT_ROW_HEIGHT;
  canvas->drawLine(0, inputTop, rect.width(), inputTop, borderPaint);

  //==========================================================================
  // 6. Draw Children (Buttons, Editor)
  //==========================================================================
  drawChildren(canvas);

  // Restore in correct order
  if (needsAlphaLayer) {
      canvas->restore();  // Restore alpha layer
  }
  canvas->restore();  // Restore initial save
}

void WingmanPanel::drawChatArea(SkCanvas *canvas) {
  if (messages_.empty()) return;
  
  auto bounds = getLocalBounds().toFloat();
  float chatTop = HEADER_HEIGHT + PADDING;
  float chatBottom = bounds.getHeight() - INPUT_ROW_HEIGHT - PADDING;
  float chatHeight = chatBottom - chatTop;
  float maxBubbleWidth = (bounds.getWidth() - PADDING * 2) * BUBBLE_MAX_WIDTH_RATIO;
  
  // Clip to chat area
  canvas->save();
  SkRect clipRect = SkRect::MakeLTRB(0, chatTop, bounds.getWidth(), chatBottom);
  canvas->clipRect(clipRect);
  
  // Calculate total content height if needed
  if (contentHeight_ <= 0) {
    contentHeight_ = 0;
    for (auto &msg : messages_) {
      msg.cachedHeight = calculateMessageHeight(msg, maxBubbleWidth);
      contentHeight_ += msg.cachedHeight + PADDING;
    }
  }
  
  // Draw messages with scroll offset
  float y = chatTop - scrollOffset_;
  
  for (const auto &msg : messages_) {
    // Skip if above visible area
    if (y + msg.cachedHeight < chatTop) {
      y += msg.cachedHeight + PADDING;
      continue;
    }
    
    // Stop if below visible area
    if (y > chatBottom) break;
    
    drawMessage(canvas, msg, y, maxBubbleWidth);
    y += msg.cachedHeight + PADDING;
  }
  
  canvas->restore();
  
  // Draw scroll indicator if content overflows
  if (contentHeight_ > chatHeight) {
    float scrollRatio = scrollOffset_ / (contentHeight_ - chatHeight);
    float indicatorHeight = std::max(20.0f, chatHeight * (chatHeight / contentHeight_));
    float indicatorY = chatTop + (chatHeight - indicatorHeight) * scrollRatio;
    
    SkPaint indicatorPaint;
    indicatorPaint.setAntiAlias(true);
    indicatorPaint.setColor(design::withAlpha(design::colors::TEXT_TERTIARY, 0.4f));
    
    SkRect indicatorRect = SkRect::MakeXYWH(
        bounds.getWidth() - 6, indicatorY, 3, indicatorHeight);
    canvas->drawRoundRect(indicatorRect, 1.5f, 1.5f, indicatorPaint);
  }
}

void WingmanPanel::drawMessage(SkCanvas *canvas, const ChatMessage &msg, float y, float maxWidth) {
  auto bounds = getLocalBounds().toFloat();
  
  // Calculate bubble dimensions
  float bubbleWidth = std::min(maxWidth, bounds.getWidth() - PADDING * 4);
  float bubbleHeight = msg.cachedHeight;
  
  // Position based on sender
  float bubbleX = msg.isUser 
      ? bounds.getWidth() - bubbleWidth - PADDING 
      : PADDING;
  
  SkRect bubbleRect = SkRect::MakeXYWH(bubbleX, y, bubbleWidth, bubbleHeight);
  
  // Bubble background
  SkPaint bubblePaint;
  bubblePaint.setAntiAlias(true);
  
  if (msg.isUser) {
    // User message - accent color
    bubblePaint.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY, 0.8f));
  } else {
    // Assistant message - dark surface
    bubblePaint.setColor(design::colors::SURFACE_BASE);
  }
  
  canvas->drawRoundRect(bubbleRect, BUBBLE_RADIUS, BUBBLE_RADIUS, bubblePaint);
  
  // Text
  SkFont textFont = design::typography::getSkFont(13.0f);
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  
  // Simple text wrapping
  float textX = bubbleX + BUBBLE_PADDING;
  float textY = y + BUBBLE_PADDING + 14.0f; // baseline offset
  float textMaxWidth = bubbleWidth - BUBBLE_PADDING * 2;
  
  // Split into lines (basic word wrap)
  juce::String remaining = msg.text;
  while (remaining.isNotEmpty() && textY < y + bubbleHeight) {
    // Find how much text fits on this line
    int charsThatFit = 0;
    
    for (int i = 0; i < remaining.length(); ++i) {
      SkRect charBounds;
      textFont.measureText(remaining.substring(0, i + 1).toUTF8(), 
                          remaining.substring(0, i + 1).getNumBytesAsUTF8(),
                          SkTextEncoding::kUTF8, &charBounds);
      if (charBounds.width() > textMaxWidth && i > 0) break;
      charsThatFit = i + 1;
    }
    
    // Find last space for word wrap
    int breakPoint = charsThatFit;
    if (charsThatFit < remaining.length()) {
      for (int i = charsThatFit - 1; i > 0; --i) {
        if (remaining[i] == ' ') {
          breakPoint = i + 1;
          break;
        }
      }
    }
    
    juce::String line = remaining.substring(0, breakPoint).trimEnd();
    remaining = remaining.substring(breakPoint).trimStart();
    
    canvas->drawString(line.toStdString().c_str(), textX, textY, textFont, textPaint);
    textY += LINE_HEIGHT;
  }
}

float WingmanPanel::calculateMessageHeight(const ChatMessage &msg, float maxWidth) {
  SkFont textFont = design::typography::getSkFont(13.0f);
  float textMaxWidth = maxWidth - BUBBLE_PADDING * 2;
  
  // Count lines needed
  int lineCount = 1;
  juce::String remaining = msg.text;
  
  while (remaining.isNotEmpty()) {
    int charsThatFit = 0;
    
    for (int i = 0; i < remaining.length(); ++i) {
      SkRect charBounds;
      textFont.measureText(remaining.substring(0, i + 1).toUTF8(), 
                          remaining.substring(0, i + 1).getNumBytesAsUTF8(),
                          SkTextEncoding::kUTF8, &charBounds);
      if (charBounds.width() > textMaxWidth && i > 0) break;
      charsThatFit = i + 1;
    }
    
    if (charsThatFit >= remaining.length()) break;
    
    // Find last space for word wrap
    int breakPoint = charsThatFit;
    for (int i = charsThatFit - 1; i > 0; --i) {
      if (remaining[i] == ' ') {
        breakPoint = i + 1;
        break;
      }
    }
    
    remaining = remaining.substring(breakPoint).trimStart();
    if (remaining.isNotEmpty()) lineCount++;
  }
  
  return BUBBLE_PADDING * 2 + lineCount * LINE_HEIGHT;
}

//==============================================================================
// Layout
//==============================================================================

void WingmanPanel::resized() {
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

  // Store chat area bounds
  chatAreaBounds_ = bounds.toFloat();
  
  // Recalculate message heights
  recalculateLayout();
}

void WingmanPanel::recalculateLayout() {
  auto bounds = getLocalBounds().toFloat();
  float maxBubbleWidth = (bounds.getWidth() - PADDING * 2) * BUBBLE_MAX_WIDTH_RATIO;
  
  contentHeight_ = 0;
  for (auto &msg : messages_) {
    msg.cachedHeight = calculateMessageHeight(msg, maxBubbleWidth);
    contentHeight_ += msg.cachedHeight + PADDING;
  }
  
  markDirty();
}

void WingmanPanel::scrollToBottom() {
  float chatHeight = getHeight() - HEADER_HEIGHT - INPUT_ROW_HEIGHT - PADDING * 2;
  float maxScroll = std::max(0.0f, contentHeight_ - chatHeight);
  scrollOffset_ = maxScroll;
  markDirty();
}

void WingmanPanel::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
  juce::ignoreUnused(e);
  
  float chatHeight = getHeight() - HEADER_HEIGHT - INPUT_ROW_HEIGHT - PADDING * 2;
  float maxScroll = std::max(0.0f, contentHeight_ - chatHeight);
  
  // Scroll amount (negative wheel.deltaY = scroll down)
  float scrollDelta = -wheel.deltaY * 50.0f;
  scrollOffset_ = juce::jlimit(0.0f, maxScroll, scrollOffset_ + scrollDelta);
  
  markDirty();
}

//==============================================================================
// Messaging
//==============================================================================

void WingmanPanel::sendMessage() {
  if (isProcessing_) {
    DBG("Wingman: Blocked sendMessage - already processing");
    return;
  }

  auto query = inputField_->getText().trim();
  if (query.isEmpty()) return;
  
  DBG("Wingman: Sending query: " + query);

  inputField_->clear();
  appendMessage("You", query);

  isProcessing_ = true;
  
  // FAILSAFE: Force reset processing state after 10s if no response
  juce::Timer::callAfterDelay(10000, [this] {
    if (isProcessing_) {
      DBG("Wingman: Forced processing reset (timeout)");
      isProcessing_ = false;
      // Optional: Inform user of timeout
      appendMessage("System", "Request timed out. Please try again.");
    }
  });

  // Determine Grok mode
  GrokMode mode = reasoningMode_ ? GrokMode::Thinking : GrokMode::Fast;

  grokController_->executeCommand(
      query, mode,
      // Success callback
      [this](juce::String response) {
        juce::MessageManager::callAsync([this, response] {
          DBG("Wingman: Received response: " + response.substring(0, 50) + "...");
          appendMessage("Wingman", response);
          isProcessing_ = false;
        });
      },
      // Error callback
      [this](juce::String error) {
        juce::MessageManager::callAsync([this, error] {
          DBG("Wingman: Error received: " + error);
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
  ChatMessage msg;
  msg.speaker = speaker;
  msg.text = message;
  msg.isUser = (speaker == "You");
  msg.cachedHeight = 0; // Will be calculated
  
  messages_.push_back(msg);
  
  recalculateLayout();
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
