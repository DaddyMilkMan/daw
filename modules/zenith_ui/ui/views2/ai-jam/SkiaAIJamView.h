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

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../framework/GlassmorphicPanel.h"
#include "../../design-system/ZenithTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

// Forward declarations
namespace zenith {
    class GrokDAWController;
    class Engine;
    class ProjectState;
}

namespace zenith::ui {

//==============================================================================
// Types
//==============================================================================

/**
 * @brief Stem type for generated content
 */
enum class StemType {
    Drums,
    Bass,
    Chords,
    Melody,
    Vocals,
    FX,
    Other
};

/**
 * @brief Button type within a stem card
 */
enum class StemButtonType {
    Play,  ///< Play/pause button
    Solo,  ///< Solo button
    Mute   ///< Mute button
};

/**
 * @brief Generated stem data
 */
struct GeneratedStem {
    StemType type = StemType::Other;
    juce::String name;
    bool isMuted = false;
    bool isSoloed = false;
    bool isPlaying = true;
    float volume = 1.0f;
    float meterLevel = 0.0f;
    std::vector<float> waveformPreview;  // 64 samples for mini display
    juce::File audioFile;  // Associated audio file (if any)
    juce::String clipId;   // Associated clip ID in project (if any)
};

/**
 * @brief Chat message in AI conversation
 */
struct AIChatMessage {
    bool fromUser = true;
    juce::String text;
    juce::String timestamp;
    bool hasAction = false;
    juce::String actionLabel;
};

/**
 * @brief Quick action button data
 */
struct QuickAction {
    juce::String emoji;
    juce::String label;
    juce::String prompt;  // What to send to AI
};

//==============================================================================
// Main View
//==============================================================================

/**
 * @class SkiaAIJamView
 * @brief AI-powered jamming overlay
 */
class SkiaAIJamView : public SkiaComponent {
public:
    SkiaAIJamView();
    ~SkiaAIJamView() override;

    //==========================================================================
    // State Management
    //==========================================================================
    
    void setPromptText(const juce::String& text);
    juce::String getPromptText() const { return promptText_; }
    
    void setStems(const std::vector<GeneratedStem>& stems);
    const std::vector<GeneratedStem>& getStems() const { return stems_; }
    
    void addChatMessage(const AIChatMessage& message);
    void clearChat();
    
    void setThinking(bool thinking);
    bool isThinking() const { return isThinking_; }
    
    void setLoopLength(int bars);
    int getLoopLength() const { return loopBars_; }
    
    void setBPM(float bpm);
    float getBPM() const { return bpm_; }
    
    void setPlayPosition(float progress);  // 0-1
    
    //==========================================================================
    // Variation Selection
    //==========================================================================
    
    void setActiveVariation(int index);  // 0-3 for A/B/C/D
    int getActiveVariation() const { return activeVariation_; }
    
    //==========================================================================
    // AI Integration
    //==========================================================================
    
    /**
     * @brief Set the Grok AI controller for real AI generation
     */
    void setGrokController(zenith::GrokDAWController* controller);
    
    /**
     * @brief Submit a prompt to the AI
     */
    void submitPrompt(const juce::String& prompt);
    
    //==========================================================================
    // Callbacks
    //==========================================================================
    
    std::function<void(const juce::String&)> onPromptSubmit;
    std::function<void(int)> onVariationSelected;
    std::function<void(const juce::String&)> onQuickActionTriggered;
    std::function<void(int, bool)> onStemSoloChanged;
    std::function<void(int, bool)> onStemMuteChanged;
    std::function<void()> onExportToArrangement;
    std::function<void()> onRegenerate;
    
    //==========================================================================
    // SkiaComponent Overrides
    //==========================================================================
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;
    
    void onAnimationTick(float deltaMs) override;

private:
    //==========================================================================
    // Layout Constants
    //==========================================================================
    
    static constexpr float kPanelPadding = 24.0f;
    static constexpr float kTitleHeight = 60.0f;
    static constexpr float kPromptBarHeight = 70.0f;
    static constexpr float kStemSectionHeight = 160.0f;
    static constexpr float kQuickActionsHeight = 100.0f;
    static constexpr float kLoopBarHeight = 60.0f;
    static constexpr float kChatHeight = 180.0f;
    static constexpr float kStemCardWidth = 140.0f;
    static constexpr float kVariationCardWidth = 180.0f;
    
    //==========================================================================
    // State
    //==========================================================================
    
    juce::String promptText_;
    std::vector<GeneratedStem> stems_;
    std::vector<AIChatMessage> chatHistory_;
    std::vector<QuickAction> quickActions_;
    
    bool isThinking_ = false;
    int loopBars_ = 8;
    float bpm_ = 120.0f;
    float playProgress_ = 0.0f;
    int activeVariation_ = 0;
    
    // Animation
    float animPhase_ = 0.0f;
    float thinkingPhase_ = 0.0f;
    float revealProgress_ = 1.0f;  // For entrance animation
    
    // Interaction
    int hoveredStem_ = -1;
    int hoveredQuickAction_ = -1;
    int hoveredVariation_ = -1;
    bool isPromptFocused_ = false;
    
    //==========================================================================
    // Drawing Methods
    //==========================================================================
    
    void drawGlassBackground(SkCanvas* canvas, const SkRect& bounds);
    void drawTitle(SkCanvas* canvas, const SkRect& bounds);
    void drawPromptBar(SkCanvas* canvas, const SkRect& bounds);
    void drawStemCards(SkCanvas* canvas, const SkRect& bounds);
    void drawSingleStemCard(SkCanvas* canvas, const SkRect& bounds, 
                            const GeneratedStem& stem, int index, bool isHovered);
    void drawVariationsPanel(SkCanvas* canvas, const SkRect& bounds);
    void drawQuickActions(SkCanvas* canvas, const SkRect& bounds);
    void drawLoopBar(SkCanvas* canvas, const SkRect& bounds);
    void drawChatPanel(SkCanvas* canvas, const SkRect& bounds);
    void drawThinkingIndicator(SkCanvas* canvas, float x, float y);
    
    void drawMiniWaveform(SkCanvas* canvas, const SkRect& bounds,
                          const std::vector<float>& samples, SkColor color);
    
    //==========================================================================
    // Hit Testing
    //==========================================================================

    int hitTestStem(float x, float y) const;
    int hitTestQuickAction(float x, float y) const;
    int hitTestVariation(float x, float y) const;
    bool hitTestPromptBar(float x, float y) const;
    int hitTestStemButton(float x, float y, int& buttonTypeRaw) const;
    
    //==========================================================================
    // Layout Calculation
    //==========================================================================
    
    SkRect getPromptBarRect() const;
    SkRect getStemSectionRect() const;
    SkRect getQuickActionsRect() const;
    SkRect getLoopBarRect() const;
    SkRect getChatPanelRect() const;
    
    void initializeQuickActions();
    void initializeDemoContent();
    void setupTextEditor();
    void showTextEditor();
    void hideTextEditor();
    void onTextEditorSubmit();

    // void handleStemButtonClick(int stemIndex, StemButtonType buttonType);

    //==========================================================================
    // Text Input
    //==========================================================================
    
    std::unique_ptr<juce::TextEditor> textEditor_;
    bool isTextEditorVisible_ = false;
    
    //==========================================================================
    // AI Integration
    //==========================================================================

    zenith::GrokDAWController* grokController_ = nullptr;
    bool isGenerating_ = false;

    // Safe async callback support
    class WeakPtrHolder;
    class WeakPtr;
    std::unique_ptr<WeakPtrHolder> weakPtrHolder_;
    WeakPtr getWeakPtr();

    // Pending generation request
    struct GenerationRequest {
        juce::String prompt;
        juce::Time timestamp;
    };
    std::vector<GenerationRequest> requestHistory_;
    
    // Generate stems based on AI response
    void generateStemsFromAIResponse(const juce::String& response);
    void addSystemMessage(const juce::String& text);
    std::vector<float> generateWaveformFromSeed(int seed);
    std::vector<float> generateWaveformFromAudio(const juce::File& audioFile);
    std::vector<float> getWaveformForStem(const GeneratedStem& stem);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaAIJamView)
};

} // namespace zenith::ui
