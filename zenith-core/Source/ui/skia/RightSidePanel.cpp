/*
  ==============================================================================
    RightSidePanel.cpp
    Wingman AI Assistant Panel - Full Skia rendering
    Modern chat interface for AI assistant in DAW
  ==============================================================================
*/

#include "RightSidePanel.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

//==============================================================================
// Color Constants
//==============================================================================
namespace {
  constexpr SkColor BG_DARK = 0xFF0D0D0D;
  constexpr SkColor BG_PANEL = 0xFF141414;
  constexpr SkColor BG_INPUT = 0xFF1A1A1A;
  constexpr SkColor BG_MESSAGE_USER = 0xFF1E3A5F;
  constexpr SkColor BG_MESSAGE_AI = 0xFF1A1A1A;
  constexpr SkColor ACCENT = 0xFF00B4D8;
  constexpr SkColor ACCENT_DIM = 0xFF006080;
  constexpr SkColor TEXT_PRIMARY = 0xFFE0E0E0;
  constexpr SkColor TEXT_SECONDARY = 0xFF808080;
  constexpr SkColor TEXT_MUTED = 0xFF505050;
  constexpr SkColor BORDER = 0xFF2A2A2A;
  constexpr SkColor SUCCESS = 0xFF4ADE80;
  constexpr SkColor OFFLINE = 0xFFEF4444;
}

//==============================================================================
// Construction
//==============================================================================

RightSidePanel::RightSidePanel() {
  setSize(320, 600);
  
  // Add welcome message
  addMessage("Welcome to Wingman AI. I can help you with music production, mixing, and sound design.", false);
  
  startTimerHz(30); // Animation timer
}

RightSidePanel::~RightSidePanel() {
  stopTimer();
}

void RightSidePanel::resized() {
  // Layout handled in draw methods
}

void RightSidePanel::timerCallback() {
  // Animate connection pulse
  connectionPulse_ += 0.1f;
  if (connectionPulse_ > 6.28f) connectionPulse_ = 0.0f;
  repaint();
}

//==============================================================================
// Chat Interface
//==============================================================================

void RightSidePanel::addMessage(const juce::String& text, bool isUser) {
  ChatMessage msg;
  msg.text = text;
  msg.isUser = isUser;
  msg.timestamp = juce::Time::currentTimeMillis();
  messages_.push_back(msg);
  repaint();
}

//==============================================================================
// Interaction
//==============================================================================

void RightSidePanel::mouseDown(const juce::MouseEvent &e) {
  auto zone = hitTest(e.getPosition());
  
  if (zone == HitZone::SendButton) {
    // Send message logic would go here
    DBG("Send button clicked");
  }
}

void RightSidePanel::mouseMove(const juce::MouseEvent &e) {
  auto newZone = hitTest(e.getPosition());
  if (newZone != hoveredZone_) {
    hoveredZone_ = newZone;
    repaint();
  }
}

void RightSidePanel::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hoveredZone_ = HitZone::None;
  repaint();
}

RightSidePanel::HitZone RightSidePanel::hitTest(juce::Point<int> pos) const {
  float y = static_cast<float>(pos.y);
  float x = static_cast<float>(pos.x);
  float h = static_cast<float>(getHeight());
  float w = static_cast<float>(getWidth());
  
  // Input area at bottom
  if (y > h - INPUT_HEIGHT - PADDING) {
    if (x > w - 50) {
      return HitZone::SendButton;
    }
    return HitZone::InputField;
  }
  
  // Chat area
  if (y > HEADER_HEIGHT && y < h - INPUT_HEIGHT - PADDING) {
    return HitZone::ChatArea;
  }
  
  return HitZone::None;
}

//==============================================================================
// Skia Rendering
//==============================================================================

#ifdef ZENITH_USE_SKIA
void RightSidePanel::drawSkia(SkCanvas *canvas) {
  if (!canvas) return;
  
  drawBackground(canvas);
  drawHeader(canvas);
  drawChatArea(canvas);
  drawInputArea(canvas);
}

void RightSidePanel::drawBackground(SkCanvas *canvas) {
  float w = static_cast<float>(getWidth());
  float h = static_cast<float>(getHeight());
  
  // Main background
  SkPaint bgPaint;
  bgPaint.setColor(BG_PANEL);
  canvas->drawRect(SkRect::MakeWH(w, h), bgPaint);
  
  // Left border
  SkPaint borderPaint;
  borderPaint.setColor(BORDER);
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawLine(0, 0, 0, h, borderPaint);
}

void RightSidePanel::drawHeader(SkCanvas *canvas) {
  float w = static_cast<float>(getWidth());
  
  // Header background
  SkPaint headerPaint;
  headerPaint.setColor(BG_DARK);
  canvas->drawRect(SkRect::MakeXYWH(0, 0, w, HEADER_HEIGHT), headerPaint);
  
  // Bottom border
  SkPaint borderPaint;
  borderPaint.setColor(BORDER);
  canvas->drawLine(0, HEADER_HEIGHT, w, HEADER_HEIGHT, borderPaint);
  
  // Wingman icon (simplified AI brain icon)
  float iconX = PADDING + 8;
  float iconY = HEADER_HEIGHT / 2;
  
  SkPaint iconPaint;
  iconPaint.setAntiAlias(true);
  iconPaint.setColor(ACCENT);
  iconPaint.setStyle(SkPaint::kStroke_Style);
  iconPaint.setStrokeWidth(2.0f);
  
  // Draw simple brain/circuit icon
  canvas->drawCircle(iconX, iconY, 10, iconPaint);
  canvas->drawLine(iconX - 6, iconY, iconX + 6, iconY, iconPaint);
  canvas->drawLine(iconX, iconY - 6, iconX, iconY + 6, iconPaint);
  
  // Title
  SkFont titleFont;
  titleFont.setSize(14.0f);
  titleFont.setEmbolden(true);
  
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(TEXT_PRIMARY);
  
  canvas->drawString("Wingman AI", PADDING + 28, HEADER_HEIGHT / 2 + 5, titleFont, textPaint);
  
  // Connection status indicator
  drawConnectionStatus(canvas);
}

void RightSidePanel::drawConnectionStatus(SkCanvas *canvas) {
  float w = static_cast<float>(getWidth());
  float statusX = w - PADDING - 8;
  float statusY = HEADER_HEIGHT / 2;
  
  SkPaint statusPaint;
  statusPaint.setAntiAlias(true);
  
  if (isConnected_) {
    statusPaint.setColor(SUCCESS);
  } else {
    // Pulsing offline indicator
    float alpha = 0.5f + 0.5f * std::sin(connectionPulse_);
    statusPaint.setColor(SkColorSetA(OFFLINE, static_cast<int>(alpha * 255)));
  }
  
  canvas->drawCircle(statusX, statusY, 5, statusPaint);
  
  // Status text
  SkFont statusFont;
  statusFont.setSize(10.0f);
  
  SkPaint statusTextPaint;
  statusTextPaint.setAntiAlias(true);
  statusTextPaint.setColor(TEXT_MUTED);
  
  canvas->drawString(isConnected_ ? "Online" : "Offline", 
                     statusX - 45, statusY + 3, statusFont, statusTextPaint);
}

void RightSidePanel::drawChatArea(SkCanvas *canvas) {
  float w = static_cast<float>(getWidth());
  float h = static_cast<float>(getHeight());
  float chatTop = HEADER_HEIGHT + PADDING;
  float chatBottom = h - INPUT_HEIGHT - PADDING * 2;
  float chatHeight = chatBottom - chatTop;
  
  // Chat area background
  SkPaint chatBgPaint;
  chatBgPaint.setColor(BG_DARK);
  
  SkRect chatRect = SkRect::MakeXYWH(PADDING, chatTop, w - PADDING * 2, chatHeight);
  SkRRect chatRRect = SkRRect::MakeRectXY(chatRect, CORNER_RADIUS, CORNER_RADIUS);
  canvas->drawRRect(chatRRect, chatBgPaint);
  
  // Clip to chat area
  canvas->save();
  canvas->clipRRect(chatRRect);
  
  // Draw messages
  float yPos = chatTop + PADDING;
  SkFont messageFont;
  messageFont.setSize(12.0f);
  
  for (const auto& msg : messages_) {
    float msgWidth = w - PADDING * 6;
    float msgHeight = 60.0f; // Simplified - would calculate based on text
    
    // Message bubble
    SkRect msgRect = SkRect::MakeXYWH(
      msg.isUser ? w - PADDING * 2 - msgWidth - PADDING : PADDING * 2,
      yPos,
      msgWidth,
      msgHeight
    );
    
    SkPaint msgPaint;
    msgPaint.setAntiAlias(true);
    msgPaint.setColor(msg.isUser ? BG_MESSAGE_USER : BG_MESSAGE_AI);
    
    SkRRect msgRRect = SkRRect::MakeRectXY(msgRect, 8, 8);
    canvas->drawRRect(msgRRect, msgPaint);
    
    // Message text
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(TEXT_PRIMARY);
    
    // Wrap text (simplified - just truncate for now)
    juce::String displayText = msg.text;
    if (displayText.length() > 80) {
      displayText = displayText.substring(0, 77) + "...";
    }
    
    canvas->drawString(displayText.toRawUTF8(), 
                       msgRect.left() + 10, 
                       msgRect.top() + 20, 
                       messageFont, textPaint);
    
    // Sender label
    SkFont labelFont;
    labelFont.setSize(10.0f);
    
    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(msg.isUser ? ACCENT : TEXT_MUTED);
    
    canvas->drawString(msg.isUser ? "You" : "Wingman", 
                       msgRect.left() + 10, 
                       msgRect.bottom() - 10, 
                       labelFont, labelPaint);
    
    yPos += msgHeight + MESSAGE_GAP;
  }
  
  canvas->restore();
  
  // Empty state
  if (messages_.empty()) {
    SkFont emptyFont;
    emptyFont.setSize(13.0f);
    
    SkPaint emptyPaint;
    emptyPaint.setAntiAlias(true);
    emptyPaint.setColor(TEXT_MUTED);
    
    canvas->drawString("Ask Wingman anything about", 
                       chatRect.centerX() - 80, chatRect.centerY() - 10, 
                       emptyFont, emptyPaint);
    canvas->drawString("music production...", 
                       chatRect.centerX() - 55, chatRect.centerY() + 10, 
                       emptyFont, emptyPaint);
  }
}

void RightSidePanel::drawInputArea(SkCanvas *canvas) {
  float w = static_cast<float>(getWidth());
  float h = static_cast<float>(getHeight());
  float inputY = h - INPUT_HEIGHT - PADDING;
  
  // Input field background
  SkRect inputRect = SkRect::MakeXYWH(PADDING, inputY, w - PADDING * 2 - 50, INPUT_HEIGHT - 8);
  
  SkPaint inputPaint;
  inputPaint.setAntiAlias(true);
  inputPaint.setColor(hoveredZone_ == HitZone::InputField ? 0xFF1E1E1E : BG_INPUT);
  
  SkRRect inputRRect = SkRRect::MakeRectXY(inputRect, CORNER_RADIUS, CORNER_RADIUS);
  canvas->drawRRect(inputRRect, inputPaint);
  
  // Border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(hoveredZone_ == HitZone::InputField ? ACCENT_DIM : BORDER);
  canvas->drawRRect(inputRRect, borderPaint);
  
  // Placeholder text
  SkFont inputFont;
  inputFont.setSize(12.0f);
  
  SkPaint placeholderPaint;
  placeholderPaint.setAntiAlias(true);
  placeholderPaint.setColor(TEXT_MUTED);
  
  if (inputText_.isEmpty()) {
    canvas->drawString("Ask Wingman...", inputRect.left() + 12, inputRect.centerY() + 4, 
                       inputFont, placeholderPaint);
  }
  
  // Send button
  float btnX = w - PADDING - 40;
  float btnY = inputY + 4;
  float btnSize = INPUT_HEIGHT - 16;
  
  SkRect btnRect = SkRect::MakeXYWH(btnX, btnY, btnSize, btnSize);
  
  SkPaint btnPaint;
  btnPaint.setAntiAlias(true);
  btnPaint.setColor(hoveredZone_ == HitZone::SendButton ? ACCENT : ACCENT_DIM);
  
  SkRRect btnRRect = SkRRect::MakeRectXY(btnRect, CORNER_RADIUS, CORNER_RADIUS);
  canvas->drawRRect(btnRRect, btnPaint);
  
  // Send arrow icon
  SkPaint arrowPaint;
  arrowPaint.setAntiAlias(true);
  arrowPaint.setColor(0xFFFFFFFF);
  arrowPaint.setStyle(SkPaint::kStroke_Style);
  arrowPaint.setStrokeWidth(2.0f);
  arrowPaint.setStrokeCap(SkPaint::kRound_Cap);
  
  float cx = btnRect.centerX();
  float cy = btnRect.centerY();
  
  SkPath arrow;
  arrow.moveTo(cx - 6, cy);
  arrow.lineTo(cx + 6, cy);
  arrow.moveTo(cx + 2, cy - 5);
  arrow.lineTo(cx + 6, cy);
  arrow.lineTo(cx + 2, cy + 5);
  
  canvas->drawPath(arrow, arrowPaint);
}
#endif

} // namespace zenith
