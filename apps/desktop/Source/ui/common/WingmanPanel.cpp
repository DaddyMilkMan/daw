/*
  ==============================================================================

    WingmanPanel.cpp
    Created: 2025-11-29 (Redesigned: 2026-01-17)
    Author:  Zenith Team

    FLUID INTELLIGENCE INTERFACE - Iteration 2
    Refined based on "Modern Clean" feedback.
    - Removed "engineer" buttons (replaced with frameless icons).
    - Decluttered layout.
    - Softer, more cohesive aesthetic.

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
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>

namespace zenith {

//==============================================================================
// THEME: "Zenith Void" (Refined)
//==============================================================================
namespace theme {
    static constexpr SkColor BG_VOID = SkColorSetRGB(8, 8, 10); // Deepest matte black
    static constexpr SkColor BG_SIDEBAR = SkColorSetRGB(12, 12, 14);
    
    // Softer accent for a more "Product Design" feel (less "Hacker")
    static constexpr SkColor ACCENT_PRIMARY = SkColorSetRGB(110, 110, 255); // Softer Blue
    static constexpr SkColor ACCENT_GLOW = SkColorSetARGB(40, 110, 110, 255);
    
    // Floating Input
    static constexpr SkColor INPUT_BG = SkColorSetRGB(22, 22, 26);
    static constexpr SkColor INPUT_BORDER = SkColorSetARGB(30, 255, 255, 255); // Subtle white stroke
    
    // Text
    static constexpr SkColor TEXT_HEAD = SkColorSetRGB(240, 240, 240);
    static constexpr SkColor TEXT_BODY = SkColorSetRGB(180, 180, 190);
    static constexpr SkColor TEXT_MUTED = SkColorSetRGB(90, 90, 100);
}

//==============================================================================
// Constructor
//==============================================================================

WingmanPanel::WingmanPanel(CommandAPI& api, Engine& engine)
    : commandAPI_(api), engine_(engine) {
    
    grokController_ = std::make_unique<GrokDAWController>(commandAPI_);
    
    // Input Field: Frameless text input (container drawn separately)
    textInput_ = std::make_unique<SkiaTextInput>();
    textInput_->setPlaceholder("Ask Wingman...");
    textInput_->setPillShape(false);
    textInput_->setFontSize(15.0f);
    textInput_->onReturnKey = [this] { sendMessage(); };
    
    // Remove default background from the component itself
    // We handle the visual container in drawInputIsland
    // textInput_->setTransparent(true); // If supported, or just rely on styling
    
    addAndMakeVisible(textInput_.get());

    initializeInterface();
}

WingmanPanel::~WingmanPanel() = default;

void WingmanPanel::initializeInterface() {
    // Clean start
    createNewSession(); 
}

//==============================================================================
// Interaction Logic
//==============================================================================

void WingmanPanel::sendMessage() {
    juce::String text = textInput_->getText().trim();
    if (text.isEmpty()) return;
    
    WingmanMessage msg;
    msg.sender = "User";
    msg.content = text;
    messages_.push_back(msg);
    textInput_->clear();
    
    // Fake "Thinking" state
    isProcessing_ = true;
    updateLayout();
    scrollOffset_ = std::max(0.0f, layout_.chatRect.height()); // Auto-scroll
    repaint();
    
    juce::Timer::callAfterDelay(800, [this] {
        receiveMessage("I can help with that. Analyzing your project structure...");
        isProcessing_ = false;
        repaint();
    });
}

void WingmanPanel::receiveMessage(const juce::String& text) {
    WingmanMessage msg;
    msg.sender = "Wingman";
    msg.content = text;
    messages_.push_back(msg);
    updateLayout();
}

void WingmanPanel::createNewSession() {
    sessions_.clear(); // Just clear for demo visual cleanliness
    WingmanSession s; s.id="1"; s.title="New Session"; s.isActive=true;
    sessions_.push_back(s);
    messages_.clear();
    repaint();
}

//==============================================================================
// Lifecycle & Layout
//==============================================================================

void WingmanPanel::visibilityChanged() { if (isVisible()) updateLayout(); }
void WingmanPanel::resized() { updateLayout(); }

void WingmanPanel::updateLayout() {
    auto b = getLocalBounds().toFloat();
    float w = b.getWidth();
    float h = b.getHeight();
    
    float sidebarW = sidebarOpen_ ? 220.0f : 0.0f;
    
    // Regions
    layout_.sidebarRect = SkRect::MakeXYWH(0, 0, sidebarW, h);
    layout_.contentRect = SkRect::MakeXYWH(sidebarW, 0, w - sidebarW, h);
    
    // Header Buttons (Floating, Top Right)
    // No more "Toolbar", just floating icons
    float btnSize = 32.0f;
    float pad = 20.0f;
    
    layout_.settingsBtn = SkRect::MakeXYWH(w - pad - btnSize, pad, btnSize, btnSize);
    layout_.toggleSidebarBtn = SkRect::MakeXYWH(sidebarW + pad, pad, btnSize, btnSize);
    
    // Input Island (Floating Bottom)
    float inputW = std::min((w - sidebarW) * 0.7f, 600.0f); // Narrower for cleaner look
    float inputH = 50.0f;
    float inputBottomMargin = 40.0f;
    
    float inputX = sidebarW + ((w - sidebarW) - inputW) / 2.0f;
    float inputY = h - inputH - inputBottomMargin;
    
    layout_.inputContainerRect = SkRect::MakeXYWH(inputX, inputY, inputW, inputH);
    
    // Buttons inside input (Brain Left, Send Right)
    layout_.brainBtn = SkRect::MakeXYWH(inputX + 10.0f, inputY + 9.0f, 32.0f, 32.0f);
    layout_.sendBtn = SkRect::MakeXYWH(inputX + inputW - 42.0f, inputY + 9.0f, 32.0f, 32.0f);
    
    // Text Input Rect
    layout_.inputFieldRect = SkRect::MakeXYWH(inputX + 50.0f, inputY + 8.0f, inputW - 100.0f, inputH - 16.0f);
    
    if (textInput_) {
        textInput_->setBounds((int)layout_.inputFieldRect.left(), (int)layout_.inputFieldRect.top(),
                              (int)layout_.inputFieldRect.width(), (int)layout_.inputFieldRect.height());
    }
    
    // Chat Area
    layout_.chatRect = SkRect::MakeLTRB(sidebarW, 80.0f, w, inputY - 20.0f);
}

//==============================================================================
// Mouse
//==============================================================================

void WingmanPanel::mouseMove(const juce::MouseEvent& e) {
    auto p = e.getPosition();
    float x = (float)p.x; float y = (float)p.y;
    
    bool changed = false;
    auto check = [&](bool& s, const SkRect& r) { if (s != r.contains(x,y)) { s = r.contains(x,y); changed=true; } };
    
    check(interaction_.hoverSend, layout_.sendBtn);
    check(interaction_.hoverBrain, layout_.brainBtn);
    check(interaction_.hoverSettings, layout_.settingsBtn);
    check(interaction_.hoverSidebarToggle, layout_.toggleSidebarBtn);
    
    if (changed) repaint();
}

void WingmanPanel::mouseDown(const juce::MouseEvent& e) {
    auto p = e.getPosition();
    float x = (float)p.x; float y = (float)p.y;
    
    if (layout_.toggleSidebarBtn.contains(x,y)) { sidebarOpen_ = !sidebarOpen_; resized(); repaint(); }
    else if (layout_.sendBtn.contains(x,y)) sendMessage();
    else if (layout_.brainBtn.contains(x,y)) { isReasoningMode_ = !isReasoningMode_; repaint(); }
}
void WingmanPanel::mouseUp(const juce::MouseEvent&) {}
void WingmanPanel::mouseExit(const juce::MouseEvent&) { interaction_ = {}; repaint(); }
void WingmanPanel::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) {
    scrollOffset_ -= wheel.deltaY * 40.0f;
    if (scrollOffset_ < 0) scrollOffset_ = 0;
    repaint();
}

//==============================================================================
// RENDERING
//==============================================================================

void WingmanPanel::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;
    
    // 1. Background
    canvas->drawColor(theme::BG_VOID);
    
    // 2. Sidebar
    if (sidebarOpen_) drawSidebar(canvas);
    
    // 3. Chat
    drawChatStream(canvas);
    
    // 4. Input Island
    drawInputIsland(canvas);
    
    // 5. Floating Controls (Header)
    drawHeader(canvas);
}

void WingmanPanel::drawSidebar(SkCanvas* canvas) {
    SkPaint bg;
    bg.setColor(theme::BG_SIDEBAR);
    canvas->drawRect(layout_.sidebarRect, bg);
    
    // Divider
    SkPaint border;
    border.setColor(SkColorSetA(SK_ColorWHITE, 10)); // Ultra subtle
    canvas->drawLine(layout_.sidebarRect.right(), 0, layout_.sidebarRect.right(), layout_.sidebarRect.bottom(), border);
    
    // Title
    SkFont headFont = design::typography::getSkFont(11.0f, design::FontWeight::Bold);
    SkPaint textP; textP.setColor(theme::TEXT_MUTED); textP.setAntiAlias(true);
    canvas->drawString("LIBRARY", 20.0f, 60.0f, headFont, textP);
    
    // Sessions
    float y = 90.0f;
    SkFont itemFont = design::typography::getSkFont(13.0f);
    textP.setColor(theme::TEXT_BODY);
    
    for (const auto& s : sessions_) {
        canvas->drawString(s.title.toStdString().c_str(), 20.0f, y, itemFont, textP);
        y += 32.0f;
    }
}

void WingmanPanel::drawHeader(SkCanvas* canvas) {
    // Menu Icon (Top Left)
    icons::IconStyle style;
    style.color = interaction_.hoverSidebarToggle ? theme::TEXT_HEAD : theme::TEXT_MUTED;
    icons::drawIconCentered(canvas, icons::Menu(), layout_.toggleSidebarBtn, 20.0f, style);
    
    // Settings Icon (Top Right)
    style.color = interaction_.hoverSettings ? theme::TEXT_HEAD : theme::TEXT_MUTED;
    icons::drawIconCentered(canvas, icons::Settings(), layout_.settingsBtn, 20.0f, style);
}

void WingmanPanel::drawInputIsland(SkCanvas* canvas) {
    // "Clean Pill" aesthetic - no heavy borders, no bevels
    
    // 1. Soft Shadow
    SkPaint shadow;
    shadow.setColor(theme::ACCENT_GLOW);
    shadow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 24.0f));
    canvas->drawRoundRect(layout_.inputContainerRect.makeOffset(0, 4), 25.0f, 25.0f, shadow);
    
    // 2. Background
    SkPaint bg;
    bg.setColor(theme::INPUT_BG);
    bg.setAntiAlias(true);
    canvas->drawRoundRect(layout_.inputContainerRect, 25.0f, 25.0f, bg);
    
    // 3. Subtle Stroke
    SkPaint border;
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(theme::INPUT_BORDER);
    border.setAntiAlias(true);
    canvas->drawRoundRect(layout_.inputContainerRect, 25.0f, 25.0f, border);
    
    // 4. Brain Icon (Left)
    icons::IconStyle brainStyle;
    brainStyle.color = isReasoningMode_ ? theme::ACCENT_PRIMARY : theme::TEXT_MUTED;
    if (isReasoningMode_) { brainStyle.glowColor = theme::ACCENT_PRIMARY; brainStyle.glowRadius = 10.0f; }
    icons::drawIconCentered(canvas, icons::Sparkles(), layout_.brainBtn, 18.0f, brainStyle);
    
    // 5. Send Icon (Right) - Only visible if text present or hovered
    bool hasText = !textInput_->getText().trim().isEmpty();
    if (hasText || interaction_.hoverSend) {
        icons::IconStyle sendStyle;
        sendStyle.color = theme::TEXT_HEAD;
        sendStyle.filled = true; // Filled arrow
        icons::drawIconCentered(canvas, icons::SendArrow(), layout_.sendBtn, 18.0f, sendStyle);
    }
}

void WingmanPanel::drawChatStream(SkCanvas* canvas) {
    if (messages_.empty()) {
        drawEmptyState(canvas);
        return;
    }
    
    canvas->save();
    canvas->clipRect(layout_.chatRect);
    
    float y = layout_.chatRect.top() - scrollOffset_;
    float contentW = std::min(layout_.chatRect.width() * 0.7f, 650.0f); // Restrained reading width
    float contentX = layout_.chatRect.left() + (layout_.chatRect.width() - contentW) / 2.0f;
    
    for (const auto& msg : messages_) {
        float h = measureMessageHeight(msg, contentW);
        
        if (y + h > layout_.chatRect.top() && y < layout_.chatRect.bottom()) {
            drawMessageBubble(canvas, msg, y, contentX, contentW);
        }
        y += h + 24.0f;
    }
    
    canvas->restore();
}

void WingmanPanel::drawEmptyState(SkCanvas* canvas) {
    // Minimalist centered logo
    float cx = layout_.chatRect.centerX();
    float cy = layout_.chatRect.centerY() - 40.0f;
    
    SkRect logoR = SkRect::MakeXYWH(cx - 24.0f, cy - 24.0f, 48.0f, 48.0f);
    icons::IconStyle style;
    style.color = SkColorSetA(theme::TEXT_MUTED, 100);
    style.strokeWidth = 1.5f;
    icons::drawIconCentered(canvas, icons::Brain(), logoR, 40.0f, style);
    
    SkFont font = design::typography::getSkFont(14.0f);
    SkPaint paint;
    paint.setColor(theme::TEXT_MUTED);
    paint.setAntiAlias(true);
    
    std::string t = "How can I assist?";
    float w = font.measureText(t.c_str(), t.length(), SkTextEncoding::kUTF8);
    canvas->drawString(t.c_str(), cx - w/2.0f, cy + 50.0f, font, paint);
}

//==============================================================================
// Text Helpers
//==============================================================================

void WingmanPanel::drawMessageBubble(SkCanvas* canvas, const WingmanMessage& msg, float y, float x, float w) {
    if (msg.sender == "User") {
        // User: Right aligned, invisible background, just bold text (Modern messaging style)
        // Or subtle pill
        
        float bubbleW = w; // Full width available to measure against
        SkFont font = design::typography::getSkFont(15.0f);
        SkPaint paint; paint.setColor(theme::TEXT_BODY); paint.setAntiAlias(true);
        
        // Right align logic handled by drawing text at X + indent?
        // Actually, let's keep it simple: User gets a right-aligned pill
        
        // Measure text width roughly
        float textW = font.measureText(msg.content.toUTF8(), msg.content.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
        textW = std::min(textW, w);
        float pillW = textW + 32.0f;
        float pillX = x + w - pillW;
        
        SkRect pill = SkRect::MakeXYWH(pillX, y, pillW, measureMessageHeight(msg, w));
        
        SkPaint bg;
        bg.setColor(SkColorSetRGB(30, 30, 35)); // Very subtle grey
        bg.setAntiAlias(true);
        canvas->drawRoundRect(pill, 16.0f, 16.0f, bg);
        
        drawWrappedText(canvas, msg.content, pillX + 16.0f, y + 26.0f, pillW - 32.0f, font, paint);
        
    } else {
        // Wingman: Left aligned, clean text, no bubble
        // Icon on left
        SkRect iconR = SkRect::MakeXYWH(x, y, 20.0f, 20.0f);
        icons::IconStyle is; is.color = theme::ACCENT_PRIMARY; is.filled=true;
        icons::drawIconCentered(canvas, icons::Sparkles(), iconR, 18.0f, is);
        
        SkFont font = design::typography::getSkFont(15.0f);
        SkPaint paint; paint.setColor(theme::TEXT_HEAD); paint.setAntiAlias(true);
        
        drawWrappedText(canvas, msg.content, x + 36.0f, y + 16.0f, w - 36.0f, font, paint);
    }
}

float WingmanPanel::measureMessageHeight(const WingmanMessage& msg, float width) {
    // Rough calc
    return (msg.content.length() / 50 + 1) * 24.0f + 30.0f;
}

void WingmanPanel::drawWrappedText(SkCanvas* canvas, const juce::String& text, float x, float startY, float maxWidth, const SkFont& font, const SkPaint& paint) {
    // Simple wrap
    float y = startY;
    float lineHeight = 24.0f;
    juce::String rem = text;
    
    while(rem.isNotEmpty()) {
        int len = rem.length();
        int cut = len;
        for(int i=1; i<=len; ++i) {
            if (font.measureText(rem.substring(0,i).toUTF8(), rem.substring(0,i).getNumBytesAsUTF8(), SkTextEncoding::kUTF8) > maxWidth) {
                cut = i-1; break;
            }
        }
        if (cut < 1) cut = 1;
        
        canvas->drawString(rem.substring(0, cut).toStdString().c_str(), x, y, font, paint);
        rem = rem.substring(cut).trimStart();
        y += lineHeight;
    }
}

} // namespace zenith