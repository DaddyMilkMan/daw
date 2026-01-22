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
#include "../network/GrokDAWController.h"
#include "../../commands/CommandAPI.h"
#include "Engine.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

//==============================================================================
// Data Structures
//==============================================================================

struct WingmanMessage {
    juce::String id;
    juce::String sender; // "User" or "Wingman"
    juce::String content;
    int64_t timestamp;
    bool isReasoning = false;
    
    // Layout Cache
    float cachedHeight = 0.0f;
    float cachedWidth = 0.0f;
};

struct WingmanSession {
    juce::String id;
    juce::String title;
    juce::String dateLabel;
    bool isActive = false;
};

//==============================================================================
// WingmanPanel Class
//==============================================================================

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
    void mouseExit(const juce::MouseEvent& e) override;

    // Rendering
    void drawSkia(SkCanvas* canvas) override;

private:
    // Core Dependencies
    CommandAPI& commandAPI_;
    Engine& engine_;
    std::unique_ptr<GrokDAWController> grokController_;

    // UI State
    bool sidebarOpen_ = true; // Default open for "wide" feel
    bool isReasoningMode_ = false; // "Smart" mode
    bool isProcessing_ = false;
    float scrollOffset_ = 0.0f;
    float targetScrollOffset_ = 0.0f; // For smooth scroll
    
    // Layout Metrics (Calculated in resized)
    struct Layout {
        SkRect sidebarRect;
        SkRect contentRect;
        SkRect headerRect;
        SkRect chatRect;
        SkRect inputContainerRect;
        SkRect inputFieldRect;
        
        // Button Hit Zones
        SkRect toggleSidebarBtn;
        SkRect newChatBtn;
        SkRect settingsBtn;
        SkRect sendBtn;
        SkRect brainBtn;
    } layout_;

    // Interactive State
    struct Interaction {
        bool hoverSend = false;
        bool hoverBrain = false;
        bool hoverSettings = false;
        bool hoverNewChat = false;
        bool hoverSidebarToggle = false;
        
        // Input Focus
        bool inputFocused = false;
    } interaction_;

    // Data
    std::vector<WingmanMessage> messages_;
    std::vector<WingmanSession> sessions_;
    
    // Components
    std::unique_ptr<SkiaTextInput> textInput_;

    // Methods
    void initializeInterface();
    void updateLayout();
    void sendMessage();
    void receiveMessage(const juce::String& text);
    void createNewSession();
    
    // Drawing Helpers (High Fidelity)
    void drawSidebar(SkCanvas* canvas);
    void drawHeader(SkCanvas* canvas);
    void drawChatStream(SkCanvas* canvas);
    void drawInputIsland(SkCanvas* canvas);
    void drawEmptyState(SkCanvas* canvas);
    
    // Text Layout
    float measureMessageHeight(const WingmanMessage& msg, float width);
    void drawMessageBubble(SkCanvas* canvas, const WingmanMessage& msg, float y, float x, float w);
    void drawWrappedText(SkCanvas* canvas, const juce::String& text, float x, float y, float width, const SkFont& font, const SkPaint& paint);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanPanel)
};

} // namespace zenith