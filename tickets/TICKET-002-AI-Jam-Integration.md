# TICKET-002: Complete AI Jam View Integration

**Priority**: P0 (Critical)  
**Type**: Feature Implementation  
**Estimated Effort**: 3-4 days  
**Component**: UI / AI / Networking  

---

## Problem Statement

The AI Jam view exists visually but has **no functional integration** with the Grok AI system:

1. **`setGrokController()` is an empty stub** (SkiaAIJamView.cpp:294-296)
2. **View switching disabled** in ZenithMainLayout (only Arrangement/Session wired)
3. **No async API handling** - prompts would block UI thread
4. **No error handling** for API failures

---

## Evidence

### Empty Controller Integration
```cpp
// SkiaAIJamView.cpp:294-296
void SkiaAIJamView::setGrokController(zenith::GrokDAWController* controller) {
    grokController_ = controller;  // Stores pointer, never uses it
}
```

### Disabled View Switching
```cpp
// ZenithMainLayout.cpp:55-66
void ZenithMainLayout::setupCallbacks() {
    transportBar_->onViewChange = [this](SkiaTransportBar::ActiveView view) {
        switch (view) {
            case SkiaTransportBar::ActiveView::Arrangement:
                viewSwitcher_->setActiveView(ViewType::Arrangement);
                break;
            case SkiaTransportBar::ActiveView::Session:
                viewSwitcher_->setActiveView(ViewType::Session);
                break;
            // AIJam case MISSING
        }
    };
}
```

### Synchronous Demo Mode Only
```cpp
// SkiaAIJamView.cpp:316-327
if (grokController_) {
    // TODO: Implement async API call to Grok
    // For now, simulate a response
    juce::Timer::callAfterDelay(2000, [this, prompt]() {
        generateStemsFromAIResponse("Generated stems based on prompt: " + prompt);
    });
} else {
    // Demo mode - generate mock stems
    generateStemsFromAIResponse("Demo stems generated for prompt: " + prompt);
}
```

---

## Requirements

### Functional Requirements

1. **Real Grok API Integration**: AI Jam must make actual API calls to Grok
2. **Async Operation**: API calls must not block UI thread
3. **View Switching**: AI Jam must be accessible via transport bar
4. **Error Handling**: Network failures must be handled gracefully
5. **Stem Integration**: Generated stems must be loadable into the project

### Non-Functional Requirements

1. **Response Time**: First response within 5 seconds (streaming)
2. **UI Responsiveness**: No UI freezing during API calls
3. **Memory Safety**: No leaks during async operations
4. **Thread Safety**: All UI updates on message thread

---

## Implementation Plan

### Step 1: Add AI Jam to ViewSwitcher (4 hours)

```cpp
// ViewSwitcher.h - Add AI Jam view

class ViewSwitcher : public SkiaComponent {
private:
    std::unique_ptr<SkiaArrangementView> arrangementView_;
    std::unique_ptr<SkiaSessionView> sessionView_;
    std::unique_ptr<SkiaAIJamView> aiJamView_;  // ADD
    
    // Track AI Jam visibility separately (it's an overlay, not replacement)
    bool aiJamVisible_ = false;
    
public:
    void setAIJamVisible(bool visible);
    bool isAIJamVisible() const { return aiJamVisible_; }
    
    SkiaAIJamView* getAIJamView() { return aiJamView_.get(); }
};

// ViewSwitcher.cpp - Implementation

ViewSwitcher::ViewSwitcher(Engine& engine, ProjectState& projectState) {
    // ... existing view creation ...
    
    // Create AI Jam view
    aiJamView_ = std::make_unique<SkiaAIJamView>();
    addChildComponent(aiJamView_.get());
    aiJamView_->setVisible(false);
}

void ViewSwitcher::drawSkia(SkCanvas* canvas) {
    // Draw active main view (Arrangement or Session)
    switch (currentView_) {
        case ViewType::Arrangement:
            drawArrangementView(canvas, arrangementAlpha);
            break;
        case ViewType::Session:
            drawSessionView(canvas, sessionAlpha);
            break;
    }
    
    // Draw AI Jam overlay if visible (on top)
    if (aiJamVisible_ && aiJamView_) {
        canvas->save();
        canvas->translate(aiJamView_->getX(), aiJamView_->getY());
        aiJamView_->drawSkia(canvas);
        canvas->restore();
    }
}

void ViewSwitcher::setAIJamVisible(bool visible) {
    if (aiJamVisible_ == visible) return;
    
    aiJamVisible_ = visible;
    aiJamView_->setVisible(visible);
    
    if (visible) {
        aiJamView_->grabKeyboardFocus();
    }
    
    markDirty();
}
```

### Step 2: Enable AI Jam View Switching (2 hours)

```cpp
// SkiaTransportBar.h - Add AI Jam to ActiveView enum

enum class ActiveView { 
    Arrangement, 
    Session,
    AIJam  // ADD
};

// SkiaTransportBar.cpp - Add AI Jam button

void SkiaTransportBar::drawViewToggle(SkCanvas* canvas) {
    // ... existing Arrangement/Session buttons ...
    
    // ADD: AI Jam button
    drawButton(canvas, x, y, BTN_VIEW_JAM, "AI", activeView_ == ActiveView::AIJam, true);
}

// ZenithMainLayout.cpp - Wire AI Jam switching

void ZenithMainLayout::setupCallbacks() {
    transportBar_->onViewChange = [this](SkiaTransportBar::ActiveView view) {
        switch (view) {
            case SkiaTransportBar::ActiveView::Arrangement:
                viewSwitcher_->setAIJamVisible(false);  // Hide AI Jam
                viewSwitcher_->setActiveView(ViewType::Arrangement);
                break;
            case SkiaTransportBar::ActiveView::Session:
                viewSwitcher_->setAIJamVisible(false);  // Hide AI Jam
                viewSwitcher_->setActiveView(ViewType::Session);
                break;
            case SkiaTransportBar::ActiveView::AIJam:  // ADD
                viewSwitcher_->setAIJamVisible(true);
                break;
        }
    };
}
```

### Step 3: Implement Async Grok Integration (8 hours)

```cpp
// SkiaAIJamView.h - Refactor for async

class SkiaAIJamView : public SkiaComponent {
public:
    void setGrokController(zenith::GrokDAWController* controller);
    
private:
    zenith::GrokDAWController* grokController_ = nullptr;
    
    // Async handling
    std::unique_ptr<juce::ThreadPool> aiThreadPool_;
    std::atomic<bool> hasPendingRequest_{false};
    std::atomic<bool> requestCancelled_{false};
    
    // Cancel previous request when new one starts
    juce::CancellationToken currentToken_;
    
    // Internal methods
    void submitPromptInternal(const juce::String& prompt);
    void handleAIResponse(const juce::var& response);
    void handleAIError(const juce::String& error);
    
    // Stem generation from response
    void generateStemsFromResponse(const juce::var& response);
};

// SkiaAIJamView.cpp - Async implementation

SkiaAIJamView::SkiaAIJamView() {
    // Create thread pool for AI operations (2 threads max)
    aiThreadPool_ = std::make_unique<juce::ThreadPool>(2);
    
    initializeQuickActions();
    initializeDemoContent();  // Keep demo content as fallback
}

void SkiaAIJamView::submitPrompt(const juce::String& prompt) {
    if (prompt.isEmpty()) return;
    
    // Add user message to chat
    AIChatMessage userMsg;
    userMsg.fromUser = true;
    userMsg.text = prompt;
    userMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
    addChatMessage(userMsg);
    
    // Cancel any pending request
    requestCancelled_ = true;
    
    // Check if we have a real controller
    if (!grokController_) {
        // Demo mode fallback
        setThinking(true);
        juce::Timer::callAfterDelay(2000, [this, prompt]() {
            generateStemsFromAIResponse("Demo: " + prompt);
        });
        return;
    }
    
    // Real async implementation
    setThinking(true);
    hasPendingRequest_ = true;
    requestCancelled_ = false;
    
    // Capture state for thread safety
    auto* controller = grokController_;
    juce::WeakReference<SkiaAIJamView> weakThis(this);
    
    aiThreadPool_->addJob([this, weakThis, controller, prompt]() {
        // Check cancellation before starting
        if (requestCancelled_.load()) return;
        
        // Build request
        juce::DynamicObject::Ptr request = new juce::DynamicObject();
        request->setProperty("prompt", prompt);
        request->setProperty("bpm", bpm_);
        request->setProperty("loopBars", loopBars_);
        request->setProperty("style", "electronic");  // Default, could be UI-selected
        
        // Make API call (blocking in this thread, but thread is not UI thread)
        auto response = controller->generateStems(juce::var(request.get()));
        
        // Check cancellation after API call
        if (requestCancelled_.load()) return;
        
        // Return to UI thread
        juce::MessageManager::callAsync([weakThis, response]() {
            if (weakThis == nullptr) return;
            
            if (response.hasProperty("error")) {
                weakThis->handleAIError(response["error"].toString());
            } else {
                weakThis->handleAIResponse(response);
            }
        });
    });
}

void SkiaAIJamView::handleAIResponse(const juce::var& response) {
    hasPendingRequest_ = false;
    setThinking(false);
    
    // Add AI response to chat
    AIChatMessage aiMsg;
    aiMsg.fromUser = false;
    
    if (response.hasProperty("description")) {
        aiMsg.text = response["description"].toString();
    } else {
        aiMsg.text = "Generated stems based on your request.";
    }
    
    aiMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
    addChatMessage(aiMsg);
    
    // Generate stems from response
    generateStemsFromResponse(response);
}

void SkiaAIJamView::handleAIError(const juce::String& error) {
    hasPendingRequest_ = false;
    setThinking(false);
    
    AIChatMessage errorMsg;
    errorMsg.fromUser = false;
    errorMsg.text = "Error: " + error;
    errorMsg.isError = true;  // Add error styling
    errorMsg.timestamp = juce::Time::getCurrentTime().toString(true, true);
    addChatMessage(errorMsg);
    
    // Add retry action
    addSystemMessage("Click to retry or try a different prompt.");
}

void SkiaAIJamView::generateStemsFromResponse(const juce::var& response) {
    std::vector<GeneratedStem> newStems;
    
    if (response.hasProperty("stems") && response["stems"].isArray()) {
        auto stemsArray = response["stems"];
        
        for (int i = 0; i < stemsArray.size(); ++i) {
            auto stemData = stemsArray[i];
            
            GeneratedStem stem;
            stem.name = stemData.getProperty("name", "Stem " + juce::String(i + 1)).toString();
            stem.type = parseStemType(stemData.getProperty("type", "other").toString());
            
            // If waveform data provided, use it; otherwise generate placeholder
            if (stemData.hasProperty("waveformData")) {
                stem.waveformPreview = parseWaveformData(stemData["waveformData"]);
            } else {
                stem.waveformPreview = generateWaveformFromSeed(stem.name.hashCode());
            }
            
            // If audio URL provided, store it
            if (stemData.hasProperty("audioUrl")) {
                stem.audioUrl = stemData["audioUrl"].toString();
            }
            
            newStems.push_back(stem);
        }
    }
    
    // If no stems in response, generate demo stems as fallback
    if (newStems.empty()) {
        newStems = generateDemoStems();
    }
    
    setStems(newStems);
    
    // Notify that stems are ready
    if (onStemsGenerated) {
        onStemsGenerated(newStems);
    }
}
```

### Step 4: Add Stem-to-Project Integration (4 hours)

```cpp
// SkiaAIJamView.h - Add project integration

class SkiaAIJamView : public SkiaComponent {
public:
    // Callback when user wants to add stems to project
    std::function<void(const std::vector<GeneratedStem>&)> onAddToProject;
    std::function<void(const GeneratedStem&)> onPreviewStem;
    
    // Add stem to project button handler
    void addStemToProject(int stemIndex);
    void addAllStemsToProject();
    
private:
    void drawStemActions(SkCanvas* canvas, const SkRect& bounds);
};

// Add to stem card UI
void SkiaAIJamView::drawSingleStemCard(SkCanvas* canvas, const SkRect& bounds, 
                                        const GeneratedStem& stem, int index, bool isHovered) {
    // ... existing card drawing ...
    
    // ADD: "Add to Project" button when hovered
    if (isHovered) {
        SkRect addButtonRect = SkRect::MakeXYWH(
            bounds.centerX() - 50,
            bounds.centerY() - 15,
            100, 30
        );
        
        SkPaint buttonPaint;
        buttonPaint.setColor(design::colors::ACCENT_PRIMARY);
        canvas->drawRoundRect(addButtonRect, 4.0f, 4.0f, buttonPaint);
        
        SkPaint textPaint;
        textPaint.setColor(design::colors::TEXT_PRIMARY);
        textPaint.setAntiAlias(true);
        SkFont font = design::getSkFont(12.0f, design::FontWeight::Bold);
        canvas->drawString("Add to Project", 
                          addButtonRect.centerX() - 40, 
                          addButtonRect.centerY() + 4, 
                          font, textPaint);
    }
}

void SkiaAIJamView::mouseDown(const juce::MouseEvent& e) {
    // ... existing hit testing ...
    
    // ADD: Check for "Add to Project" button click
    int stemIdx = hitTestStemButton(x, y, btnType);
    if (stemIdx != -1 && btnType == (int)StemButtonType::AddToProject) {
        addStemToProject(stemIdx);
        return;
    }
}
```

### Step 5: Wire Everything Together in MainComponent (2 hours)

```cpp
// MainWindow.cpp - MainComponent constructor

MainComponent::MainComponent(...) {
    // ... setup ...
    
    // Wire AI Jam callbacks
    if (auto* aiJamView = newUILayout->getViewSwitcher()->getAIJamView()) {
        aiJamView->setGrokController(commandAPI->getGrokController());
        
        aiJamView->onStemsGenerated = [this](const std::vector<GeneratedStem>& stems) {
            // Optionally auto-add to project or just notify
            DBG("AI generated " + juce::String(stems.size()) + " stems");
        };
        
        aiJamView->onAddToProject = [this](const std::vector<GeneratedStem>& stems) {
            // Add stems to current project
            for (const auto& stem : stems) {
                // Create audio clip from stem
                projectState.addAudioClipFromURL(stem.audioUrl, stem.name);
            }
        };
    }
}
```

---

## Testing Checklist

### Functional Tests

- [ ] AI Jam button appears in transport bar
- [ ] Clicking AI Jam button shows AI Jam view
- [ ] Clicking Arrangement/Session hides AI Jam view
- [ ] Prompt submission works with real Grok API
- [ ] UI remains responsive during API call
- [ ] Thinking indicator animates during API call
- [ ] Response appears in chat panel
- [ ] Stems are generated from response
- [ ] Error messages appear for API failures
- [ ] "Add to Project" button adds stems to project

### Thread Safety Tests

- [ ] Multiple rapid prompts don't crash
- [ ] Cancelled requests don't cause use-after-free
- [ ] UI updates only happen on message thread
- [ ] Thread pool shuts down cleanly on destruction

### Error Handling Tests

- [ ] Network timeout handled gracefully
- [ ] Invalid API key shows error message
- [ ] Server errors display user-friendly message
- [ ] Demo mode works when no API key configured

---

## Definition of Done

- [ ] AI Jam view accessible from transport bar
- [ ] Real Grok API integration functional
- [ ] Async operation (no UI blocking)
- [ ] Error handling for all failure modes
- [ ] Stems can be added to project
- [ ] Thread-safe implementation
- [ ] All tests pass
- [ ] Code review approved

---

## Related Files

| File | Action |
|------|--------|
| `apps/desktop/Source/ui/views2/core/ViewSwitcher.h` | Add AI Jam view |
| `apps/desktop/Source/ui/views2/core/ViewSwitcher.cpp` | Implement AI Jam visibility |
| `apps/desktop/Source/ui/views2/ZenithMainLayout.cpp` | Wire view switching |
| `apps/desktop/Source/ui/views2/ai-jam/SkiaAIJamView.h` | Add async infrastructure |
| `apps/desktop/Source/ui/views2/ai-jam/SkiaAIJamView.cpp` | Implement async API calls |
| `apps/desktop/Source/ui/views2/common/SkiaTransportBar.h` | Add AI Jam to enum |
| `apps/desktop/Source/ui/views2/common/SkiaTransportBar.cpp` | Add AI Jam button |
| `apps/desktop/Source/ui/common/MainWindow.cpp` | Wire controller |
