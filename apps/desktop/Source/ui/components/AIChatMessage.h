/*
  ==============================================================================

    AIChatMessage.h
    Created: 2025-12-29
    Author:  Zenith DAW Team

    AI Chat Message Bubble Component for Wingman Panel
    
    Features:
    - User messages: Right aligned, solid BG_02 background
    - AI messages: Left aligned, glassmorphic with cyan accent
    - Code blocks with monospace font and dark background
    - Timestamps in TEXT_TERTIARY
    - "Thinking..." shimmer animation state
    - Long press to copy, hover-copy for code

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include "../framework/GlassmorphicPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <string>

namespace zenith {

/**
 * @brief Message type for styling differentiation
 */
enum class ChatMessageType {
    User,       // Right-aligned, solid background
    AI,         // Left-aligned, glassmorphic 
    System,     // Centered, subtle styling
    Error       // Left-aligned, error accent
};

/**
 * @brief Represents a parsed segment of the message (text or code)
 */
struct MessageSegment {
    enum class Type { Text, CodeBlock, InlineCode };
    
    Type type = Type::Text;
    juce::String content;
    juce::String language; // For code blocks
};

/**
 * @brief AI Chat Message Bubble Component
 * 
 * Renders individual chat messages in the Wingman AI panel with
 * proper styling based on message type:
 * 
 * USER MESSAGES:
 * - Align: Right
 * - Background: BG_02 solid
 * - Text: TEXT_PRIMARY
 * - No border
 * 
 * AI MESSAGES:
 * - Align: Left  
 * - Background: Glassmorphic (0.6 opacity, blur)
 * - Border: 1px CYAN at 30% opacity
 * - Text: TEXT_PRIMARY
 * - Code blocks with MONO font, darker background
 * 
 * METADATA:
 * - Timestamp below message: TEXT_TERTIARY, FONT_XS
 * - "Thinking..." state: Animated gradient shimmer
 * 
 * INTERACTIONS:
 * - Long press: Copy text
 * - Hover on code: "Copy" button appears
 */
class AIChatMessage : public SkiaComponent {
public:
    //==========================================================================
    // Constants
    //==========================================================================
    
    static constexpr float MAX_WIDTH_RATIO = 0.80f;     // 80% of parent
    static constexpr float PADDING = design::spacing::SM; // 8px
    static constexpr float CORNER_RADIUS = design::dimensions::RADIUS_SM; // 8px
    static constexpr float CODE_BLOCK_PADDING = 6.0f;
    static constexpr float TIMESTAMP_MARGIN = 4.0f;
    static constexpr int LONG_PRESS_MS = 500;
    
    //==========================================================================
    // Construction
    //==========================================================================
    
    /**
     * @brief Create a new chat message bubble
     * @param message The message content (supports basic markdown)
     * @param type The message type (User/AI/System/Error)
     * @param timestamp The message timestamp
     */
    AIChatMessage(const juce::String& message, 
                  ChatMessageType type,
                  const juce::Time& timestamp = juce::Time::getCurrentTime());
    
    ~AIChatMessage() override;
    
    //==========================================================================
    // State Management
    //==========================================================================
    
    /**
     * @brief Set the "thinking" state with animated shimmer
     * @param thinking True to show thinking animation
     */
    void setThinking(bool thinking);
    bool isThinking() const { return isThinking_; }
    
    /**
     * @brief Update the message content
     * @param message New message content
     */
    void setMessage(const juce::String& message);
    const juce::String& getMessage() const { return rawMessage_; }
    
    /**
     * @brief Get the message type
     */
    ChatMessageType getMessageType() const { return messageType_; }
    
    /**
     * @brief Calculate the preferred height for the given width
     * @param availableWidth The available width for the message
     * @return The preferred height including timestamp
     */
    float calculatePreferredHeight(float availableWidth) const;
    
    //==========================================================================
    // Callbacks
    //==========================================================================
    
    /** Called when the message text is copied (long press or button) */
    std::function<void(const juce::String&)> onCopy;
    
    /** Called when a code block is copied */
    std::function<void(const juce::String&)> onCodeCopy;
    
    // Animation state
    animation::AnimatedValue<float> opacity{0.0f};
    animation::AnimatedValue<float> yOffset{20.0f};
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    void timerCallback() override;
    
private:
    //==========================================================================
    // Internal Types
    //==========================================================================
    
    struct CodeBlockInfo {
        juce::Rectangle<float> bounds;
        juce::String code;
        bool isHovered = false;
    };
    
    //==========================================================================
    // Message Parsing
    //==========================================================================
    
    void parseMessage();
    std::vector<MessageSegment> parseMarkdown(const juce::String& text);
    
    //==========================================================================
    // Drawing Helpers
    //==========================================================================
    
    void drawUserMessage(SkCanvas* canvas, const SkRect& bounds);
    void drawAIMessage(SkCanvas* canvas, const SkRect& bounds);
    void drawSystemMessage(SkCanvas* canvas, const SkRect& bounds);
    void drawErrorMessage(SkCanvas* canvas, const SkRect& bounds);
    
    void drawMessageContent(SkCanvas* canvas, const SkRect& contentBounds, bool alignRight);
    void drawCodeBlock(SkCanvas* canvas, const SkRect& bounds, const juce::String& code, 
                       const juce::String& language, int index);
    void drawTimestamp(SkCanvas* canvas, const SkRect& messageBounds, bool alignRight);
    void drawThinkingShimmer(SkCanvas* canvas, const SkRect& bounds);
    void drawCopyButton(SkCanvas* canvas, const SkRect& bounds, bool isHovered);
    
    SkRect calculateBubbleBounds(float parentWidth) const;
    float measureTextHeight(const juce::String& text, float maxWidth, const SkFont& font) const;
    
    //==========================================================================
    // Copy Functionality
    //==========================================================================
    
    void copyToClipboard(const juce::String& text);
    void handleLongPress();
    
    //==========================================================================
    // Member Data
    //==========================================================================
    
    // Content
    juce::String rawMessage_;
    std::vector<MessageSegment> segments_;
    ChatMessageType messageType_;
    juce::Time timestamp_;
    
    // State
    bool isThinking_ = false;
    float shimmerPhase_ = 0.0f;
    bool isPressed_ = false;
    juce::int64 pressStartTime_ = 0;
    bool longPressTriggered_ = false;
    
    // Code block interaction
    std::vector<CodeBlockInfo> codeBlocks_;
    int hoveredCodeBlockIndex_ = -1;
    bool copyButtonHovered_ = false;
    
    // Cached layout
    mutable float cachedWidth_ = 0.0f;
    mutable float cachedHeight_ = 0.0f;
    mutable bool layoutDirty_ = true;
    
    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIChatMessage)
};

} // namespace zenith
