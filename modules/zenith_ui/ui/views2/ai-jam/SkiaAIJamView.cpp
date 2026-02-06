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

#include "SkiaAIJamView.h"
#include "../../design-system/ZenithTheme.h"
#include "../../../network/GrokDAWController.h"
#include "../../../engine/Engine.h"
#include "../../../engine/ProjectState.h"

namespace zenith::ui {

//==============================================================================
// WeakPtr Implementation (for safe async callbacks)
//==============================================================================

class SkiaAIJamView::WeakPtrHolder {
public:
    std::weak_ptr<bool> validFlag;
    SkiaAIJamView* view = nullptr;
};

class SkiaAIJamView::WeakPtr {
public:
    std::shared_ptr<bool> validFlag;
    SkiaAIJamView* view = nullptr;
    
    WeakPtr(SkiaAIJamView* v) 
        : validFlag(std::make_shared<bool>(true)), view(v) {}
    
    bool isValid() const { return validFlag && *validFlag && view != nullptr; }
    
    void invalidate() {
        if (validFlag) *validFlag = false;
        view = nullptr;
    }
};

//==============================================================================
// Construction/Destruction
//==============================================================================

SkiaAIJamView::SkiaAIJamView()
    : weakPtrHolder_(std::make_unique<WeakPtrHolder>())
{
    setWantsKeyboardFocus(true);
    setName("AI Jam View");
    
    initializeQuickActions();
    initializeDemoContent();
    
    weakPtrHolder_->validFlag = getWeakPtr().validFlag;
    weakPtrHolder_->view = this;
}

SkiaAIJamView::~SkiaAIJamView() {
    // Cancel any pending operations
    if (grokController_ && isGenerating_) {
        grokController_->cancel();
    }
    
    // Invalidate weak pointers
    weakPtrHolder_->view = nullptr;
}

//==============================================================================
// GrokController Integration
//==============================================================================

void SkiaAIJamView::setGrokController(zenith::GrokDAWController* controller) {
    grokController_ = controller;
    
    if (grokController_ != nullptr) {
        // Set up context provider for Grok
        grokController_->setContextProvider([this]() -> juce::var {
            // Provide current DAW context
            juce::DynamicObject::Ptr context = new juce::DynamicObject();
            
            context->setProperty("bpm", bpm_);
            context->setProperty("loopBars", loopBars_);
            context->setProperty("activeVariation", activeVariation_);
            context->setProperty("stemCount", static_cast<int>(stems_.size()));
            
            // Add stem info
            juce::Array<juce::var> stemInfo;
            for (const auto& stem : stems_) {
                juce::DynamicObject::Ptr stemObj = new juce::DynamicObject();
                stemObj->setProperty("name", stem.name);
                stemObj->setProperty("type", static_cast<int>(stem.type));
                stemObj->setProperty("isMuted", stem.isMuted);
                stemObj->setProperty("isSoloed", stem.isSoloed);
                stemInfo.add(stemObj.get());
            }
            context->setProperty("stems", stemInfo);
            
            // Add chat history context (last 5 messages)
            juce::Array<juce::var> chatContext;
            int startIdx = std::max(0, static_cast<int>(chatHistory_.size()) - 5);
            for (int i = startIdx; i < chatHistory_.size(); ++i) {
                juce::DynamicObject::Ptr msgObj = new juce::DynamicObject();
                msgObj->setProperty("fromUser", chatHistory_[i].fromUser);
                msgObj->setProperty("text", chatHistory_[i].text);
                chatContext.add(msgObj.get());
            }
            context->setProperty("recentChat", chatContext);
            
            return context.get();
        });
        
        // Add welcome message indicating connection
        addSystemMessage("Connected to Grok AI. Describe the music you want to create!");
    } else {
        addSystemMessage("AI features unavailable. Running in demo mode.");
    }
}

void SkiaAIJamView::submitPrompt(const juce::String& prompt) {
    if (prompt.isEmpty()) return;
    
    // Add user message to chat
    AIChatMessage userMsg;
    userMsg.fromUser = true;
    userMsg.text = prompt;
    userMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
    addChatMessage(userMsg);
    
    // Store prompt
    promptText_ = prompt;
    
    // Set thinking state
    setThinking(true);
    isGenerating_ = true;
    
    // Store request
    GenerationRequest request;
    request.prompt = prompt;
    request.timestamp = juce::Time::getCurrentTime();
    requestHistory_.push_back(request);
    
    // If we have a Grok controller, use it for real AI generation
    if (grokController_ != nullptr && grokController_->isReady()) {
        // Create a weak pointer for safe async callback
        auto weakPtr = getWeakPtr();
        
        // Execute command via GrokController
        grokController_->executeCommand(
            prompt,
            GrokMode::Fast,  // Use fast mode for stem generation
            // On success
            [this, weakPtr, prompt](juce::String response) {
                juce::MessageManager::callAsync([this, weakPtr, response, prompt]() {
                    if (!weakPtr.isValid()) return;
                    
                    setThinking(false);
                    isGenerating_ = false;
                    
                    // Add AI response to chat
                    AIChatMessage aiMsg;
                    aiMsg.fromUser = false;
                    aiMsg.text = response;
                    aiMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
                    addChatMessage(aiMsg);
                    
                    // Generate stems from the AI response
                    parseAIResponseAndGenerateStems(response);
                });
            },
            // On error
            [this, weakPtr](juce::String error) {
                juce::MessageManager::callAsync([this, weakPtr, error]() {
                    if (!weakPtr.isValid()) return;
                    
                    setThinking(false);
                    isGenerating_ = false;
                    
                    // Show error in chat
                    AIChatMessage errorMsg;
                    errorMsg.fromUser = false;
                    errorMsg.text = "Error: " + error;
                    errorMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
                    errorMsg.hasAction = true;
                    errorMsg.actionLabel = "Retry";
                    addChatMessage(errorMsg);
                    
                    if (onError)
                        onError(error);
                });
            },
            // On progress
            [this, weakPtr](juce::String status) {
                juce::MessageManager::callAsync([this, weakPtr, status]() {
                    if (!weakPtr.isValid()) return;
                    // Could update a progress indicator here
                    DBG("AI Generation progress: " + status);
                });
            }
        );
    } else {
        // No Grok controller available - use demo mode with delay
        juce::Timer::callAfterDelay(1500, [this, prompt]() {
            setThinking(false);
            isGenerating_ = false;
            
            // Generate demo stems
            generateDemoStemsForPrompt(prompt);
            
            // Add demo response
            AIChatMessage aiMsg;
            aiMsg.fromUser = false;
            aiMsg.text = "[Demo Mode] Generated stems for: " + prompt;
            aiMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
            addChatMessage(aiMsg);
            
            addSystemMessage("Connect to Grok AI for real generation.");
        });
    }
}

void SkiaAIJamView::parseAIResponseAndGenerateStems(const juce::String& response) {
    // Parse the AI response to determine what stems to generate
    // This is a smart parser that extracts intent and generates appropriate stems
    
    juce::String lowerResponse = response.toLowerCase();
    
    std::vector<GeneratedStem> newStems;
    
    // Determine genre/style from response
    bool isLoFi = lowerResponse.contains("lo-fi") || lowerResponse.contains("lofi") || 
                  lowerResponse.contains("chill") || lowerResponse.contains("ambient");
    bool isElectronic = lowerResponse.contains("electronic") || lowerResponse.contains("edm") ||
                        lowerResponse.contains("house") || lowerResponse.contains("techno");
    bool isRock = lowerResponse.contains("rock") || lowerResponse.contains("guitar");
    bool isJazz = lowerResponse.contains("jazz") || lowerResponse.contains("sax");
    bool isOrchestral = lowerResponse.contains("orchestral") || lowerResponse.contains("cinematic") ||
                        lowerResponse.contains("film");
    
    // Check for specific instrument requests
    bool wantsDrums = lowerResponse.contains("drum") || lowerResponse.contains("beat") ||
                      lowerResponse.contains("percussion") || lowerResponse.contains("kick");
    bool wantsBass = lowerResponse.contains("bass") || lowerResponse.contains("sub");
    bool wantsChords = lowerResponse.contains("chord") || lowerResponse.contains("pad") ||
                        lowerResponse.contains("harmony");
    bool wantsMelody = lowerResponse.contains("melody") || lowerResponse.contains("lead") ||
                        lowerResponse.contains("solo");
    bool wantsVocals = lowerResponse.contains("vocal") || lowerResponse.contains("voice") ||
                       lowerResponse.contains("singing");
    bool wantsFX = lowerResponse.contains("fx") || lowerResponse.contains("effect") ||
                   lowerResponse.contains("riser") || lowerResponse.contains("sweep");
    
    // Generate stems based on detected intent
    if (wantsDrums || newStems.empty()) {
        GeneratedStem drums;
        drums.type = StemType::Drums;
        drums.name = isLoFi ? "Lo-Fi Drums" : isElectronic ? "Electronic Drums" : "Acoustic Drums";
        drums.waveformPreview = generateWaveformFromSeed(response.hashCode() + 1);
        newStems.push_back(drums);
    }
    
    if (wantsBass || newStems.empty()) {
        GeneratedStem bass;
        bass.type = StemType::Bass;
        bass.name = isRock ? "Bass Guitar" : isElectronic ? "Sub Bass" : "Upright Bass";
        bass.waveformPreview = generateWaveformFromSeed(response.hashCode() + 2);
        newStems.push_back(bass);
    }
    
    if (wantsChords || (!isRock && !isJazz)) {
        GeneratedStem chords;
        chords.type = StemType::Chords;
        chords.name = isLoFi ? "Jazz Chords" : isOrchestral ? "String Pad" : "Synth Chords";
        chords.waveformPreview = generateWaveformFromSeed(response.hashCode() + 3);
        newStems.push_back(chords);
    }
    
    if (wantsMelody) {
        GeneratedStem melody;
        melody.type = StemType::Melody;
        melody.name = isJazz ? "Sax Melody" : isRock ? "Guitar Lead" : "Synth Lead";
        melody.waveformPreview = generateWaveformFromSeed(response.hashCode() + 4);
        newStems.push_back(melody);
    }
    
    if (wantsVocals) {
        GeneratedStem vocals;
        vocals.type = StemType::Vocals;
        vocals.name = "Vocals";
        vocals.waveformPreview = generateWaveformFromSeed(response.hashCode() + 5);
        newStems.push_back(vocals);
    }
    
    if (wantsFX) {
        GeneratedStem fx;
        fx.type = StemType::FX;
        fx.name = "FX & Atmosphere";
        fx.waveformPreview = generateWaveformFromSeed(response.hashCode() + 6);
        newStems.push_back(fx);
    }
    
    setStems(newStems);
    
    // Add quick action suggestions based on the generated content
    juce::String suggestions = "Try: ";
    if (!wantsDrums) suggestions += "Add drums, ";
    if (!wantsBass) suggestions += "Add bass, ";
    if (!wantsMelody) suggestions += "Add melody, ";
    suggestions += "Make it darker, Change genre, Extend to 16 bars";
    
    addSystemMessage(suggestions);
}

void SkiaAIJamView::generateDemoStemsForPrompt(const juce::String& prompt) {
    std::vector<GeneratedStem> newStems;
    
    // Create contextually relevant demo stems
    juce::String lowerPrompt = prompt.toLowerCase();
    
    GeneratedStem drums;
    drums.type = StemType::Drums;
    if (lowerPrompt.contains("chill") || lowerPrompt.contains("lofi")) {
        drums.name = "Chill Drums";
    } else if (lowerPrompt.contains("heavy") || lowerPrompt.contains("rock")) {
        drums.name = "Heavy Drums";
    } else {
        drums.name = "Drums";
    }
    drums.waveformPreview = generateWaveformFromSeed(prompt.hashCode() + 100);
    newStems.push_back(drums);
    
    GeneratedStem bass;
    bass.type = StemType::Bass;
    bass.name = "Bass";
    bass.waveformPreview = generateWaveformFromSeed(prompt.hashCode() + 200);
    newStems.push_back(bass);
    
    GeneratedStem chords;
    chords.type = StemType::Chords;
    chords.name = "Chords";
    chords.waveformPreview = generateWaveformFromSeed(prompt.hashCode() + 300);
    newStems.push_back(chords);
    
    if (lowerPrompt.contains("melody") || lowerPrompt.contains("lead")) {
        GeneratedStem melody;
        melody.type = StemType::Melody;
        melody.name = "Melody";
        melody.waveformPreview = generateWaveformFromSeed(prompt.hashCode() + 400);
        newStems.push_back(melody);
    }
    
    setStems(newStems);
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
    
    // 2. Draw Sections
    drawTitle(canvas, titleRect);
    drawPromptBar(canvas, promptRect);
    drawChatPanel(canvas, chatRect);

    // Add quick actions if enabled
    float quickActionsY = chatRect.bottom() + 20;
    SkRect quickActionsRect = SkRect::MakeXYWH(kPanelPadding, quickActionsY,
                                              skBounds.width() - (2 * kPanelPadding), kQuickActionsHeight);
    drawQuickActions(canvas, quickActionsRect);

    // Add variations panel if enabled
    float variationsY = quickActionsRect.bottom() + 20;
    SkRect variationsRect = SkRect::MakeXYWH(kPanelPadding, variationsY,
                                            skBounds.width() - (2 * kPanelPadding), 120);
    drawVariationsPanel(canvas, variationsRect);

    // Add loop bar if enabled
    float loopY = variationsRect.bottom() + 20;
    SkRect loopRect = SkRect::MakeXYWH(kPanelPadding, loopY,
                                      skBounds.width() - (2 * kPanelPadding), kLoopBarHeight);
    drawLoopBar(canvas, loopRect);

    // Draw stems at the bottom
    float stemsY = loopRect.bottom() + 20;
    SkRect stemsRect = SkRect::MakeXYWH(kPanelPadding, stemsY,
                                        skBounds.width() - (2 * kPanelPadding), kStemSectionHeight);
    drawStemCards(canvas, stemsRect);
    
    // 3. Thinking Indicator (Overlay)
    if (isThinking_) {
        drawThinkingIndicator(canvas, skBounds.centerX(), skBounds.centerY());
    }
    
    // 4. Connection status indicator
    if (grokController_ == nullptr || !grokController_->isReady()) {
        SkFont statusFont = design::getSkFont(10.0f);
        SkPaint statusPaint;
        statusPaint.setColor(design::colors::NEON_ORANGE);
        statusPaint.setAntiAlias(true);
        
        juce::String status = "[Demo Mode]";
        canvas->drawString(status.toStdString().c_str(), 
                          skBounds.width() - 80, 20, statusFont, statusPaint);
    }
}

void SkiaAIJamView::resized() {
    if (textEditor_ != nullptr) {
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
        thinkingPhase_ += deltaMs * 0.005f;
        markDirty();
    } else {
        thinkingPhase_ = 0.0f;
    }
    
    SkiaComponent::onAnimationTick(deltaMs);
}

//==============================================================================
// Mouse Handling
//==============================================================================

void SkiaAIJamView::mouseDown(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;
    
    // Check close button
    auto bounds = getLocalBounds().toFloat();
    float btnSize = 32.0f;
    SkRect closeBounds = SkRect::MakeXYWH(bounds.getRight() - btnSize - 16.0f, 
                                          kTitleHeight/2.0f - (btnSize/2.0f) + 8.0f, 
                                          btnSize, btnSize);
                                          
    if (closeBounds.contains(x, y)) {
        if (auto* p = findParentComponentOfClass<juce::Component>()) {
            p->removeChildComponent(this);
            delete this;
            return;
        }
    }
    
    // Check prompt bar
    if (hitTestPromptBar(x, y)) {
        isPromptFocused_ = true;
        showTextEditor();
    } else {
        if (isPromptFocused_) {
            isPromptFocused_ = false;
            hideTextEditor();
        }
    }
    
    // Check quick actions
    int quickActionIdx = hitTestQuickAction(x, y);
    if (quickActionIdx >= 0 && quickActionIdx < static_cast<int>(quickActions_.size())) {
        const auto& action = quickActions_[quickActionIdx];
        if (onQuickActionTriggered)
            onQuickActionTriggered(action.prompt);
        submitPrompt(action.prompt);
        return;
    }
    
    // Check variations
    int variationIdx = hitTestVariation(x, y);
    if (variationIdx >= 0) {
        setActiveVariation(variationIdx);
        return;
    }
    
    // Check stem buttons
    int btnType = 0;
    int stemIdx = hitTestStemButton(x, y, btnType);
    if (stemIdx >= 0 && stemIdx < static_cast<int>(stems_.size())) {
        auto& stem = stems_[stemIdx];
        
        if (btnType == static_cast<int>(StemButtonType::Solo)) {
            stem.isSoloed = !stem.isSoloed;
            if (onStemSoloChanged) onStemSoloChanged(stemIdx, stem.isSoloed);
        } else if (btnType == static_cast<int>(StemButtonType::Mute)) {
            stem.isMuted = !stem.isMuted;
            if (onStemMuteChanged) onStemMuteChanged(stemIdx, stem.isMuted);
        } else if (btnType == static_cast<int>(StemButtonType::Play)) {
            stem.isPlaying = !stem.isPlaying;
        }
        markDirty();
    }
}

void SkiaAIJamView::mouseDoubleClick(const juce::MouseEvent& e) {
    // Double-click on stem could open detailed editor
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;
    
    int stemIdx = hitTestStem(x, y);
    if (stemIdx >= 0 && stemIdx < static_cast<int>(stems_.size())) {
        // Could emit signal to open stem editor
        DBG("Double-clicked stem: " + stems_[stemIdx].name);
    }
}

void SkiaAIJamView::mouseMove(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;

    int previousHoveredStem = hoveredStem_;
    hoveredStem_ = hitTestStem(x, y);

    int previousHoveredQuickAction = hoveredQuickAction_;
    hoveredQuickAction_ = hitTestQuickAction(x, y);

    int previousHoveredVariation = hoveredVariation_;
    hoveredVariation_ = hitTestVariation(x, y);

    if (previousHoveredStem != hoveredStem_ ||
        previousHoveredQuickAction != hoveredQuickAction_ ||
        previousHoveredVariation != hoveredVariation_) {
        markDirty();
    }
}

void SkiaAIJamView::mouseExit(const juce::MouseEvent&) {
    hoveredStem_ = -1;
    hoveredQuickAction_ = -1;
    hoveredVariation_ = -1;
    markDirty();
}

bool SkiaAIJamView::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    if (key == juce::KeyPress::returnKey) {
        if (isPromptFocused_ && textEditor_ != nullptr) {
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
        if (auto* p = findParentComponentOfClass<juce::Component>()) {
            p->removeChildComponent(this);
            delete this;
            return true;
        }
    }
    
    // Keyboard shortcuts for variations
    if (key.getKeyCode() >= '1' && key.getKeyCode() <= '4') {
        setActiveVariation(key.getKeyCode() - '1');
        return true;
    }
    
    return SkiaComponent::keyPressed(key, origin);
}

//==============================================================================
// State Management
//==============================================================================

void SkiaAIJamView::setPromptText(const juce::String& text) {
    promptText_ = text;
    if (textEditor_ != nullptr) {
        textEditor_->setText(text, false);
    }
    markDirty();
}

void SkiaAIJamView::setStems(const std::vector<GeneratedStem>& stems) {
    stems_ = stems;
    markDirty();
}

void SkiaAIJamView::addChatMessage(const AIChatMessage& message) {
    chatHistory_.push_back(message);
    markDirty();

    if (onChatScrolledToBottom)
        onChatScrolledToBottom();
}

void SkiaAIJamView::clearChat() {
    chatHistory_.clear();
    markDirty();
}

void SkiaAIJamView::setThinking(bool thinking) {
    if (isThinking_ != thinking) {
        isThinking_ = thinking;
        markDirty();

        if (thinking) {
            startTimerHz(60);
        } else {
            stopTimer();
        }
    }
}

void SkiaAIJamView::setLoopLength(int bars) {
    loopBars_ = juce::jlimit(1, 64, bars);
    markDirty();
}

void SkiaAIJamView::setBPM(float bpm) {
    bpm_ = juce::jlimit(20.0f, 999.0f, bpm);
    markDirty();
}

void SkiaAIJamView::setPlayPosition(float progress) {
    playProgress_ = juce::jlimit(0.0f, 1.0f, progress);
    markDirty();
}

void SkiaAIJamView::setActiveVariation(int index) {
    activeVariation_ = juce::jlimit(0, 3, index);
    markDirty();

    if (onVariationSelected)
        onVariationSelected(activeVariation_);
    
    // Request variation from AI if connected
    if (grokController_ != nullptr && grokController_->isReady()) {
        juce::String variationPrompt = "Generate variation " + juce::String(char('A' + activeVariation_)) + 
                                       " of the current stems";
        submitPrompt(variationPrompt);
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

void SkiaAIJamView::initializeQuickActions() {
    quickActions_.clear();

    QuickAction makeDarker;
    makeDarker.emoji = "🌑";
    makeDarker.label = "Darker";
    makeDarker.prompt = "Make this darker and moodier with more bass";
    quickActions_.push_back(makeDarker);

    QuickAction addBridge;
    addBridge.emoji = "🎵";
    addBridge.label = "Bridge";
    addBridge.prompt = "Add a bridge section that builds tension";
    quickActions_.push_back(addBridge);

    QuickAction changeGenre;
    changeGenre.emoji = "🎭";
    changeGenre.label = "Genre";
    changeGenre.prompt = "Transform this into a different genre";
    quickActions_.push_back(changeGenre);

    QuickAction extendSection;
    extendSection.emoji = "➕";
    extendSection.label = "Extend";
    extendSection.prompt = "Extend this to 16 bars with more variation";
    quickActions_.push_back(extendSection);
}

void SkiaAIJamView::initializeDemoContent() {
    AIChatMessage welcomeMsg;
    welcomeMsg.fromUser = false;
    welcomeMsg.text = "Welcome to AI Jam! Describe what you want to create, and I'll generate stems for you.";
    welcomeMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
    addChatMessage(welcomeMsg);

    promptText_ = "Create a chill lo-fi beat";

    std::vector<GeneratedStem> demoStems;

    GeneratedStem drums;
    drums.type = StemType::Drums;
    drums.name = "Lo-Fi Drums";
    drums.waveformPreview = generateWaveformFromSeed(12345);
    demoStems.push_back(drums);

    GeneratedStem bass;
    bass.type = StemType::Bass;
    bass.name = "Smooth Bass";
    bass.waveformPreview = generateWaveformFromSeed(23456);
    demoStems.push_back(bass);

    setStems(demoStems);

    loopBars_ = 8;
    bpm_ = 120.0f;
    playProgress_ = 0.0f;
}

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
    textEditor_->setText(promptText_, false);
    textEditor_->setVisible(true);
    textEditor_->grabKeyboardFocus();
    isTextEditorVisible_ = true;
}

void SkiaAIJamView::hideTextEditor() {
    if (textEditor_) {
        promptText_ = textEditor_->getText();
        textEditor_->setVisible(false);
        isTextEditorVisible_ = false;
    }
    markDirty();
}

void SkiaAIJamView::onTextEditorSubmit() {
    if (textEditor_) {
        juce::String text = textEditor_->getText();
        if (text.isNotEmpty()) {
            submitPrompt(text);
            textEditor_->clear();
        }
        isPromptFocused_ = false;
        hideTextEditor();
    }
}

void SkiaAIJamView::addSystemMessage(const juce::String& text) {
    AIChatMessage sysMsg;
    sysMsg.fromUser = false;
    sysMsg.text = text;
    sysMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
    sysMsg.hasAction = true;
    addChatMessage(sysMsg);
}

std::vector<float> SkiaAIJamView::generateWaveformFromSeed(int seed) {
    std::vector<float> waveform(64);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    // Generate interesting waveform patterns
    for (int i = 0; i < 64; ++i) {
        float t = i / 64.0f;
        
        // Combine sine waves with noise for organic look
        float sine1 = std::sin(t * 10.0f + seed) * 0.3f;
        float sine2 = std::sin(t * 23.0f + seed * 0.5f) * 0.2f;
        float noise = dis(gen) * 0.3f;
        
        // Add envelope
        float envelope = std::sin(t * 3.14159f) * 0.5f + 0.5f;
        
        waveform[i] = (sine1 + sine2 + noise) * envelope;
    }

    return waveform;
}

std::vector<float> SkiaAIJamView::generateWaveformFromAudio(const juce::File& audioFile) {
    if (!audioFile.existsAsFile()) {
        return generateWaveformFromSeed(audioFile.hashCode());
    }
    
    // TODO: Implement actual audio analysis
    // Would use AudioFormatManager to read file and calculate RMS per frame
    return generateWaveformFromSeed(audioFile.hashCode());
}

std::vector<float> SkiaAIJamView::getWaveformForStem(const GeneratedStem& stem) {
    if (!stem.waveformPreview.empty()) {
        return stem.waveformPreview;
    }
    return generateWaveformFromSeed(stem.name.hashCode());
}

//==============================================================================
// Drawing Methods
//==============================================================================

void SkiaAIJamView::drawGlassBackground(SkCanvas* canvas, const SkRect& bounds) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 16.0f;
    opts.useBackdropBlur = true;
    opts.customTintColor = SkColorSetA(design::colors::BG_00, 240);
    
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
}

void SkiaAIJamView::drawTitle(SkCanvas* canvas, const SkRect& bounds) {
    SkFont titleFont = design::getDisplayFont(24.0f);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    
    juce::String title = "AI Jam Session";
    std::string titleStr = title.toStdString();
    
    float textWidth = titleFont.measureText(titleStr.c_str(), titleStr.length(), SkTextEncoding::kUTF8);
    float x = bounds.centerX() - (textWidth / 2.0f);
    float y = bounds.centerY() + 8.0f;
    
    canvas->drawString(titleStr.c_str(), x, y, titleFont, textPaint);
    
    SkFont subFont = design::getSkFont(12.0f);
    textPaint.setColor(design::colors::TEXT_SECONDARY);
    
    juce::String sub = grokController_ && grokController_->isReady() 
                       ? "Powered by Grok AI" : "[Demo Mode]";
    std::string subStr = sub.toStdString();
    float subWidth = subFont.measureText(subStr.c_str(), subStr.length(), SkTextEncoding::kUTF8);
    
    canvas->drawString(subStr.c_str(), bounds.centerX() - (subWidth / 2.0f), y + 20, subFont, textPaint);

    // Close button
    float btnSize = 32.0f;
    float padding = 16.0f;
    SkRect btnRect = SkRect::MakeXYWH(bounds.right() - btnSize - padding, 
                                      bounds.centerY() - (btnSize/2), 
                                      btnSize, btnSize);
                                      
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
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
    
    if (promptText_.isEmpty() && !isPromptFocused_ && !isTextEditorVisible_) {
        SkFont font = design::getSkFont(16.0f);
        SkPaint paint;
        paint.setColor(design::colors::TEXT_TERTIARY);
        paint.setAntiAlias(true);
        
        juce::String ph = "Describe the music you want to create...";
        canvas->drawString(ph.toStdString().c_str(), bounds.left() + 16, bounds.centerY() + 6, font, paint);
    }
    
    if (promptText_.isNotEmpty() && !isTextEditorVisible_) {
        SkFont font = design::getSkFont(16.0f);
        SkPaint paint;
        paint.setColor(design::colors::TEXT_PRIMARY);
        paint.setAntiAlias(true);
        canvas->drawString(promptText_.toStdString().c_str(), bounds.left() + 16, bounds.centerY() + 6, font, paint);
    }
}

void SkiaAIJamView::drawStemCards(SkCanvas* canvas, const SkRect& bounds) {
    if (stems_.empty()) {
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
        
        if (cardRect.right() > bounds.right()) break;
        
        bool isHovered = (static_cast<int>(i) == hoveredStem_);
        drawSingleStemCard(canvas, cardRect, stems_[i], static_cast<int>(i), isHovered);
        
        x += kStemCardWidth + spacing;
    }
}

void SkiaAIJamView::drawSingleStemCard(SkCanvas* canvas, const SkRect& bounds, 
                            const GeneratedStem& stem, int index, bool isHovered) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    
    // Highlight if hovered or playing
    if (isHovered) {
        opts.customTintColor = SkColorSetA(design::colors::ACCENT_PRIMARY, 40);
    }
    
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
    
    // Stem name
    SkFont font = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(stem.isMuted ? design::colors::TEXT_TERTIARY : design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    
    juce::String name = stem.name.isNotEmpty() ? stem.name : "Stem " + juce::String(index + 1);
    canvas->drawString(name.toStdString().c_str(), bounds.fLeft + 12, bounds.fTop + 24, font, textPaint);
    
    // Waveform
    SkRect waveRect = SkRect::MakeXYWH(bounds.fLeft + 4, bounds.fTop + 36, bounds.width() - 8, 80);
    auto waveform = getWaveformForStem(stem);
    SkColor waveColor = stem.isMuted ? design::colors::TEXT_TERTIARY : 
                        stem.isSoloed ? design::colors::NEON_GREEN : design::colors::ACCENT_PRIMARY;
    drawMiniWaveform(canvas, waveRect, waveform, waveColor);
    
    // Control buttons
    float btnWidth = bounds.width() / 3.0f;
    float btnY = bounds.bottom() - 32.0f;
    SkFont btnFont = design::getSkFont(12.0f);
    
    // Solo
    if (stem.isSoloed) textPaint.setColor(design::colors::NEON_GREEN);
    else textPaint.setColor(design::colors::TEXT_SECONDARY);
    canvas->drawString("S", bounds.fLeft + (btnWidth * 0.5f) - 4, btnY + 20, btnFont, textPaint);
    
    // Mute
    if (stem.isMuted) textPaint.setColor(design::colors::NEON_RED);
    else textPaint.setColor(design::colors::TEXT_SECONDARY);
    canvas->drawString("M", bounds.fLeft + (btnWidth * 1.5f) - 4, btnY + 20, btnFont, textPaint);
    
    // Play indicator
    textPaint.setColor(stem.isPlaying ? design::colors::ACCENT_PRIMARY : design::colors::TEXT_SECONDARY);
    canvas->drawString("P", bounds.fLeft + (btnWidth * 2.5f) - 4, btnY + 20, btnFont, textPaint);
}

void SkiaAIJamView::drawVariationsPanel(SkCanvas* canvas, const SkRect& bounds) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    SkFont titleFont = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString("Variations", bounds.left() + 16, bounds.top() + 20, titleFont, titlePaint);

    float cardWidth = kVariationCardWidth;
    float spacing = 16.0f;
    float startX = bounds.left() + 16;
    float cardY = bounds.top() + 40;

    for (int i = 0; i < 4; ++i) {
        SkRect cardRect = SkRect::MakeXYWH(startX + i * (cardWidth + spacing), cardY, 
                                           cardWidth, bounds.height() - 60);
        
        bool isHovered = (i == hoveredVariation_);
        bool isActive = (i == activeVariation_);

        if (isActive) {
            SkPaint activeBg;
            activeBg.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
            canvas->drawRoundRect(cardRect, 8.0f, 8.0f, activeBg);
        } else if (isHovered) {
            SkPaint hoverBg;
            hoverBg.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 15));
            canvas->drawRoundRect(cardRect, 8.0f, 8.0f, hoverBg);
        } else {
            GlassmorphicPanel::Options cardOpts;
            cardOpts.style = GlassmorphicPanel::Style::Subtle;
            cardOpts.cornerRadius = 8.0f;
            GlassmorphicPanel::drawWithOptions(canvas, cardRect, cardOpts);
        }

        SkFont labelFont = design::getSkFont(18.0f, design::FontWeight::Bold);
        SkPaint labelPaint;
        labelPaint.setColor(isActive ? design::colors::ACCENT_PRIMARY : design::colors::TEXT_SECONDARY);
        labelPaint.setAntiAlias(true);
        canvas->drawString(juce::String::charToString('A' + i), 
                          cardRect.centerX() - 8, cardRect.top() + 25, labelFont, labelPaint);

        // Mini waveform
        SkRect waveRect = SkRect::MakeXYWH(cardRect.left() + 8, cardRect.top() + 35, 
                                           cardRect.width() - 16, 30);
        std::vector<float> waveSamples = generateWaveformFromSeed(i * 12345);
        drawMiniWaveform(canvas, waveRect, waveSamples, 
                        isActive ? design::colors::ACCENT_PRIMARY : design::colors::TEXT_TERTIARY);
    }
}

void SkiaAIJamView::drawQuickActions(SkCanvas* canvas, const SkRect& bounds) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    SkFont titleFont = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString("Quick Actions", bounds.left() + 16, bounds.top() + 20, titleFont, titlePaint);

    float buttonSize = 60.0f;
    float spacing = 16.0f;
    float startX = bounds.left() + 16;
    float buttonY = bounds.top() + 50;

    for (size_t i = 0; i < quickActions_.size() && i < 4; ++i) {
        SkRect buttonRect = SkRect::MakeXYWH(startX + i * (buttonSize + spacing), 
                                             buttonY, buttonSize, buttonSize);

        SkPaint bgPaint;
        if (static_cast<int>(i) == hoveredQuickAction_) {
            bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
        } else {
            bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
        }
        bgPaint.setAntiAlias(true);
        canvas->drawRoundRect(buttonRect, 12.0f, 12.0f, bgPaint);

        SkFont emojiFont = design::getSkFont(24.0f);
        SkPaint emojiPaint;
        emojiPaint.setColor(design::colors::TEXT_PRIMARY);
        emojiPaint.setAntiAlias(true);
        canvas->drawString(quickActions_[i].emoji.toStdString().c_str(),
                          buttonRect.centerX() - 8, buttonRect.centerY() + 8, emojiFont, emojiPaint);

        SkFont labelFont = design::getSkFont(10.0f);
        SkPaint labelPaint;
        labelPaint.setColor(design::colors::TEXT_SECONDARY);
        labelPaint.setAntiAlias(true);
        float labelWidth = labelFont.measureText(quickActions_[i].label.toStdString().c_str(),
                            quickActions_[i].label.length(), SkTextEncoding::kUTF8);
        canvas->drawString(quickActions_[i].label.toStdString().c_str(),
                          buttonRect.centerX() - labelWidth / 2.0f,
                          buttonRect.bottom() + 16, labelFont, labelPaint);
    }
}

void SkiaAIJamView::drawLoopBar(SkCanvas* canvas, const SkRect& bounds) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    SkFont titleFont = design::getSkFont(12.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);
    canvas->drawString("Loop: " + juce::String(loopBars_) + " bars", 
                      bounds.left() + 16, bounds.top() + 20, titleFont, titlePaint);

    SkFont bpmFont = design::getSkFont(12.0f);
    SkPaint bpmPaint;
    bpmPaint.setColor(design::colors::TEXT_SECONDARY);
    bpmPaint.setAntiAlias(true);
    canvas->drawString("BPM: " + juce::String((int)bpm_), 
                      bounds.left() + bounds.width() - 80, bounds.top() + 20, bpmFont, bpmPaint);

    float trackY = bounds.centerY() - 4;
    float trackHeight = 8.0f;
    SkRect trackRect = SkRect::MakeXYWH(bounds.left() + 16, trackY, bounds.width() - 32, trackHeight);

    SkPaint trackBg;
    trackBg.setColor(SkColorSetA(design::colors::BG_01, 150));
    canvas->drawRoundRect(trackRect, 4.0f, 4.0f, trackBg);

    float playheadX = bounds.left() + 16 + (trackRect.width() * playProgress_);
    SkRect playheadRect = SkRect::MakeXYWH(playheadX - 2, trackRect.top() - 2, 4, trackHeight + 4);

    SkPaint playheadPaint;
    playheadPaint.setColor(design::colors::ACCENT_PRIMARY);
    canvas->drawRoundRect(playheadRect, 2.0f, 2.0f, playheadPaint);

    if (playProgress_ > 0) {
        SkRect progressRect = SkRect::MakeXYWH(trackRect.left(), trackRect.top(),
                                             trackRect.width() * playProgress_, trackRect.height());
        SkPaint progressPaint;
        progressPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 100));
        canvas->drawRoundRect(progressRect, 4.0f, 4.0f, progressPaint);
    }
}

void SkiaAIJamView::drawChatPanel(SkCanvas* canvas, const SkRect& bounds) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);
    
    float y = bounds.bottom() - 20;
    SkFont msgFont = design::getSkFont(14.0f);
    SkFont timeFont = design::getSkFont(10.0f);
    
    for (auto it = chatHistory_.rbegin(); it != chatHistory_.rend(); ++it) {
        if (y < bounds.top() + 20) break;
        
        const auto& msg = *it;
        
        // Message background for user messages
        if (msg.fromUser) {
            SkPaint userBg;
            userBg.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 20));
            userBg.setAntiAlias(true);
            
            juce::String prefix = "You: ";
            float prefixWidth = msgFont.measureText(prefix.toRawUTF8(), prefix.getNumBytesAsUTF8());
            float textWidth = msgFont.measureText(msg.text.toRawUTF8(), msg.text.getNumBytesAsUTF8());
            
            SkRect msgRect = SkRect::MakeXYWH(bounds.left() + 12, y - 18, 
                                              prefixWidth + textWidth + 16, 24);
            canvas->drawRoundRect(msgRect, 6.0f, 6.0f, userBg);
        }
        
        // Sender name
        SkPaint namePaint;
        namePaint.setColor(msg.fromUser ? design::colors::ACCENT_PRIMARY : design::colors::ACCENT_SECONDARY);
        namePaint.setAntiAlias(true);
        juce::String sender = msg.fromUser ? "You: " : "AI: ";
        canvas->drawString(sender.toStdString().c_str(), bounds.left() + 16, y, msgFont, namePaint);
        
        // Message text
        SkPaint textPaint;
        textPaint.setColor(design::colors::TEXT_PRIMARY);
        textPaint.setAntiAlias(true);
        float nameWidth = msgFont.measureText(sender.toRawUTF8(), sender.getNumBytesAsUTF8());
        canvas->drawString(msg.text.toStdString().c_str(), bounds.left() + 20 + nameWidth, y, msgFont, textPaint);
        
        // Timestamp
        SkPaint timePaint;
        timePaint.setColor(design::colors::TEXT_TERTIARY);
        timePaint.setAntiAlias(true);
        canvas->drawString(msg.timestamp.substring(11, 16).toStdString().c_str(),
                          bounds.right() - 50, y, timeFont, timePaint);
        
        y -= 28.0f;
    }
}

void SkiaAIJamView::drawThinkingIndicator(SkCanvas* canvas, float x, float y) {
    SkPaint paint;
    paint.setColor(design::colors::ACCENT_PRIMARY);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(3.0f);
    paint.setAntiAlias(true);
    
    float radius = 20.0f + (sinf(thinkingPhase_) * 5.0f);
    float alpha = 0.5f + (sinf(thinkingPhase_) * 0.5f);
    paint.setAlphaf(alpha);
    
    canvas->drawCircle(x, y, radius, paint);
    
    // Animated dots
    SkFont font = design::getSkFont(14.0f, design::FontWeight::Bold);
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    
    int dotCount = static_cast<int>(thinkingPhase_ / 2.0f) % 4;
    juce::String dots = "Thinking";
    for (int i = 0; i < dotCount; ++i) dots += ".";
    
    float w = font.measureText(dots.toRawUTF8(), dots.getNumBytesAsUTF8());
    canvas->drawString(dots.toStdString().c_str(), x - (w/2), y + radius + 25, font, textPaint);
}

void SkiaAIJamView::drawMiniWaveform(SkCanvas* canvas, const SkRect& bounds,
                          const std::vector<float>& samples, SkColor color) {
    if (samples.empty() || samples.size() < 2) return;

    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(bounds, 4.0f, 4.0f, bgPaint);

    SkPaint wavePaint;
    wavePaint.setColor(color);
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setStrokeWidth(1.5f);
    wavePaint.setAntiAlias(true);

    float centerY = bounds.centerY();
    float stepX = bounds.width() / (float)samples.size();
    float scaleY = bounds.height() * 0.35f;

    SkPath path;
    for (size_t i = 0; i < samples.size(); ++i) {
        float x = bounds.left() + i * stepX;
        float y = centerY + samples[i] * scaleY;

        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    canvas->drawPath(path, wavePaint);

    // Glow for active stems
    if (color == design::colors::ACCENT_PRIMARY || color == design::colors::NEON_GREEN) {
        SkPaint glowPaint;
        glowPaint.setColor(color);
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(3.0f);
        glowPaint.setAlphaf(0.2f);
        canvas->drawPath(path, glowPaint);
    }
}

//==============================================================================
// Hit Testing
//==============================================================================

bool SkiaAIJamView::hitTestPromptBar(float x, float y) const {
    return getPromptBarRect().contains(x, y);
}

int SkiaAIJamView::hitTestStem(float x, float y) const {
    auto sectionRect = getStemSectionRect();
    if (!sectionRect.contains(x, y)) return -1;
    
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
    
    auto sectionRect = getStemSectionRect();
    float localY = y - sectionRect.fTop;
    float buttonAreaY = kStemSectionHeight - 40.0f;
    
    if (localY < buttonAreaY) return -1;
    
    float relX = x - sectionRect.fLeft;
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
    float quickActionsY = chatY + kChatHeight + 20;
    float variationsY = quickActionsY + kQuickActionsHeight + 20;
    float loopY = variationsY + 120 + 20;
    float stemsY = loopY + kLoopBarHeight + 20;
    return SkRect::MakeXYWH(kPanelPadding, stemsY, 
                            bounds.getWidth() - (2 * kPanelPadding), kStemSectionHeight);
}

SkRect SkiaAIJamView::getChatPanelRect() const {
    auto bounds = getLocalBounds().toFloat();
    float promptY = kTitleHeight + 20;
    float chatY = promptY + kPromptBarHeight + 20;
    return SkRect::MakeXYWH(kPanelPadding, chatY, 
                            bounds.getWidth() - (2 * kPanelPadding), kChatHeight);
}

int SkiaAIJamView::hitTestQuickAction(float x, float y) const {
    auto bounds = getLocalBounds().toFloat();
    float promptY = kTitleHeight + 20;
    float chatY = promptY + kPromptBarHeight + 20;
    float quickActionsY = chatY + kChatHeight + 20;
    SkRect quickActionsRect = SkRect::MakeXYWH(kPanelPadding, quickActionsY,
                                              bounds.getWidth() - (2 * kPanelPadding), kQuickActionsHeight);

    if (!quickActionsRect.contains(x, y)) return -1;

    float buttonSize = 60.0f;
    float spacing = 16.0f;
    float startX = kPanelPadding + 16;
    float buttonY = quickActionsY + 50;

    for (int i = 0; i < 4; ++i) {
        SkRect buttonRect = SkRect::MakeXYWH(startX + i * (buttonSize + spacing), buttonY, buttonSize, buttonSize);
        if (buttonRect.contains(x, y)) return i;
    }

    return -1;
}

int SkiaAIJamView::hitTestVariation(float x, float y) const {
    auto bounds = getLocalBounds().toFloat();
    float promptY = kTitleHeight + 20;
    float chatY = promptY + kPromptBarHeight + 20;
    float quickActionsY = chatY + kChatHeight + 20;
    float variationsY = quickActionsY + kQuickActionsHeight + 20;
    float variationsHeight = 120.0f;
    
    SkRect variationsRect = SkRect::MakeXYWH(kPanelPadding, variationsY,
                                            bounds.getWidth() - (2 * kPanelPadding), variationsHeight);

    if (!variationsRect.contains(x, y)) return -1;

    float cardWidth = kVariationCardWidth;
    float spacing = 16.0f;
    float startX = kPanelPadding + 16;
    float cardY = variationsY + 40;

    for (int i = 0; i < 4; ++i) {
        SkRect cardRect = SkRect::MakeXYWH(startX + i * (cardWidth + spacing), cardY, 
                                           cardWidth, variationsHeight - 60);
        if (cardRect.contains(x, y)) return i;
    }

    return -1;
}

SkiaAIJamView::WeakPtr SkiaAIJamView::getWeakPtr() { 
    return WeakPtr(this); 
}

} // namespace zenith::ui
