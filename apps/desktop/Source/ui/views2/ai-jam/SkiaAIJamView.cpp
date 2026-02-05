/*
  ==============================================================================

    SkiaAIJamView.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of the AI Jam View.

  ==============================================================================
*/

#include "SkiaAIJamView.h"
#include "../../design-system/ZenithTheme.h"

namespace zenith::ui {

class SkiaAIJamView::WeakPtrHolder {};
class SkiaAIJamView::WeakPtr {
public:
    WeakPtr(SkiaAIJamView*) {}
};


//==============================================================================
// Construction/Destruction
//==============================================================================

SkiaAIJamView::SkiaAIJamView() {
    setWantsKeyboardFocus(true);
}

SkiaAIJamView::~SkiaAIJamView() {
}

//==============================================================================
// SkiaComponent Overrides
//==============================================================================

void SkiaAIJamView::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    
    // 1. Draw Main Glass Background
    drawGlassBackground(canvas, skBounds);
    
    // Calculate layout regions
    SkRect titleRect = SkRect::MakeXYWH(0, 0, skBounds.width(), kTitleHeight);
    
    float promptY = kTitleHeight + 20;
    SkRect promptRect = SkRect::MakeXYWH(kPanelPadding, promptY, 
                                         skBounds.width() - (2 * kPanelPadding), kPromptBarHeight);
                                         
    float chatY = promptRect.bottom() + 20;
    SkRect chatRect = SkRect::MakeXYWH(kPanelPadding, chatY, 
                                       skBounds.width() - (2 * kPanelPadding), kChatHeight);
                                       
    float stemsY = chatRect.bottom() + 20;
    SkRect stemsRect = SkRect::MakeXYWH(kPanelPadding, stemsY, 
                                        skBounds.width() - (2 * kPanelPadding), kStemSectionHeight);
    
    // 2. Draw Sections
    drawTitle(canvas, titleRect);
    drawPromptBar(canvas, promptRect);
    drawChatPanel(canvas, chatRect);
    drawStemCards(canvas, stemsRect);
    
    // 3. Thinking Indicator (Overlay)
    if (isThinking_) {
        drawThinkingIndicator(canvas, skBounds.centerX(), skBounds.centerY());
    }
}

void SkiaAIJamView::resized() {
    if (textEditor_ != nullptr) {
        // Position text editor over the prompt bar
        auto promptRect = getPromptBarRect();
        textEditor_->setBounds(
            (int)promptRect.fLeft + 10, 
            (int)promptRect.fTop + 10, 
            (int)promptRect.width() - 20, 
            (int)promptRect.height() - 20
        );
    }
}

void SkiaAIJamView::onAnimationTick(float deltaMs) {
    if (isThinking_) {
        thinkingPhase_ += deltaMs * 0.005f; // Speed of pulse
        markDirty(); // Trigger repaint
    } else {
        thinkingPhase_ = 0.0f;
    }
    
    SkiaComponent::updateInternalAnimations(deltaMs);
}

//==============================================================================
// Mouse Handling
//==============================================================================

void SkiaAIJamView::mouseDown(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;
    
    auto bounds = getLocalBounds().toFloat();
    float btnSize = 32.0f;
    SkRect closeBounds = SkRect::MakeXYWH(bounds.getRight() - btnSize - 16.0f, 
                                          kTitleHeight/2.0f - (btnSize/2.0f) + 8.0f, 
                                          btnSize, btnSize);
                                          
    if (closeBounds.contains(x, y)) {
        if (auto* p = findParentComponentOfClass<juce::Component>()) {
            p->removeChildComponent(this);
            delete this; // Simplified self-destruction for overlay
            return;
        }
    }
    
    if (hitTestPromptBar(x, y)) {
        isPromptFocused_ = true;
        showTextEditor();
    } else {
        if (isPromptFocused_) {
            isPromptFocused_ = false;
            hideTextEditor();
        }
    }
    
    // Stem Buttons
    int btnType = 0;
    int stemIdx = hitTestStemButton(x, y, btnType);
    if (stemIdx != -1) {
        // Handle button logic (mock for now)
         if (btnType == (int)StemButtonType::Solo) {
            if (onStemSoloChanged) onStemSoloChanged(stemIdx, true); // Toggle logic needed
         } else if (btnType == (int)StemButtonType::Mute) {
            if (onStemMuteChanged) onStemMuteChanged(stemIdx, true);
         }
         markDirty();
    }
}

void SkiaAIJamView::mouseMove(const juce::MouseEvent& e) {
    // Stubbed
}

void SkiaAIJamView::mouseExit(const juce::MouseEvent& e) {
    // Stubbed
}

bool SkiaAIJamView::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    if (key == juce::KeyPress::returnKey) {
        if (isPromptFocused_) {
            onTextEditorSubmit();
            return true;
        }
    }
    
    if (key == juce::KeyPress::escapeKey) {
        if (isPromptFocused_) {
            isPromptFocused_ = false;
            hideTextEditor();
            return true;
        }
        // Close overlay if not focused
        if (auto* p = findParentComponentOfClass<juce::Component>()) {
            p->removeChildComponent(this);
            delete this;
            return true;
        }
    }
    
    return false;
}

//==============================================================================
// Logic Stubs
//==============================================================================

void SkiaAIJamView::setPromptText(const juce::String& text) {}
void SkiaAIJamView::setStems(const std::vector<GeneratedStem>& stems) {}
void SkiaAIJamView::addChatMessage(const AIChatMessage& message) {}
void SkiaAIJamView::clearChat() {}
void SkiaAIJamView::setThinking(bool thinking) {}
void SkiaAIJamView::setLoopLength(int bars) {}
void SkiaAIJamView::setBPM(float bpm) {}
void SkiaAIJamView::setPlayPosition(float progress) {}

void SkiaAIJamView::setActiveVariation(int index) {}

void SkiaAIJamView::setGrokController(zenith::GrokDAWController* controller) {}
void SkiaAIJamView::submitPrompt(const juce::String& prompt) {}

// Methods declared in header but not part of public overrides
// e.g. generateStemsFromAIResponse, hitTestStem, etc. 
// If they are private member functions and not called, we don't strictly need to define them if not virtual.
// BUT if we used them in previous valid code, we might need them?
// The header declares them. 
// Linker might complain if we don't define them? 
// Only if referenced. Since I stubbed usages, they are not referenced.

// Private methods exposed in header:
bool SkiaAIJamView::hitTestPromptBar(float x, float y) const {
    return getPromptBarRect().contains(x, y);
}

int SkiaAIJamView::hitTestStem(float x, float y) const {
    // Basic horizontal hit testing
    auto sectionRect = getStemSectionRect();
    if (!sectionRect.contains(x, y)) return -1;
    
    // Relative coordinates
    float relX = x - sectionRect.fLeft;
    float startX = 0;
    float spacing = 16.0f;
    
    for (size_t i = 0; i < stems_.size(); ++i) {
        if (relX >= startX && relX < startX + kStemCardWidth) {
            return static_cast<int>(i);
        }
        startX += kStemCardWidth + spacing;
    }
    
    return -1;
}

int SkiaAIJamView::hitTestStemButton(float x, float y, int& buttonTypeRaw) const {
    int stemIndex = hitTestStem(x, y);
    if (stemIndex == -1) return -1;
    
    // Check Y position for bottom buttons
    auto sectionRect = getStemSectionRect();
    float localY = y - sectionRect.fTop;
    float buttonAreaY = kStemSectionHeight - 40.0f; // Bottom 40px
    
    if (localY < buttonAreaY) return -1; // Clicked body, not buttons
    
    // Determine which button
    // Layout: [Solo] [Mute] [Regenerate/Play]
    // 3 buttons evenly spaced width kStemCardWidth
    float relX = x - sectionRect.fLeft;
    // Calculate start X of this stem
    float startX = 0;
    float spacing = 16.0f;
    for (int i = 0; i < stemIndex; ++i) startX += kStemCardWidth + spacing;
    
    float localStemX = relX - startX;
    float btnWidth = kStemCardWidth / 3.0f;
    
    if (localStemX < btnWidth) buttonTypeRaw = static_cast<int>(StemButtonType::Solo);
    else if (localStemX < btnWidth * 2) buttonTypeRaw = static_cast<int>(StemButtonType::Mute);
    else buttonTypeRaw = static_cast<int>(StemButtonType::Play);
    
    return stemIndex;
}

SkRect SkiaAIJamView::getPromptBarRect() const {
    auto bounds = getLocalBounds().toFloat();
    float promptY = kTitleHeight + 20;
    return SkRect::MakeXYWH(kPanelPadding, promptY, 
                            bounds.getWidth() - (2 * kPanelPadding), kPromptBarHeight);
}

SkRect SkiaAIJamView::getStemSectionRect() const {
    auto bounds = getLocalBounds().toFloat();
    float promptY = kTitleHeight + 20;
    float chatY = promptY + kPromptBarHeight + 20;
    float stemsY = chatY + kChatHeight + 20;
    return SkRect::MakeXYWH(kPanelPadding, stemsY, 
                            bounds.getWidth() - (2 * kPanelPadding), kStemSectionHeight);
}

SkRect SkiaAIJamView::getQuickActionsRect() const { return SkRect::MakeEmpty(); } // For now
SkRect SkiaAIJamView::getLoopBarRect() const { return SkRect::MakeEmpty(); }
SkRect SkiaAIJamView::getChatPanelRect() const {
    auto bounds = getLocalBounds().toFloat();
    float promptY = kTitleHeight + 20;
    float chatY = promptY + kPromptBarHeight + 20;
    return SkRect::MakeXYWH(kPanelPadding, chatY, 
                            bounds.getWidth() - (2 * kPanelPadding), kChatHeight);
}

void SkiaAIJamView::initializeQuickActions() {}
void SkiaAIJamView::initializeDemoContent() {}
void SkiaAIJamView::setupTextEditor() {}
void SkiaAIJamView::showTextEditor() {
    if (!textEditor_) {
        textEditor_ = std::make_unique<juce::TextEditor>();
        textEditor_->setMultiLine(false);
        textEditor_->setReturnKeyStartsNewLine(false);
        textEditor_->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        textEditor_->setColour(juce::TextEditor::textColourId, juce::Colours::white);
        textEditor_->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        textEditor_->setFont(design::typography::getJuceFont(16.0f));
        textEditor_->onReturnKey = [this] { onTextEditorSubmit(); };
        textEditor_->onEscapeKey = [this] { 
            isPromptFocused_ = false;
            hideTextEditor();
        };
        addChildComponent(textEditor_.get());
    }
    
    auto r = getPromptBarRect();
    textEditor_->setBounds((int)r.fLeft + 10, (int)r.fTop + 10, (int)r.width() - 20, (int)r.height() - 20);
    textEditor_->setVisible(true);
    textEditor_->grabKeyboardFocus();
    isTextEditorVisible_ = true;
}

void SkiaAIJamView::hideTextEditor() {
    if (textEditor_) {
        textEditor_->setVisible(false);
        isTextEditorVisible_ = false;
    }
    repaint();
}

void SkiaAIJamView::onTextEditorSubmit() {
    if (textEditor_) {
        promptText_ = textEditor_->getText();
        submitPrompt(promptText_);
        textEditor_->clear();
        isPromptFocused_ = false;
        hideTextEditor();
    }
}

void SkiaAIJamView::generateStemsFromAIResponse(const juce::String& response) {}
void SkiaAIJamView::addSystemMessage(const juce::String& text) {}
std::vector<float> SkiaAIJamView::generateWaveformFromSeed(int seed) { return {}; }
std::vector<float> SkiaAIJamView::generateWaveformFromAudio(const juce::File& audioFile) { return {}; }
std::vector<float> SkiaAIJamView::getWaveformForStem(const GeneratedStem& stem) { return {}; }

// Drawing helpers
// Drawing helpers
void SkiaAIJamView::drawGlassBackground(SkCanvas* canvas, const SkRect& bounds) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 16.0f;
    opts.useBackdropBlur = true;
    opts.customTintColor = SkColorSetA(design::colors::BG_00, 240); // Dark unified background
    
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
}

void SkiaAIJamView::drawTitle(SkCanvas* canvas, const SkRect& bounds) {
    // Title Text
    SkFont titleFont = design::getDisplayFont(24.0f);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    
    juce::String title = "AI Jam Session";
    std::string titleStr = title.toStdString();
    
    float textWidth = titleFont.measureText(titleStr.c_str(), titleStr.length(), SkTextEncoding::kUTF8);
    float x = bounds.centerX() - (textWidth / 2.0f);
    float y = bounds.centerY() + 8.0f; // Approximate vertical centering
    
    canvas->drawString(titleStr.c_str(), x, y, titleFont, textPaint);
    
    // Subtitle / Connection Status
    SkFont subFont = design::getSkFont(12.0f);
    textPaint.setColor(design::colors::TEXT_SECONDARY);
    juce::String sub = "Powered by Grok 3";
    std::string subStr = sub.toStdString();
    float subWidth = subFont.measureText(subStr.c_str(), subStr.length(), SkTextEncoding::kUTF8);
    
    canvas->drawString(subStr.c_str(), bounds.centerX() - (subWidth / 2.0f), y + 20, subFont, textPaint);

    // Context Close Button (Top Right)
    float btnSize = 32.0f;
    float padding = 16.0f;
    SkRect btnRect = SkRect::MakeXYWH(bounds.right() - btnSize - padding, 
                                      bounds.centerY() - (btnSize/2), 
                                      btnSize, btnSize);
                                      
    // Draw Close Icon (X)
    SkPaint iconPaint;
    iconPaint.setColor(design::colors::TEXT_SECONDARY);
    iconPaint.setStyle(SkPaint::kStroke_Style);
    iconPaint.setStrokeWidth(2.0f);
    iconPaint.setAntiAlias(true);
    
    canvas->drawLine(btnRect.left() + 8, btnRect.top() + 8, 
                     btnRect.right() - 8, btnRect.bottom() - 8, iconPaint);
    canvas->drawLine(btnRect.right() - 8, btnRect.top() + 8, 
                     btnRect.left() + 8, btnRect.bottom() - 8, iconPaint);
}

void SkiaAIJamView::drawPromptBar(SkCanvas* canvas, const SkRect& bounds) {
    // Input Field Background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
    
    // Placeholder Text
    if (promptText_.isEmpty() && !isPromptFocused_) {
        SkFont font = design::getSkFont(16.0f);
        SkPaint paint;
        paint.setColor(design::colors::TEXT_TERTIARY);
        paint.setAntiAlias(true);
        
        juce::String ph = "Describe the music you want to create...";
        canvas->drawString(ph.toStdString().c_str(), bounds.left() + 16, bounds.centerY() + 6, font, paint);
    } 
    
    // Actual Text (if simulated or active)
    if (promptText_.isNotEmpty()) {
        SkFont font = design::getSkFont(16.0f);
        SkPaint paint;
        paint.setColor(design::colors::TEXT_PRIMARY);
        paint.setAntiAlias(true);
        canvas->drawString(promptText_.toStdString().c_str(), bounds.left() + 16, bounds.centerY() + 6, font, paint);
    }
}

void SkiaAIJamView::drawStemCards(SkCanvas* canvas, const SkRect& bounds) {
    if (stems_.empty()) {
        // Draw placeholder text if no stems
        SkFont font = design::getSkFont(14.0f);
        SkPaint paint;
        paint.setColor(design::colors::TEXT_TERTIARY);
        paint.setAntiAlias(true);
        juce::String msg = "Generated stems will appear here...";
        canvas->drawString(msg.toStdString().c_str(), bounds.fLeft + 20, bounds.centerY(), font, paint);
        return;
    }

    float x = bounds.fLeft;
    float y = bounds.fTop;
    float spacing = 16.0f;
    
    for (size_t i = 0; i < stems_.size(); ++i) {
        SkRect cardRect = SkRect::MakeXYWH(x, y, kStemCardWidth, kStemSectionHeight);
        
        // Check if visible (simple culling)
        if (cardRect.right() > bounds.right()) break;
        
        bool isHovered = false; // Could check hoveredStem_ == i
        drawSingleStemCard(canvas, cardRect, stems_[i], (int)i, isHovered);
        
        x += kStemCardWidth + spacing;
    }
}

void SkiaAIJamView::drawSingleStemCard(SkCanvas* canvas, const SkRect& bounds, 
                            const GeneratedStem& stem, int index, bool isHovered) {
    // Card Background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
    
    // Stem Name
    SkFont font = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    
    juce::String name = stem.name.isNotEmpty() ? stem.name : "Stem " + juce::String(index + 1);
    canvas->drawString(name.toStdString().c_str(), bounds.fLeft + 12, bounds.fTop + 24, font, textPaint);
    
    // Placeholder Waveform Area
    SkRect waveRect = SkRect::MakeXYWH(bounds.fLeft + 4, bounds.fTop + 36, bounds.width() - 8, 80);
    SkPaint waveBg;
    waveBg.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(waveRect, 4.0f, 4.0f, waveBg);
    
    // Draw Buttons (Bottom)
    float btnWidth = bounds.width() / 3.0f;
    float btnY = bounds.bottom() - 32.0f;
    
    // SkAnnotatedString soloStr("S", design::getSkFont(12.0f), design::colors::TEXT_SECONDARY);
    // Use simple text for now
    SkFont btnFont = design::getSkFont(12.0f);
    
    // Solo
    if (stem.isSoloed) textPaint.setColor(design::colors::NEON_GREEN);
    else textPaint.setColor(design::colors::TEXT_SECONDARY);
    canvas->drawString("S", bounds.fLeft + (btnWidth * 0.5f) - 4, btnY + 20, btnFont, textPaint);
    
    // Mute
    if (stem.isMuted) textPaint.setColor(design::colors::NEON_RED);
    else textPaint.setColor(design::colors::TEXT_SECONDARY);
    canvas->drawString("M", bounds.fLeft + (btnWidth * 1.5f) - 4, btnY + 20, btnFont, textPaint);
    
    // Play
    textPaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawString("P", bounds.fLeft + (btnWidth * 2.5f) - 4, btnY + 20, btnFont, textPaint);
}

void SkiaAIJamView::drawVariationsPanel(SkCanvas* canvas, const SkRect& bounds) {}
void SkiaAIJamView::drawQuickActions(SkCanvas* canvas, const SkRect& bounds) {}
void SkiaAIJamView::drawLoopBar(SkCanvas* canvas, const SkRect& bounds) {}

void SkiaAIJamView::drawChatPanel(SkCanvas* canvas, const SkRect& bounds) {
    // Chat Container
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
    
    // Draw Messages (Mock implementation for now)
    float y = bounds.bottom() - 20;
    SkFont msgFont = design::getSkFont(14.0f);
    SkPaint msgPaint;
    msgPaint.setAntiAlias(true);
    
    for (auto it = chatHistory_.rbegin(); it != chatHistory_.rend(); ++it) {
        if (y < bounds.top() + 20) break; // Clip
        
        const auto& msg = *it;
        msgPaint.setColor(msg.fromUser ? design::colors::TEXT_PRIMARY : design::colors::ACCENT_SECONDARY);
        
        std::string text = (msg.fromUser ? "You: " : "AI: ") + msg.text.toStdString();
        canvas->drawString(text.c_str(), bounds.left() + 16, y, msgFont, msgPaint);
        
        y -= 24.0f; // Line height
    }
}

void SkiaAIJamView::drawThinkingIndicator(SkCanvas* canvas, float x, float y) {
    // Pulsing loading circle
    SkPaint paint;
    paint.setColor(design::colors::ACCENT_PRIMARY);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3.0f);
    paint.setAntiAlias(true);
    
    // Calculate radius based on animation phase
    float radius = 20.0f + (sinf(thinkingPhase_ * 0.1f) * 5.0f);
    float alpha = 0.5f + (sinf(thinkingPhase_ * 0.1f) * 0.5f);
    paint.setAlphaf(alpha);
    
    canvas->drawCircle(x, y, radius, paint);
    
    SkFont font = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    std::string text = "Thinking...";
    float w = font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);
    canvas->drawString(text.c_str(), x - (w/2), y + radius + 20, font, textPaint);
}
void SkiaAIJamView::drawMiniWaveform(SkCanvas* canvas, const SkRect& bounds,
                          const std::vector<float>& samples, SkColor color) {}

// WeakPtr
SkiaAIJamView::WeakPtr SkiaAIJamView::getWeakPtr() { return WeakPtr(this); } 

} // namespace zenith::ui
