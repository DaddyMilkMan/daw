/*
  ==============================================================================

    WingmanPanel.h
    Created: 2025-11-29
    Redesigned: 2026-01-17
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
#include "../controls/SkiaTextInput.h"
#include "../design-system/InteractionHelper.h"
#include "../network/GrokDAWController.h"
#include "../../commands/CommandAPI.h"
#include "../../engine/Engine.h"
#include "../settings/ModernSettingsPanel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <unordered_map>
#include <vector>

#include "WingmanTextLayout.h"

namespace zenith {

//==============================================================================
// Data Structures
//==============================================================================

struct Suggestion {
    juce::String id;
    juce::String label;
    juce::String intent;
    bool isUserCreated = false;
    bool isEditable = true;
};

struct WingmanMessage {
    juce::String id;
    juce::String sender; // "User" or "Wingman"
    juce::String content;
    juce::String reasoning;
    int64_t timestamp;
    bool isReasoning = false; // Legacy flag (keep for compat or repurpose)
    bool hasReasoning = false; // TRUE if reasoning field is populated
    
    // Layout Cache
    CachedTextLayout layout;
    float cachedHeight = 0.0f;
    float cachedWidth = 0.0f;
};

struct WingmanSession {
    juce::String id;
    juce::String title;
    juce::String dateLabel;
    bool isActive = false;
    bool reasoningEnabled = false; // Default OFF for new chats
    std::vector<WingmanMessage> messages;
    std::vector<Suggestion> suggestions;
};

//==============================================================================
// WingmanPanel Class
//==============================================================================

class SettingsOverlayButton;
class HistoryOverlayButton;

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

    // UI State
    bool sidebarOpen_ = false;
    bool sidebarAutoCollapsed_ = false;
    bool isReasoningMode_ = false; // "Smart" mode
    bool isProcessing_ = false;
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

        // Chat content geometry (centered reading column)
        float chatContentX = 0.0f;
        float chatContentW = 0.0f;

        // Empty-state hero
        SkRect emptyHeroCard;

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
        std::vector<int> sessionSourceIndices; // Row index -> sessions_ index
        SkRect historySearchRect;
        
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
    juce::String launchGreeting_;
    
    // Components
    std::unique_ptr<SkiaTextInput> textInput_;
    std::unique_ptr<SkiaTextInput> renameInput_;
    std::unique_ptr<SkiaTextInput> historySearchInput_;
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
    std::unique_ptr<ModernSettingsPanel> settingsPanel_;
    std::unique_ptr<SettingsOverlayButton> settingsOverlayBtn_;
    std::unique_ptr<HistoryOverlayButton> historyOverlayBtn_;
    bool settingsOpen_ = false;
    float settingsSlideAmount_ = 0.0f;
    float settingsTarget_ = 0.0f;
    
    // Refresh Icon Animation
    float refreshRotation_ = 0.0f;
    float refreshVelocity_ = 0.0f;
    
    void toggleSettings();
    void toggleHistory();
    void updateSettingsAnimation(float dt);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith
