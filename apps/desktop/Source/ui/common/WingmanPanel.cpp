/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Updated for Premium UI)
    Author:  Marcus Williams (UX Team)

    Complete Wingman Panel Implementation
    - Real Glassmorphism using GlassmorphicPanel
    - Smooth 60FPS Animations
    - Natural Language Processing Interface

  ==============================================================================
*/

#include "WingmanPanel.h"
#include "../../ai/SampleHunterAgent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../framework/GlassmorphicPanel.h"
#include "../../engine/ZenithLogger.h"

namespace zenith {

using namespace design;

//==============================================================================
// CONSTANTS
//==============================================================================
namespace layout {
    constexpr float HEADER_HEIGHT = 48.0f;
    constexpr float CONTEXT_HEIGHT = 32.0f;
    constexpr float INPUT_AREA_HEIGHT = 64.0f;
    constexpr float SUGGESTIONS_HEIGHT = 40.0f;
    constexpr float PANEL_WIDTH = 380.0f;
    constexpr float SIDE_PADDING = 16.0f;
    constexpr float MSG_BUBBLE_PADDING = 12.0f;
    constexpr float MSG_SPACING = 16.0f;
}

//==============================================================================
// CONSTRUCTOR
//==============================================================================
WingmanPanel::WingmanPanel(CommandAPI &api, Engine &engine)
    : commandAPI(api), engine_(engine) {
  ZENITH_LOG_UI(zenith::LogLevel::Info, "WingmanPanel: Constructor called");
  
  // Initialize Controllers
  grokController = std::make_unique<GrokDAWController>(commandAPI);

  // Setup Input Field (JUCE Component text editor on top of Skia)
  inputField_ = std::make_unique<juce::TextEditor>("Input");
  inputField_->setMultiLine(false);
  inputField_->setReturnKeyStartsNewLine(false);
  inputField_->setPopupMenuEnabled(true);
  inputField_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
  inputField_->setColour(juce::TextEditor::textColourId, design::toJuceColour(colors::TEXT_PRIMARY));
  inputField_->setColour(juce::TextEditor::highlightColourId, design::toJuceColour(colors::ACCENT_PRIMARY).withAlpha(0.2f));
  inputField_->setColour(juce::TextEditor::focusedOutlineColourId, design::toJuceColour(colors::ACCENT_PRIMARY));
  inputField_->setFont(design::typography::getJuceFont(design::typography::FONT_MD));
  inputField_->setTextToShowWhenEmpty(inputPlaceholder_, design::toJuceColour(colors::TEXT_SECONDARY));
  inputField_->addListener(this);
  addAndMakeVisible(inputField_.get());

  // Initialize Default Suggestions
  updateSuggestions();

  // Initial Welcome Message
  appendToConversation("Wingman", "Hello! I'm ready to help you mix.", false);

  // Start Animation Loop
  startTimerHz(60);
  
  // Init Grok - check for API key
  if (!initializeGrok()) {
      // No API key configured - show warning
      appendToConversation("Wingman", 
          "⚠️ No Grok API key found. Set GROK_API_KEY environment variable or configure in Settings to enable AI features.", 
          false);
  }
}

WingmanPanel::~WingmanPanel() {
    inputField_->removeListener(this);
}

//==============================================================================
// COMPONENT OVERRIDES
//==============================================================================
void WingmanPanel::paint(juce::Graphics &g) {
    // No standard JUCE painting - pure Skia
}

void WingmanPanel::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect panelRect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    
    // 1. Background (Glassmorphic)
    GlassmorphicPanel::draw(canvas, panelRect, GlassmorphicPanel::Style::Elevated);
    
    // Define Areas
    float currentY = 0.0f;
    
    // 2. Header
    canvas->save();
    drawHeader(canvas);
    canvas->restore();
    currentY += layout::HEADER_HEIGHT;
    
    // 3. Context Indicator
    canvas->save();
    canvas->translate(0, currentY);
    drawContextIndicator(canvas);
    canvas->restore();
    currentY += layout::CONTEXT_HEIGHT;
    
    // 4. Chat Area (Scrollable)
    float footerHeight = layout::INPUT_AREA_HEIGHT;
    if (!suggestions_.empty()) footerHeight += layout::SUGGESTIONS_HEIGHT;
    
    float chatHeight = bounds.getHeight() - currentY - footerHeight;
    SkRect chatRect = SkRect::MakeXYWH(0, currentY, bounds.getWidth(), chatHeight);
    
    canvas->save();
    canvas->clipRect(chatRect);
    canvas->translate(0, currentY - scrollY_.get()); // Apply Scroll
    drawChatArea(canvas);
    canvas->restore();
    
    // 5. Suggestions & Input (Bottom anchored)
    float bottomStart = bounds.getHeight() - footerHeight;
    
    canvas->save();
    canvas->translate(0, bottomStart);
    
    if (!suggestions_.empty()) {
        drawSuggestions(canvas);
        canvas->translate(0, layout::SUGGESTIONS_HEIGHT);
    }
    
    drawInputArea(canvas);
    canvas->restore();
}

void WingmanPanel::resized() {
    auto bounds = getLocalBounds();
    
    // Layout constants must match drawInputArea EXACTLY
    constexpr float inputH = 36.0f;
    constexpr float margin = 12.0f;
    constexpr float btnSize = 32.0f;
    constexpr float spacing = 8.0f;
    
    // Input field width: total - margins - 2 buttons - spacing
    float inputW = bounds.getWidth() - (margin * 2) - btnSize - spacing - btnSize - spacing;
    float inputAreaHeight = layout::INPUT_AREA_HEIGHT;
    
    // The Skia canvas is translated to (height - INPUT_AREA_HEIGHT) before drawing
    // So the input box's absolute Y is:
    float canvasY = bounds.getHeight() - inputAreaHeight;
    float localInputY = (inputAreaHeight - inputH) / 2.0f;
    float absoluteInputY = canvasY + localInputY;
    
    // Position the JUCE TextEditor to overlay the Skia-drawn input box
    // Add small padding inside the visual box for text
    inputField_->setBounds(
        static_cast<int>(margin + 6),  // Horizontal padding inside box
        static_cast<int>(absoluteInputY + 6), // Vertical padding 
        static_cast<int>(inputW - 12),  // Width minus padding
        static_cast<int>(inputH - 12)   // Height minus padding
    );
    
    layoutMessages();
}


void WingmanPanel::timerCallback() {
    float dt = 1000.0f / 60.0f;
    bool animate = false;
    
    // Update Animations
    if (scrollY_.update(dt)) {
        animate = true;
        layoutMessages(); // Keep child components in sync with animated scroll
    }
    if (panelWidth_.update(dt)) animate = true;
    if (typingIndicatorOpacity_.update(dt)) animate = true;
    
    // Process pulsing dot
    processingDotPhase_ += 0.1f;
    if (processingDotPhase_ > juce::MathConstants<float>::twoPi) {
        processingDotPhase_ -= juce::MathConstants<float>::twoPi;
    }
    if (isProcessing_) animate = true;
    
    
    // Suggestion hover animations
    for (auto& sug : suggestions_) {
        if (sug.hoverProgress.update(dt)) animate = true;
    }
    
    if (animate) repaint();
}

void WingmanPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& d) {
    targetScrollY_ = std::clamp(targetScrollY_ - d.deltaY * 100.0f, 0.0f, maxScrollY_);
    scrollY_.setTarget(targetScrollY_, 200, zenith::animation::Easing::EaseOutQuart);
    repaint();
}

//==============================================================================
// RENDERING HELPERS
//==============================================================================

void WingmanPanel::drawHeader(SkCanvas* canvas) {
    // Title
    SkFont font = design::typography::getDisplayFont(18);
    SkPaint paint;
    paint.setColor(colors::TEXT_PRIMARY);
    paint.setAntiAlias(true);
    
    canvas->drawString("Wingman", layout::SIDE_PADDING, 30, font, paint);
    
    // Cyan Underline
    SkPaint linePaint;
    linePaint.setColor(colors::CYAN);
    linePaint.setStrokeWidth(2.0f);
    canvas->drawLine(layout::SIDE_PADDING, 36, layout::SIDE_PADDING + 80, 36, linePaint);
    
    // Minimize Button (Chevron)
    minimizeBtnBounds_ = {getWidth() - 40.0f, 8.0f, 32.0f, 32.0f};
    SkPath chevron;
    float mx = minimizeBtnBounds_.getX() + 10;
    float my = minimizeBtnBounds_.getY() + 8;
    
    if (isMinimized_) {
        // Point Left
         chevron.moveTo(mx + 8, my);
         chevron.lineTo(mx, my + 8);
         chevron.lineTo(mx + 8, my + 16);
    } else {
        // Point Right
         chevron.moveTo(mx + 4, my);
         chevron.lineTo(mx + 12, my + 8);
         chevron.lineTo(mx + 4, my + 16);
    }
    
    SkPaint btnPaint;
    btnPaint.setColor(isMinimizeHovered_ ? colors::TEXT_PRIMARY : colors::TEXT_SECONDARY);
    btnPaint.setStyle(SkPaint::kStroke_Style);
    btnPaint.setStrokeWidth(2.0f);
    btnPaint.setStrokeCap(SkPaint::kRound_Cap);
    btnPaint.setAntiAlias(true);
    
    canvas->drawPath(chevron, btnPaint);
}

void WingmanPanel::drawContextIndicator(SkCanvas* canvas) {
    float cy = 20.0f;
    float padding = layout::SIDE_PADDING;
    
    // Pulsing Dot (Status)
    SkPaint dotPaint;
    float alpha = isProcessing_ ? (0.5f + 0.5f * std::sin(processingDotPhase_)) : 1.0f;
    dotPaint.setColor(design::withAlpha(colors::CYAN, alpha));
    dotPaint.setAntiAlias(true);
    
    canvas->drawCircle(padding + 4, cy - 4, 4, dotPaint);
    
    if (isProcessing_) {
        // Glow ring
        SkPaint glowPaint;
        glowPaint.setColor(design::withAlpha(colors::CYAN, 0.3f));
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(2.0f);
        glowPaint.setAntiAlias(true);
        canvas->drawCircle(padding + 4, cy - 4, 8 + std::sin(processingDotPhase_)*2, glowPaint);
    }
    
    // Context Text
    SkFont font = design::typography::getSkFont(design::typography::FONT_XS);
    SkPaint textPaint;
    textPaint.setColor(colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);
    
    juce::String text = isProcessing_ ? "Thinking..." : ("Context: " + currentContext_);
    canvas->drawString(text.toRawUTF8(), padding + 20, cy, font, textPaint);
}

void WingmanPanel::appendToConversation(const juce::String &speaker, const juce::String &message, bool isUser) {
    // NEW Logic: Create AIChatMessage component
    auto type = isUser ? ChatMessageType::User : ChatMessageType::AI;
    auto msgComponent = std::make_unique<AIChatMessage>(message, type);
    
    // Set timestamp and copy callback
    msgComponent->setThinking(false);
    msgComponent->onCopy = [](const juce::String& text) {
       juce::SystemClipboard::copyTextToClipboard(text);
    };
    
    addAndMakeVisible(msgComponent.get());
    messages_.push_back(std::move(msgComponent));
    
    // Limit history
    if (messages_.size() > 50) {
        messages_.pop_front();
    }
    
    layoutMessages();
    
    // Auto-scroll to bottom
    maxScrollY_ = std::max(0.0f, contentHeight_ - (getHeight() - layout::HEADER_HEIGHT - layout::INPUT_AREA_HEIGHT - 100));
    targetScrollY_ = maxScrollY_;
    scrollY_.setTarget(targetScrollY_, 400, zenith::animation::Easing::EaseOutQuart);

    repaint();
}

void WingmanPanel::layoutMessages() {
    float y = 0.0f;
    float width = getWidth() - layout::SIDE_PADDING * 2;
    float startX = layout::SIDE_PADDING;
    float scrollV = scrollY_.get();
    
    // Header + Context area is where chat starts
    float chatYStart = layout::HEADER_HEIGHT + layout::CONTEXT_HEIGHT;
    
    for (auto& msg : messages_) {
        float h = msg->calculatePreferredHeight(width);
        
        // Position relative to scroll
        msg->setBounds(startX, chatYStart + (y - scrollV), width, h);
        
        y += h + layout::MSG_SPACING;
        
        // Optimize: Hide if completely out of view
        float componentBottom = chatYStart + (y - scrollV);
        float componentTop = componentBottom - h - layout::MSG_SPACING;
        msg->setVisible(componentBottom > chatYStart && componentTop < getHeight() - layout::INPUT_AREA_HEIGHT);
    }
    
    contentHeight_ = y;
    maxScrollY_ = std::max(0.0f, contentHeight_ - (getHeight() - chatYStart - layout::INPUT_AREA_HEIGHT - 60));
}

void WingmanPanel::drawChatArea(SkCanvas* canvas) {
    // Child components (AIChatMessages) draw themselves.
    // We just handle the scroll container background/clip if needed.
    // Since we are adding them as child components, they paint on top of this component's paint().
    // However, this drawChatArea is called from drawSkia.
    // If we want components to scroll, we need to move them or use a Viewport.
    // Currently WingmanPanel seems to manually offset Y in drawMessage?
    // "SkRect bubbleRect = SkRect::MakeXYWH(x, y + yOff, ..."
    
    // If we switch to Components, we must implement scrolling by `setBounds` translation.
    // layoutMessages() does static layout. `resized()` or `scroll changed` should offset them.
    // Or we use a juce::Viewport?
    // WingmanPanel seems to be a single SkiaComponent.
    // Moving 50 child components every frame for scroll might be heavy for JUCE, but fine for 50 items.
    
    // For now, let's rely on layoutMessages being called when scroll changes?
    // Or add a `updateMessagePositions()` method called on scroll/timer.
}

// drawMessage removed

void WingmanPanel::drawSuggestions(SkCanvas* canvas) {
    float startX = layout::SIDE_PADDING;
    float y = 5.0f; // Padding top within Suggestion Area
    
    for (auto& sug : suggestions_) {
        SkFont font = design::typography::getSkFont(design::typography::FONT_SM);
        float width = font.measureText(sug.text.toRawUTF8(), sug.text.getNumBytesAsUTF8(), SkTextEncoding::kUTF8) + 24;
        
        SkRect rect = SkRect::MakeXYWH(startX, y, width, 24);
        sug.bounds = juce::Rectangle<float>(rect.left(), rect.top(), rect.width(), rect.height()); // Update hit bounds relative to suggestion area
        sug.bounds.translate(0, getHeight() - layout::INPUT_AREA_HEIGHT - layout::SUGGESTIONS_HEIGHT); // Screen space
        
        float hover = sug.hoverProgress.get();
        
        // Scale transform on hover
        canvas->save();
        if (hover > 0.01f) {
            canvas->translate(rect.centerX(), rect.centerY());
            canvas->scale(1.0f + (0.05f * hover), 1.0f + (0.05f * hover));
            canvas->translate(-rect.centerX(), -rect.centerY());
        }
        
        // Pill Background
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        SkColor baseColor = design::colors::BG_03;
        SkColor hoverColor = design::colors::BG_04;
        bgPaint.setColor(design::interpolateColor(baseColor, hoverColor, hover));
        
        canvas->drawRRect(SkRRect::MakeRectXY(rect, 4, 4), bgPaint);
        
        // Border/Glow
        SkPaint borderPaint;
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setAntiAlias(true);
        borderPaint.setColor(design::interpolateColor(design::colors::BORDER_SUBTLE, design::colors::BORDER_FOCUS, hover));
        borderPaint.setStrokeWidth(1.0f);
        
        canvas->drawRRect(SkRRect::MakeRectXY(rect, 4, 4), borderPaint);
        
        // Text
        SkPaint textPaint;
        textPaint.setColor(colors::TEXT_SECONDARY);
        if (hover > 0.5f) textPaint.setColor(colors::TEXT_PRIMARY);
        textPaint.setAntiAlias(true);
        
        canvas->drawString(sug.text.toRawUTF8(), startX + 12, y + 17, font, textPaint);
        
        canvas->restore();
        
        startX += width + 8;
    }
}

void WingmanPanel::drawInputArea(SkCanvas* canvas) {
    SkRect area = SkRect::MakeXYWH(0, 0, getWidth(), layout::INPUT_AREA_HEIGHT);
    
    // Glass Background for Input Area - subtle
    SkPaint bgPaint;
    bgPaint.setColor(design::withAlpha(design::colors::BG_01, 0.9f));
    canvas->drawRect(area, bgPaint);
    
    // Subtle top border line
    SkPaint borderLine;
    borderLine.setColor(design::withAlpha(design::colors::BORDER_SUBTLE, 0.5f));
    borderLine.setStrokeWidth(1.0f);
    canvas->drawLine(0, 0, getWidth(), 0, borderLine);
    
    // Layout constants
    constexpr float inputH = 36.0f;
    constexpr float margin = 12.0f;
    constexpr float btnSize = 32.0f;
    constexpr float spacing = 8.0f;
    constexpr float cornerRadius = 4.0f; // Square-ish corners
    
    // Calculate widths: [input][brain btn][send btn]
    float inputW = getWidth() - (margin * 2) - btnSize - spacing - btnSize - spacing;
    float inputY = (area.height() - inputH) / 2;
    
    SkRect inputRect = SkRect::MakeXYWH(margin, inputY, inputW, inputH);
    
    // Input Box Background
    SkPaint inputBgPaint;
    inputBgPaint.setColor(design::colors::BG_02);
    inputBgPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(inputRect, cornerRadius, cornerRadius), inputBgPaint);
    
    // Subtle border for input (not flashy)
    SkPaint inputBorderPaint;
    inputBorderPaint.setStyle(SkPaint::kStroke_Style);
    inputBorderPaint.setColor(design::colors::BORDER_SUBTLE);
    inputBorderPaint.setStrokeWidth(1.0f);
    inputBorderPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(inputRect, cornerRadius, cornerRadius), inputBorderPaint);
    
    // Focus highlight (subtle cyan, not overwhelming)
    if (inputField_->hasKeyboardFocus(true)) {
        SkPaint focusPaint;
        focusPaint.setStyle(SkPaint::kStroke_Style);
        focusPaint.setColor(design::withAlpha(colors::ACCENT_PRIMARY, 0.6f));
        focusPaint.setStrokeWidth(1.5f);
        focusPaint.setAntiAlias(true);
        canvas->drawRRect(SkRRect::MakeRectXY(inputRect, cornerRadius, cornerRadius), focusPaint);
    }
    
    // Brain Button (left of send)
    float bx = inputRect.right() + spacing;
    float by = inputY + (inputH - btnSize) / 2;
    brainBtnBounds_ = juce::Rectangle<float>(bx, by + (getHeight() - layout::INPUT_AREA_HEIGHT), btnSize, btnSize);
    
    SkRect bRect = SkRect::MakeXYWH(bx, by, btnSize, btnSize);
    SkPaint bPaint;
    
    // Brain button color: pink when active, subtle gray otherwise
    if (isBrainActive_) {
        bPaint.setColor(SkColorSetARGB(200, 255, 100, 150)); // Pink
    } else {
        bPaint.setColor(isBrainHovered_ ? design::colors::BG_03 : design::colors::BG_02);
    }
    bPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(bRect, cornerRadius, cornerRadius), bPaint);
    
    // Brain Icon (simple brain shape - two lobes)
    SkPaint brainIconPaint;
    brainIconPaint.setColor(isBrainActive_ ? SK_ColorWHITE : colors::TEXT_SECONDARY);
    brainIconPaint.setStyle(SkPaint::kStroke_Style);
    brainIconPaint.setStrokeWidth(1.5f);
    brainIconPaint.setAntiAlias(true);
    brainIconPaint.setStrokeCap(SkPaint::kRound_Cap);
    
    float bcx = bx + btnSize / 2;
    float bcy = by + btnSize / 2;
    
    // Left lobe
    SkPath brainPath;
    brainPath.moveTo(bcx, bcy - 6);
    brainPath.cubicTo(bcx - 8, bcy - 8, bcx - 10, bcy + 2, bcx - 6, bcy + 6);
    brainPath.cubicTo(bcx - 8, bcy + 10, bcx - 2, bcy + 10, bcx, bcy + 6);
    // Right lobe
    brainPath.cubicTo(bcx + 2, bcy + 10, bcx + 8, bcy + 10, bcx + 6, bcy + 6);
    brainPath.cubicTo(bcx + 10, bcy + 2, bcx + 8, bcy - 8, bcx, bcy - 6);
    
    canvas->drawPath(brainPath, brainIconPaint);
    
    // Center line
    canvas->drawLine(bcx, bcy - 4, bcx, bcy + 4, brainIconPaint);
    
    // Send Button (square with arrow)
    float sx = bx + btnSize + spacing;
    float sy = by;
    sendBtnBounds_ = juce::Rectangle<float>(sx, sy + (getHeight() - layout::INPUT_AREA_HEIGHT), btnSize, btnSize);
    
    SkRect sRect = SkRect::MakeXYWH(sx, sy, btnSize, btnSize);
    SkPaint sPaint;
    sPaint.setColor(colors::CYAN);
    float sendAlpha = isSendHovered_ ? 1.0f : (inputField_->getText().isEmpty() ? 0.4f : 0.8f);
    sPaint.setAlphaf(sendAlpha);
    sPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(sRect, cornerRadius, cornerRadius), sPaint);
    
    // Arrow Icon (right-pointing send arrow)
    SkPath arrow;
    float acx = sx + btnSize / 2;
    float acy = sy + btnSize / 2;
    
    // Simple right arrow: >
    arrow.moveTo(acx - 4, acy - 6);
    arrow.lineTo(acx + 6, acy);
    arrow.lineTo(acx - 4, acy + 6);
    
    SkPaint arrowPaint;
    arrowPaint.setColor(colors::BG_00);
    arrowPaint.setStyle(SkPaint::kStroke_Style);
    arrowPaint.setStrokeWidth(2.5f);
    arrowPaint.setStrokeCap(SkPaint::kRound_Cap);
    arrowPaint.setStrokeJoin(SkPaint::kRound_Join);
    arrowPaint.setAntiAlias(true);
    canvas->drawPath(arrow, arrowPaint);
    
    // Remove old voice button - replaced by brain button
    voiceBtnBounds_ = juce::Rectangle<float>(); // Clear bounds
}

//==============================================================================
// INTERACTION & LOGIC
//==============================================================================

void WingmanPanel::mouseDown(const juce::MouseEvent& e) {
    if (minimizeBtnBounds_.contains(e.position)) {
        toggleMinimize();
        return;
    }
    
    if (sendBtnBounds_.contains(e.position)) {
        sendCommand();
        return;
    }
    
    if (brainBtnBounds_.contains(e.position)) {
        isBrainActive_ = !isBrainActive_;
        repaint();
        return;
    }
    
    // Check suggestions (coordinates are tricky due to nesting)
    juce::Point<float> localPos = e.position;
    for (const auto& sug : suggestions_) {
        // sug.bounds was stored in screen coords during paint
        juce::Rectangle<float> hitRect(sug.bounds.getX(), sug.bounds.getY(), sug.bounds.getWidth(), sug.bounds.getHeight());
        if (hitRect.contains(localPos)) {
           if (onSuggestionClicked) onSuggestionClicked(sug.id);
           inputField_->setText(sug.text);
           return;
        }
    }
}

void WingmanPanel::mouseMove(const juce::MouseEvent& e) {
    bool nextMin = minimizeBtnBounds_.contains(e.position);
    bool nextSend = sendBtnBounds_.contains(e.position);
    bool nextBrain = brainBtnBounds_.contains(e.position);
    
    if (nextMin != isMinimizeHovered_ || nextSend != isSendHovered_ || nextBrain != isBrainHovered_) {
        isMinimizeHovered_ = nextMin;
        isSendHovered_ = nextSend;
        isBrainHovered_ = nextBrain;
        repaint();
    }
    
    // Suggestions hover
    juce::Point<float> localPos = e.position;
    for (auto& sug : suggestions_) {
        juce::Rectangle<float> hitRect(sug.bounds.getX(), sug.bounds.getY(), sug.bounds.getWidth(), sug.bounds.getHeight());
        bool hovered = hitRect.contains(localPos);
        if (hovered != sug.isHovered) {
             sug.isHovered = hovered;
             // Animate hover
             sug.hoverProgress.setTarget(hovered ? 1.0f : 0.0f, 200, zenith::animation::Easing::EaseOut);
        }
    }
}

void WingmanPanel::mouseUp(const juce::MouseEvent& e) {
    // Standard cleanup
}

void WingmanPanel::mouseExit(const juce::MouseEvent& e) {
    isMinimizeHovered_ = false;
    isSendHovered_ = false;
    isBrainHovered_ = false;
    repaint();
}

void WingmanPanel::textEditorReturnKeyPressed(juce::TextEditor &editor) {
    if (&editor == inputField_.get()) {
        sendCommand();
    }
}

void WingmanPanel::sendCommand() {
    juce::String text = inputField_->getText().trim();
    if (text.isEmpty()) return;
    
    // UI Update
    inputField_->clear();
    appendToConversation("You", text, true);
    
    // State Update
    isProcessing_ = true;
    
    // Send to Controller
    grokController->executeCommand(
        text, 
        GrokMode::Fast,
        [this](juce::String response) {
            juce::MessageManager::callAsync([this, response]() { // Thread safety
                isProcessing_ = false;
                appendToConversation("Wingman", response, false);
            });
        },
        [this](juce::String error) {
            juce::MessageManager::callAsync([this, error]() {
                isProcessing_ = false;
                appendToConversation("Wingman", "Error: " + error, false);
            });
        }
    );
}

// Methods moved/updated above

void WingmanPanel::updateSuggestions() {
    suggestions_.clear();
    
    // Add default suggestions based on current context
    if (currentContext_ == "Arrangement") {
        suggestions_.push_back({"Compress vocals", "cmd_compress"});
        suggestions_.push_back({"Add reverb", "cmd_reverb"});
        suggestions_.push_back({"Fix timing", "cmd_quantize"});
    } else if (currentContext_ == "Mixer") {
        suggestions_.push_back({"Balance tracks", "cmd_balance"});
        suggestions_.push_back({"Solo lead", "cmd_solo_lead"});
    } else {
        suggestions_.push_back({"Explain feature", "cmd_help"});
    }
    
    repaint();
}

//==============================================================================
// HELPER LOGIC
//==============================================================================

bool WingmanPanel::initializeGrok(const juce::String &apiKey) {
    return grokController->initialize(apiKey);
}

bool WingmanPanel::isGrokReady() const {
    return grokController->isReady();
}

void WingmanPanel::toggleMinimize() {
    isMinimized_ = !isMinimized_;
    // Animate parent container or internal state
    // For now just local state for the button icon
    repaint();
}

void WingmanPanel::setContext(const juce::String& contextName) {
    currentContext_ = contextName;
    updateSuggestions();
    repaint();
}

// SampleHunter Callbacks
void WingmanPanel::sampleDownloaded(const ai::FoundSample &sample) { 
    appendToConversation("Wingman", "Downloaded " + sample.localFile.getFileName(), false); 
}

void WingmanPanel::sampleAnalyzed(const ai::FoundSample &sample) {
    // Optional: Notify user or update UI
}

void WingmanPanel::sampleImported(const juce::File &file) { 
    appendToConversation("Wingman", "Imported " + file.getFileName(), false); 
}

void WingmanPanel::huntingProgressChanged(float progress, const juce::String &status) {
    // Optional: Show global progress if hunting
}

void WingmanPanel::huntingComplete(const ai::HuntingStats &stats, bool success) {
    // Optional: Show summary
    if (success) {
        appendToConversation("Wingman", stats.getSummary(), false);
    } else {
        appendToConversation("Wingman", "Sample hunt failed or cancelled.", false);
    }
}

} // namespace zenith
