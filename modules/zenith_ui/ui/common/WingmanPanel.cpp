/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "WingmanPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
#include "../design-system/SvgIcon.h"
#include "zenith_core/engine/ZenithLogger.h"
#include "../../network/SecureKeyStore.h"
#include "zenith_core/engine/Settings.h"
#include "../dialogs/SettingsComponent.h"
#include <juce_core/juce_core.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <cstring>
#include <cmath>
#include <algorithm>

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
        
        svgicons::Style style;
        style.color = design::interpolateColor(theme::TEXT_MUTED, theme::TEXT_HEAD, hoverAmount);
        style.glowRadius = 8.0f * hoverAmount;
        style.glowColor = theme::ACCENT_PRIMARY;
        
        svgicons::drawIconCentered(canvas, svgicons::IconId::Settings, bounds, 20.0f, style);
    }

private:
    float hoverAmount = 0.0f;
    bool isHovered = false;
};

//==============================================================================
// Constructor
//==============================================================================

WingmanPanel::WingmanPanel(CommandAPI& api, Engine& engine)
    : commandAPI_(api), engine_(engine),
      edgeSwipe_(),
      sidebarSpring_(animation::SpringConfig::responsive()) {
    
    // Configure edge swipe detector
    gesture::EdgeSwipeDetector::Config edgeConfig;
    edgeConfig.edge = gesture::EdgeSwipeDetector::Edge::Left;
    edgeConfig.edgeWidth = 24.0f;
    edgeConfig.minVelocity = 250.0f;
    edgeConfig.minDistance = 60.0f;
    edgeConfig.maxPerpendicular = 120.0f;
    edgeSwipe_ = gesture::EdgeSwipeDetector(edgeConfig);
    
    grokController_ = std::make_unique<GrokDAWController>(commandAPI_);
    grokController_->initialize();
    
    // Configure sidebar drag controller
    gesture::PanelDragController::Config sidebarConfig;
    sidebarConfig.minPosition = 0.0f;           // closed
    sidebarConfig.maxPosition = kSidebarWidth;  // open
    sidebarConfig.snapThreshold = 0.35f;
    sidebarConfig.velocityThreshold = 350.0f;
    sidebarConfig.rubberBandFactor = 0.25f;
    sidebarDrag_.setConfig(sidebarConfig);
    
    // Configure session reorder
    sessionReorder_.onReorder = [this](int from, int to) {
        if (from >= 0 && from < (int)sessions_.size() &&
            to >= 0 && to <= (int)sessions_.size() && from != to) {
            auto session = std::move(sessions_[(size_t)from]);
            sessions_.erase(sessions_.begin() + from);
            if (to > from) to--;
            sessions_.insert(sessions_.begin() + to, std::move(session));
            rebuildSessionGroups();
            saveSessionsToDisk();
        }
    };
    
    // Input Field: Frameless text input (container drawn separately)
    textInput_ = std::make_unique<SkiaTextInput>();
    textInput_->setPlaceholder("Ask Wingman...");
    textInput_->setPillShape(false);
    textInput_->setFontSize(15.0f);
    textInput_->setDrawBackground(false);
    textInput_->setTextColor(theme::TEXT_HEAD);
    textInput_->setPlaceholderColor(theme::TEXT_MUTED);
    textInput_->setCursorColor(theme::ACCENT_PRIMARY);
    textInput_->onReturnKey = [this] { sendMessage(); };
    
    addAndMakeVisible(textInput_.get());

    renameInput_ = std::make_unique<SkiaTextInput>();
    renameInput_->setPlaceholder("Rename session");
    renameInput_->setPillShape(false);
    renameInput_->setFontSize(13.0f);
    renameInput_->setDrawBackground(false);
    renameInput_->setTextColor(theme::TEXT_HEAD);
    renameInput_->setPlaceholderColor(theme::TEXT_MUTED);
    renameInput_->setCursorColor(theme::ACCENT_PRIMARY);
    renameInput_->onReturnKey = [this] { commitRenameSession(); };
    renameInput_->onEscapeKey = [this] { cancelRenameSession(); };
    addChildComponent(renameInput_.get());

    settingsPanel_ = std::make_unique<SettingsComponent>(engine);
    addChildComponent(settingsPanel_.get());
    settingsPanel_->setVisible(false);
    settingsPanel_->setChatReasoningStateProvider([this]() {
        if (auto* session = getActiveSessionPtr()) {
            return session->reasoningEnabled;
        }
        return false;
    });
    settingsPanel_->setChatGoToChatAction([this]() {
        settingsOpen_ = false;
        settingsTarget_ = 0.0f;
        if (settingsPanel_) {
            settingsPanel_->setVisible(true);
        }
        highlightReasoningButton();
        repaint();
    });

    // Overlay Button for Settings (Always on top)
    settingsOverlayBtn_ = std::make_unique<SettingsOverlayButton>();
    settingsOverlayBtn_->onClick = [this] { toggleSettings(); };
    addChildComponent(settingsOverlayBtn_.get());
    settingsOverlayBtn_->setVisible(true);

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

    if (!grokController_->isReady() && !awaitingApiKey_) {
        grokController_->initialize();
    }

    if (awaitingApiKey_) {
        bool ok = grokController_->initialize(trimmed);
        WingmanMessage statusMsg;
        statusMsg.id = juce::Uuid().toString();
        statusMsg.sender = "Wingman";
        statusMsg.content = ok
            ? "Key saved. Ask me anything."
            : "That key did not work. Paste a valid Grok API key.";
        statusMsg.timestamp = juce::Time::getCurrentTime().toMilliseconds();
        if (ok) awaitingApiKey_ = false;
        messages_.push_back(statusMsg);
        syncActiveSessionMessages();
        saveSessionsToDisk();
        recalcChatMetrics();
        scrollToBottom();
        repaint();
        return;
    }

    if (!grokController_->isReady()) {
        WingmanMessage statusMsg;
        statusMsg.id = juce::Uuid().toString();
        statusMsg.sender = "Wingman";
        statusMsg.content =
            "Grok API key missing. Paste your key to connect (it will not be shown).";
        statusMsg.timestamp = juce::Time::getCurrentTime().toMilliseconds();
        awaitingApiKey_ = true;
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
    
    // Get AI settings
    auto modelMode = Settings::getInstance().getAIModelMode();
    
    grokController_->executeCommandWithReasoning(
        trimmed,
        isReasoningMode_ ? GrokMode::Thinking : GrokMode::Fast,
        [safeThis](juce::String response, juce::String reasoning) {
            juce::MessageManager::callAsync([safeThis, response, reasoning]() {
                if (!safeThis) return;
                
                const bool wasAtBottom = safeThis->isScrolledToBottom();
                
                safeThis->receiveMessage(response, reasoning);
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
        },
        modelMode);
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

//==============================================================================
// STAGGERED SESSION ANIMATIONS
//==============================================================================

void WingmanPanel::animateSessionsIn() {
    // Resize animation array to match sessions
    sessionRevealAmounts_.resize(sessions_.size());
    
    // Stagger the animations with 40ms delay between each
    const float staggerDelayMs = 40.0f;
    
    for (size_t i = 0; i < sessions_.size(); ++i) {
        auto& reveal = sessionRevealAmounts_[i];
        reveal.set(0.0f);  // Start hidden
        
        // Delay animation start based on index
        // Using timer callback would be ideal, but for now we use initial value offset
        reveal.setTarget(1.0f, 250 + (int)(i * staggerDelayMs), animation::Easing::EaseOutCubic);
    }
}

void WingmanPanel::rebuildSessionGroups() {
    sessionGroups_.clear();
    
    juce::Time now = juce::Time::getCurrentTime();
    juce::Time todayStart = juce::Time(now.getYear(), now.getMonth(), now.getDayOfMonth(), 0, 0, 0);
    juce::Time yesterdayStart = todayStart - juce::RelativeTime::days(1);
    juce::Time thisWeekStart = todayStart - juce::RelativeTime::days(now.getDayOfWeek());
    juce::Time thisMonthStart = juce::Time(now.getYear(), now.getMonth(), 1, 0, 0, 0);
    
    SessionGroup today, yesterday, thisWeek, thisMonth, older;
    today.label = "Today";
    yesterday.label = "Yesterday";
    thisWeek.label = "This Week";
    thisMonth.label = "This Month";
    older.label = "Older";
    
    for (size_t i = 0; i < sessions_.size(); ++i) {
        const auto& session = sessions_[i];
        
        // Parse session date (approximate from dateLabel)
        // In a real implementation, store actual timestamp
        juce::Time sessionTime = juce::Time::getCurrentTime(); // Fallback
        
        // Simple heuristic based on dateLabel content
        if (session.dateLabel.containsIgnoreCase("today") || 
            session.dateLabel.startsWith(now.formatted("%d %b"))) {
            today.sessionIndices.push_back((int)i);
        } else if (session.dateLabel.containsIgnoreCase("yesterday")) {
            yesterday.sessionIndices.push_back((int)i);
        } else {
            // For now, put in "older"
            older.sessionIndices.push_back((int)i);
        }
    }
    
    // Only add non-empty groups
    if (!today.sessionIndices.empty()) sessionGroups_.push_back(std::move(today));
    if (!yesterday.sessionIndices.empty()) sessionGroups_.push_back(std::move(yesterday));
    if (!thisWeek.sessionIndices.empty()) sessionGroups_.push_back(std::move(thisWeek));
    if (!thisMonth.sessionIndices.empty()) sessionGroups_.push_back(std::move(thisMonth));
    if (!older.sessionIndices.empty()) sessionGroups_.push_back(std::move(older));
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
            sidebarOpen_ = false;
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
    
    // ==========================================================================
    // RESPONSIVE LAYOUT MODE SELECTION
    // ==========================================================================
    LayoutMode prevMode = layoutMode_;
    if (w < kCompactBreakpoint) {
        layoutMode_ = LayoutMode::Compact;
    } else if (w >= kExpandedBreakpoint) {
        layoutMode_ = LayoutMode::Expanded;
    } else {
        layoutMode_ = LayoutMode::Standard;
    }
    
    // Handle mode transitions
    if (prevMode != layoutMode_) {
        if (layoutMode_ == LayoutMode::Expanded) {
            // Force sidebar open in expanded mode
            sidebarOpen_ = true;
            sidebarTarget_ = 1.0f;
        } else if (layoutMode_ == LayoutMode::Compact && !historyToggledByUser_) {
            // Auto-close in compact mode
            sidebarOpen_ = false;
            sidebarTarget_ = 0.0f;
        }
    }
    
    // Update spring target
    float targetSidebarWidth = sidebarOpen_ ? kSidebarWidth : 0.0f;
    sidebarSpring_.setTarget(targetSidebarWidth);
    
    // Get current animated sidebar width (from spring or drag)
    float sidebarW;
    if (sidebarDragging_) {
        sidebarW = sidebarDrag_.getState().position;
    } else {
        sidebarW = sidebarSpring_.getPosition();
    }
    sidebarW = juce::jlimit(0.0f, kSidebarWidth, sidebarW);
    sidebarOpenAmount_ = sidebarW / kSidebarWidth;
    
    // ==========================================================================
    // LAYOUT GEOMETRY BY MODE
    // ==========================================================================
    
    switch (layoutMode_) {
        case LayoutMode::Expanded: {
            // Persistent docked sidebar - content always shifted
            layout_.sidebarRect = SkRect::MakeXYWH(0, 0, kSidebarWidth, h);
            layout_.contentOffsetX = kSidebarWidth;
            layout_.contentRect = SkRect::MakeXYWH(kSidebarWidth, 0, w - kSidebarWidth, h);
            layout_.showBackdrop = false;
            break;
        }
        
        case LayoutMode::Standard: {
            // Push-to-reveal: content shifts right as sidebar opens
            layout_.sidebarRect = SkRect::MakeXYWH(0, 0, sidebarW, h);
            layout_.contentOffsetX = sidebarW;
            layout_.contentRect = SkRect::MakeXYWH(sidebarW, 0, w - sidebarW, h);
            layout_.showBackdrop = false;
            break;
        }
        
        case LayoutMode::Compact: {
            // Sheet-style overlay with backdrop blur
            layout_.sidebarRect = SkRect::MakeXYWH(0, 0, std::min(sidebarW, w * 0.85f), h);
            layout_.contentOffsetX = 0;  // Content doesn't move
            layout_.contentRect = SkRect::MakeXYWH(0, 0, w, h);
            layout_.showBackdrop = sidebarOpenAmount_ > 0.01f;
            layout_.backdropRect = SkRect::MakeXYWH(0, 0, w, h);
            break;
        }
    }
    
    // Edge swipe zone (left edge of screen)
    layout_.sidebarEdgeZone = SkRect::MakeXYWH(0, 0, 24.0f, h);
    
    // ==========================================================================
    // CONTENT LAYOUT (relative to contentRect)
    // ==========================================================================
    float contentX = layout_.contentRect.left();
    float contentW = layout_.contentRect.width();
    
    // Settings Panel Layout
    if (settingsPanel_) {
        float panelX = w * (1.0f - settingsSlideAmount_);
        settingsPanel_->setBounds((int)panelX, 0, (int)w, (int)h);
    }
    
    // Header Buttons (Floating, Top Right of content area)
    float btnSize = 32.0f;
    float pad = 20.0f;
    
    float settingsX = contentX + contentW - pad - btnSize;
    float historyX = contentX + pad;
    layout_.settingsBtn = SkRect::MakeXYWH(settingsX, pad, btnSize, btnSize);

    // Sync Overlay Button
    if (settingsOverlayBtn_) {
        settingsOverlayBtn_->setBounds((int)settingsX, (int)pad, (int)btnSize, (int)btnSize);
        settingsOverlayBtn_->toFront(true);
    }
    
    // Hide toggle button in Expanded mode (sidebar always visible)
    if (layoutMode_ == LayoutMode::Expanded) {
        layout_.toggleSidebarBtn = SkRect::MakeEmpty();
    } else {
        layout_.toggleSidebarBtn = SkRect::MakeXYWH(historyX, pad, btnSize, btnSize);
    }
    layout_.newChatBtn = SkRect::MakeXYWH(historyX + btnSize + 10.0f, pad, btnSize, btnSize);
    
    // Input Island (Floating Bottom, centered in content area)
    float inputW = std::min(720.0f, std::max(240.0f, contentW - 48.0f));
    float inputH = 50.0f;
    float inputBottomMargin = 26.0f;
    
    float inputX = contentX + (contentW - inputW) / 2.0f;
    float inputY = h - inputH - inputBottomMargin;
    
    layout_.inputContainerRect = SkRect::MakeXYWH(inputX, inputY, inputW, inputH);
    
    // Buttons inside input (Lightbulb Left, Send Right)
    layout_.brainBtn = SkRect::MakeXYWH(inputX + 10.0f, inputY + 9.0f, 32.0f, 32.0f);
    layout_.sendBtn = SkRect::MakeXYWH(inputX + inputW - 42.0f, inputY + 9.0f, 32.0f, 32.0f);
    
    // Text Input Rect
    layout_.inputFieldRect = SkRect::MakeXYWH(inputX + 50.0f, inputY + 8.0f, inputW - 100.0f, inputH - 16.0f);
    
    if (textInput_) {
        textInput_->setBounds((int)layout_.inputFieldRect.left(), (int)layout_.inputFieldRect.top(),
                              (int)layout_.inputFieldRect.width(), (int)layout_.inputFieldRect.height());
    }
    
    // Chat Area (within content rect)
    layout_.chatRect = SkRect::MakeLTRB(contentX, 80.0f, contentX + contentW, inputY - 20.0f);

    float chatContentPad = (contentW < 360.0f) ? 16.0f : 40.0f;
    float contentMax = std::max(0.0f, contentW - chatContentPad * 2.0f);

    // Centered reading column (used by both chat stream and empty state)
    layout_.chatContentW = std::min(680.0f, contentMax);
    layout_.chatContentW = std::max(180.0f, layout_.chatContentW);
    if (layout_.chatContentW > contentMax) layout_.chatContentW = contentMax;
    if (contentW < 360.0f) {
        layout_.chatContentX = layout_.chatRect.left() + chatContentPad;
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
    sessionStates_.resize(sessions_.size());
    editStates_.resize(sessions_.size());
    deleteStates_.resize(sessions_.size());

    if (sidebarW > 1.0f) {
        float y = 78.0f;
        const float itemH = 32.0f;
        const float itemGap = 6.0f;
        const float itemX = 12.0f;
        const float itemW = std::max(0.0f, sidebarW - 24.0f);
        const float iconSize = 16.0f;
        const float iconPad = 8.0f;

        for (size_t i = 0; i < sessions_.size(); ++i) {
            SkRect itemRect = SkRect::MakeXYWH(itemX, y - 18.0f, itemW, itemH);
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
            y += itemH + itemGap;
        }
    }

    if (renameInput_) {
        if (editingSessionIndex_ >= 0 &&
            (size_t)editingSessionIndex_ < layout_.sessionItemRects.size()) {
            SkRect itemRect = layout_.sessionItemRects[(size_t)editingSessionIndex_];
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
    const float heroH = (contentW < 360.0f) ? 260.0f : 300.0f;
    float heroX = layout_.chatRect.left() + (layout_.chatRect.width() - heroW) * 0.5f;
    float heroOffset = (contentW < 360.0f) ? 90.0f : 120.0f;
    float heroY = layout_.chatRect.centerY() - heroH * 0.5f - heroOffset;
    heroY = juce::jlimit(layout_.chatRect.top() + 20.0f, layout_.chatRect.bottom() - heroH - 20.0f, heroY);
    layout_.emptyHeroCard = SkRect::MakeXYWH(heroX, heroY, heroW, heroH);

    // Suggestion chips (stacked: works on narrow and wide)
    // We now layout dynamic suggestions based on active session
    layout_.suggestionChips.clear();
    layout_.regenerateBtn = SkRect::MakeEmpty();
    layout_.suggestionsAreaRect = SkRect::MakeEmpty();
    
    if (auto* session = getActiveSessionPtr()) {
        // Requirement: Only show suggestions if chat is empty
        if (session->messages.empty()) {
            const float chipH = 32.0f;
            const float chipGapX = 8.0f;
            const float chipGapY = 8.0f;
            
            // Layout inside the Hero Card (Bubble)
            // Bottom of hero card minus padding
            const float areaBot = layout_.emptyHeroCard.bottom() - 20.0f;
            
            float regenSize = 24.0f;
            float currentX = 0.0f; // relative
            float currentY = 0.0f; // relative
            float rowHeight = chipH;
            
            // Constrain width to fit inside hero card with padding
            float constraintsW = std::min(440.0f, layout_.emptyHeroCard.width() - 80.0f); // Narrower for vertical stack looks better
            
            SkFont chipFont = design::typography::getSkFont(13.0f);
            std::vector<SkRect> relativeRects;
            
            // Calculate layout: Vertical Stack
            for (auto& s : session->suggestions) {
                // Ignore label width for sizing, we use full constraint width for uniform look
                // But we could auto-size to largest content?
                // Let's use full width for clean vertical stack.
                
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
            float regenX = startX - regenSize - 8.0f;
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

    if (layout_.sessionItemRects.size() == sessionStates_.size()) {
        for (size_t i = 0; i < layout_.sessionItemRects.size(); ++i) {
            check(sessionStates_[i].isHovered, layout_.sessionItemRects[i]);
            check(editStates_[i].isHovered, layout_.sessionEditHitRects[i]);
            check(deleteStates_[i].isHovered, layout_.sessionDeleteHitRects[i]);
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
    float w = (float)getWidth();
    float h = (float)getHeight();

    if (reasoningOverlayOpen_) {
        if (reasoningOverlayCloseRect_.contains(x, y) ||
            !reasoningOverlayRect_.contains(x, y)) {
            closeReasoningOverlay();
        }
        return;
    }
    
    // ==========================================================================
    // EDGE SWIPE DETECTION (for opening sidebar)
    // ==========================================================================
    if (!sidebarOpen_ && layoutMode_ != LayoutMode::Expanded) {
        if (edgeSwipe_.onDown({ x, y }, w, h)) {
            sidebarDragging_ = true;
            sidebarDrag_.beginDrag(0.0f);
            return;
        }
    }
    
    // ==========================================================================
    // BACKDROP TAP TO CLOSE (Compact mode)
    // ==========================================================================
    if (layoutMode_ == LayoutMode::Compact && sidebarOpen_ && 
        x > layout_.sidebarRect.right()) {
        sidebarOpen_ = false;
        sidebarSpring_.setTarget(0.0f);
        historyToggledByUser_ = true;
        repaint();
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

    if (!layout_.toggleSidebarBtn.isEmpty() && layout_.toggleSidebarBtn.contains(x,y)) {
        interaction_.sidebarToggle.isPressed = true;
        sidebarOpen_ = !sidebarOpen_;
        sidebarSpring_.setTarget(sidebarOpen_ ? kSidebarWidth : 0.0f);
        historyToggledByUser_ = true;
        
        // Trigger staggered session animations when opening
        if (sidebarOpen_) {
            animateSessionsIn();
        }
        resized();
        repaint();
    }
    else if (layout_.newChatBtn.contains(x, y)) {
        interaction_.newChat.isPressed = true;
        sidebarOpen_ = false;
        sidebarSpring_.setTarget(0.0f);
        sidebarSpring_.setPosition(0.0f);
        historyToggledByUser_ = true;
        updateLayout();
        createNewSession();
        resized();
        repaint();
    }
    else if (sidebarOpenAmount_ > 0.2f && !layout_.sessionItemRects.empty()) {
        for (size_t i = 0; i < layout_.sessionItemRects.size(); ++i) {
            if (layout_.sessionDeleteHitRects[i].contains(x, y)) {
                if (e.mods.isShiftDown()) {
                    deleteSessionAt((int)i);
                    return;
                }

                uint32_t nowMs = juce::Time::getMillisecondCounter();
                if ((int)i == pendingDeleteIndex_ && nowMs < pendingDeleteUntilMs_) {
                    deleteSessionAt((int)i);
                    return;
                }

                pendingDeleteIndex_ = (int)i;
                pendingDeleteUntilMs_ = nowMs + 2000;
                repaint();
                return;
            }
            if (layout_.sessionEditHitRects[i].contains(x, y)) {
                if (editingSessionIndex_ == (int)i && renameInput_) {
                    renameInput_->grabKeyboardFocus();
                    return;
                }
                beginRenameSession((int)i);
                return;
            }
            if (layout_.sessionItemRects[i].contains(x, y)) {
                // Start session swipe detection
                sessionSwipe_.onDown({ x, y });
                swipingSessionIndex_ = (int)i;
                
                // Also start potential drag-to-reorder
                sessionReorder_.onDown({ x, y }, (int)i, 
                    juce::Time::getMillisecondCounterHiRes());
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

    if (!layout_.sidebarRect.contains(x, y) ||
        (pendingDeleteIndex_ >= 0 &&
         (size_t)pendingDeleteIndex_ < layout_.sessionDeleteRects.size() &&
         !layout_.sessionDeleteRects[(size_t)pendingDeleteIndex_].contains(x, y))) {
        pendingDeleteIndex_ = -1;
    }
}
void WingmanPanel::mouseUp(const juce::MouseEvent&) {
    // ==========================================================================
    // SIDEBAR EDGE SWIPE COMPLETION
    // ==========================================================================
    if (sidebarDragging_ && edgeSwipe_.isTracking()) {
        bool shouldOpen = edgeSwipe_.onUp() || sidebarDrag_.endDrag();
        sidebarOpen_ = shouldOpen;
        sidebarSpring_.setTarget(shouldOpen ? kSidebarWidth : 0.0f);
        
        // Transfer velocity to spring for natural momentum
        float velocity = sidebarDrag_.getVelocity();
        sidebarSpring_.setVelocity(velocity);
        
        if (shouldOpen) {
            animateSessionsIn();
        }
        
        historyToggledByUser_ = true;
        edgeSwipe_.reset();
        sidebarDragging_ = false;
        repaint();
    }
    
    // ==========================================================================
    // SESSION SWIPE ACTION COMPLETION
    // ==========================================================================
    if (swipingSessionIndex_ >= 0) {
        auto action = sessionSwipe_.onUp();
        
        if (action == gesture::SwipeActionDetector::Direction::Left) {
            // Swipe left = delete
            deleteSessionAt(swipingSessionIndex_);
        } else if (action == gesture::SwipeActionDetector::Direction::Right) {
            // Swipe right = rename
            beginRenameSession(swipingSessionIndex_);
        } else if (!sessionSwipe_.getState().isLocked) {
            // No swipe action - treat as tap to select
            sidebarOpen_ = false;
            sidebarSpring_.setTarget(0.0f);
            historyToggledByUser_ = true;
            setActiveSessionIndex(swipingSessionIndex_);
            saveSessionsToDisk();
            updateLayout();
        }
        
        swipingSessionIndex_ = -1;
        sessionSwipe_.reset();
        repaint();
    }
    
    // ==========================================================================
    // SESSION DRAG-TO-REORDER COMPLETION
    // ==========================================================================
    if (reorderingSessionIndex_ >= 0) {
        sessionReorder_.onUp();
        reorderingSessionIndex_ = -1;
        repaint();
    }
    
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
    auto p = e.getPosition();
    float x = (float)p.x; 
    float y = (float)p.y;
    double timestamp = juce::Time::getMillisecondCounterHiRes();
    
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
    
    // ==========================================================================
    // SIDEBAR EDGE SWIPE DRAG
    // ==========================================================================
    if (sidebarDragging_ && edgeSwipe_.isTracking()) {
        // Update drag position
        auto state = sidebarDrag_.updateDrag(x, timestamp);
        sidebarSpring_.setPosition(state.position);
        updateLayout();
        repaint();
        return;
    }
    
    // ==========================================================================
    // SESSION SWIPE ACTIONS
    // ==========================================================================
    if (swipingSessionIndex_ >= 0 && sessionSwipe_.isActive()) {
        auto state = sessionSwipe_.onMove({ x, y });
        if (!state.isLocked && sessionReorder_.getState().isDragging) {
            // Switched to drag-reorder mode
            swipingSessionIndex_ = -1;
            sessionSwipe_.reset();
        } else {
            repaint();
            return;
        }
    }
    
    // ==========================================================================
    // SESSION DRAG-TO-REORDER
    // ==========================================================================
    if (reorderingSessionIndex_ >= 0 || sessionReorder_.getState().isDragging) {
        float listTop = 78.0f;
        float listBottom = (float)getHeight() - 100.0f;
        auto state = sessionReorder_.onMove({ x, y }, listTop, listBottom, timestamp);
        
        if (state.isDragging) {
            reorderingSessionIndex_ = state.sourceIndex;
        }
        repaint();
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

    // 1. Background (subtle gradient + vignette)
    const SkRect bounds = SkRect::MakeWH((float)getWidth(), (float)getHeight());

    SkColor stops[] = {
        SkColorSetRGB(10, 10, 14),
        theme::BG_VOID
    };
    SkPoint pts[] = { {0.0f, 0.0f}, {0.0f, bounds.height()} };
    SkPaint bg;
    bg.setShader(SkGradientShader::MakeLinear(pts, stops, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(bounds, bg);

    // ANIMATION TRANSFORM: CHAT SLIDES LEFT (for settings panel)
    canvas->save();
    canvas->translate(-getWidth() * settingsSlideAmount_, 0);

    SkColor vignetteColors[] = {
        SkColorSetA(SK_ColorBLACK, 0),
        SkColorSetA(SK_ColorBLACK, 120)
    };
    SkPoint center = { bounds.centerX(), bounds.centerY() };
    SkPaint vignette;
    vignette.setShader(SkGradientShader::MakeRadial(center, std::max(bounds.width(), bounds.height()) * 0.75f,
                                                   vignetteColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(bounds, vignette);
    
    // 2. Draw content first (so sidebar can overlay in Compact mode)
    canvas->save();
    canvas->clipRect(layout_.contentRect);
    drawChatStream(canvas);
    drawChatScrollbar(canvas);
    drawInputIsland(canvas);
    drawHeader(canvas);
    canvas->restore();
    
    // 3. Backdrop for Compact mode (dims content behind sidebar)
    if (layout_.showBackdrop && sidebarOpenAmount_ > 0.01f) {
        float backdropAlpha = sidebarOpenAmount_ * 0.6f;
        
        // Draw scrim with subtle blur effect
        BackdropBlur::drawBlurredPanel(canvas, layout_.backdropRect, 
                                        0.0f,  // No corner radius
                                        8.0f * sidebarOpenAmount_,  // Blur radius
                                        SK_ColorBLACK, backdropAlpha,
                                        false);  // No highlight
    }
    
    // 4. Sidebar (on top in Compact mode, pushed in Standard/Expanded)
    if (sidebarOpenAmount_ > 0.01f) {
        drawSidebar(canvas);
    }
    
    canvas->restore(); // Restore the main translation (chat slide)
    
    // Draw children (SettingsPanel, etc.) in untransformed coordinate space
    drawChildren(canvas);

    if (reasoningOverlayOpen_) {
        drawReasoningOverlay(canvas);
    }
}

void WingmanPanel::drawSidebar(SkCanvas* canvas) {
    float sidebarAlpha = juce::jlimit(0.0f, 1.0f, sidebarOpenAmount_);
    const SkRect& sidebarRect = layout_.sidebarRect;
    
    // ==========================================================================
    // MULTI-LAYER SHADOW (Compact mode - sidebar floats over content)
    // ==========================================================================
    
    if (layoutMode_ == LayoutMode::Compact && sidebarAlpha > 0.01f) {
        // Layer 1: Deep ambient shadow
        SkPaint shadow1;
        shadow1.setAntiAlias(true);
        shadow1.setColor(SkColorSetA(SK_ColorBLACK, (uint8_t)(80 * sidebarAlpha)));
        shadow1.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 40.0f));
        canvas->drawRoundRect(sidebarRect.makeOffset(20, 0), 16.0f, 16.0f, shadow1);
        
        // Layer 2: Mid-range shadow
        SkPaint shadow2;
        shadow2.setAntiAlias(true);
        shadow2.setColor(SkColorSetA(SK_ColorBLACK, (uint8_t)(120 * sidebarAlpha)));
        shadow2.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 16.0f));
        canvas->drawRoundRect(sidebarRect.makeOffset(8, 0), 16.0f, 16.0f, shadow2);
        
        // Layer 3: Contact shadow (tight, high opacity)
        SkPaint shadow3;
        shadow3.setAntiAlias(true);
        shadow3.setColor(SkColorSetA(SK_ColorBLACK, (uint8_t)(180 * sidebarAlpha)));
        shadow3.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        canvas->drawRoundRect(sidebarRect.makeOffset(2, 0), 16.0f, 16.0f, shadow3);
    }
    
    // ==========================================================================
    // SIDEBAR BACKGROUND WITH GLASS EFFECT
    // ==========================================================================
    
    if (layoutMode_ == LayoutMode::Compact) {
        // Glassmorphic panel with blur and gradient
        BackdropBlur::drawBlurredPanel(canvas, sidebarRect,
                                        16.0f,  // Corner radius on right side
                                        12.0f,  // Blur radius
                                        theme::BG_SIDEBAR, 0.85f * sidebarAlpha,
                                        true);  // Draw highlight
    } else {
        // Standard opaque background with subtle gradient
        SkColor gradColors[] = {
            SkColorSetARGB((uint8_t)(255 * sidebarAlpha), 14, 14, 18),
            SkColorSetARGB((uint8_t)(255 * sidebarAlpha), 10, 10, 14)
        };
        SkPoint gradPts[] = { {sidebarRect.left(), sidebarRect.top()},
                              {sidebarRect.left(), sidebarRect.bottom()} };
        SkPaint bg;
        bg.setAntiAlias(true);
        bg.setShader(SkGradientShader::MakeLinear(gradPts, gradColors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRect(sidebarRect, bg);
    }

    // ==========================================================================
    // EDGE GLOW AND DIVIDER
    // ==========================================================================
    
    // Gradient fade at right edge instead of hard line
    {
        SkColor edgeColors[] = {
            SkColorSetA(theme::ACCENT_PRIMARY, (uint8_t)(20 * sidebarAlpha)),
            SkColorSetA(theme::ACCENT_PRIMARY, 0)
        };
        SkPoint edgePts[] = { 
            {sidebarRect.right() - 20.0f, 0.0f}, 
            {sidebarRect.right(), 0.0f} 
        };
        SkPaint edgeGlow;
        edgeGlow.setShader(SkGradientShader::MakeLinear(edgePts, edgeColors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRect(SkRect::MakeLTRB(sidebarRect.right() - 20.0f, 0, 
                                          sidebarRect.right(), sidebarRect.bottom()), edgeGlow);
        
        // Subtle divider line
        SkPaint divider;
        divider.setColor(SkColorSetA(SK_ColorWHITE, (uint8_t)(18 * sidebarAlpha)));
        divider.setAntiAlias(true);
        canvas->drawLine(sidebarRect.right(), 0, sidebarRect.right(), sidebarRect.bottom(), divider);
        
        // =======================================================================
        // DRAG HANDLE AFFORDANCE (visual hint for edge drag)
        // =======================================================================
        if (layoutMode_ != LayoutMode::Expanded) {
            float handleY = sidebarRect.height() * 0.5f;
            float handleX = sidebarRect.right() - 6.0f;
            float handleH = 40.0f;
            float dotSpacing = 8.0f;
            
            SkPaint handlePaint;
            handlePaint.setAntiAlias(true);
            handlePaint.setColor(SkColorSetA(SK_ColorWHITE, (uint8_t)(50 * sidebarAlpha)));
            
            // Three vertical dots as grab indicator
            for (int i = -1; i <= 1; ++i) {
                float dotY = handleY + i * dotSpacing;
                canvas->drawCircle(handleX, dotY, 2.0f, handlePaint);
            }
        }
    }

    // ==========================================================================
    // SIDEBAR HEADER (New Chat button area)
    // ==========================================================================
    
    {
        SkFont headerFont = design::typography::getSkFont(11.0f, design::FontWeight::Medium);
        SkPaint headerP;
        headerP.setColor(SkColorSetA(theme::TEXT_MUTED, (uint8_t)(180 * sidebarAlpha)));
        headerP.setAntiAlias(true);
        canvas->drawString("HISTORY", 16.0f, 50.0f, headerFont, headerP);
    }

    // ==========================================================================
    // SESSIONS LIST WITH STAGGERED ANIMATIONS
    // ==========================================================================
    
    SkFont itemFont = design::typography::getSkFont(13.0f);
    SkPaint textP;
    textP.setColor(SkColorSetA(theme::TEXT_BODY, (uint8_t)(SkColorGetA(theme::TEXT_BODY) * sidebarAlpha)));
    textP.setAntiAlias(true);

    for (size_t i = 0; i < sessions_.size(); ++i) {
        const auto& s = sessions_[i];
        if (i >= layout_.sessionItemRects.size()) break;
        
        // Get staggered reveal amount for this session
        float revealAmount = 1.0f;
        if (i < sessionRevealAmounts_.size()) {
            revealAmount = sessionRevealAmounts_[i].get();
        }
        
        // Skip rendering if not visible
        if (revealAmount < 0.01f) continue;
        
        SkRect itemRect = layout_.sessionItemRects[i];
        
        // Apply staggered slide-in animation
        float slideOffset = (1.0f - revealAmount) * -30.0f;
        float itemAlpha = sidebarAlpha * revealAmount;
        
        canvas->save();
        canvas->translate(slideOffset, 0);
        
        // Session swipe offset (for swipe-to-reveal actions)
        float swipeOffset = 0.0f;
        if (swipingSessionIndex_ == (int)i) {
            swipeOffset = sessionSwipe_.getState().offset;
            itemRect.offset(swipeOffset, 0);
        }
        
        // Background with hover state
        SkPaint itemBg;
        itemBg.setAntiAlias(true);
        SkColor baseActive = SkColorSetARGB(140, 30, 30, 40);
        SkColor baseIdle = SkColorSetARGB(80, 20, 20, 28);
        SkColor blended = s.isActive ? baseActive : baseIdle;
        
        if (i < sessionStates_.size()) {
            float hoverBlend = sessionStates_[i].hoverAmount;
            blended = design::interpolateColor(blended, 
                SkColorSetARGB(180, 40, 40, 55), hoverBlend * 0.5f);
        }
        
        if ((int)i == pendingDeleteIndex_) {
            blended = design::interpolateColor(blended, SkColorSetARGB(160, 80, 30, 35), 0.7f);
        }
        
        itemBg.setColor(SkColorSetA(blended, (uint8_t)(SkColorGetA(blended) * itemAlpha)));
        canvas->drawRoundRect(itemRect, 10.0f, 10.0f, itemBg);
        
        // Active indicator (left accent bar)
        if (s.isActive) {
            SkPaint accentBar;
            accentBar.setAntiAlias(true);
            accentBar.setColor(SkColorSetA(theme::ACCENT_PRIMARY, (uint8_t)(200 * itemAlpha)));
            SkRect barRect = SkRect::MakeXYWH(itemRect.left() + 4.0f, 
                                               itemRect.top() + 6.0f, 
                                               3.0f, 
                                               itemRect.height() - 12.0f);
            canvas->drawRoundRect(barRect, 1.5f, 1.5f, accentBar);
        }

        // Session title
        if ((int)i != editingSessionIndex_) {
            SkRect textClip = SkRect::MakeLTRB(itemRect.left() + 14.0f,
                                               itemRect.top(),
                                               layout_.sessionEditRects[i].left() - 8.0f,
                                               itemRect.bottom());
            canvas->save();
            canvas->clipRect(textClip);
            
            SkPaint sessionTextP = textP;
            sessionTextP.setColor(SkColorSetA(
                s.isActive ? theme::TEXT_HEAD : theme::TEXT_BODY,
                (uint8_t)(255 * itemAlpha)));
            canvas->drawString(s.title.toStdString().c_str(), 
                              itemRect.left() + 14.0f, 
                              itemRect.centerY() + 4.0f, 
                              itemFont, sessionTextP);
            canvas->restore();
        }

        // Edit icon
        if (i < layout_.sessionEditRects.size()) {
            svgicons::Style editStyle;
            float editAlpha = itemAlpha * (0.4f + 0.6f * editStates_[i].hoverAmount);
            editStyle.color = SkColorSetA(
                design::interpolateColor(theme::TEXT_MUTED, theme::TEXT_HEAD, editStates_[i].hoverAmount),
                (uint8_t)(255 * editAlpha));
            editStyle.glowColor = theme::ACCENT_PRIMARY;
            editStyle.glowRadius = 8.0f * editStates_[i].hoverAmount;
            svgicons::drawIconCentered(canvas, svgicons::IconId::Edit,
                                       layout_.sessionEditRects[i], 14.0f, editStyle);
        }

        // Delete icon
        if (i < layout_.sessionDeleteRects.size()) {
            icons::IconStyle delStyle;
            float dangerEmphasis = ((int)i == pendingDeleteIndex_) ? 1.0f : deleteStates_[i].hoverAmount;
            float delAlpha = itemAlpha * (0.4f + 0.6f * dangerEmphasis);
            delStyle.color = SkColorSetA(
                design::interpolateColor(theme::TEXT_MUTED, SkColorSetRGB(255, 100, 100), dangerEmphasis),
                (uint8_t)(255 * delAlpha));
            delStyle.strokeWidth = 1.5f;
            icons::drawIconCentered(canvas, icons::Delete(),
                                    layout_.sessionDeleteRects[i], 14.0f, delStyle);
        }
        
        canvas->restore(); // Restore stagger translation
    }
}

void WingmanPanel::drawHeader(SkCanvas* canvas) {
    // History Icon (Top Left)
    icons::IconStyle style;
    style.color = design::interpolateColor(theme::TEXT_MUTED, theme::TEXT_HEAD,
                                           interaction_.sidebarToggle.hoverAmount);
    style.strokeWidth = 1.6f;
    style.glowColor = theme::ACCENT_PRIMARY;
    style.glowRadius = 6.0f * interaction_.sidebarToggle.getGlowIntensity();
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
        icons::drawIconCentered(canvas, icons::History(), local, 20.0f, style);
        canvas->restore();
    }

    icons::IconStyle plusStyle;
    plusStyle.color = design::interpolateColor(theme::TEXT_MUTED, theme::TEXT_HEAD,
                                               interaction_.newChat.hoverAmount);
    plusStyle.strokeWidth = 1.6f;
    plusStyle.glowColor = theme::ACCENT_PRIMARY;
    plusStyle.glowRadius = 6.0f * interaction_.newChat.getGlowIntensity();
    {
        float hoverScale = 1.0f + 0.08f * interaction_.newChat.hoverAmount;
        float pressScale = 1.0f - 0.05f * interaction_.newChat.pressAmount;
        float scale = hoverScale * pressScale;
        float cx = layout_.newChatBtn.centerX();
        float cy = layout_.newChatBtn.centerY();
        canvas->save();
        canvas->translate(cx, cy);
        canvas->scale(scale, scale);
        SkRect local = SkRect::MakeXYWH(-layout_.newChatBtn.width() * 0.5f,
                                        -layout_.newChatBtn.height() * 0.5f,
                                        layout_.newChatBtn.width(),
                                        layout_.newChatBtn.height());
        icons::drawIconCentered(canvas, icons::Plus(), local, 20.0f, plusStyle);
        canvas->restore();
    }
    
    // Settings Icon is now a child component (SettingsOverlayButton)
    // svgicons::drawIconCentered(canvas, svgicons::IconId::Settings, layout_.settingsBtn, 20.0f, settingsStyle);
}

void WingmanPanel::drawInputIsland(SkCanvas* canvas) {
    // "Clean Pill" aesthetic - no heavy borders, no bevels

    const float radius = 25.0f;
    const bool focused = (textInput_ && textInput_->hasKeyboardFocus(true));

    // 1. Shadow (neutral by default; accent glow only when focused/hovered)
    {
        SkPaint shadow;
        shadow.setAntiAlias(true);
        shadow.setColor(SkColorSetA(SK_ColorBLACK, 160));
        shadow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 20.0f));
        canvas->drawRoundRect(layout_.inputContainerRect.makeOffset(0, 8), radius, radius, shadow);

        if (focused || interaction_.input.hoverAmount > 0.01f) {
            SkPaint glow;
            glow.setAntiAlias(true);
            glow.setColor(SkColorSetA(theme::ACCENT_PRIMARY, focused ? 55 : 35));
            float glowRadius = focused ? 18.0f : (10.0f + 8.0f * interaction_.input.hoverAmount);
            glow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, glowRadius));
            canvas->drawRoundRect(layout_.inputContainerRect.makeOffset(0, 6), radius, radius, glow);
        }
    }

    // 2. Background (soft gradient)
    {
        SkColor colors[] = {
            SkColorSetRGB(24, 24, 30),
            SkColorSetRGB(18, 18, 24)
        };
        SkPoint pts[] = { {layout_.inputContainerRect.left(), layout_.inputContainerRect.top()},
                          {layout_.inputContainerRect.left(), layout_.inputContainerRect.bottom()} };
        SkPaint bg;
        bg.setAntiAlias(true);
        bg.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRoundRect(layout_.inputContainerRect, radius, radius, bg);
    }

    // 3. Stroke (slightly stronger on focus)
    {
        SkPaint border;
        border.setAntiAlias(true);
        border.setStyle(SkPaint::kStroke_Style);
        border.setStrokeWidth(1.0f);
        border.setColor(focused ? SkColorSetA(theme::ACCENT_PRIMARY, 80)
                                : SkColorSetA(SK_ColorWHITE, 24));
        canvas->drawRoundRect(layout_.inputContainerRect, radius, radius, border);

        // Top highlight (adds depth with almost no visual noise)
        SkPaint hl;
        hl.setAntiAlias(true);
        hl.setColor(SkColorSetA(SK_ColorWHITE, 20));
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
            SkColorSetARGB(210, 18, 18, 22),
            SkColorSetARGB(210, 14, 14, 18)
        };
        SkPoint pts[] = { {card.left(), card.top()}, {card.left(), card.bottom()} };
        SkPaint fill;
        fill.setAntiAlias(true);
        fill.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRoundRect(card, 24.0f, 24.0f, fill);
    }

    // Card border
    SkPaint border;
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(SkColorSetA(SK_ColorWHITE, 22));
    border.setAntiAlias(true);
    canvas->drawRoundRect(card, 24.0f, 24.0f, border);

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

    float subSize = (card.width() < 320.0f) ? 12.0f : 13.0f;
    SkFont subFont = design::typography::getSkFont(subSize);
    textP.setColor(SkColorSetA(theme::TEXT_BODY, 180));
    const char* sub = "Ask for an improvement or choose a suggestion.";
    float subW = subFont.measureText(sub, strlen(sub), SkTextEncoding::kUTF8);
    float subY = headY + textGap + 10.0f;
    canvas->drawString(sub, cx - subW * 0.5f, subY, subFont, textP);

    drawSuggestions(canvas);
}

void WingmanPanel::drawSuggestionChip(SkCanvas* canvas, const SkRect& r, const Suggestion* s, float hoverAmount) {
    if (!s) return;
    
    SkPaint fill;
    fill.setAntiAlias(true);
    fill.setColor(design::interpolateColor(SkColorSetARGB(170, 22, 22, 28),
                                           SkColorSetARGB(190, 36, 36, 42),
                                           hoverAmount));
    canvas->drawRoundRect(r, 16.0f, 16.0f, fill);

    SkPaint stroke;
    stroke.setAntiAlias(true);
    stroke.setStyle(SkPaint::kStroke_Style);
    stroke.setStrokeWidth(1.0f);
    stroke.setColor(design::interpolateColor(SkColorSetA(SK_ColorWHITE, 24),
                                             SkColorSetA(theme::ACCENT_PRIMARY, 70),
                                             hoverAmount));
    canvas->drawRoundRect(r, 16.0f, 16.0f, stroke);

    // Small leading icon (subtle) - Sparkles or Edit? 
    // Defaults to sparkles.
    SkRect iconR = SkRect::MakeXYWH(r.left() + 8.0f, r.centerY() - 7.0f, 14.0f, 14.0f);
    icons::IconStyle ic;
    ic.color = design::interpolateColor(SkColorSetA(theme::TEXT_MUTED, 160),
                                        theme::ACCENT_PRIMARY,
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
    // Use spring physics for natural, bouncy animation
    float dtMs = dt * 1000.0f;
    
    if (!sidebarDragging_) {
        // Update spring only when not being dragged
        if (sidebarSpring_.isAnimating()) {
            sidebarSpring_.update(dtMs);
            updateLayout();
            repaint();
        }
    }
    
    // Update staggered session reveal animations
    for (auto& reveal : sessionRevealAmounts_) {
        reveal.update(dtMs);
    }
}

void WingmanPanel::highlightReasoningButton() {
    reasoningHighlightUntilMs_ = juce::Time::getMillisecondCounter() + 1400;
    reasoningHighlightAmount_ = 1.0f;
}

void WingmanPanel::toggleSettings() {
    settingsOpen_ = !settingsOpen_;
    settingsTarget_ = settingsOpen_ ? 1.0f : 0.0f;
    if (settingsPanel_) {
        settingsPanel_->setVisible(true);
    }
    if (settingsOverlayBtn_) {
        // Ensure the overlay button stays on top of the panel
        settingsOverlayBtn_->toFront(true);
    }
    repaint();
}

void WingmanPanel::updateSettingsAnimation(float dt) {
    if (std::abs(settingsSlideAmount_ - settingsTarget_) < 0.001f) {
        settingsSlideAmount_ = settingsTarget_;
        if (!settingsOpen_) {
            if (settingsPanel_) settingsPanel_->setVisible(false);
        }
    } else {
        float speed = 10.0f; // Snappy
        settingsSlideAmount_ += (settingsTarget_ - settingsSlideAmount_) * speed * dt;
        repaint();
    }
    
    // Sync Settings Panel Position and Child Transforms
    if (settingsPanel_) {
        float panelX = getWidth() * (1.0f - settingsSlideAmount_);
        settingsPanel_->setTopLeftPosition((int)panelX, 0);
    }
    
    // Hide text inputs when sliding to avoid floaty native widgets (if they aren't fully integrated)
    // Actually, using AffineTransform on components works well in JUCE.
    if (textInput_) {
        textInput_->setTransform(juce::AffineTransform::translation(-getWidth() * settingsSlideAmount_, 0));
    }
    if (renameInput_) {
        renameInput_->setTransform(juce::AffineTransform::translation(-getWidth() * settingsSlideAmount_, 0));
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
        grokController_->executeCommand(
            "Generate 3 short, creative, and distinct suggestions for what I can ask you to help with in this DAW session. Return ONLY JSON array format like [{\"label\": \"...\", \"intent\": \"...\"}, ...]. Keep labels under 20 chars.",
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
    juce::PopupMenu m;
    m.addItem(1, "Edit Suggestion");
    m.addItem(2, "Delete Suggestion");
    
    // safe capture
    juce::String sId = s.id;
    juce::String sLabel = s.label;
    juce::String sIntent = s.intent;
    auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);
    
    m.showMenuAsync(juce::PopupMenu::Options(), [safeThis, sId, sLabel, sIntent](int result) {
        if (!safeThis) return;
        if (result == 1) {
            // Edit
             // Simple hack: Re-use the existing suggestion struct but passed by value to avoiding dangling ptrs
             Suggestion copy; copy.id = sId; copy.label = sLabel; copy.intent = sIntent;
             safeThis->showEditSuggestionDialog(copy);
        }
        else if (result == 2) {
            // Delete
            safeThis->deleteSuggestion(sId);
        }
    });
}

void WingmanPanel::showEditSuggestionDialog(const Suggestion& s) {
    auto w = std::make_shared<juce::AlertWindow>("Edit Suggestion", "Modify the label and intent.", juce::AlertWindow::NoIcon);
    
    w->addTextEditor("label", s.label, "Label:");
    w->addTextEditor("intent", s.intent, "Intent (Prompt):");
    
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    
    // Capture shared_ptr to keep window alive until callback completes
    auto safeThis = juce::Component::SafePointer<WingmanPanel>(this);
    juce::String sId = s.id;
    
    w->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, sId, w](int result) {
        if (safeThis && result == 1) {
            safeThis->updateSuggestion(sId, w->getTextEditorContents("label"), w->getTextEditorContents("intent"));
        }
        // shared_ptr auto-deletes window when lambda is destroyed
    }));
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
        bg.setColor(SkColorSetRGB(30, 30, 35)); // Very subtle grey
        bg.setAntiAlias(true);
        canvas->drawRoundRect(pill, 16.0f, 16.0f, bg);
        
        drawWrappedText(canvas, msg.content, pillX + 16.0f, y + 26.0f, pillW - 32.0f, font, paint);
        
    } else {
        // Wingman: Left aligned, clean text, no bubble
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
            const float bubbleH = 96.0f;
            const float bubbleGap = 16.0f;
            SkRect bubbleRect = SkRect::MakeXYWH(textX, currentY + bubbleGap,
                                                 textW, bubbleH);
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
        bg.setColor(SkColorSetRGB(18, 18, 22));
        bg.setAntiAlias(true);
        canvas->drawRoundRect(r, 8.0f, 8.0f, bg);
        
        SkPaint border;
        border.setStyle(SkPaint::kStroke_Style);
        border.setColor(SkColorSetA(SK_ColorWHITE, 20));
        border.setAntiAlias(true);
        canvas->drawRoundRect(r, 8.0f, 8.0f, border);
        
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

    if (block.type == BlockType::Table) {
        float py = y;
        for (const auto& row : block.rows) {
            float px = x;
            for (const auto& cell : row.cells) {
                SkRect r = SkRect::MakeXYWH(px, py, cell.width, row.height);
                
                SkPaint border;
                border.setStyle(SkPaint::kStroke_Style);
                border.setColor(SkColorSetA(SK_ColorWHITE, 30));
                canvas->drawRect(r, border);

                float cpy = py + 6.0f;
                for (const auto& line : cell.lines) {
                    float cpx = px + 6.0f;
                    for (const auto& span : line.spans) {
                        paint.setColor(span.style == SpanStyle::Bold ? SK_ColorWHITE : theme::TEXT_BODY);
                        canvas->drawString(span.text.c_str(), cpx, cpy + span.font.getSize(), span.font, paint);
                        cpx += span.width;
                    }
                    cpy += line.height;
                }
                px += cell.width;
            }
            py += row.height;
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
    bg.setColor(SkColorSetARGB(170, 16, 16, 20));
    canvas->drawRoundRect(bubbleRect, 14.0f, 14.0f, bg);

    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    float hover = (hoveredReasoningId_ == msg.id) ? 1.0f : 0.0f;
    border.setColor(design::interpolateColor(SkColorSetA(SK_ColorWHITE, 18),
                                             SkColorSetA(theme::ACCENT_PRIMARY, 60),
                                             hover));
    border.setStrokeWidth(1.0f);
    canvas->drawRoundRect(bubbleRect, 14.0f, 14.0f, border);

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
    panel.setColor(SkColorSetARGB(235, 14, 14, 18));
    canvas->drawRoundRect(reasoningOverlayRect_, 18.0f, 18.0f, panel);

    SkPaint border;
    border.setAntiAlias(true);
    border.setStyle(SkPaint::kStroke_Style);
    border.setStrokeWidth(1.0f);
    border.setColor(SkColorSetA(SK_ColorWHITE, 22));
    canvas->drawRoundRect(reasoningOverlayRect_, 18.0f, 18.0f, border);

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
