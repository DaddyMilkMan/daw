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
#include "../design-system/SvgIcon.h"
#include "../controls/ContextMenuManager.h"
#include "../controls/SkiaAlertWindow.h"
#include "../../engine/ZenithLogger.h"
#include "../../Settings.h"
#include "../settings/ModernSettingsPanel.h"
#include <juce_core/juce_core.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace zenith {

namespace {
const std::vector<juce::String> kGreetings = {
    "What are we building today?",
    "Need a fresh pair of ears?",
    "Let's track something great.",
    "Ready to finish that mix?",
    "Where should we start?"
};
}

//==============================================================================
// THEME: "Zenith Void" (Refined & Desaturated)
//==============================================================================
namespace theme {
    // Standardized on Design System BG_01 for total DAW cohesion
    static inline SkColor getBG_VOID() { return design::colors::BG_01; }
    static inline SkColor getBG_SIDEBAR() { return design::colors::BG_00; }
    
    // Desaturated accent for professional color science (reduced halation)
    static constexpr SkColor ACCENT_PRIMARY = SkColorSetRGB(90, 115, 230); // Desaturated Slate Blue
    static constexpr SkColor ACCENT_GLOW = SkColorSetARGB(30, 90, 115, 230);
    
    // Floating Input
    static constexpr SkColor INPUT_BG = SkColorSetRGB(28, 32, 48);
    static constexpr SkColor INPUT_BORDER = SkColorSetARGB(70, 110, 140, 210);
    
    // Text
    static constexpr SkColor TEXT_HEAD = SkColorSetRGB(230, 230, 235);
    static constexpr SkColor TEXT_BODY = SkColorSetRGB(165, 170, 185);
    static constexpr SkColor TEXT_MUTED = SkColorSetRGB(85, 90, 110);
}

namespace wingui {
    static constexpr float kRadiusPanel = 16.0f;
    static constexpr float kRadiusCard = 14.0f;
    static constexpr float kRadiusChip = 10.0f;
    static constexpr float kRadiusPill = 22.0f; // 44px height / 2
    static constexpr float kAnimSidebarSpeed = 6.8f;
    static constexpr float kAnimSettingsSpeed = 7.2f;

    static constexpr SkColor SURFACE_0 = SkColorSetRGB(12, 14, 20);
    static constexpr SkColor SURFACE_1 = SkColorSetRGB(22, 26, 40);
    static constexpr SkColor SURFACE_2 = SkColorSetRGB(28, 34, 52);
    static constexpr SkColor SURFACE_ACTIVE = SkColorSetRGB(42, 58, 98);
    static constexpr SkColor BORDER_SOFT = SkColorSetARGB(80, 100, 120, 160);
    static constexpr SkColor BORDER_STRONG = SkColorSetARGB(130, 120, 150, 210);
}

//==============================================================================
// Constructor
//==============================================================================

//==============================================================================
// SettingsOverlayButton
//==============================================================================
class SettingsOverlayButton : public SkiaComponent {
public:
    SettingsOverlayButton() {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    
    std::function<void()> onClick;
    
    void mouseUp(const juce::MouseEvent& e) override {
        if (onClick) onClick();
    }
    
    void mouseEnter(const juce::MouseEvent&) override { isHovered = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { isHovered = false; repaint(); }
    
    void update(float dt) {
        float target = isHovered ? 1.0f : 0.0f;
        float diff = target - hoverAmount;
        if (std::abs(diff) > 0.001f) {
            hoverAmount += diff * 10.0f * dt;
            repaint();
        } else {
            hoverAmount = target;
        }
    }
    
    void drawSkia(SkCanvas* canvas) override {
        update(1.0f/60.0f); // Simple update in draw for now
        
        SkRect bounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());

        // Strong visual button shell so settings button matches new Wingman theme.
        SkPaint btnFill;
        btnFill.setAntiAlias(true);
        btnFill.setColor(design::interpolateColor(wingui::SURFACE_1, wingui::SURFACE_ACTIVE, hoverAmount * 0.45f));
        canvas->drawRoundRect(bounds, wingui::kRadiusChip, wingui::kRadiusChip, btnFill);

        // No outline: keep shell fill-only to avoid border clutter.
        
        svgicons::Style style;
        style.color = design::interpolateColor(SkColorSetRGB(170, 188, 220), SK_ColorWHITE, hoverAmount);
        style.glowRadius = 8.0f * hoverAmount;
        style.glowColor = theme::ACCENT_PRIMARY;
        
        svgicons::drawIconCentered(canvas, svgicons::IconId::Settings, bounds, 17.0f, style);
    }

private:
    float hoverAmount = 0.0f;
    bool isHovered = false;
};

class HistoryOverlayButton : public SkiaComponent {
public:
    HistoryOverlayButton() {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    std::function<void()> onClick;

    void mouseUp(const juce::MouseEvent&) override {
        if (onClick) onClick();
    }

    void mouseEnter(const juce::MouseEvent&) override { isHovered = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { isHovered = false; repaint(); }

    void drawSkia(SkCanvas* canvas) override {
        float target = isHovered ? 1.0f : 0.0f;
        hoverAmount += (target - hoverAmount) * 0.2f;

        const auto local = SkRect::MakeWH((float)getWidth(), (float)getHeight());
        SkPaint btnFill;
        btnFill.setAntiAlias(true);
        btnFill.setColor(design::interpolateColor(wingui::SURFACE_1, wingui::SURFACE_ACTIVE, hoverAmount * 0.45f));
        canvas->drawRoundRect(local, wingui::kRadiusChip, wingui::kRadiusChip, btnFill);

        // No outline: keep shell fill-only to avoid border clutter.

        icons::IconStyle historyStyle;
        historyStyle.color = design::interpolateColor(SkColorSetRGB(170, 188, 220), SK_ColorWHITE, hoverAmount);
        historyStyle.strokeWidth = 1.6f;
        historyStyle.glowColor = theme::ACCENT_PRIMARY;
        historyStyle.glowRadius = 6.0f * hoverAmount;
        icons::drawIconCentered(canvas, icons::History(), local, 17.0f, historyStyle);
    }

private:
    float hoverAmount = 0.0f;
    bool isHovered = false;
};

//==============================================================================
// Constructor
//==============================================================================

WingmanPanel::WingmanPanel(CommandAPI& api, Engine& engine)
    : commandAPI_(api), engine_(engine) {

    const int launchCount = Settings::getInstance().getAppLaunchCount();
    if (!kGreetings.empty()) {
        const int idx = std::max(0, launchCount - 1) % (int)kGreetings.size();
        launchGreeting_ = kGreetings[(size_t)idx];
    } else {
        launchGreeting_ = "Zenith is ready to assist with your arrangement.";
    }
    
    grokController_ = std::make_unique<GrokDAWController>(commandAPI_);
    grokController_->initialize();
    
    // Input Field: Frameless text input (container drawn separately)
    textInput_ = std::make_unique<SkiaTextInput>();
    textInput_->setPlaceholder("Ask Wingman...");
    textInput_->setPillShape(false);
    textInput_->setDrawChrome(false); // Composer shell is painted by WingmanPanel
    textInput_->setPlaceholderColor(SkColorSetARGB(180, 206, 220, 242)); // Improve contrast in composer
    textInput_->setFontSize(15.0f);
    textInput_->onReturnKey = [this] { sendMessage(); };
    
    addAndMakeVisible(textInput_.get());

    renameInput_ = std::make_unique<SkiaTextInput>();
    renameInput_->setPlaceholder("Rename session");
    renameInput_->setPillShape(false);
    renameInput_->setDrawChrome(false);
    renameInput_->setFontSize(13.0f);
    renameInput_->onReturnKey = [this] { commitRenameSession(); };
    renameInput_->onEscapeKey = [this] { cancelRenameSession(); };
    addChildComponent(renameInput_.get());

    historySearchInput_ = std::make_unique<SkiaTextInput>();
    historySearchInput_->setPlaceholder("Search chats...");
    historySearchInput_->setPillShape(false);
    historySearchInput_->setDrawChrome(false);
    historySearchInput_->setPlaceholderColor(SkColorSetARGB(170, 190, 206, 232));
    historySearchInput_->setFontSize(13.0f);
    historySearchInput_->onTextChanged = [this](const juce::String&) {
        updateLayout();
        repaint();
    };
    addChildComponent(historySearchInput_.get());
    historySearchInput_->setVisible(false);

    settingsPanel_ = std::make_unique<ModernSettingsPanel>();
    addChildComponent(settingsPanel_.get());
    settingsPanel_->setVisible(false);

    // Overlay Button for Settings (Always on top)
    settingsOverlayBtn_ = std::make_unique<SettingsOverlayButton>();
    settingsOverlayBtn_->onClick = [this] { toggleSettings(); };
    addChildComponent(settingsOverlayBtn_.get());
    settingsOverlayBtn_->setVisible(true);

    historyOverlayBtn_ = std::make_unique<HistoryOverlayButton>();
    historyOverlayBtn_->onClick = [this] { toggleHistory(); };
    addChildComponent(historyOverlayBtn_.get());
    historyOverlayBtn_->setVisible(true);

    settingsOpen_ = false;
    settingsSlideAmount_ = 0.0f;
    settingsTarget_ = 0.0f;

    initializeInterface();

    svgicons::preload({svgicons::IconId::Settings, svgicons::IconId::Wing, svgicons::IconId::Edit});
}

WingmanPanel::~WingmanPanel() = default;

void WingmanPanel::initializeInterface() {
    isLoadingSessions_ = true;
    loadSessionsFromDisk();
    if (sessions_.empty()) {
        isLoadingSessions_ = false;
        createNewSession();
        return;
    }
    setActiveSessionIndex(getActiveSessionIndex());
    if (!sessions_.empty()) {
        isReasoningMode_ = sessions_[(size_t)getActiveSessionIndex()].reasoningEnabled;
    }
    isLoadingSessions_ = false;
}

//==============================================================================
// Interaction Logic
//==============================================================================

void WingmanPanel::sendMessage() {
    juce::String text = textInput_->getText().trim();
    if (text.isEmpty()) return;
    textInput_->clear();
    sendPrompt(text);
}

void WingmanPanel::sendPrompt(const juce::String& prompt) {
    const juce::String trimmed = prompt.trim();
    if (trimmed.isEmpty()) return;

    if (!grokController_->isReady()) {
        grokController_->initialize();
    }

    if (!grokController_->isReady()) {
        WingmanMessage statusMsg;
        statusMsg.id = juce::Uuid().toString();
        statusMsg.sender = "Wingman";
        statusMsg.content =
            "Wingman is not configured for this build. API access is managed by Zenith.";
        statusMsg.timestamp = juce::Time::getCurrentTime().toMilliseconds();
        messages_.push_back(statusMsg);
        syncActiveSessionMessages();
        saveSessionsToDisk();
        recalcChatMetrics();
        scrollToBottom();
        repaint();
        return;
    }

    WingmanMessage msg;
    msg.id = juce::Uuid().toString();
    msg.sender = "User";
    msg.content = trimmed;
    msg.timestamp = juce::Time::getCurrentTime().toMilliseconds();
    messages_.push_back(msg);
    syncActiveSessionMessages();
    saveSessionsToDisk();

    isProcessing_ = true;
    processingStatus_ = "Connecting...";
    recalcChatMetrics();
    scrollToBottom();
    repaint();

    auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);
    grokController_->executeCommand(
        trimmed,
        isReasoningMode_ ? GrokMode::Thinking : GrokMode::Fast,
        [safeThis](juce::String response) {
            juce::MessageManager::callAsync([safeThis, response]() {
                if (!safeThis) return;
                
                const bool wasAtBottom = safeThis->isScrolledToBottom();
                
                safeThis->receiveMessage(response, {});
                safeThis->isProcessing_ = false;
                safeThis->processingStatus_.clear();
                safeThis->recalcChatMetrics();
                
                if (wasAtBottom && !safeThis->messages_.empty()) {
                    safeThis->scrollToTopOfMessage(safeThis->messages_.back().id);
                }
                
                safeThis->repaint();
            });
        },
        [safeThis](juce::String error) {
            juce::MessageManager::callAsync([safeThis, error]() {
                if (!safeThis) return;
                
                const bool wasAtBottom = safeThis->isScrolledToBottom();
                
                WingmanMessage msg;
                msg.id = juce::Uuid().toString();
                msg.sender = "Wingman";
                msg.content = "Error: " + error;
                msg.timestamp = juce::Time::getCurrentTime().toMilliseconds();
                safeThis->messages_.push_back(msg);
                safeThis->syncActiveSessionMessages();
                safeThis->saveSessionsToDisk();
                safeThis->isProcessing_ = false;
                safeThis->processingStatus_.clear();
                safeThis->recalcChatMetrics();
                
                if (wasAtBottom) {
                    safeThis->scrollToTopOfMessage(msg.id);
                }
                
                safeThis->repaint();
            });
        },
        [safeThis](juce::String status) {
            juce::MessageManager::callAsync([safeThis, status]() {
                if (!safeThis) return;
                safeThis->processingStatus_ = status;
                safeThis->repaint();
            });
        });
}

void WingmanPanel::receiveMessage(const juce::String& text, const juce::String& reasoning) {
    WingmanMessage msg;
    msg.id = juce::Uuid().toString();
    msg.sender = "Wingman";
    msg.content = text;
    msg.reasoning = reasoning;
    msg.hasReasoning = reasoning.isNotEmpty();
    msg.timestamp = juce::Time::getCurrentTime().toMilliseconds();
    messages_.push_back(msg);
    syncActiveSessionMessages();
    saveSessionsToDisk();
    recalcChatMetrics();
}

void WingmanPanel::createNewSession() {
    syncActiveSessionMessages();
    isLoadingSessions_ = true;
    WingmanSession s;
    s.id = juce::Uuid().toString();
    s.title = "New Session";
    s.dateLabel = juce::Time::getCurrentTime().toString(true, true);
    s.isActive = true;
    
    // Fix: Ensure suggestions are populated for new sessions
    ensureDefaultSuggestions(s);
    
    for (auto& session : sessions_) session.isActive = false;
    sessions_.insert(sessions_.begin(), s);
    setActiveSessionIndex(0);
    isLoadingSessions_ = false;
    recalcChatMetrics();
    scrollToBottom();
    repaint();
    saveSessionsToDisk();
}

void WingmanPanel::loadSessionsFromDisk() {
    sessions_.clear();
    messages_.clear();

    const juce::File file = getSessionsFile();
    if (!file.existsAsFile()) return;

    const juce::String text = file.loadFileAsString();
    if (text.isEmpty()) return;

    const juce::var root = juce::JSON::parse(text);
    const auto* rootObj = root.getDynamicObject();
    if (rootObj == nullptr) return;

    const juce::var sessionsVar = rootObj->getProperty("sessions");
    const auto* sessionsArray = sessionsVar.getArray();
    if (sessionsArray == nullptr) return;

    for (const auto& sessionVar : *sessionsArray) {
        const auto* sessionObj = sessionVar.getDynamicObject();
        if (sessionObj == nullptr) continue;

        WingmanSession session;
        session.id = sessionObj->getProperty("id").toString();
        session.title = sessionObj->getProperty("title").toString();
        session.dateLabel = sessionObj->getProperty("dateLabel").toString();
        session.isActive = (bool)sessionObj->getProperty("isActive");
        if (sessionObj->hasProperty("reasoningEnabled")) {
            session.reasoningEnabled = (bool)sessionObj->getProperty("reasoningEnabled");
        }

        const juce::var messagesVar = sessionObj->getProperty("messages");
        if (const auto* msgArray = messagesVar.getArray()) {
            for (const auto& msgVar : *msgArray) {
                const auto* msgObj = msgVar.getDynamicObject();
                if (msgObj == nullptr) continue;

                WingmanMessage msg;
                msg.id = msgObj->getProperty("id").toString();
                msg.sender = msgObj->getProperty("sender").toString();
                msg.content = msgObj->getProperty("content").toString();
                msg.reasoning = msgObj->getProperty("reasoning").toString();
                if (msgObj->hasProperty("hasReasoning")) {
                    msg.hasReasoning = (bool)msgObj->getProperty("hasReasoning");
                } else {
                    msg.hasReasoning = msg.reasoning.isNotEmpty();
                }
                msg.timestamp = static_cast<juce::int64>(msgObj->getProperty("timestamp"));
                msg.isReasoning = (bool)msgObj->getProperty("isReasoning");
                session.messages.push_back(msg);
            }
        }

        const juce::var suggestionsVar = sessionObj->getProperty("suggestions");
        if (const auto* sArray = suggestionsVar.getArray()) {
            for (const auto& sVar : *sArray) {
                const auto* sObj = sVar.getDynamicObject();
                if (sObj == nullptr) continue;
                Suggestion s;
                s.id = sObj->getProperty("id").toString();
                s.label = sObj->getProperty("label").toString();
                s.intent = sObj->getProperty("intent").toString();
                s.isUserCreated = (bool)sObj->getProperty("isUserCreated");
                if (sObj->hasProperty("isEditable")) {
                    s.isEditable = (bool)sObj->getProperty("isEditable");
                }
                session.suggestions.push_back(s);
            }
        }
        // Ensure defaults if empty (legacy migration or fresh load of old data)
        if (session.suggestions.empty()) {
            ensureDefaultSuggestions(session);
        }

        sessions_.push_back(session);
    }
}

void WingmanPanel::saveSessionsToDisk() {
    syncActiveSessionMessages();
    juce::Array<juce::var> sessionsArray;

    for (const auto& session : sessions_) {
        auto* sessionObj = new juce::DynamicObject();
        sessionObj->setProperty("id", session.id);
        sessionObj->setProperty("title", session.title);
        sessionObj->setProperty("dateLabel", session.dateLabel);
        sessionObj->setProperty("isActive", session.isActive);
        sessionObj->setProperty("reasoningEnabled", session.reasoningEnabled);

        juce::Array<juce::var> messagesArray;
        for (const auto& msg : session.messages) {
            auto* msgObj = new juce::DynamicObject();
            msgObj->setProperty("id", msg.id);
            msgObj->setProperty("sender", msg.sender);
            msgObj->setProperty("content", msg.content);
            msgObj->setProperty("reasoning", msg.reasoning);
            msgObj->setProperty("hasReasoning", msg.hasReasoning);
            msgObj->setProperty("timestamp", juce::var(static_cast<juce::int64>(msg.timestamp)));
            msgObj->setProperty("isReasoning", msg.isReasoning);
            msgObj->setProperty("isReasoning", msg.isReasoning);
            messagesArray.add(juce::var(msgObj));
        }
        sessionObj->setProperty("messages", messagesArray);

        juce::Array<juce::var> suggestionsArray;
        for (const auto& s : session.suggestions) {
            auto* sObj = new juce::DynamicObject();
            sObj->setProperty("id", s.id);
            sObj->setProperty("label", s.label);
            sObj->setProperty("intent", s.intent);
            sObj->setProperty("isUserCreated", s.isUserCreated);
            sObj->setProperty("isEditable", s.isEditable);
            suggestionsArray.add(juce::var(sObj));
        }
        sessionObj->setProperty("suggestions", suggestionsArray);

        sessionsArray.add(juce::var(sessionObj));
    }

    auto* root = new juce::DynamicObject();
    root->setProperty("version", 1);
    root->setProperty("sessions", sessionsArray);

    const juce::String json = juce::JSON::toString(juce::var(root), true);
    const juce::File file = getSessionsFile();
    file.replaceWithText(json);
}

void WingmanPanel::setActiveSessionIndex(int index) {
    if (sessions_.empty()) return;
    if (!isLoadingSessions_) {
        syncActiveSessionMessages();
    }
    index = juce::jlimit(0, (int)sessions_.size() - 1, index);
    for (auto& session : sessions_) session.isActive = false;
    sessions_[(size_t)index].isActive = true;
    messages_ = sessions_[(size_t)index].messages;
    isReasoningMode_ = sessions_[(size_t)index].reasoningEnabled; // Sync UI state
    recalcChatMetrics();
    scrollToBottom();
    if (settingsPanel_) {
        settingsPanel_->repaint();
    }
    repaint();
}

int WingmanPanel::getActiveSessionIndex() const {
    for (size_t i = 0; i < sessions_.size(); ++i) {
        if (sessions_[i].isActive) return (int)i;
    }
    return sessions_.empty() ? -1 : 0;
}

void WingmanPanel::syncActiveSessionMessages() {
    int index = getActiveSessionIndex();
    if (index < 0 || index >= (int)sessions_.size()) return;
    sessions_[(size_t)index].messages = messages_;
}

juce::File WingmanPanel::getSessionsFile() const {
    juce::File dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("ZenithDAW");
    dir.createDirectory();
    return dir.getChildFile("wingman_sessions.json");
}

void WingmanPanel::recalcChatMetrics() {
    if (messages_.empty()) {
        chatContentHeight_ = 0.0f;
        maxScroll_ = 0.0f;
        scrollOffset_ = 0.0f;
        updateScrollbarLayout();
        return;
    }

    const float contentW = (chatWrapWidth_ > 0.0f ? chatWrapWidth_ : 600.0f);
    float y = 0.0f;
    for (const auto& msg : messages_) {
        y += measureMessageHeight(msg, contentW) + 24.0f;
    }
    if (y > 0.0f) y -= 24.0f;

    chatContentHeight_ = y;
    maxScroll_ = std::max(0.0f, chatContentHeight_ - layout_.chatRect.height());
    scrollOffset_ = juce::jlimit(0.0f, maxScroll_, scrollOffset_);
    updateScrollbarLayout();
}

void WingmanPanel::scrollToBottom() {
    scrollOffset_ = maxScroll_;
}

bool WingmanPanel::isScrolledToBottom() const {
    // We use a small tolerance (10px) to consider the user "at the bottom" 
    // even if they are slightly off due to rounding or sub-pixel metrics.
    return scrollOffset_ >= (maxScroll_ - 10.0f);
}

void WingmanPanel::scrollToTopOfMessage(const juce::String& messageId) {
    if (messages_.empty()) return;

    float y = 0.0f;
    const float contentW = layout_.chatContentW;
    bool found = false;

    for (const auto& msg : messages_) {
        if (msg.id == messageId) {
            found = true;
            break;
        }
        y += measureMessageHeight(msg, contentW) + 24.0f;
    }

    if (found) {
        // Adjust for a small top margin so the message isn't mashed against the header
        scrollOffset_ = juce::jlimit(0.0f, maxScroll_, y - 10.0f);
    } else {
        scrollToBottom();
    }
}

//==============================================================================
// Lifecycle & Layout
//==============================================================================

void WingmanPanel::visibilityChanged() {
    if (isVisible()) {
        if (!historyToggledByUser_) {
            sidebarOpen_ = false; // default to main chat view
        }
        updateLayout();
    }
}
void WingmanPanel::resized() {
    lastResizeMs_ = juce::Time::getMillisecondCounter();
    updateLayout(true);
}

void WingmanPanel::updateLayout(bool deferChatReflow) {
    auto b = getLocalBounds().toFloat();
    float w = b.getWidth();
    float h = b.getHeight();
    
    sidebarAutoCollapsed_ = false;
    sidebarTarget_ = sidebarOpen_ ? 1.0f : 0.0f;

    float sidebarW = w;
    float contentShift = 0.0f;
    
    // Regions
    // Regions
    layout_.sidebarRect = SkRect::MakeXYWH(0, 0, sidebarW, h);
    layout_.contentRect = SkRect::MakeXYWH(0, 0, w, h);
    
    // Settings Panel Layout
    if (settingsPanel_) {
        // Positioned based on slide amount. Managed in updateSettingsAnimation mostly,
        // but we ensure correct size here.
        float panelX = w * (1.0f - settingsSlideAmount_);
        settingsPanel_->setBounds((int)panelX, 0, (int)w, (int)h);
    }
    
    // Header Buttons (Floating, Top Right)
    float btnSize = 34.0f;
    float pad = 20.0f;
    
    float settingsX = w - pad - btnSize;
    float historyX = pad;
    layout_.settingsBtn = SkRect::MakeXYWH(settingsX, pad, btnSize, btnSize);

    // Sync Overlay Button
    if (settingsOverlayBtn_) {
        // It simply stays at the computed layout position (Top Right)
        // It does NOT slide with the content, effectively acting as "Global"
        settingsOverlayBtn_->setBounds((int)settingsX, (int)pad, (int)btnSize, (int)btnSize);
        settingsOverlayBtn_->toFront(true); // Ensure on top of settings panel
    }
    if (historyOverlayBtn_) {
        historyOverlayBtn_->setBounds((int)historyX, (int)pad, (int)btnSize, (int)btnSize);
        historyOverlayBtn_->toFront(true);
    }
    
    layout_.toggleSidebarBtn = SkRect::MakeXYWH(historyX, pad, btnSize, btnSize); // History toggle
    layout_.newChatBtn = SkRect::MakeXYWH(w - pad - btnSize, pad, btnSize, btnSize); // History view new chat
    
    // Input Island (Floating Bottom)
    float availableW = w;
    // Wider by default to avoid the "floating island in a void" look
    float inputW = std::min(720.0f, std::max(240.0f, availableW - 48.0f));
    float inputH = 50.0f;
    float inputBottomMargin = 26.0f;
    
    float inputX = contentShift + (availableW - inputW) / 2.0f;
    float inputY = h - inputH - inputBottomMargin;
    
    layout_.inputContainerRect = SkRect::MakeXYWH(inputX, inputY, inputW, inputH);
    
    // Buttons inside input (Lightbulb Left, Send Right)
    layout_.brainBtn = SkRect::MakeXYWH(inputX + 10.0f, inputY + 9.0f, 32.0f, 32.0f);
    layout_.sendBtn = SkRect::MakeXYWH(inputX + inputW - 42.0f, inputY + 9.0f, 32.0f, 32.0f);
    
    // Text Input Rect
    const float inputLeftInset = 44.0f;
    const float inputRightInset = 44.0f;
    layout_.inputFieldRect =
        SkRect::MakeXYWH(inputX + inputLeftInset, inputY + 7.0f, inputW - (inputLeftInset + inputRightInset), inputH - 14.0f);
    
    if (textInput_) {
        textInput_->setBounds((int)layout_.inputFieldRect.left(), (int)layout_.inputFieldRect.top(),
                              (int)layout_.inputFieldRect.width(), (int)layout_.inputFieldRect.height());
        textInput_->setVisible(!sidebarOpen_ && settingsSlideAmount_ < 0.98f);
    }
    
    // Chat Area
    layout_.chatRect = SkRect::MakeLTRB(0.0f, 80.0f, w, inputY - 20.0f);

    float contentW = w;
    float contentPad = (contentW < 360.0f) ? 16.0f : 40.0f;
    float contentMax = std::max(0.0f, contentW - contentPad * 2.0f);

    // Centered reading column (used by both chat stream and empty state)
    layout_.chatContentW = std::min(680.0f, contentMax);
    layout_.chatContentW = std::max(180.0f, layout_.chatContentW);
    if (layout_.chatContentW > contentMax) layout_.chatContentW = contentMax;
    if (contentW < 360.0f) {
        layout_.chatContentX = layout_.chatRect.left() + contentPad;
    } else {
        layout_.chatContentX = layout_.chatRect.left() + (contentW - layout_.chatContentW) * 0.5f;
    }
    if (!deferChatReflow || chatWrapWidth_ <= 0.0f) {
        chatWrapWidth_ = layout_.chatContentW;
        chatWrapX_ = layout_.chatContentX;
    }

    // Sidebar session layout (for hit testing + edit/delete actions)
    layout_.sessionItemRects.clear();
    layout_.sessionEditRects.clear();
    layout_.sessionDeleteRects.clear();
    layout_.sessionEditHitRects.clear();
    layout_.sessionDeleteHitRects.clear();
    layout_.sessionSourceIndices.clear();
    layout_.historySearchRect = SkRect::MakeXYWH(14.0f, 88.0f, std::max(0.0f, sidebarW - 28.0f), 34.0f);
    sessionStates_.resize(sessions_.size());
    editStates_.resize(sessions_.size());
    deleteStates_.resize(sessions_.size());

    if (sidebarW > 1.0f) {
        const juce::String query = historySearchInput_ ? historySearchInput_->getText().trim().toLowerCase() : juce::String();
        float y = 136.0f;
        const float itemH = 32.0f;
        const float itemGap = 8.0f;
        const float itemX = 12.0f;
        const float itemW = std::max(0.0f, sidebarW - 24.0f);
        const float iconSize = 16.0f;
        const float iconPad = 8.0f;

        for (size_t i = 0; i < sessions_.size(); ++i) {
            const auto& s = sessions_[i];
            bool matches = true;
            if (query.isNotEmpty()) {
                matches = s.title.toLowerCase().contains(query) || s.dateLabel.toLowerCase().contains(query);
                if (!matches && !s.messages.empty()) {
                    for (const auto& m : s.messages) {
                        if (m.content.toLowerCase().contains(query)) {
                            matches = true;
                            break;
                        }
                    }
                }
            }
            if (!matches) continue;

            SkRect itemRect = SkRect::MakeXYWH(itemX, y, itemW, itemH);
            SkRect deleteRect = SkRect::MakeXYWH(itemRect.right() - iconPad - iconSize,
                                                 itemRect.top() + (itemH - iconSize) * 0.5f,
                                                 iconSize, iconSize);
            SkRect editRect = SkRect::MakeXYWH(deleteRect.left() - iconPad - iconSize,
                                               deleteRect.top(),
                                               iconSize, iconSize);
            const float hitPad = 10.0f;
            SkRect deleteHit = SkRect::MakeLTRB(deleteRect.left() - hitPad,
                                                deleteRect.top() - hitPad,
                                                deleteRect.right() + hitPad,
                                                deleteRect.bottom() + hitPad);
            SkRect editHit = SkRect::MakeLTRB(editRect.left() - hitPad,
                                              editRect.top() - hitPad,
                                              editRect.right() + hitPad,
                                              editRect.bottom() + hitPad);

            layout_.sessionItemRects.push_back(itemRect);
            layout_.sessionEditRects.push_back(editRect);
            layout_.sessionDeleteRects.push_back(deleteRect);
            layout_.sessionEditHitRects.push_back(editHit);
            layout_.sessionDeleteHitRects.push_back(deleteHit);
            layout_.sessionSourceIndices.push_back((int)i);
            y += itemH + itemGap;
        }
    }

    if (historySearchInput_) {
        historySearchInput_->setBounds((int)layout_.historySearchRect.left(), (int)layout_.historySearchRect.top(),
                                       (int)layout_.historySearchRect.width(), (int)layout_.historySearchRect.height());
        historySearchInput_->setVisible(sidebarOpenAmount_ > 0.45f && settingsSlideAmount_ < 0.98f);
    }

    if (renameInput_) {
        int rowIndex = -1;
        for (size_t row = 0; row < layout_.sessionSourceIndices.size(); ++row) {
            if (layout_.sessionSourceIndices[row] == editingSessionIndex_) {
                rowIndex = (int)row;
                break;
            }
        }
        if (rowIndex >= 0 && (size_t)rowIndex < layout_.sessionItemRects.size()) {
            SkRect itemRect = layout_.sessionItemRects[(size_t)rowIndex];
            float inputX = itemRect.left() + 12.0f;
            float inputY = itemRect.top() + 4.0f;
            float inputW = std::max(0.0f, itemRect.width() - 56.0f);
            float inputH = itemRect.height() - 8.0f;
            renameInput_->setBounds((int)inputX, (int)inputY, (int)inputW, (int)inputH);
            renameInput_->setVisible(true);
        } else {
            renameInput_->setVisible(false);
        }
    }

    // Empty-state hero card geometry
    const float heroW = std::min(560.0f, contentMax);
    // Extra breathing room so header text and suggestion chips never crowd each other.
    float heroH = (h < 700.0f) ? 250.0f : 320.0f;
    if (heroW < 320.0f) {
        heroH += 36.0f;
    }
    if (heroW < 260.0f) {
        heroH += 24.0f;
    }
    float heroX = layout_.chatRect.left() + (layout_.chatRect.width() - heroW) * 0.5f;
    float heroOffset = (h < 700.0f) ? 52.0f : 90.0f;
    float heroY = layout_.chatRect.centerY() - heroH * 0.5f - heroOffset;
    heroY = juce::jlimit(layout_.chatRect.top() + 10.0f, layout_.chatRect.bottom() - heroH - 10.0f, heroY);
    layout_.emptyHeroCard = SkRect::MakeXYWH(heroX, heroY, heroW, heroH);

    // Suggestion chips (stacked: works on narrow and wide)
    // We now layout dynamic suggestions based on active session
    layout_.suggestionChips.clear();
    layout_.regenerateBtn = SkRect::MakeEmpty();
    layout_.suggestionsAreaRect = SkRect::MakeEmpty();
    
    if (auto* session = getActiveSessionPtr()) {
        // Requirement: Only show suggestions if chat is empty
        if (session->messages.empty()) {
            const float chipH = 44.0f; // Professional Hit Target Standard
            const float chipGapX = 8.0f;
            const float chipGapY = 8.0f;
            
            // Layout inside the Hero Card (Bubble)
            // Bottom of hero card minus padding
            const float areaBot = layout_.emptyHeroCard.bottom() - 20.0f;
            
            float regenSize = 28.0f;
            float currentX = 0.0f; // relative
            float currentY = 0.0f; // relative
            float rowHeight = chipH;
            
            // Constrain width to fit inside hero card with padding
            float constraintsW = std::min(400.0f, layout_.emptyHeroCard.width() - 60.0f); 
            
            SkFont chipFont = design::typography::getSkFont(14.0f);
            std::vector<SkRect> relativeRects;
            
            // Calculate layout: Vertical Stack (Responsive check: only show top 3 on tiny screens)
            size_t maxChips = (h < 600.0f) ? 2 : (h < 800.0f) ? 3 : session->suggestions.size();
            switch (Settings::getInstance().getSuggestionFrequency()) {
                case Settings::WingmanSuggestionFrequency::Low: maxChips = std::min(maxChips, (size_t)2); break;
                case Settings::WingmanSuggestionFrequency::Balanced: maxChips = std::min(maxChips, (size_t)3); break;
                case Settings::WingmanSuggestionFrequency::High: default: break;
            }
            for (size_t i = 0; i < std::min(session->suggestions.size(), maxChips); ++i) {
                relativeRects.push_back(SkRect::MakeXYWH(0.0f, currentY, constraintsW, chipH));
                currentY += rowHeight + chipGapY;
            }
            
            float totalH = currentY; // No extra gap at end
            if (totalH > 0) totalH -= chipGapY; // Remove last gap
            
            // Position absolute
            // Center the block horizontally within the Hero Card
            float blockW = constraintsW;
            float startX = layout_.emptyHeroCard.centerX() - (blockW * 0.5f);
            
            // Position bottom anchor inside card
            float startY = areaBot - totalH;

            // Compute dynamic header reserve so wrapped greeting text cannot overlap chips.
            const float cardW = layout_.emptyHeroCard.width();
            const float logoSize = juce::jlimit(56.0f, 96.0f, cardW * 0.28f);
            const float contentShiftUp = (cardW < 360.0f) ? 28.0f : 40.0f;
            const float headY = layout_.emptyHeroCard.top() + 16.0f - contentShiftUp + logoSize - 2.0f;
            const float textGap = 14.0f;
            const float subY = headY + textGap + 8.0f;
            const float subSize = (cardW < 320.0f) ? 11.5f : 12.5f;
            const float subPad = (cardW < 360.0f) ? 22.0f : 28.0f;
            const float subW = std::max(120.0f, cardW - subPad * 2.0f);
            const juce::String subText = launchGreeting_.isNotEmpty()
                                             ? launchGreeting_
                                             : "Zenith is ready to assist with your arrangement.";
            SkFont subFont = design::typography::getSkFont(subSize);
            const int subLines = std::max(1, countWrappedLines(subText, subW, subFont));
            const float subLineH = 22.0f; // Match drawWrappedText line height
            const float subBottom = subY + (float)subLines * subLineH;
            const float minStartY = subBottom + 14.0f;
            startY = std::max(startY, minStartY);
            
            layout_.suggestionsAreaRect = SkRect::MakeXYWH(startX, startY, blockW, totalH);
            
            for (size_t i=0; i<relativeRects.size(); ++i) {
                 SkRect r = relativeRects[i];
                 r.offset(startX, startY);
                 Layout::ChipLayout cl;
                 cl.rect = r;
                 cl.data = &session->suggestions[i];
                 layout_.suggestionChips.push_back(cl);
            }
            
            // Regen btn: Left of the suggestion block, aligned with the bottom-most chip
            // This ensures stability: the button won't jump around if the stack height changes
            float regenX = startX - regenSize - 2.0f;
            float regenY = areaBot - (chipH + regenSize) * 0.5f;
            layout_.regenerateBtn = SkRect::MakeXYWH(regenX, regenY, regenSize, regenSize);
        }
    }

    if (deferChatReflow) {
        scheduleChatReflow();
        updateScrollbarLayout();
    } else {
        chatReflowPending_ = false;
        recalcChatMetrics();
    }
}

void WingmanPanel::scheduleChatReflow() {
    chatReflowPending_ = true;
    const uint32_t resizeStamp = lastResizeMs_;
    auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);
    juce::Timer::callAfterDelay(chatReflowDebounceMs, [safeThis, resizeStamp]() {
        if (!safeThis) return;
        if (safeThis->lastResizeMs_ != resizeStamp) return;
        safeThis->chatReflowPending_ = false;
        safeThis->chatWrapWidth_ = safeThis->layout_.chatContentW;
        safeThis->chatWrapX_ = safeThis->layout_.chatContentX;
        safeThis->recalcChatMetrics();
        safeThis->repaint();
    });
}

void WingmanPanel::updateScrollbarLayout() {
    if (layout_.chatRect.height() <= 1.0f) {
        layout_.scrollbarTrack = SkRect::MakeEmpty();
        layout_.scrollbarThumb = SkRect::MakeEmpty();
        return;
    }

    const float trackPad = 6.0f;
    const float trackW = 6.0f;
    const float trackX = layout_.chatRect.right() - trackW - trackPad;
    const float trackY = layout_.chatRect.top() + trackPad;
    const float trackH = std::max(0.0f, layout_.chatRect.height() - trackPad * 2.0f);
    layout_.scrollbarTrack = SkRect::MakeXYWH(trackX, trackY, trackW, trackH);

    if (maxScroll_ <= 0.0f || trackH <= 0.0f) {
        layout_.scrollbarThumb = SkRect::MakeEmpty();
        return;
    }

    const float visibleRatio = layout_.chatRect.height() / (layout_.chatRect.height() + maxScroll_);
    const float minThumbH = 28.0f;
    float thumbH = std::max(minThumbH, trackH * visibleRatio);
    float maxThumbY = trackY + trackH - thumbH;
    float thumbY = trackY;
    if (maxScroll_ > 0.0f && maxThumbY > trackY) {
        const float t = scrollOffset_ / maxScroll_;
        thumbY = trackY + (trackH - thumbH) * t;
    }
    layout_.scrollbarThumb = SkRect::MakeXYWH(trackX, thumbY, trackW, thumbH);
}

void WingmanPanel::setScrollFromThumbTop(float thumbTop) {
    if (maxScroll_ <= 0.0f || layout_.scrollbarThumb.isEmpty()) return;

    const SkRect& track = layout_.scrollbarTrack;
    const float thumbH = layout_.scrollbarThumb.height();
    const float minY = track.top();
    const float maxY = track.bottom() - thumbH;
    if (maxY <= minY) return;

    const float clamped = juce::jlimit(minY, maxY, thumbTop);
    const float t = (clamped - minY) / (maxY - minY);
    scrollOffset_ = juce::jlimit(0.0f, maxScroll_, t * maxScroll_);
    updateScrollbarLayout();
}

//==============================================================================
// Mouse
//==============================================================================

void WingmanPanel::mouseMove(const juce::MouseEvent& e) {
    auto p = e.getPosition();
    float x = (float)p.x; float y = (float)p.y;
    
    bool changed = false;
    auto check = [&](bool& s, const SkRect& r) { if (s != r.contains(x,y)) { s = r.contains(x,y); changed=true; } };
    
    check(interaction_.send.isHovered, layout_.sendBtn);
    check(interaction_.brain.isHovered, layout_.brainBtn);
    check(interaction_.settings.isHovered, layout_.settingsBtn);
    check(interaction_.sidebarToggle.isHovered, layout_.toggleSidebarBtn);
    check(interaction_.newChat.isHovered, layout_.newChatBtn);
    if (!layout_.scrollbarThumb.isEmpty() || !layout_.scrollbarTrack.isEmpty()) {
        const bool overThumb = layout_.scrollbarThumb.contains(x, y);
        const bool overTrack = layout_.scrollbarTrack.contains(x, y);
        if (interaction_.scrollbar.isHovered != (overThumb || overTrack)) {
            interaction_.scrollbar.isHovered = (overThumb || overTrack);
            changed = true;
        }
    } else if (interaction_.scrollbar.isHovered) {
        interaction_.scrollbar.isHovered = false;
        changed = true;
    }

    // Input hover (helps the pill feel interactive even when empty)
    check(interaction_.input.isHovered, layout_.inputContainerRect);

    // Suggestion chips
    for (const auto& chip : layout_.suggestionChips) {
        if (!chip.data) continue;
        bool now = chip.rect.contains(x, y);
        auto& state = interaction_.suggestionStates[chip.data->id.toStdString()];
        if (state.isHovered != now) {
            state.isHovered = now;
            changed = true;
        }
    }
    
    // Regenerate Btn
    check(interaction_.regenerate.isHovered, layout_.regenerateBtn);

    if (!layout_.sessionItemRects.empty()) {
        for (size_t i = 0; i < layout_.sessionItemRects.size(); ++i) {
            if (i >= layout_.sessionSourceIndices.size()) break;
            const int sourceIdx = layout_.sessionSourceIndices[i];
            if (sourceIdx < 0 || (size_t)sourceIdx >= sessionStates_.size()) continue;
            check(sessionStates_[(size_t)sourceIdx].isHovered, layout_.sessionItemRects[i]);
            check(editStates_[(size_t)sourceIdx].isHovered, layout_.sessionEditHitRects[i]);
            check(deleteStates_[(size_t)sourceIdx].isHovered, layout_.sessionDeleteHitRects[i]);
        }
    }

    juce::String nextReasoningId;
    for (const auto& entry : reasoningBubbleRects_) {
        if (entry.second.contains(x, y)) {
            nextReasoningId = entry.first;
            break;
        }
    }
    if (hoveredReasoningId_ != nextReasoningId) {
        hoveredReasoningId_ = nextReasoningId;
        changed = true;
    }

    if (changed) repaint();
}

void WingmanPanel::mouseDown(const juce::MouseEvent& e) {
    auto p = e.getPosition();
    float x = (float)p.x; float y = (float)p.y;

    if (reasoningOverlayOpen_) {
        if (reasoningOverlayCloseRect_.contains(x, y) ||
            !reasoningOverlayRect_.contains(x, y)) {
            closeReasoningOverlay();
        }
        return;
    }

    if (!layout_.scrollbarThumb.isEmpty() && layout_.scrollbarThumb.contains(x, y)) {
        interaction_.scrollbar.isPressed = true;
        scrollbarDragging_ = true;
        scrollbarDragOffsetY_ = y - layout_.scrollbarThumb.top();
        return;
    }
    if (!layout_.scrollbarTrack.isEmpty() && layout_.scrollbarTrack.contains(x, y)) {
        interaction_.scrollbar.isPressed = true;
        const float thumbH = layout_.scrollbarThumb.isEmpty() ? 28.0f : layout_.scrollbarThumb.height();
        setScrollFromThumbTop(y - thumbH * 0.5f);
        scrollbarDragging_ = true;
        scrollbarDragOffsetY_ = thumbH * 0.5f;
        repaint();
        return;
    }
    
    if (!hoveredReasoningId_.isEmpty()) {
        auto it = reasoningBubbleRects_.find(hoveredReasoningId_.toStdString());
        if (it != reasoningBubbleRects_.end() && it->second.contains(x, y)) {
            openReasoningOverlay(hoveredReasoningId_);
            return;
        }
    }

    if (layout_.toggleSidebarBtn.contains(x,y)) {
        interaction_.sidebarToggle.isPressed = true;
        toggleHistory();
        repaint();
    }
    else if (sidebarOpenAmount_ > 0.5f && layout_.newChatBtn.contains(x, y)) {
        interaction_.newChat.isPressed = true;
        createNewSession();
        // Force back to main chat immediately.
        sidebarOpen_ = false;
        sidebarTarget_ = 0.0f;
        historyToggledByUser_ = true;
        repaint();
        return;
    }
    else if (sidebarOpenAmount_ > 0.5f && layout_.historySearchRect.contains(x, y)) {
        if (historySearchInput_) {
            historySearchInput_->grabKeyboardFocus();
        }
        return;
    }
    else if (sidebarOpenAmount_ > 0.5f && !layout_.sessionItemRects.empty()) {
        for (size_t i = 0; i < layout_.sessionItemRects.size(); ++i) {
            if (i >= layout_.sessionSourceIndices.size()) break;
            const int sourceIdx = layout_.sessionSourceIndices[i];
            if (sourceIdx < 0 || (size_t)sourceIdx >= sessions_.size()) continue;

            if (layout_.sessionDeleteHitRects[i].contains(x, y)) {
                if (e.mods.isShiftDown()) {
                    deleteSessionAt(sourceIdx);
                    return;
                }

                uint32_t nowMs = juce::Time::getMillisecondCounter();
                if (sourceIdx == pendingDeleteIndex_ && nowMs < pendingDeleteUntilMs_) {
                    deleteSessionAt(sourceIdx);
                    return;
                }

                pendingDeleteIndex_ = sourceIdx;
                pendingDeleteUntilMs_ = nowMs + 2000;
                repaint();
                return;
            }
            if (layout_.sessionEditHitRects[i].contains(x, y)) {
                if (editingSessionIndex_ == sourceIdx && renameInput_) {
                    renameInput_->grabKeyboardFocus();
                    return;
                }
                beginRenameSession(sourceIdx);
                return;
            }
            if (layout_.sessionItemRects[i].contains(x, y)) {
                sidebarOpen_ = false;
                sidebarTarget_ = 0.0f;
                historyToggledByUser_ = true;
                setActiveSessionIndex(sourceIdx);
                saveSessionsToDisk();
                return;
            }
        }
    }
    else if (layout_.sendBtn.contains(x,y)) sendMessage();
    else if (layout_.brainBtn.contains(x,y)) { 
        if (!sessions_.empty()) {
            int idx = getActiveSessionIndex();
            if (idx >= 0) {
                sessions_[(size_t)idx].reasoningEnabled = !sessions_[(size_t)idx].reasoningEnabled;
                isReasoningMode_ = sessions_[(size_t)idx].reasoningEnabled;
                saveSessionsToDisk();
            }
        }
        if (settingsPanel_) {
            settingsPanel_->repaint();
        }
        repaint(); 
    }
    // Settings Toggle
    // Settings Toggle - Handled by Overlay Button now
    /*else if (layout_.settingsBtn.contains(x,y)) {
        toggleSettings();
    }*/
    else if (layout_.inputContainerRect.contains(x, y) || layout_.inputFieldRect.contains(x, y)) {
        if (textInput_) textInput_->grabKeyboardFocus();
        interaction_.inputFocused = true;
        repaint();
    }
      else if (messages_.empty()) {
        // Nothing special for empty, suggestions are handled below
    }
    
    // Suggestions Click
    for (const auto& chip : layout_.suggestionChips) {
        if (chip.rect.contains(x, y) && chip.data) {
             if (e.mods.isRightButtonDown()) {
                 showSuggestionContextMenu(*chip.data);
                 return;
             }
             sendPrompt(chip.data->intent);
             return;
        }
    }
    
    if (layout_.regenerateBtn.contains(x,y)) {
        interaction_.regenerate.isPressed = true;
        regenerateSuggestions();
        repaint();
    }

    bool pendingDeleteHit = false;
    if (pendingDeleteIndex_ >= 0) {
        for (size_t row = 0; row < layout_.sessionSourceIndices.size(); ++row) {
            if (layout_.sessionSourceIndices[row] == pendingDeleteIndex_) {
                if (row < layout_.sessionDeleteRects.size()) {
                    pendingDeleteHit = layout_.sessionDeleteRects[row].contains(x, y);
                }
                break;
            }
        }
    }
    if (!layout_.sidebarRect.contains(x, y) || !pendingDeleteHit) {
        pendingDeleteIndex_ = -1;
    }
}
void WingmanPanel::mouseUp(const juce::MouseEvent&) {
    interaction_.sidebarToggle.isPressed = false;
    interaction_.newChat.isPressed = false;
    interaction_.scrollbar.isPressed = false;
    scrollbarDragging_ = false;
    reasoningOverlayDragging_ = false;
}
void WingmanPanel::mouseExit(const juce::MouseEvent&) {
    interaction_.send.isHovered = false;
    interaction_.brain.isHovered = false;
    interaction_.settings.isHovered = false;
    interaction_.sidebarToggle.isHovered = false;
    interaction_.newChat.isHovered = false;
    interaction_.sidebarToggle.isPressed = false;
    interaction_.input.isHovered = false;
    interaction_.newChat.isPressed = false;
    interaction_.scrollbar.isHovered = false;
    interaction_.scrollbar.isPressed = false;
    hoveredReasoningId_.clear();
    hoveredReasoningId_.clear();
    for (auto& entry : interaction_.suggestionStates) entry.second.isHovered = false;
    interaction_.regenerate.isHovered = false;
    repaint();
}

void WingmanPanel::mouseDrag(const juce::MouseEvent& e) {
    if (reasoningOverlayOpen_) {
        if (!reasoningOverlayDragging_) {
            if (!reasoningOverlayRect_.contains((float)e.getPosition().x,
                                                (float)e.getPosition().y)) {
                return;
            }
            reasoningOverlayDragging_ = true;
            reasoningOverlayDragStartY_ = (float)e.getPosition().y;
        }

        float deltaY = (float)e.getPosition().y - reasoningOverlayDragStartY_;
        if (deltaY > 60.0f) {
            closeReasoningOverlay();
            reasoningOverlayDragging_ = false;
        }
        return;
    }

    if (scrollbarDragging_) {
        setScrollFromThumbTop((float)e.getPosition().y - scrollbarDragOffsetY_);
        repaint();
    }
}

bool WingmanPanel::keyPressed(const juce::KeyPress& key) {
    if (reasoningOverlayOpen_) {
        if (key == juce::KeyPress::escapeKey) {
            closeReasoningOverlay();
            return true;
        }
    }
    return false;
}
void WingmanPanel::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) {
    const float scrollScale = wheel.isSmooth ? 800.0f : 320.0f;

    if (reasoningOverlayOpen_) {
        const WingmanMessage* target = nullptr;
        for (const auto& msg : messages_) {
            if (msg.id == reasoningOverlayMessageId_) {
                target = &msg;
                break;
            }
        }
        if (target != nullptr) {
            float fullHeight = measureReasoningHeight(target->reasoning, reasoningOverlayRect_.width() - 36.0f);
            float maxScroll = std::max(0.0f, fullHeight - (reasoningOverlayRect_.height() - 62.0f));
            reasoningOverlayScroll_ -= wheel.deltaY * scrollScale;
            reasoningOverlayScroll_ = juce::jlimit(0.0f, maxScroll, reasoningOverlayScroll_);
            repaint();
            return;
        }
    }

    if (!hoveredReasoningId_.isEmpty() && !reasoningOverlayOpen_) {
        for (const auto& msg : messages_) {
            if (msg.id == hoveredReasoningId_) {
                float maxWidth = std::max(60.0f, layout_.chatContentW - 36.0f);
                float fullHeight = measureReasoningHeight(msg.reasoning, maxWidth);
                const float collapsedHeight = 96.0f;
                float maxScroll = std::max(0.0f, fullHeight - collapsedHeight);
                float& offset = reasoningScrollOffsets_[msg.id.toStdString()];
                offset -= wheel.deltaY * scrollScale;
                offset = juce::jlimit(0.0f, maxScroll, offset);
                repaint();
                return;
            }
        }
    }

    scrollOffset_ -= wheel.deltaY * scrollScale;
    scrollOffset_ = juce::jlimit(0.0f, maxScroll_, scrollOffset_);
    updateScrollbarLayout();
    repaint();
}

//==============================================================================
// RENDERING
//==============================================================================

void WingmanPanel::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;
    updateHoverAnimations(1.0f / 60.0f);
    updateSidebarAnimation(1.0f / 60.0f);
    updateSettingsAnimation(1.0f / 60.0f);

    // 1. Background (subtle gradient + vignette to avoid the "flat black slab" look)
    const SkRect bounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());

    SkColor stops[] = {
        SkColorSetRGB(24, 32, 48), // Elevation 1
        theme::getBG_VOID()        // Standard Base
    };
    SkPoint pts[] = { {0.0f, 0.0f}, {0.0f, bounds.height()} };
    SkPaint bg;
    bg.setShader(SkGradientShader::MakeLinear(pts, stops, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(bounds, bg);

    // ANIMATION TRANSFORM: CHAT SLIDES LEFT
    canvas->save();
    canvas->translate(-getWidth() * settingsSlideAmount_, 0);

    SkColor vignetteColors[] = {
        SkColorSetARGB(0, 0, 0, 0),
        SkColorSetARGB(60, 10, 12, 18)
    };
    SkPoint center = { bounds.centerX(), bounds.centerY() };
    SkPaint vignette;
    vignette.setShader(SkGradientShader::MakeRadial(center, std::max(bounds.width(), bounds.height()) * 0.75f,
                                                   vignetteColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(bounds, vignette);
    
    // 2. Two full-page views with horizontal slide transition
    // amount=0 -> Chat page centered, amount=1 -> History page centered.
    canvas->save();
    canvas->clipRect(layout_.contentRect);

    const float pageW = (float)getWidth();
    const float historyX = (sidebarOpenAmount_ - 1.0f) * pageW;
    const float chatX = sidebarOpenAmount_ * pageW;

    // History page
    canvas->save();
    canvas->translate(historyX, 0.0f);
    drawSidebar(canvas);
    canvas->restore();

    // Chat page
    canvas->save();
    canvas->translate(chatX, 0.0f);
    drawChatStream(canvas);
    drawChatScrollbar(canvas);
    drawInputIsland(canvas);
    canvas->restore();

    // Header controls are static and drive page transitions.
    drawHeader(canvas);
    canvas->restore();
    canvas->restore(); // Restore the main translation (chat slide)
    
    // Draw children (SettingsPanel, etc.) in untransformed coordinate space
    // Their positions/transforms are managed in updateSettingsAnimation
    drawChildren(canvas);

    if (reasoningOverlayOpen_) {
        drawReasoningOverlay(canvas);
    }
    
    // canvas->restore(); // Removed extra restore (Restore handled above)
}

void WingmanPanel::drawSidebar(SkCanvas* canvas) {
    float sidebarAlpha = juce::jlimit(0.0f, 1.0f, sidebarOpenAmount_);
    const SkRect srect = layout_.sidebarRect;

    // 1) Sidebar gradient background (noticeably richer than flat fill)
    {
        SkColor bgColors[] = {
            SkColorSetARGB((uint8_t)(220 * sidebarAlpha), 20, 34, 58),
            SkColorSetARGB((uint8_t)(228 * sidebarAlpha), 14, 24, 42)
        };
        SkPoint bgPts[] = {{srect.left(), srect.top()}, {srect.left(), srect.bottom()}};
        SkPaint bg;
        bg.setAntiAlias(true);
        bg.setShader(SkGradientShader::MakeLinear(bgPts, bgColors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRect(srect, bg);
    }

    // 2) History container shell so the list doesn't float in a void
    const SkRect listShell = SkRect::MakeLTRB(srect.left() + 10.0f, 56.0f, srect.right() - 10.0f, srect.bottom() - 14.0f);
    {
        SkPaint shell;
        shell.setAntiAlias(true);
        shell.setColor(SkColorSetARGB((uint8_t)(172 * sidebarAlpha), 18, 28, 48));
        canvas->drawRoundRect(listShell, 14.0f, 14.0f, shell);
    }

    // 3) Header strip title in history view
    {
        SkFont headerFont = design::typography::getSkFont(12.5f, design::FontWeight::Bold);
        SkPaint hp;
        hp.setAntiAlias(true);
        hp.setColor(SkColorSetARGB((uint8_t)(220 * sidebarAlpha), 205, 220, 246));
        const char* htxt = "History";
        canvas->drawString(htxt, listShell.left() + 12.0f, 78.0f, headerFont, hp);
    }

    // Search bar shell (actual input is a SkiaTextInput child)
    {
        SkPaint searchBg;
        searchBg.setAntiAlias(true);
        searchBg.setColor(SkColorSetARGB((uint8_t)(190 * sidebarAlpha), 24, 36, 62));
        canvas->drawRoundRect(layout_.historySearchRect, 10.0f, 10.0f, searchBg);
    }

    // 4) New Chat (+) button in history view
    {
        const float hover = interaction_.newChat.hoverAmount;
        SkPaint btnFill;
        btnFill.setAntiAlias(true);
        SkColor base = design::interpolateColor(wingui::SURFACE_1, wingui::SURFACE_ACTIVE, hover * 0.45f);
        btnFill.setColor(SkColorSetA(base, (uint8_t)(SkColorGetA(base) * sidebarAlpha)));
        canvas->drawRoundRect(layout_.newChatBtn, wingui::kRadiusChip, wingui::kRadiusChip, btnFill);

        icons::IconStyle addStyle;
        addStyle.color = design::interpolateColor(SkColorSetRGB(170, 188, 220), SK_ColorWHITE, hover);
        addStyle.strokeWidth = 1.8f;
        addStyle.glowColor = theme::ACCENT_PRIMARY;
        addStyle.glowRadius = 6.0f * interaction_.newChat.getGlowIntensity();
        icons::drawIconCentered(canvas, icons::Plus(), layout_.newChatBtn, 17.0f, addStyle);
    }

    // Sessions list (pushed down under header strip)
    SkFont itemFont = design::typography::getSkFont(13.0f);
    SkPaint textP;
    textP.setColor(SkColorSetA(theme::TEXT_BODY, (uint8_t)(SkColorGetA(theme::TEXT_BODY) * sidebarAlpha)));
    textP.setAntiAlias(true);

    for (size_t row = 0; row < layout_.sessionItemRects.size(); ++row) {
        if (row >= layout_.sessionSourceIndices.size()) break;
        const int sourceIdx = layout_.sessionSourceIndices[row];
        if (sourceIdx < 0 || (size_t)sourceIdx >= sessions_.size()) continue;
        const auto& s = sessions_[(size_t)sourceIdx];
        SkRect itemRect = layout_.sessionItemRects[row];
        SkPaint itemBg;
        itemBg.setAntiAlias(true);
        SkColor baseActive = SkColorSetARGB(194, 58, 86, 140);
        SkColor baseIdle = SkColorSetARGB(154, 30, 44, 74);
        SkColor blended = s.isActive ? baseActive : baseIdle;
        if ((size_t)sourceIdx < sessionStates_.size()) {
            blended = design::interpolateColor(baseIdle, baseActive, 0.35f + sessionStates_[(size_t)sourceIdx].hoverAmount * 0.45f);
        }
        if (sourceIdx == pendingDeleteIndex_) {
            blended = design::interpolateColor(blended, SkColorSetARGB(140, 60, 20, 24), 0.6f);
        }
        itemBg.setColor(SkColorSetA(blended, (uint8_t)(SkColorGetA(blended) * sidebarAlpha)));
        canvas->drawRoundRect(itemRect, wingui::kRadiusChip, wingui::kRadiusChip, itemBg);

        if (sourceIdx != editingSessionIndex_) {
            SkRect textClip = SkRect::MakeLTRB(itemRect.left() + 12.0f,
                                               itemRect.top(),
                                               layout_.sessionEditRects[row].left() - 8.0f,
                                               itemRect.bottom());
            canvas->save();
            canvas->clipRect(textClip);
            canvas->drawString(s.title.toStdString().c_str(), 24.0f, itemRect.centerY() + 5.0f, itemFont, textP);
            canvas->restore();
        }

        if (row < layout_.sessionEditRects.size()) {
            svgicons::Style editStyle;
            editStyle.color = design::interpolateColor(theme::TEXT_MUTED, theme::TEXT_HEAD,
                                                       editStates_[(size_t)sourceIdx].hoverAmount);
            editStyle.glowColor = theme::ACCENT_PRIMARY;
            editStyle.glowRadius = 6.0f * editStates_[(size_t)sourceIdx].hoverAmount;
            svgicons::drawIconCentered(canvas, svgicons::IconId::Edit,
                                       layout_.sessionEditRects[row], 14.0f, editStyle);
        }

        if (row < layout_.sessionDeleteRects.size()) {
            icons::IconStyle delStyle;
            float dangerEmphasis = (sourceIdx == pendingDeleteIndex_) ? 1.0f : deleteStates_[(size_t)sourceIdx].hoverAmount;
            delStyle.color = design::interpolateColor(theme::TEXT_MUTED, SkColorSetRGB(255, 110, 110),
                                                      dangerEmphasis);
            delStyle.strokeWidth = 1.5f;
            icons::drawIconCentered(canvas, icons::Delete(),
                                    layout_.sessionDeleteRects[row], 14.0f, delStyle);
        }
    }

    if (layout_.sessionItemRects.empty()) {
        const juce::String query = historySearchInput_ ? historySearchInput_->getText().trim() : juce::String();
        if (query.isNotEmpty()) {
            SkFont emptyFont = design::typography::getSkFont(12.0f, design::FontWeight::Regular);
            SkPaint emptyPaint;
            emptyPaint.setAntiAlias(true);
            emptyPaint.setColor(SkColorSetARGB((uint8_t)(180 * sidebarAlpha), 150, 166, 194));
            canvas->drawString("No chats match your search.", listShell.left() + 14.0f, layout_.historySearchRect.bottom() + 30.0f, emptyFont, emptyPaint);
        }
    }
}

void WingmanPanel::drawHeader(SkCanvas* canvas) {
    icons::IconStyle historyStyle;
    historyStyle.color = design::interpolateColor(theme::TEXT_MUTED, theme::TEXT_HEAD,
                                                  interaction_.sidebarToggle.hoverAmount);
    historyStyle.strokeWidth = 1.6f;
    historyStyle.glowColor = theme::ACCENT_PRIMARY;
    historyStyle.glowRadius = 6.0f * interaction_.sidebarToggle.getGlowIntensity();
    {
        float hoverScale = 1.0f + 0.08f * interaction_.sidebarToggle.hoverAmount;
        float pressScale = 1.0f - 0.05f * interaction_.sidebarToggle.pressAmount;
        float scale = hoverScale * pressScale;
        float cx = layout_.toggleSidebarBtn.centerX();
        float cy = layout_.toggleSidebarBtn.centerY();
        canvas->save();
        canvas->translate(cx, cy);
        canvas->scale(scale, scale);
        SkRect local = SkRect::MakeXYWH(-layout_.toggleSidebarBtn.width() * 0.5f,
                                        -layout_.toggleSidebarBtn.height() * 0.5f,
                                        layout_.toggleSidebarBtn.width(),
                                        layout_.toggleSidebarBtn.height());
        icons::drawIconCentered(canvas, icons::History(), local, 20.0f, historyStyle);
        canvas->restore();
    }

    // Settings Icon is now a child component (SettingsOverlayButton)
    // svgicons::drawIconCentered(canvas, svgicons::IconId::Settings, layout_.settingsBtn, 20.0f, settingsStyle);
}

void WingmanPanel::drawInputIsland(SkCanvas* canvas) {
    // "Clean Pill" aesthetic - no heavy borders, no bevels

    const float radius = 25.0f;
    const bool focused = (textInput_ && textInput_->hasKeyboardFocus(true));

    // 1. Subtle Glow (accent glow only when focused/hovered, no expensive invisible shadows)
    if (focused || interaction_.input.hoverAmount > 0.01f) {
        SkPaint glow;
        glow.setAntiAlias(true);
        glow.setColor(SkColorSetA(theme::ACCENT_PRIMARY, focused ? 60 : 40));
        float glowRadius = focused ? 16.0f : (10.0f + 6.0f * interaction_.input.hoverAmount);
        glow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, glowRadius));
        canvas->drawRoundRect(layout_.inputContainerRect.makeOffset(0, 4), radius, radius, glow);
    }

    // 2. Background (soft gradient - Elevation based)
    {
        SkColor colors[] = {
            SkColorSetRGB(38, 46, 68), // Slightly lighter for elevation
            SkColorSetRGB(28, 32, 48)
        };
        SkPoint pts[] = { {layout_.inputContainerRect.left(), layout_.inputContainerRect.top()},
                          {layout_.inputContainerRect.left(), layout_.inputContainerRect.bottom()} };
        SkPaint bg;
        bg.setAntiAlias(true);
        bg.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRoundRect(layout_.inputContainerRect, radius, radius, bg);
    }

    // 3. Stroke (focus-only emphasis to avoid outline clutter)
    {
        if (focused) {
            SkPaint border;
            border.setAntiAlias(true);
            border.setStyle(SkPaint::kStroke_Style);
            border.setStrokeWidth(1.4f);
            border.setColor(SkColorSetA(theme::ACCENT_PRIMARY, 145));
            canvas->drawRoundRect(layout_.inputContainerRect, radius, radius, border);
        }

        // Top highlight (adds depth with almost no visual noise)
        SkPaint hl;
        hl.setAntiAlias(true);
        hl.setColor(SkColorSetARGB(42, 210, 228, 255));
        SkRect r = layout_.inputContainerRect;
        canvas->drawLine(r.left() + 16.0f, r.top() + 1.0f, r.right() - 16.0f, r.top() + 1.0f, hl);
    }
    
    // 4. Lightbulb Icon (Left)
    const SkColor kWingmanYellow = SkColorSetRGB(255, 214, 64);
    float brainHover = std::max(interaction_.brain.hoverAmount, reasoningHighlightAmount_);
    icons::IconStyle brainStyle;
    brainStyle.color = isReasoningMode_ ? kWingmanYellow
                                        : design::interpolateColor(theme::TEXT_MUTED, kWingmanYellow,
                                                                   brainHover * 0.6f);
    if (isReasoningMode_ || brainHover > 0.1f) {
        brainStyle.glowColor = kWingmanYellow;
        brainStyle.glowRadius = isReasoningMode_ ? 16.0f : 12.0f * brainHover;
    }
    brainStyle.strokeWidth = 1.6f;
    icons::drawIconCentered(canvas, icons::Lightbulb(), layout_.brainBtn, 18.0f, brainStyle);
    
    // 5. Send Icon (Right) - visible even when empty (disabled state)
    const bool hasText = textInput_ && !textInput_->getText().trim().isEmpty();
    icons::IconStyle sendStyle;
    sendStyle.filled = false;
    sendStyle.color = hasText ? design::interpolateColor(theme::TEXT_HEAD, theme::ACCENT_PRIMARY,
                                                         interaction_.send.hoverAmount * 0.5f)
                              : design::interpolateColor(SkColorSetA(theme::TEXT_MUTED, 150),
                                                         theme::TEXT_HEAD,
                                                         interaction_.send.hoverAmount * 0.5f);
    if (interaction_.send.hoverAmount > 0.01f) {
        sendStyle.glowColor = theme::ACCENT_PRIMARY;
        sendStyle.glowRadius = 6.0f + 6.0f * interaction_.send.hoverAmount;
    }
    sendStyle.strokeWidth = 1.8f;
    {
        float hoverScale = 1.0f + 0.06f * interaction_.send.hoverAmount;
        float cx = layout_.sendBtn.centerX();
        float cy = layout_.sendBtn.centerY();
        canvas->save();
        canvas->translate(cx, cy);
        canvas->scale(hoverScale, hoverScale);
        SkRect local = SkRect::MakeXYWH(-layout_.sendBtn.width() * 0.5f,
                                        -layout_.sendBtn.height() * 0.5f,
                                        layout_.sendBtn.width(),
                                        layout_.sendBtn.height());
        icons::drawIconCentered(canvas, icons::SendArrowUp(), local, 18.0f, sendStyle);
        canvas->restore();
    }

    if (isProcessing_ && processingStatus_.isNotEmpty()) {
        SkPaint statusPaint;
        statusPaint.setAntiAlias(true);
        statusPaint.setColor(SkColorSetA(theme::TEXT_MUTED, 200));
        SkFont statusFont = design::typography::getSkFont(12.0f);
        auto statusText = processingStatus_.toStdString();
        float statusX = layout_.inputContainerRect.left() + 10.0f;
        float statusY = layout_.inputContainerRect.top() - 8.0f;
        canvas->drawString(statusText.c_str(), statusX, statusY, statusFont, statusPaint);
    }
}

void WingmanPanel::drawChatStream(SkCanvas* canvas) {
    if (messages_.empty()) {
        drawEmptyState(canvas);
        return;
    }
    
    canvas->save();
    canvas->clipRect(layout_.chatRect);

    reasoningBubbleRects_.clear();
    
    // Ensure scroll stays in-bounds
    scrollOffset_ = juce::jlimit(0.0f, maxScroll_, scrollOffset_);

    float y = layout_.chatRect.top() - scrollOffset_;
    const float contentW = (chatWrapWidth_ > 0.0f ? chatWrapWidth_ : layout_.chatContentW);
    const float contentX = (chatWrapWidth_ > 0.0f ? chatWrapX_ : layout_.chatContentX);
    
    for (const auto& msg : messages_) {
        float h = measureMessageHeight(msg, contentW);
        
        if (y + h > layout_.chatRect.top() && y < layout_.chatRect.bottom()) {
            drawMessageBubble(canvas, msg, y, contentX, contentW);
        }
        y += h + 24.0f;
    }
    
    canvas->restore();
}

void WingmanPanel::drawChatScrollbar(SkCanvas* canvas) {
    if (!canvas || layout_.scrollbarThumb.isEmpty()) return;

    const float hover = interaction_.scrollbar.hoverAmount;
    const float press = interaction_.scrollbar.pressAmount;

    SkPaint trackPaint;
    trackPaint.setAntiAlias(true);
    trackPaint.setColor(SkColorSetA(SK_ColorWHITE, (uint8_t)(20 + 25 * hover)));
    canvas->drawRoundRect(layout_.scrollbarTrack, 4.0f, 4.0f, trackPaint);

    SkPaint thumbPaint;
    thumbPaint.setAntiAlias(true);
    thumbPaint.setColor(design::interpolateColor(
        SkColorSetA(SK_ColorWHITE, 70),
        theme::ACCENT_PRIMARY,
        std::max(hover, press)));
    canvas->drawRoundRect(layout_.scrollbarThumb, 4.0f, 4.0f, thumbPaint);
}

void WingmanPanel::drawEmptyState(SkCanvas* canvas) {
    // Empty state should feel "intentional" rather than "nothing here".
    // We draw a subtle glass hero card with clear hierarchy and quick prompts.

    const SkRect card = layout_.emptyHeroCard;

    // Shadow
    SkPaint shadow;
    shadow.setColor(SkColorSetA(SK_ColorBLACK, 140));
    shadow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 18.0f));
    canvas->drawRoundRect(card.makeOffset(0, 8), 24.0f, 24.0f, shadow);

    // Card fill (soft vertical gradient)
    {
        SkColor colors[] = {
            SkColorSetARGB(220, 32, 44, 70),
            SkColorSetARGB(220, 22, 30, 52)
        };
        SkPoint pts[] = { {card.left(), card.top()}, {card.left(), card.bottom()} };
        SkPaint fill;
        fill.setAntiAlias(true);
        fill.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRoundRect(card, 24.0f, 24.0f, fill);
    }

    // Top sheen for depth (lighter than a full outline)
    SkPaint sheen;
    sheen.setAntiAlias(true);
    sheen.setColor(SkColorSetARGB(56, 214, 228, 255));
    canvas->drawLine(card.left() + 22.0f, card.top() + 1.0f,
                     card.right() - 22.0f, card.top() + 1.0f, sheen);

    // Center mark (bigger, with gentle glow)
    const float cx = card.centerX();
    const float topY = card.top();

    float logoSize = juce::jlimit(56.0f, 96.0f, card.width() * 0.28f);
    float contentShiftUp = (card.width() < 360.0f) ? 28.0f : 40.0f;
    SkRect logoR = SkRect::MakeXYWH(cx - logoSize * 0.5f, topY + 16.0f - contentShiftUp, logoSize, logoSize);
    icons::IconStyle logoStyle;
    logoStyle.color = SkColorSetA(theme::TEXT_HEAD, 210);
    logoStyle.strokeWidth = 1.8f;
    logoStyle.glowColor = theme::ACCENT_PRIMARY;
    logoStyle.glowRadius = 16.0f;
    svgicons::Style wingStyle;
    wingStyle.color = logoStyle.color;
    wingStyle.glowColor = logoStyle.glowColor;
    wingStyle.glowRadius = logoStyle.glowRadius;
    wingStyle.filled = true;
    float logoIcon = logoSize * 0.85f;
    svgicons::drawIconCentered(canvas, svgicons::IconId::Wing, logoR, logoIcon, wingStyle);

    // Headline + subhead
    SkPaint textP;
    textP.setAntiAlias(true);
    textP.setColor(theme::TEXT_HEAD);
    float headSize = (card.width() < 320.0f) ? 16.0f : 18.0f;
    SkFont headFont = design::typography::getSkFont(headSize, design::FontWeight::Bold);
    const char* head = "Wingman";
    float headW = headFont.measureText(head, strlen(head), SkTextEncoding::kUTF8);
    const float textGap = 14.0f;
    float headY = topY + 16.0f - contentShiftUp + logoSize - 2.0f;
    canvas->drawString(head, cx - headW * 0.5f, headY, headFont, textP);

    float subSize = (card.width() < 320.0f) ? 11.5f : 12.5f;
    SkFont subFont = design::typography::getSkFont(subSize);
    textP.setColor(SkColorSetA(theme::TEXT_BODY, 214));
    const juce::String sub = launchGreeting_.isNotEmpty()
                                 ? launchGreeting_
                                 : "Zenith is ready to assist with your arrangement.";
    float subY = headY + textGap + 8.0f;
    const float subPad = (card.width() < 360.0f) ? 22.0f : 28.0f;
    const float subX = card.left() + subPad;
    const float subW = std::max(120.0f, card.width() - subPad * 2.0f);

    // Centered wrapped text (hero subtitle) for clean visual hierarchy.
    juce::StringArray wrappedLines;
    {
        juce::StringArray words;
        words.addTokens(sub, " ", "");
        words.removeEmptyStrings();

        juce::String current;
        for (int i = 0; i < words.size(); ++i) {
            const juce::String w = words[i];
            const juce::String candidate = current.isEmpty() ? w : (current + " " + w);
            const auto candUtf8 = candidate.toStdString();
            const float candW = subFont.measureText(candUtf8.c_str(), candUtf8.size(), SkTextEncoding::kUTF8);

            if (!current.isEmpty() && candW > subW) {
                wrappedLines.add(current);
                current = w;
            } else {
                current = candidate;
            }
        }
        if (current.isNotEmpty()) {
            wrappedLines.add(current);
        }
        if (wrappedLines.isEmpty()) {
            wrappedLines.add(sub);
        }
    }

    const float subLineH = 22.0f;
    float lineY = subY;
    for (int i = 0; i < wrappedLines.size(); ++i) {
        const std::string line = wrappedLines[i].toStdString();
        float lineW = subFont.measureText(line.c_str(), line.size(), SkTextEncoding::kUTF8);
        float lineX = card.centerX() - lineW * 0.5f;
        lineX = juce::jlimit(subX, subX + subW - lineW, lineX);
        canvas->drawString(line.c_str(), lineX, lineY, subFont, textP);
        lineY += subLineH;
    }

    drawSuggestions(canvas);
}

void WingmanPanel::drawSuggestionChip(SkCanvas* canvas, const SkRect& r, const Suggestion* s, float hoverAmount) {
    if (!s) return;
    
    SkPaint fill;
    fill.setAntiAlias(true);
    fill.setColor(design::interpolateColor(SkColorSetARGB(170, 42, 56, 86),
                                           SkColorSetARGB(214, 72, 106, 168),
                                           hoverAmount));
    canvas->drawRoundRect(r, wingui::kRadiusPill, wingui::kRadiusPill, fill);

    // Small leading icon (subtle) - Sparkles or Edit? 
    // Defaults to sparkles.
    SkRect iconR = SkRect::MakeXYWH(r.left() + 8.0f, r.centerY() - 7.0f, 14.0f, 14.0f);
    icons::IconStyle ic;
    ic.color = design::interpolateColor(SkColorSetA(theme::TEXT_BODY, 210),
                                        SkColorSetRGB(168, 214, 255),
                                        hoverAmount);
    ic.strokeWidth = 1.6f;
    icons::drawIconCentered(canvas, icons::Sparkles(), iconR, 12.0f, ic);

    SkPaint tp;
    tp.setAntiAlias(true);
    tp.setColor(design::interpolateColor(SkColorSetA(theme::TEXT_BODY, 220),
                                         theme::TEXT_HEAD,
                                         hoverAmount));
    SkFont f = design::typography::getSkFont(13.0f);

    auto utf8 = s->label.toStdString();
    const float textX = r.left() + 28.0f; // tighter padding
    const float textY = r.centerY() + 5.0f;
    canvas->drawString(utf8.c_str(), textX, textY, f, tp);
}

void WingmanPanel::drawSuggestions(SkCanvas* canvas) {
    if (layout_.suggestionChips.empty()) return;

    for (const auto& chip : layout_.suggestionChips) {
        float hover = 0.0f;
        auto it = interaction_.suggestionStates.find(chip.data->id.toStdString());
        if (it != interaction_.suggestionStates.end()) {
            hover = it->second.hoverAmount;
        }
        drawSuggestionChip(canvas, chip.rect, chip.data, hover);
    }
    
    // Draw Regenerate Btn
    if (!layout_.regenerateBtn.isEmpty()) {
        SkRect r = layout_.regenerateBtn;
        float hover = interaction_.regenerate.hoverAmount;
        float press = interaction_.regenerate.pressAmount;
        
        icons::IconStyle is;
        is.color = design::interpolateColor(theme::TEXT_MUTED, theme::ACCENT_PRIMARY, hover);
        is.strokeWidth = 1.5f; // Thinner stroke for clarity
        is.glowColor = theme::ACCENT_PRIMARY;
        is.glowRadius = 6.0f * press; // Subtle glow only on press
        
        canvas->save();
        // Spin animation: Physics based
        canvas->rotate(refreshRotation_, r.centerX(), r.centerY());
        
        // Use the new high-fidelity Refresh icon
        icons::drawIconCentered(canvas, icons::Refresh(), r, 16.0f, is);
        canvas->restore();
    }
}

void WingmanPanel::updateHoverAnimations(float dt) {
    bool animating = false;
    uint32_t nowMs = juce::Time::getMillisecondCounter();
    if (pendingDeleteIndex_ >= 0 && nowMs >= pendingDeleteUntilMs_) {
        pendingDeleteIndex_ = -1;
    }
    if (nowMs < reasoningHighlightUntilMs_) {
        float pulse = 0.5f + 0.5f * std::sin((nowMs % 700) / 700.0f * juce::MathConstants<float>::twoPi);
        reasoningHighlightAmount_ = 0.4f + 0.6f * pulse;
        animating = true;
    } else if (reasoningHighlightAmount_ > 0.0f) {
        reasoningHighlightAmount_ = 0.0f;
    }
    auto update = [&](InteractionState& state) {
        state.update(dt);
        animating |= state.isAnimating();
    };

    update(interaction_.send);
    update(interaction_.brain);
    // update(interaction_.settings); // Handled by child component
    update(interaction_.sidebarToggle);
    update(interaction_.newChat);
    update(interaction_.input);
    update(interaction_.input);
    update(interaction_.regenerate);
    update(interaction_.scrollbar);
    for (auto& entry : interaction_.suggestionStates) update(entry.second);
    for (auto& state : sessionStates_) update(state);
    for (auto& state : editStates_) update(state);
    for (auto& state : deleteStates_) update(state);

    // Refresh Icon Physics
    if (std::abs(refreshVelocity_) > 0.1f) {
        refreshRotation_ += refreshVelocity_ * dt * 360.0f; // Velocity in rotations/sec
        refreshVelocity_ *= 0.92f; // Friction
        animating = true;
    } else {
        refreshVelocity_ = 0.0f;
        // Optional: Snap to nearest 45 or 90 deg? Nah, free spin is fine.
    }

    if (animating) repaint();
}

void WingmanPanel::updateSidebarAnimation(float dt) {
    float speed = wingui::kAnimSidebarSpeed;
    float next = sidebarOpenAmount_ + (sidebarTarget_ - sidebarOpenAmount_) * speed * dt;
    if (std::abs(next - sidebarOpenAmount_) > 0.0001f) {
        sidebarOpenAmount_ = next;
        sidebarOpenAmount_ = juce::jlimit(0.0f, 1.0f, sidebarOpenAmount_);
        updateLayout();
        repaint();
    } else {
        sidebarOpenAmount_ = sidebarTarget_;
    }
    if (textInput_) {
        textInput_->setVisible(!sidebarOpen_ && settingsSlideAmount_ < 0.98f);
    }
}

void WingmanPanel::highlightReasoningButton() {
    reasoningHighlightUntilMs_ = juce::Time::getMillisecondCounter() + 1400;
    reasoningHighlightAmount_ = 1.0f;
}

void WingmanPanel::toggleSettings() {
    const bool settingsVisible = settingsOpen_ || settingsSlideAmount_ > 0.02f || settingsTarget_ > 0.02f;
    const bool historyVisible = sidebarOpen_ || sidebarOpenAmount_ > 0.02f || sidebarTarget_ > 0.02f;

    // Same button on current view should always return to chat.
    // If history is visible, Settings switches to settings first.
    if (settingsVisible && !historyVisible) {
        settingsOpen_ = false;
        settingsTarget_ = 0.0f;
    } else {
        settingsOpen_ = true;
        settingsTarget_ = 1.0f;
    }

    // Settings and History are mutually exclusive views.
    if (sidebarOpen_ || sidebarTarget_ > 0.0f || sidebarOpenAmount_ > 0.0f) {
        sidebarOpen_ = false;
        sidebarTarget_ = 0.0f;
    }

    if (settingsPanel_) {
        if (settingsOpen_) {
            settingsPanel_->refreshFromSettings();
            settingsPanel_->startShowAnimation();
        } else {
            settingsPanel_->startHideAnimation();
        }
        settingsPanel_->setVisible(true);
    }
    if (settingsOverlayBtn_) {
        // Ensure the overlay button stays on top of the panel
        settingsOverlayBtn_->toFront(true);
    }
    if (historyOverlayBtn_) {
        historyOverlayBtn_->toFront(true);
    }
    repaint();
}

void WingmanPanel::toggleHistory() {
    const bool settingsVisible = settingsOpen_ || settingsSlideAmount_ > 0.02f || settingsTarget_ > 0.02f;
    const bool historyVisible = sidebarOpen_ && !settingsVisible;

    if (historyVisible) {
        sidebarOpen_ = false;
        sidebarTarget_ = 0.0f;
    } else {
        sidebarOpen_ = true;
        sidebarTarget_ = 1.0f;
    }

    if (settingsOpen_ || settingsTarget_ > 0.0f || settingsSlideAmount_ > 0.0f) {
        settingsOpen_ = false;
        settingsTarget_ = 0.0f;
        if (settingsPanel_) settingsPanel_->startHideAnimation();
    }

    historyToggledByUser_ = true;
    if (textInput_) {
        textInput_->setVisible(!sidebarOpen_ && settingsSlideAmount_ < 0.98f);
    }
}

void WingmanPanel::updateSettingsAnimation(float dt) {
    if (std::abs(settingsSlideAmount_ - settingsTarget_) < 0.001f) {
        settingsSlideAmount_ = settingsTarget_;
        if (!settingsOpen_) {
            if (settingsPanel_) settingsPanel_->setVisible(false);
        }
    } else {
        float speed = wingui::kAnimSettingsSpeed;
        settingsSlideAmount_ += (settingsTarget_ - settingsSlideAmount_) * speed * dt;
        repaint();
    }
    
    // Sync Settings Panel Position and Child Transforms
    if (settingsPanel_) {
        float panelX = getWidth() * (1.0f - settingsSlideAmount_);
        settingsPanel_->setTopLeftPosition((int)panelX, 0);
    }
    
    const float settingsShiftX = -getWidth() * settingsSlideAmount_;
    const float chatShiftX = sidebarOpenAmount_ * getWidth();
    const float historyShiftX = (sidebarOpenAmount_ - 1.0f) * getWidth();

    // Keep child widgets aligned with the same page transforms used in drawSkia().
    if (textInput_) {
        textInput_->setTransform(juce::AffineTransform::translation(settingsShiftX + chatShiftX, 0));
        textInput_->setVisible(sidebarOpenAmount_ < 0.45f && settingsSlideAmount_ < 0.98f);
    }
    if (renameInput_) {
        renameInput_->setTransform(juce::AffineTransform::translation(settingsShiftX + historyShiftX, 0));
    }
    if (historySearchInput_) {
        historySearchInput_->setTransform(juce::AffineTransform::translation(settingsShiftX + historyShiftX, 0));
    }

    const bool settingsVisible = settingsSlideAmount_ > 0.45f || settingsTarget_ > 0.45f || settingsOpen_;
    const bool historyVisible = sidebarOpenAmount_ > 0.45f || sidebarTarget_ > 0.45f || sidebarOpen_;

    // Reduce header clutter:
    // - hide History icon while Settings is active
    // - hide Settings icon while History is active
    if (historyOverlayBtn_) {
        historyOverlayBtn_->setVisible(!settingsVisible);
    }
    if (settingsOverlayBtn_) {
        settingsOverlayBtn_->setVisible(!historyVisible);
    }
    if (historySearchInput_) {
        historySearchInput_->setVisible(historyVisible && !settingsVisible);
    }
}

void WingmanPanel::beginRenameSession(int index) {
    if (index < 0 || index >= (int)sessions_.size()) return;
    editingSessionIndex_ = index;
    if (renameInput_) {
        renameInput_->setText(sessions_[(size_t)index].title);
        renameInput_->setVisible(true);
        resized();
        auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);
        juce::MessageManager::callAsync([safeThis]() {
            if (!safeThis) return;
            if (safeThis->renameInput_) {
                safeThis->renameInput_->grabKeyboardFocus();
            }
        });
    }
    resized();
    repaint();
}

void WingmanPanel::commitRenameSession() {
    if (editingSessionIndex_ < 0 || !renameInput_) return;
    auto title = renameInput_->getText().trim();
    if (title.isNotEmpty() && (size_t)editingSessionIndex_ < sessions_.size()) {
        sessions_[(size_t)editingSessionIndex_].title = title;
        saveSessionsToDisk();
    }
    editingSessionIndex_ = -1;
    renameInput_->clear();
    renameInput_->setVisible(false);
    resized();
    repaint();
}

void WingmanPanel::cancelRenameSession() {
    editingSessionIndex_ = -1;
    if (renameInput_) {
        renameInput_->clear();
        renameInput_->setVisible(false);
    }
    resized();
    repaint();
}

void WingmanPanel::deleteSessionAt(int index) {
    if (index < 0 || index >= (int)sessions_.size()) return;
    
    // Reset interaction state immediately to prevent logic races
    pendingDeleteIndex_ = -1;
    pendingDeleteUntilMs_ = 0;

    bool wasActive = sessions_[(size_t)index].isActive;

    if (index == editingSessionIndex_) cancelRenameSession();
    sessions_.erase(sessions_.begin() + index);
    
    // CRITICAL: Sync layout vectors immediately to match new sessions size
    // This prevents potential out-of-bounds access in drawSidebar/mouseMove
    updateLayout();

    if (sessions_.empty()) {
        createNewSession();
        return;
    }

    if (wasActive) {
        int next = std::min(index, (int)sessions_.size() - 1);
        
        // Prevent syncing the deleted session's messages to the new active session
        // We are switching context because the old one is gone, so we discard current `messages_`
        bool wasLoading = isLoadingSessions_;
        isLoadingSessions_ = true; 
        setActiveSessionIndex(next);
        isLoadingSessions_ = wasLoading;
    }

    recalcChatMetrics();
    // updateLayout was already called, but resized() ensures full flow if needed
    resized(); 
    repaint();
    saveSessionsToDisk();
}

//==============================================================================
// Suggestion Logic
//==============================================================================

WingmanSession* WingmanPanel::getActiveSessionPtr() {
    int idx = getActiveSessionIndex();
    if (idx >= 0 && idx < (int)sessions_.size()) {
        return &sessions_[(size_t)idx];
    }
    return nullptr;
}

void WingmanPanel::ensureDefaultSuggestions(WingmanSession& session) {
    if (!session.suggestions.empty()) return;

    session.suggestions.push_back({ juce::Uuid().toString(), "Explain Structure", "Explain this project structure", false, true });
    session.suggestions.push_back({ juce::Uuid().toString(), "Design Pad", "Design a lush pad for this track", false, true });
    session.suggestions.push_back({ juce::Uuid().toString(), "Vocal Chain", "Draft a vocal chain for this track", false, true });
}

void WingmanPanel::addSuggestion(const juce::String& label, const juce::String& intent) {
    if (auto* s = getActiveSessionPtr()) {
        s->suggestions.push_back({ juce::Uuid().toString(), label, intent, true, true });
        saveSessionsToDisk();
        updateLayout();
        repaint();
    }
}

void WingmanPanel::updateSuggestion(const juce::String& id, const juce::String& label, const juce::String& intent) {
    if (auto* s = getActiveSessionPtr()) {
        for (auto& sug : s->suggestions) {
            if (sug.id == id) {
                sug.label = label;
                sug.intent = intent;
                saveSessionsToDisk();
                updateLayout();
                repaint();
                return;
            }
        }
    }
}

void WingmanPanel::deleteSuggestion(const juce::String& id) {
    if (auto* s = getActiveSessionPtr()) {
        auto it = std::remove_if(s->suggestions.begin(), s->suggestions.end(),
                                 [&](const Suggestion& sug) { return sug.id == id; });
        if (it != s->suggestions.end()) {
            s->suggestions.erase(it, s->suggestions.end());
            saveSessionsToDisk();
            updateLayout();
            repaint();
        }
    }
}

void WingmanPanel::regenerateSuggestions() {
    // FUTURE: Enable AI-powered suggestion generation when DAW is fully matured
    // Target Model: grok-4-1-fast-non-reasoning
    /*
    if (false) { // Disabled for now
        auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);
        int suggestionCount = 3;
        switch (Settings::getInstance().getSuggestionFrequency()) {
            case Settings::WingmanSuggestionFrequency::Low: suggestionCount = 2; break;
            case Settings::WingmanSuggestionFrequency::High: suggestionCount = 4; break;
            case Settings::WingmanSuggestionFrequency::Balanced:
            default: suggestionCount = 3; break;
        }
        grokController_->executeCommand(
            "Generate " + juce::String(suggestionCount) + " short, creative, and distinct suggestions for what I can ask you to help with in this DAW session. Return ONLY JSON array format like [{\"label\": \"...\", \"intent\": \"...\"}, ...]. Keep labels under 20 chars.",
            GrokMode::Fast, // Uses grok-4-1-fast-non-reasoning
            [safeThis](juce::String response) {
                // Parse JSON and update s->suggestions...
            },
            nullptr
        );
        return; 
    }
    */

    if (auto* s = getActiveSessionPtr()) {
        // Trigger spin animation
        refreshVelocity_ = 2.5f; // Rev up!
        
        s->suggestions.clear();
        // deterministic "randomness" or just rotating set for now as per requirements
        // Requirement 6: "Clears existing bubbles... Inserts a new set... idempotent/deterministic"
        // We will just cycle between 2 sets for demo/implementation purposes.
        
        static bool toggle = false;
        toggle = !toggle;
        
        if (toggle) {
            s->suggestions.push_back({ juce::Uuid().toString(), "Mix Tips", "Give me mixing tips for these stems", false, true });
            s->suggestions.push_back({ juce::Uuid().toString(), "Bassline Idea", "Generate a groovy bassline MIDI", false, true });
            s->suggestions.push_back({ juce::Uuid().toString(), "Mastering", "Suggest mastering chain settings", false, true });
        } else {
            // Restore defaults
             ensureDefaultSuggestions(*s);
        }
        
        saveSessionsToDisk();
        updateLayout();
        repaint();
    }
}

void WingmanPanel::showSuggestionContextMenu(const Suggestion& s) {
    auto menu = ContextMenuManager::createMenu();

    // safe capture
    juce::String sId = s.id;
    juce::String sLabel = s.label;
    juce::String sIntent = s.intent;
    auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);

    menu->addItem(1, "Edit Suggestion", true, false, [safeThis, sId, sLabel, sIntent]() {
        if (!safeThis) return;
        Suggestion copy;
        copy.id = sId;
        copy.label = sLabel;
        copy.intent = sIntent;
        safeThis->showEditSuggestionDialog(copy);
    });
    menu->addItem(2, "Delete Suggestion", true, false, [safeThis, sId]() {
        if (!safeThis) return;
        safeThis->deleteSuggestion(sId);
    });

    ContextMenuManager::getInstance().showMenuAtMouse(std::move(menu));
}

void WingmanPanel::showEditSuggestionDialog(const Suggestion& s) {
    auto* alert = new SkiaAlertWindow("Edit Suggestion", "Modify the label and intent.", SkiaAlertWindow::IconType::NoIcon);

    alert->addTextEditor("label", s.label, "Label:");
    alert->addTextEditor("intent", s.intent, "Intent (Prompt):");
    alert->addButton("Save", SkiaAlertWindow::Result::Button1, SkiaButton::Style::Primary);
    alert->addButton("Cancel", SkiaAlertWindow::Result::Cancelled, SkiaButton::Style::Secondary);

    auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);
    juce::String sId = s.id;
    alert->showAsync([safeThis, sId, alert](SkiaAlertWindow::Result result) {
        if (safeThis && result == SkiaAlertWindow::Result::Button1) {
            safeThis->updateSuggestion(
                sId,
                alert->getTextEditorContents("label"),
                alert->getTextEditorContents("intent"));
        }
        delete alert;
    });

    addAndMakeVisible(alert);
    alert->setCentreRelative(0.5f, 0.45f);
}

void WingmanPanel::drawMessageBubble(SkCanvas* canvas, const WingmanMessage& msg, float y, float x, float w) {
    if (msg.sender == "User") {
        // User gets a right-aligned pill
        
        SkFont font = design::typography::getSkFont(15.0f);
        SkPaint paint; paint.setColor(theme::TEXT_BODY); paint.setAntiAlias(true);
        
        float textW = font.measureText(msg.content.toUTF8(), msg.content.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
        textW = std::min(textW, w);
        float pillW = textW + 32.0f;
        float pillX = x + w - pillW;
        
        SkRect pill = SkRect::MakeXYWH(pillX, y, pillW, measureMessageHeight(msg, w));
        
        SkPaint bg;
        bg.setColor(wingui::SURFACE_2);
        bg.setAntiAlias(true);
        canvas->drawRoundRect(pill, wingui::kRadiusPill, wingui::kRadiusPill, bg);
        
        drawWrappedText(canvas, msg.content, pillX + 16.0f, y + 26.0f, pillW - 32.0f, font, paint);
        
    } else {
        // Wingman: Left aligned themed bubble (same language as settings/suggestion surfaces).
        SkRect iconR = SkRect::MakeXYWH(x, y, 24.0f, 24.0f);
        
        svgicons::Style wingStyle;
        wingStyle.color = SK_ColorWHITE; 
        wingStyle.filled = true;
        
        canvas->save();
        canvas->translate(iconR.centerX(), iconR.centerY());
        canvas->scale(-1.0f, 1.0f); 
        canvas->translate(-iconR.centerX(), -iconR.centerY());
        svgicons::drawIconCentered(canvas, svgicons::IconId::Wing, iconR, 24.0f, wingStyle);
        canvas->restore();
        
        float textX = x + 36.0f;
        float textW = w - 36.0f;
        const float bubbleH = measureMessageHeight(msg, w);
        SkRect wingBubble = SkRect::MakeXYWH(textX - 10.0f, y - 2.0f, textW + 10.0f, bubbleH);

        SkPaint wbFill;
        wbFill.setAntiAlias(true);
        wbFill.setColor(wingui::SURFACE_1);
        canvas->drawRoundRect(wingBubble, wingui::kRadiusCard, wingui::kRadiusCard, wbFill);

        SkPaint wbStroke;
        wbStroke.setAntiAlias(true);
        wbStroke.setStyle(SkPaint::kStroke_Style);
        wbStroke.setStrokeWidth(1.0f);
        wbStroke.setColor(wingui::BORDER_SOFT);
        canvas->drawRoundRect(wingBubble, wingui::kRadiusCard, wingui::kRadiusCard, wbStroke);

        float currentY = y;

        auto& mutableMsg = const_cast<WingmanMessage&>(msg);
        if (mutableMsg.layout.totalHeight == 0 || mutableMsg.cachedWidth != textW) {
            mutableMsg.layout = WingmanTextLayout::layoutText(msg.content, textW, 15.0f);
            mutableMsg.cachedWidth = textW;
        }

        for (const auto& block : mutableMsg.layout.blocks) {
            currentY += block.marginTop;
            drawLayoutBlock(canvas, block, textX, currentY, textW);
            currentY += block.height + block.marginBottom;
        }

        if (msg.reasoning.isNotEmpty()) {
            const float reasoningH = 96.0f;
            const float bubbleGap = 16.0f;
            SkRect bubbleRect = SkRect::MakeXYWH(textX, currentY + bubbleGap,
                                                 textW, reasoningH);
            drawReasoningBubble(canvas, msg, bubbleRect);
        }
    }
}

void WingmanPanel::drawLayoutBlock(SkCanvas* canvas, const LayoutBlock& block, float x, float y, float w) {
    SkPaint paint;
    paint.setAntiAlias(true);

    if (block.type == BlockType::CodeBlock) {
        SkRect r = SkRect::MakeXYWH(x, y, w, block.height);
        SkPaint bg;
        bg.setColor(wingui::SURFACE_1);
        bg.setAntiAlias(true);
        canvas->drawRoundRect(r, wingui::kRadiusChip, wingui::kRadiusChip, bg);
        
        SkPaint border;
        border.setStyle(SkPaint::kStroke_Style);
        border.setColor(wingui::BORDER_SOFT);
        border.setAntiAlias(true);
        canvas->drawRoundRect(r, wingui::kRadiusChip, wingui::kRadiusChip, border);
        
        float py = y + 10.0f;
        paint.setColor(SkColorSetRGB(150, 200, 255));
        for (const auto& line : block.lines) {
            float px = x + 12.0f;
            for (const auto& span : line.spans) {
                canvas->drawString(span.text.c_str(), px, py + span.font.getSize(), span.font, paint);
                px += span.width;
            }
            py += line.height;
        }
        return;
    }

    float py = y;
    for (const auto& line : block.lines) {
        float px = x;
        if (block.type == BlockType::ListBullet) {
            SkPaint bulletPaint;
            bulletPaint.setColor(theme::ACCENT_PRIMARY);
            bulletPaint.setAntiAlias(true);
            canvas->drawCircle(px - 10.0f, py + line.height * 0.5f + 2.0f, 2.5f, bulletPaint);
        }

        for (const auto& span : line.spans) {
            if (block.type == BlockType::Header1 || block.type == BlockType::Header2) {
                paint.setColor(theme::TEXT_HEAD);
            } else if (span.style == SpanStyle::Bold) {
                paint.setColor(SK_ColorWHITE);
            } else if (span.style == SpanStyle::Code) {
                paint.setColor(theme::ACCENT_PRIMARY);
                SkRect r = SkRect::MakeXYWH(px, py + 2, span.width, line.height);
                SkPaint bg; bg.setColor(SkColorSetA(theme::ACCENT_PRIMARY, 30));
                canvas->drawRoundRect(r, 4, 4, bg);
            } else {
                paint.setColor(theme::TEXT_BODY);
            }
            
            canvas->drawString(span.text.c_str(), px, py + span.font.getSize(), span.font, paint);
            px += span.width;
        }
        py += line.height;
    }
}

void WingmanPanel::drawReasoningBubble(SkCanvas* canvas, const WingmanMessage& msg, const SkRect& bubbleRect) {
    if (!canvas) return;

    reasoningBubbleRects_[msg.id.toStdString()] = bubbleRect;

    SkPaint bg;
    bg.setAntiAlias(true);
    bg.setColor(SkColorSetARGB(190, 17, 22, 35));
    canvas->drawRoundRect(bubbleRect, wingui::kRadiusCard, wingui::kRadiusCard, bg);

    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    float hover = (hoveredReasoningId_ == msg.id) ? 1.0f : 0.0f;
    border.setColor(design::interpolateColor(SkColorSetA(SK_ColorWHITE, 18),
                                             SkColorSetA(theme::ACCENT_PRIMARY, 60),
                                             hover));
    border.setStrokeWidth(1.0f);
    canvas->drawRoundRect(bubbleRect, wingui::kRadiusCard, wingui::kRadiusCard, border);

    const float padX = 12.0f;
    const float padY = 10.0f;
    const float labelH = 14.0f;
    SkFont labelFont = design::typography::getSkFont(11.0f);
    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(SkColorSetA(theme::TEXT_MUTED, 200));
    canvas->drawString("Reasoning", bubbleRect.left() + padX,
                       bubbleRect.top() + padY + 10.0f, labelFont, labelPaint);

    SkRect contentRect = SkRect::MakeLTRB(
        bubbleRect.left() + padX,
        bubbleRect.top() + padY + labelH + 4.0f,
        bubbleRect.right() - padX,
        bubbleRect.bottom() - padY);

    float fullHeight = measureReasoningHeight(msg.reasoning, contentRect.width());
    float maxScroll = std::max(0.0f, fullHeight - contentRect.height());
    float& offset = reasoningScrollOffsets_[msg.id.toStdString()];
    offset = juce::jlimit(0.0f, maxScroll, offset);

    canvas->save();
    canvas->clipRect(contentRect);

    SkFont textFont = design::typography::getSkFont(12.0f);
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetA(theme::TEXT_BODY, 200));

    float lineHeight = 18.0f;
    float y = contentRect.top() - offset + lineHeight;

    juce::StringArray logicalLines;
    logicalLines.addLines(msg.reasoning);
    if (logicalLines.isEmpty())
        logicalLines.add(juce::String());

    for (int li = 0; li < logicalLines.size(); ++li) {
        const juce::String line = logicalLines[li];
        if (line.trim().isEmpty()) {
            y += lineHeight;
            continue;
        }

        juce::StringArray words;
        words.addTokens(line, " ", "");
        words.removeEmptyStrings();

        juce::String current;
        for (int wi = 0; wi < words.size(); ++wi) {
            const juce::String& w = words[wi];
            juce::String candidate = current.isEmpty() ? w : (current + " " + w);
            const auto candUtf8 = candidate.toStdString();
            const float candW = textFont.measureText(candUtf8.c_str(), candUtf8.size(),
                                                     SkTextEncoding::kUTF8);

            if (!current.isEmpty() && candW > contentRect.width()) {
                const auto outUtf8 = current.toStdString();
                canvas->drawString(outUtf8.c_str(), contentRect.left(), y, textFont, textPaint);
                y += lineHeight;
                current = w;
            } else {
                current = candidate;
            }
        }

        if (current.isNotEmpty()) {
            const auto outUtf8 = current.toStdString();
            canvas->drawString(outUtf8.c_str(), contentRect.left(), y, textFont, textPaint);
            y += lineHeight;
        }
    }

    canvas->restore();
}

void WingmanPanel::drawReasoningOverlay(SkCanvas* canvas) {
    if (!canvas) return;

    const float w = (float)getWidth();
    const float h = (float)getHeight();
    const float overlayRatio = 0.6f;
    const float overlayH = juce::jlimit(h * 0.5f, h * 0.7f, h * overlayRatio);

    reasoningOverlayRect_ = SkRect::MakeXYWH(0.0f, h - overlayH, w, overlayH);
    reasoningOverlayCloseRect_ = SkRect::MakeXYWH(w - 44.0f, h - overlayH + 14.0f, 28.0f, 28.0f);

    SkPaint dim;
    dim.setColor(SkColorSetA(SK_ColorBLACK, 150));
    canvas->drawRect(SkRect::MakeWH(w, h), dim);

    SkPaint panel;
    panel.setAntiAlias(true);
    panel.setColor(SkColorSetARGB(238, 14, 18, 28));
    canvas->drawRoundRect(reasoningOverlayRect_, wingui::kRadiusPanel, wingui::kRadiusPanel, panel);

    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(wingui::BORDER_SOFT);
    canvas->drawRoundRect(reasoningOverlayRect_, wingui::kRadiusPanel, wingui::kRadiusPanel, border);

    SkFont titleFont = design::typography::getSkFont(13.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setAntiAlias(true);
    titlePaint.setColor(theme::TEXT_HEAD);
    canvas->drawString("Reasoning", reasoningOverlayRect_.left() + 18.0f,
                       reasoningOverlayRect_.top() + 28.0f, titleFont, titlePaint);

    icons::IconStyle closeStyle;
    closeStyle.color = design::interpolateColor(theme::TEXT_MUTED, theme::TEXT_HEAD,
                                                interaction_.settings.hoverAmount);
    closeStyle.strokeWidth = 1.6f;
    icons::drawIconCentered(canvas, icons::Close(), reasoningOverlayCloseRect_, 14.0f, closeStyle);

    const WingmanMessage* target = nullptr;
    for (const auto& msg : messages_) {
        if (msg.id == reasoningOverlayMessageId_) {
            target = &msg;
            break;
        }
    }
    if (target == nullptr) return;

    SkRect contentRect = SkRect::MakeLTRB(
        reasoningOverlayRect_.left() + 18.0f,
        reasoningOverlayRect_.top() + 44.0f,
        reasoningOverlayRect_.right() - 18.0f,
        reasoningOverlayRect_.bottom() - 18.0f);

    float fullHeight = measureReasoningHeight(target->reasoning, contentRect.width());
    float maxScroll = std::max(0.0f, fullHeight - contentRect.height());
    reasoningOverlayScroll_ = juce::jlimit(0.0f, maxScroll, reasoningOverlayScroll_);

    canvas->save();
    canvas->clipRect(contentRect);

    SkFont textFont = design::typography::getSkFont(13.0f);
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(theme::TEXT_BODY);

    float lineHeight = 20.0f;
    float y = contentRect.top() - reasoningOverlayScroll_ + lineHeight;

    juce::StringArray logicalLines;
    logicalLines.addLines(target->reasoning);
    if (logicalLines.isEmpty())
        logicalLines.add(juce::String());

    for (int li = 0; li < logicalLines.size(); ++li) {
        const juce::String line = logicalLines[li];
        if (line.trim().isEmpty()) {
            y += lineHeight;
            continue;
        }

        juce::StringArray words;
        words.addTokens(line, " ", "");
        words.removeEmptyStrings();

        juce::String current;
        for (int wi = 0; wi < words.size(); ++wi) {
            const juce::String& w = words[wi];
            juce::String candidate = current.isEmpty() ? w : (current + " " + w);
            const auto candUtf8 = candidate.toStdString();
            const float candW = textFont.measureText(candUtf8.c_str(), candUtf8.size(),
                                                     SkTextEncoding::kUTF8);

            if (!current.isEmpty() && candW > contentRect.width()) {
                const auto outUtf8 = current.toStdString();
                canvas->drawString(outUtf8.c_str(), contentRect.left(), y, textFont, textPaint);
                y += lineHeight;
                current = w;
            } else {
                current = candidate;
            }
        }

        if (current.isNotEmpty()) {
            const auto outUtf8 = current.toStdString();
            canvas->drawString(outUtf8.c_str(), contentRect.left(), y, textFont, textPaint);
            y += lineHeight;
        }
    }

    canvas->restore();
}

void WingmanPanel::openReasoningOverlay(const juce::String& messageId) {
    reasoningOverlayPrevScroll_ = scrollOffset_;
    reasoningOverlayMessageId_ = messageId;
    reasoningOverlayScroll_ = 0.0f;
    reasoningOverlayOpen_ = true;
    repaint();
}

void WingmanPanel::closeReasoningOverlay() {
    reasoningOverlayOpen_ = false;
    reasoningOverlayMessageId_.clear();
    reasoningOverlayScroll_ = 0.0f;
    scrollOffset_ = reasoningOverlayPrevScroll_;
    repaint();
}

float WingmanPanel::measureReasoningHeight(const juce::String& reasoning, float width) const {
    SkFont font = design::typography::getSkFont(12.0f);
    float lineHeight = 18.0f;
    float pad = 8.0f;
    int lines = std::max(1, countWrappedLines(reasoning, width, font));
    return pad + lines * lineHeight + pad;
}

float WingmanPanel::measureMessageHeight(const WingmanMessage& msg, float width) {
    if (msg.sender == "User") {
        SkFont font = design::typography::getSkFont(15.0f);
        const float lineH = 22.0f;
        const float topPad = 10.0f;
        const float bottomPad = 10.0f;
        float textMaxW = std::max(60.0f, width - 32.0f);
        int lines = std::max(1, countWrappedLines(msg.content, textMaxW, font));
        return topPad + lines * lineH + bottomPad;
    }

    if (msg.sender == "Wingman") {
        auto& mutableMsg = const_cast<WingmanMessage&>(msg);
        float textMaxW = std::max(60.0f, width - 36.0f);

        if (mutableMsg.layout.totalHeight == 0 || mutableMsg.cachedWidth != textMaxW) {
            mutableMsg.layout = WingmanTextLayout::layoutText(msg.content, textMaxW, 15.0f);
            mutableMsg.cachedWidth = textMaxW;
        }

        float totalH = mutableMsg.layout.totalHeight;
        
        if (msg.reasoning.isNotEmpty()) {
            totalH += 96.0f + 32.0f; // reasoning bubble height + padding
        }
        
        return totalH + 20.0f; // Padding
    }

    return 40.0f;
}

void WingmanPanel::drawWrappedText(SkCanvas* canvas, const juce::String& text, float x, float startY, float maxWidth, const SkFont& font, const SkPaint& paint) {
    // Word wrap with forced break for long words
    if (!canvas || maxWidth <= 1.0f) return;

    const float lineHeight = 22.0f;
    float y = startY;

    juce::StringArray logicalLines;
    logicalLines.addLines(text);
    if (logicalLines.isEmpty())
        logicalLines.add(juce::String());

    for (int li = 0; li < logicalLines.size(); ++li) {
        const juce::String line = logicalLines[li];
        if (line.trim().isEmpty()) {
            y += lineHeight;
            continue;
        }

        juce::StringArray words;
        words.addTokens(line, " ", "");
        words.removeEmptyStrings();

        juce::String current;
        for (int wi = 0; wi < words.size(); ++wi) {
            const juce::String& w = words[wi];
            
            // Measure single word to see if it needs forced breaking
            const auto wUtf8 = w.toStdString();
            const float wW = font.measureText(wUtf8.c_str(), wUtf8.size(), SkTextEncoding::kUTF8);
            
            if (wW > maxWidth) {
                // Flush current buffer
                if (current.isNotEmpty()) {
                    const auto outUtf8 = current.toStdString();
                    canvas->drawString(outUtf8.c_str(), x, y, font, paint);
                    y += lineHeight;
                    current.clear();
                }
                
                // Forced break loop
                std::string chunk;
                for (char c : wUtf8) {
                    std::string nextChunk = chunk + c;
                    float chunkW = font.measureText(nextChunk.c_str(), nextChunk.size(), SkTextEncoding::kUTF8);
                    
                    if (chunkW > maxWidth && !chunk.empty()) {
                        canvas->drawString(chunk.c_str(), x, y, font, paint);
                        y += lineHeight;
                        chunk = std::string(1, c);
                    } else {
                        chunk = nextChunk;
                    }
                }
                
                // Keep remainder in 'current' for next iteration (might append next word to it)
                if (!chunk.empty()) {
                    current = juce::String(chunk);
                }
                continue;
            }

            juce::String candidate = current.isEmpty() ? w : (current + " " + w);
            const auto candUtf8 = candidate.toStdString();
            const float candW = font.measureText(candUtf8.c_str(), candUtf8.size(), SkTextEncoding::kUTF8);

            if (!current.isEmpty() && candW > maxWidth) {
                const auto outUtf8 = current.toStdString();
                canvas->drawString(outUtf8.c_str(), x, y, font, paint);
                y += lineHeight;
                current = w;
            } else {
                current = candidate;
            }
        }

        if (current.isNotEmpty()) {
            const auto outUtf8 = current.toStdString();
            canvas->drawString(outUtf8.c_str(), x, y, font, paint);
            y += lineHeight;
        }
    }
}

int WingmanPanel::countWrappedLines(const juce::String& text, float maxWidth, const SkFont& font) const {
    if (text.trim().isEmpty())
        return 1;

    if (maxWidth <= 1.0f)
        return 1;

    juce::StringArray logicalLines;
    logicalLines.addLines(text);
    if (logicalLines.isEmpty())
        logicalLines.add(juce::String());

    int total = 0;
    for (int li = 0; li < logicalLines.size(); ++li) {
        const juce::String line = logicalLines[li];
        if (line.trim().isEmpty()) {
            total += 1;
            continue;
        }

        juce::StringArray words;
        words.addTokens(line, " ", "");
        words.removeEmptyStrings();

        juce::String current;
        for (int wi = 0; wi < words.size(); ++wi) {
            const juce::String& w = words[wi];
            juce::String candidate = current.isEmpty() ? w : (current + " " + w);

            const auto candUtf8 = candidate.toStdString();
            const float candW = font.measureText(candUtf8.c_str(), candUtf8.size(), SkTextEncoding::kUTF8);

            if (!current.isEmpty() && candW > maxWidth) {
                total += 1;
                current = w;
            } else {
                current = candidate;
            }

            // Extremely long tokens: fall back to proportional splitting
            if (current == w) {
                const auto wUtf8 = w.toStdString();
                const float wW = font.measureText(wUtf8.c_str(), wUtf8.size(), SkTextEncoding::kUTF8);
                if (wW > maxWidth && maxWidth > 1.0f) {
                    const int extra = (int)std::ceil(wW / maxWidth) - 1;
                    if (extra > 0) {
                        total += extra;
                    }
                }
            }
        }

        if (current.isNotEmpty())
            total += 1;
    }

    return std::max(1, total);
}

} // namespace zenith
