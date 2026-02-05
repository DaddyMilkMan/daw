/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-29
    Redesigned: 2026-01-17
    Refactored: 2026-02-02
    Author:  Zenith Team

    FLUID INTELLIGENCE INTERFACE
    ----------------------------
    A completely custom, high-fidelity AI chat interface implemented in pure Skia.
    No legacy components. No placeholders.

    Features:
    - Cinematic dark theme (Void/Obsidian)
    - Floating input island with gradient borders
    - Kinetic scroll physics (simulated)
    - Markdown-style distinct message rendering
    - Acrylic sidebar integration

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../framework/Animation.h"
#include "../framework/GestureRecognizer.h"
#include "../framework/BackdropBlur.h"
#include "../controls/SkiaTextInput.h"
#include "../design-system/InteractionHelper.h"
#include "../network/GrokDAWController.h"
#include "../../commands/CommandAPI.h"
#include "../../engine/Engine.h"
#include "../dialogs/SettingsComponent.h"
#include "WingmanTypes.h"
#include "WingmanTextLayout.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <unordered_map>
#include <vector>

namespace zenith {

//==============================================================================
// Layout Mode - Responsive breakpoints
//==============================================================================
enum class LayoutMode {
    Compact,    // < 480px: Sheet-style sidebar overlay
    Standard,   // 480-1024px: Push-to-reveal sidebar
    Expanded    // > 1024px: Persistent docked sidebar
};

//==============================================================================
// Session Date Group for sidebar organization
//==============================================================================
struct SessionGroup {
    juce::String label;  // "Today", "Yesterday", "This Week", etc.
    std::vector<int> sessionIndices;
    float headerY = 0.0f;
    bool isCollapsed = false;
};

//==============================================================================
// WingmanPanel Class
//==============================================================================

class SettingsOverlayButton;

class WingmanPanel : public SkiaComponent {
public:
    WingmanPanel(CommandAPI& api, Engine& engine);
    ~WingmanPanel() override;

    // Lifecycle
    void resized() override;
    void visibilityChanged() override;
    
    // Interaction
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Rendering
    void drawSkia(SkCanvas* canvas) override;

private:
    // Core Dependencies
    CommandAPI& commandAPI_;
    Engine& engine_;
    std::unique_ptr<GrokDAWController> grokController_;

    // Layout Mode (responsive)
    LayoutMode layoutMode_ = LayoutMode::Standard;
    static constexpr float kCompactBreakpoint = 480.0f;
    static constexpr float kExpandedBreakpoint = 1024.0f;
    static constexpr float kSidebarWidth = 280.0f;  // Fixed sidebar width
    
    // Gesture Controllers
    gesture::EdgeSwipeDetector edgeSwipe_;
    gesture::PanelDragController sidebarDrag_;
    gesture::SwipeActionDetector sessionSwipe_;
    gesture::DragReorderDetector sessionReorder_;
    int swipingSessionIndex_ = -1;
    int reorderingSessionIndex_ = -1;
    
    // Spring-based sidebar animation
    animation::SpringSolver sidebarSpring_;
    bool sidebarDragging_ = false;
    
    // Session grouping
    std::vector<SessionGroup> sessionGroups_;
    void rebuildSessionGroups();
    void drawSessionGroup(SkCanvas* canvas, const SessionGroup& group, float& y);
    
    // Staggered animation for session list
    std::vector<animation::AnimatedValue<float>> sessionRevealAmounts_;
    void animateSessionsIn();

    // UI State
    bool sidebarOpen_ = false;
    bool sidebarAutoCollapsed_ = false;
    bool isReasoningMode_ = false; // "Smart" mode
    bool isProcessing_ = false;
    bool awaitingApiKey_ = false;
    bool historyToggledByUser_ = false;
    bool isLoadingSessions_ = false;
    float reasoningHighlightAmount_ = 0.0f;
    uint32_t reasoningHighlightUntilMs_ = 0;
    float sidebarOpenAmount_ = 0.0f;
    float sidebarTarget_ = 0.0f;
    float scrollOffset_ = 0.0f;
    float targetScrollOffset_ = 0.0f; // For smooth scroll
    juce::String processingStatus_;
    
    // Layout Metrics (Calculated in resized)
    struct Layout {
        SkRect sidebarRect;
        SkRect contentRect;
        SkRect headerRect;
        SkRect chatRect;
        SkRect inputContainerRect;
        SkRect inputFieldRect;
        SkRect suggestionsAreaRect; // Area above input for chips
        SkRect scrollbarTrack;
        SkRect scrollbarThumb;
        
        // Push-to-reveal: content offset when sidebar open
        float contentOffsetX = 0.0f;
        
        // Sidebar edge zone for swipe gesture
        SkRect sidebarEdgeZone;
        
        // Backdrop overlay (for Compact mode)
        SkRect backdropRect;
        bool showBackdrop = false;

        // Chat content geometry (centered reading column)
        float chatContentX = 0.0f;
        float chatContentW = 0.0f;

        // Empty-state hero
        SkRect emptyHeroCard;
        
        // Session swipe action reveal areas
        struct SwipeReveal {
            SkRect editActionRect;
            SkRect deleteActionRect;
            float revealAmount = 0.0f;
        };
        std::vector<SwipeReveal> sessionSwipeReveals;

        // Dynamic Suggestion Chips (Layout for current session)
        struct ChipLayout {
            SkRect rect;
            Suggestion* data = nullptr; // Pointer to actual data
        };
        std::vector<ChipLayout> suggestionChips;
        SkRect regenerateBtn; // Small icon button for regeneration

        // Sidebar sessions + actions
        std::vector<SkRect> sessionItemRects;
        std::vector<SkRect> sessionEditRects;
        std::vector<SkRect> sessionDeleteRects;
        std::vector<SkRect> sessionEditHitRects;
        std::vector<SkRect> sessionDeleteHitRects;
        
        // Sidebar group headers
        std::vector<SkRect> groupHeaderRects;
        
        // Button Hit Zones
        SkRect toggleSidebarBtn;
        SkRect newChatBtn;
        SkRect settingsBtn;
        SkRect sendBtn;
        SkRect brainBtn;
    } layout_;

    // Interactive State
    struct Interaction {
        InteractionState send;
        InteractionState brain;
        InteractionState settings;
        InteractionState sidebarToggle;
        InteractionState newChat;
        InteractionState input;
        InteractionState regenerate;
        InteractionState scrollbar;
        
        // Map ID -> State for suggestions
        std::unordered_map<std::string, InteractionState> suggestionStates;

        // Input Focus
        bool inputFocused = false;
    } interaction_;

    // Data
    std::vector<WingmanMessage> messages_;
    std::vector<WingmanSession> sessions_;
    
    // Components
    std::unique_ptr<SkiaTextInput> textInput_;
    std::unique_ptr<SkiaTextInput> renameInput_;
    int editingSessionIndex_ = -1;
    int pendingDeleteIndex_ = -1;
    uint32_t pendingDeleteUntilMs_ = 0;
    std::vector<InteractionState> sessionStates_;
    std::vector<InteractionState> editStates_;
    std::vector<InteractionState> deleteStates_;

    // Cached chat metrics
    float chatContentHeight_ = 0.0f;
    float maxScroll_ = 0.0f;
    float chatWrapWidth_ = 0.0f;
    float chatWrapX_ = 0.0f;
    uint32_t lastResizeMs_ = 0;
    bool chatReflowPending_ = false;
    static constexpr int chatReflowDebounceMs = 120;
    bool scrollbarDragging_ = false;
    float scrollbarDragOffsetY_ = 0.0f;

    // Empty state suggestions
    // Suggestion Management
    void ensureDefaultSuggestions(WingmanSession& session);
    void addSuggestion(const juce::String& label, const juce::String& intent);
    void updateSuggestion(const juce::String& id, const juce::String& newLabel, const juce::String& newIntent);
    void deleteSuggestion(const juce::String& id);
    void regenerateSuggestions();
    void showSuggestionContextMenu(const Suggestion& s);
    void showEditSuggestionDialog(const Suggestion& s);
    WingmanSession* getActiveSessionPtr();


    // Methods
    void initializeInterface();
    void updateLayout(bool deferChatReflow = false);
    void scheduleChatReflow();
    void updateScrollbarLayout();
    void setScrollFromThumbTop(float thumbTop);
    void sendMessage();
    void sendPrompt(const juce::String& prompt);
    void receiveMessage(const juce::String& text, const juce::String& reasoning);
    void createNewSession();
    void loadSessionsFromDisk();
    void saveSessionsToDisk();
    void setActiveSessionIndex(int index);
    int getActiveSessionIndex() const;
    void syncActiveSessionMessages();
    juce::File getSessionsFile() const;

    void recalcChatMetrics();
    void scrollToBottom();
    void scrollToTopOfMessage(const juce::String& messageId);
    bool isScrolledToBottom() const;
    
    // Drawing Helpers (High Fidelity)
    void drawSidebar(SkCanvas* canvas);
    void drawHeader(SkCanvas* canvas);
    void drawChatStream(SkCanvas* canvas);
    void drawInputIsland(SkCanvas* canvas);
    void drawEmptyState(SkCanvas* canvas);
    void drawChatScrollbar(SkCanvas* canvas);

    void drawSuggestionChip(SkCanvas* canvas, const SkRect& r, const Suggestion* s, float hoverAmount);
    void drawSuggestions(SkCanvas* canvas);
    void updateHoverAnimations(float dt);
    void updateSidebarAnimation(float dt);
    void highlightReasoningButton();
    void beginRenameSession(int index);
    void commitRenameSession();
    void cancelRenameSession();
    void deleteSessionAt(int index);
    
    // Text Layout
    float measureMessageHeight(const WingmanMessage& msg, float width);
    void drawMessageBubble(SkCanvas* canvas, const WingmanMessage& msg, float y, float x, float w);
    void drawLayoutBlock(SkCanvas* canvas, const LayoutBlock& block, float x, float y, float w);
    void drawReasoningBubble(SkCanvas* canvas, const WingmanMessage& msg, const SkRect& bubbleRect);
    void drawReasoningOverlay(SkCanvas* canvas);
    void openReasoningOverlay(const juce::String& messageId);
    void closeReasoningOverlay();
    float measureReasoningHeight(const juce::String& reasoning, float width) const;
    void drawWrappedText(SkCanvas* canvas, const juce::String& text, float x, float y, float width, const SkFont& font, const SkPaint& paint);

    int countWrappedLines(const juce::String& text, float maxWidth, const SkFont& font) const;

    // Reasoning UI state
    std::unordered_map<std::string, SkRect> reasoningBubbleRects_;
    std::unordered_map<std::string, float> reasoningScrollOffsets_;
    juce::String hoveredReasoningId_;
    juce::String reasoningOverlayMessageId_;
    SkRect reasoningOverlayRect_;
    SkRect reasoningOverlayCloseRect_;
    float reasoningOverlayScroll_ = 0.0f;
    float reasoningOverlayPrevScroll_ = 0.0f;
    float reasoningOverlayDragStartY_ = 0.0f;
    bool reasoningOverlayDragging_ = false;
    bool reasoningOverlayOpen_ = false;
    
    // Settings Animation
    std::unique_ptr<SettingsComponent> settingsPanel_;
    std::unique_ptr<SettingsOverlayButton> settingsOverlayBtn_;
    bool settingsOpen_ = false;
    float settingsSlideAmount_ = 0.0f;
    float settingsTarget_ = 0.0f;
    
    // Refresh Icon Animation
    float refreshRotation_ = 0.0f;
    float refreshVelocity_ = 0.0f;
    
    void toggleSettings();
    void updateSettingsAnimation(float dt);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith
