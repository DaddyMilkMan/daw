/*
  ==============================================================================

    ZenithPolySynthUI.cpp
    Created: 2025-11-27
    Author:  Zenith DAW

  ==============================================================================
*/

#include "ZenithPolySynthUI.h"
#include "ZenithLayout.h"

// Include all Skia headers BEFORE entering namespace zenith
#ifdef ZENITH_USE_SKIA
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <core/SkColor.h>
#include <core/SkSurface.h>
#include <core/SkRefCnt.h>
#include <core/SkPoint.h>
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

ZenithPolySynthUI::ZenithPolySynthUI(ZenithPolySynthProcessor &p)
    : AudioProcessorEditor(&p), 
      processor(p) {
  
  setSize(kSimpleWidth, kSimpleHeight);

  // Initialize Renderer
  Settings::getInstance().addChangeListener(this);
  recreateRenderer();

#ifdef ZENITH_USE_SKIA
  // Initialize Widgets
  addWidget<ZenithKnob>("CUTOFF", ZenithPolySynthProcessor::FilterCutoff);
  addWidget<ZenithKnob>("RES", ZenithPolySynthProcessor::FilterResonance);
  addWidget<ZenithKnob>("ENV AMT", ZenithPolySynthProcessor::FilterEnvAmount);
  addWidget<ZenithKnob>("SUB", ZenithPolySynthProcessor::SubOscLevel);
  addWidget<ZenithKnob>("NOISE", ZenithPolySynthProcessor::NoiseLevel);
  
  addWidget<ZenithSlider>("A", ZenithPolySynthProcessor::AmpAttack);
  addWidget<ZenithSlider>("D", ZenithPolySynthProcessor::AmpDecay);
  addWidget<ZenithSlider>("S", ZenithPolySynthProcessor::AmpSustain);
  addWidget<ZenithSlider>("R", ZenithPolySynthProcessor::AmpRelease);
  
  // Visualizer (JUCE Component wrapper)
  visualizer_ = std::make_unique<ZenithVisualizer>(processor);
  addAndMakeVisible(visualizer_.get());
  
  // Preset Bar (JUCE Component)
  presetBar_ = std::make_unique<ZenithPresetBar>();
  presetBar_->setCallbacks(
      [this]() { loadPrevPreset(); },
      [this]() { loadNextPreset(); },
      []() {} 
  );
  addAndMakeVisible(presetBar_.get());
  
  refreshPresetList();
  setLookAndFeel(&zenithLookAndFeel_);
  
  // Trigger initial layout
  resized();
  
  // PHASE 1: Start frame capture timer (60 FPS)
  startTimer(16); // ~60Hz frame capture
#endif
}

void ZenithPolySynthUI::recreateRenderer() {
    renderer_ = std::make_unique<SkiaRenderer>(*this, Settings::getInstance().getRenderBackend());
    renderer_->initialize();
}

void ZenithPolySynthUI::changeListenerCallback(juce::ChangeBroadcaster*) {
    recreateRenderer();
    repaint();
}

ZenithPolySynthUI::~ZenithPolySynthUI() {
  Settings::getInstance().removeChangeListener(this);
  setLookAndFeel(nullptr);
}

template <typename T>
T* ZenithPolySynthUI::addWidget(const juce::String& name, const juce::String& paramId) {
    auto widget = std::make_unique<T>(name);
    T* ptr = widget.get();
    
    // Bind parameter
    auto* param = processor.getParameters().getParameter(paramId);
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param)) {
        widget->setParameter(ranged);
    }
    
    widgets_.push_back(std::move(widget));
    return ptr;
}

void ZenithPolySynthUI::resized() {
  auto area = getLocalBounds().reduced(20);
  
  if (presetBar_) presetBar_->setBounds(area.removeFromTop(40).reduced(100, 0));
  if (visualizer_) visualizer_->setBounds(area.removeFromTop(150).reduced(10));
  
  layoutWidgets();
}

void ZenithPolySynthUI::layoutWidgets() {
    std::vector<juce::Component*> filterGroup;
    std::vector<juce::Component*> mixGroup;
    std::vector<juce::Component*> envGroup;

    for (auto& w : widgets_) {
        juce::String name = w->getName();
        if (name == "CUTOFF" || name == "RES" || name == "ENV AMT") filterGroup.push_back(w.get());
        else if (name == "SUB" || name == "NOISE") mixGroup.push_back(w.get());
        else if (name == "A" || name == "D" || name == "S" || name == "R") envGroup.push_back(w.get());
    }

    auto area = getLocalBounds().reduced(20);
    
    if (presetBar_) area.removeFromTop(40);
    if (visualizer_) area.removeFromTop(150);
    
    auto mainArea = area;
    
    // Layout Filter Group (Top Left)
    ZenithLayout::row(mainArea.removeFromTop(100).removeFromLeft(350), filterGroup, 10.0f);
    
    // Layout Mix Group (Bottom Left)
    ZenithLayout::row(mainArea.removeFromTop(80).removeFromLeft(200), mixGroup, 10.0f);
    
    // Layout Env Group (Right side)
    // Use vertical slider layout (row of sliders)
    ZenithLayout::row(area.removeFromRight(200), envGroup, 5.0f);
}

// Mouse Handling
void ZenithPolySynthUI::mouseDown(const juce::MouseEvent& e) {
    // Hit test widgets
    for (auto& w : widgets_) {
        if (w->hitTest(e.x, e.y)) {
            activeWidget_ = w.get();
            activeWidget_->onMouseDown(e);
            return;
        }
    }
    juce::Component::mouseDown(e); // Pass to children
}

void ZenithPolySynthUI::mouseDrag(const juce::MouseEvent& e) {
    if (activeWidget_) {
        activeWidget_->onMouseDrag(e);
        repaint(); // Request Skia repaint
    } else {
        juce::Component::mouseDrag(e);
    }
}

void ZenithPolySynthUI::mouseUp(const juce::MouseEvent& e) {
    if (activeWidget_) {
        activeWidget_->onMouseUp(e);
        activeWidget_ = nullptr;
        repaint();
    } else {
        juce::Component::mouseUp(e);
    }
}

void ZenithPolySynthUI::mouseMove(const juce::MouseEvent& e) {
    SkiaWidget* newHover = nullptr;
    for (auto& w : widgets_) {
        if (w->hitTest(e.x, e.y)) {
            newHover = w.get();
            break;
        }
    }
    
    if (newHover != hoveredWidget_) {
        if (hoveredWidget_) hoveredWidget_->setHovered(false);
        if (newHover) newHover->setHovered(true);
        hoveredWidget_ = newHover;
        repaint();
    }
    
    juce::Component::mouseMove(e);
}

void ZenithPolySynthUI::paint(juce::Graphics& g) {
    if (renderer_) {
        renderer_->render([this](SkCanvas* canvas) {
            drawSkiaContent(canvas);
        });
    } else {
        // Fallback for non-Skia builds
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("Zenith PolySynth", getLocalBounds(), juce::Justification::centred);
    }
}

void ZenithPolySynthUI::drawSkiaContent(SkCanvas* canvas) {
#ifdef ZENITH_USE_SKIA
    if (!canvas) return;
    
    // Debug: Verify we're NOT on Message Thread (we're on OpenGL thread)
    jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    // Get latest frame snapshot (lock-free read, NO Component access!)
    const auto* frame = frameBuffer_.getLatestFrame();
    
    // Clear background
    canvas->clear(SkColorSetRGB(20, 20, 25));
    
    // Draw radial gradient background
    auto bounds = frame->componentBounds.toFloat();
    if (!bounds.isEmpty()) {
        SkPaint paint;
        paint.setAntiAlias(true);
        
        SkPoint center = { bounds.getCentreX(), bounds.getCentreY() };
        SkColor colors[2] = { SkColorSetRGB(30, 30, 40), SkColorSetRGB(10, 10, 15) };
        float radius = std::max(bounds.getWidth(), bounds.getHeight()) * 0.8f;
        
        paint.setShader(SkGradientShader::MakeRadial(center, radius, colors, nullptr, 2, SkTileMode::kClamp));
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), paint);
        paint.setShader(nullptr);
    }
    
    // Draw all knobs from snapshot (thread-safe!)
    for (const auto& knob : frame->knobs) {
        drawKnobFromState(canvas, knob);
    }
    
    // Draw all sliders from snapshot
    for (const auto& slider : frame->sliders) {
        drawSliderFromState(canvas, slider);
    }
    
    // TODO: Draw buttons from state (when button widgets are added)
    // for (const auto& button : frame->buttons) {
    //     drawButtonFrom State(canvas, button);
    // }
    
    // TODO: Draw visualizer from state (Phase 2)
    
#else
    juce::ignoreUnused(canvas);
#endif
}

// ============================================================================
// PHASE 1: THREAD-SAFE RENDERING
// ============================================================================

void ZenithPolySynthUI::timerCallback() {
    // Called on Message Thread at 60Hz
    captureFrameSnapshot();
}

void ZenithPolySynthUI::captureFrameSnapshot() {
    // Debug: Verify we're on Message Thread
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    // Get writable buffer (Message Thread safe)
    auto* frame = frameBuffer_.getWriteBuffer();
    frame->clear();
    
    // Capture global state
    frame->isAdvancedMode = isAdvancedMode_;
    frame->componentBounds = getLocalBounds();
    frame->frameNumber++;
    frame->timestamp = juce::Time::getCurrentTime().toMilliseconds();
    
    // Snapshot all knob widgets
    for (auto& widget : widgets_) {
        if (auto* knob = dynamic_cast<ZenithKnob*>(widget.get())) {
            render::KnobRenderState kState;
            auto r = knob->getLocalBounds().toFloat();
            kState.bounds = SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight());
            kState.value = knob->getValue();
            kState.baseColor = knob->getGlowColor(); 
            kState.labelText = knob->getName();
            kState.isHovered = knob->isHovered();
            frame->knobs.push_back(kState);
        }
    }
    
    // Snapshot all slider widgets
    for (auto& widget : widgets_) {
        if (auto* slider = dynamic_cast<ZenithSlider*>(widget.get())) {
             // Construct state manually
            render::SliderRenderState sState;
            auto r = slider->getLocalBounds().toFloat();
            sState.bounds = SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight());
            sState.value = slider->getValue();
            sState.labelText = slider->getName();
            sState.isHovered = slider->isHovered();
            frame->sliders.push_back(sState);
        }
    }
    
    // TODO: Snapshot buttons when button widgets are added
   // for (auto& widget : widgets_) {
    //     if (auto* button = dynamic_cast<SkiaButton*>(widget.get())) {
    //         frame->buttons.push_back(button->captureRenderState());
    //     }
    // }
    
    // TODO: Snapshot visualizer when ready (Phase 2)

    // Atomic swap to ready buffer (lock-free)
    frameBuffer_.swapWriteToReady();
}

void ZenithPolySynthUI::drawKnobFromState(SkCanvas* canvas, const render::KnobRenderState& state) {
    if (state.bounds.isEmpty()) return;
    
    canvas->save();
    
    // Apply hover scale animation
    if (state.scale > 1.0f) {
        float cx = state.bounds.centerX();
        float cy = state.bounds.centerY();
        canvas->translate(cx, cy);
        canvas->scale(state.scale, state.scale);
        canvas->translate(-cx, -cy);
    }
    
    // Calculate knob geometry
    float cx = state.bounds.centerX();
    float cy = state.bounds.centerY();
    float radius = std::min(state.bounds.width(), state.bounds.height()) * 0.35f;
    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    
    // Arc parameters (270° rotation range)
    float startAngle = -135.0f; // -270/2 - 90
    float sweepAngle = state.value * 270.0f;
    
    // 1. Background track (full range)
    SkPaint trackPaint;
    trackPaint.setStyle(SkPaint::kStroke_Style);
    trackPaint.setStrokeWidth(2.5f);
    trackPaint.setColor(SkColorSetARGB(77, 255, 255, 255)); // 30% white
    trackPaint.setAntiAlias(true);
    trackPaint.setStrokeCap(SkPaint::kRound_Cap);
    canvas->drawArc(arcRect, startAngle, 270.0f, false, trackPaint);
    
    // 2. Glow layer (if hovered)
    if (state.glowIntensity > 0.01f) {
        SkPaint glowPaint;
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(5.0f);
        glowPaint.setColor(SkColorSetA(state.glowColor, static_cast<U8CPU>(state.glowIntensity * 102))); // 40% max alpha
        glowPaint.setAntiAlias(true);
        glowPaint.setStrokeCap(SkPaint::kRound_Cap);
        canvas->drawArc(arcRect, startAngle, sweepAngle, false, glowPaint);
    }
    
    // 3. Value arc (current value)
    SkPaint valuePaint;
    valuePaint.setStyle(SkPaint::kStroke_Style);
    valuePaint.setStrokeWidth(2.5f);
    valuePaint.setColor(state.baseColor);
    valuePaint.setAntiAlias(true);
    valuePaint.setStrokeCap(SkPaint::kRound_Cap);
    canvas->drawArc(arcRect, startAngle, sweepAngle, false, valuePaint);
    
    // 4. Label text
    if (state.labelText.isNotEmpty()) {
        SkFont font;
        font.setSize(12.0f);
        
        SkPaint textPaint;
        textPaint.setColor(SkColorSetRGB(160, 160, 165));
        
        float textY = cy + radius + 15.0f;
        float width = font.measureText(state.labelText.toRawUTF8(), state.labelText.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
        canvas->drawString(state.labelText.toRawUTF8(), cx - width / 2.0f, textY, font, textPaint);
    }
    
    canvas->restore();
}

void ZenithPolySynthUI::drawSliderFromState(SkCanvas* canvas, const render::SliderRenderState& state) {
    static ZenithSlider drawer;
    if (state.bounds.isEmpty()) return;
    
    canvas->save();
    
    const float trackWidth = 4.0f;
    const float handleSize = 12.0f;
    const float labelOffset = 18.0f;
    
    // Calculate track rect (vertically centered)
    float centerX = state.bounds.centerX();
    SkRect trackRect = SkRect::MakeXYWH(
        centerX - trackWidth / 2.0f,
        state.bounds.top(),
        trackWidth,
        state.bounds.height() - labelOffset
    );
    
    // 1. Draw background track
    SkPaint trackPaint;
    trackPaint.setStyle(SkPaint::kFill_Style);
    trackPaint.setColor(SkColorSetARGB(77, 255, 255, 255)); // 30% white
    trackPaint.setAntiAlias(true);
    canvas->drawRoundRect(trackRect, 2.0f, 2.0f, trackPaint);
    
    // 2. Draw filled value bar (bottom to value)
    if (state.value > 0.0f) {
        float fillHeight = trackRect.height() * state.value;
        SkRect fillRect = SkRect::MakeXYWH(
            trackRect.x(),
            trackRect.bottom() - fillHeight,
            trackRect.width(),
            fillHeight
        );
        
        SkPaint fillPaint;
        fillPaint.setStyle(SkPaint::kFill_Style);
        fillPaint.setColor(state.color);
        fillPaint.setAntiAlias(true);
        
        // Add glow if hovered or dragging
        if (state.isHovered || state.isDragging) {
            SkPaint glowPaint = fillPaint;
            glowPaint.setColor(SkColorSetARGB(102, 74, 158, 255)); // 40% cyan glow
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
            canvas->drawRoundRect(fillRect, 2.0f, 2.0f, glowPaint);
        }
        
        canvas->drawRoundRect(fillRect, 2.0f, 2.0f, fillPaint);
    }
    
    // 3. Draw value indicator handle
    float handleY = trackRect.bottom() - (trackRect.height() * state.value);
    
    SkPaint handlePaint;
    handlePaint.setStyle(SkPaint::kFill_Style);
    handlePaint.setColor(SK_ColorWHITE);
    handlePaint.setAntiAlias(true);
    
    // Slightly larger if hovered
    float actualHandleSize = state.isHovered ? handleSize * 1.1f : handleSize;
    
    canvas->drawCircle(centerX, handleY, actualHandleSize / 2.0f, handlePaint);
    
    // Handle border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.5f);
    borderPaint.setColor(SkColorSetARGB(180, 0, 0, 0)); // Semi-transparent black
    borderPaint.setAntiAlias(true);
    canvas->drawCircle(centerX, handleY, actualHandleSize / 2.0f, borderPaint);
    
    // 4. Draw label text below slider
    if (state.labelText.isNotEmpty()) {
        SkFont font;
        font.setSize(11.0f);
        font.setEdging(SkFont::Edging::kAntiAlias);
        
        SkPaint textPaint;
        textPaint.setColor(SkColorSetRGB(160, 160, 165));
        
        float textY = state.bounds.bottom() - 2.0f;
        float textWidth = font.measureText(state.labelText.toRawUTF8(), state.labelText.getNumBytesAsUTF8(), SkTextEncoding::kUTF8);
        
        canvas->drawString(state.labelText.toRawUTF8(), centerX - textWidth / 2.0f, textY, font, textPaint);
    }
    
    canvas->restore();
}

// ============================================================================
// PRESET MANAGEMENT (IMPLEMENTED)
// ============================================================================

void ZenithPolySynthUI::toggleAdvancedMode() {
    isAdvancedMode_ = !isAdvancedMode_;
    
    // Resize the entire UI to accommodate advanced controls
    const int newWidth = isAdvancedMode_ ? kAdvancedWidth : kSimpleWidth;
    const int newHeight = isAdvancedMode_ ? kAdvancedHeight : kSimpleHeight;
    setSize(newWidth, newHeight);
    
    // Trigger layout update
    resized();
    
    DBG("Advanced mode: " + juce::String(isAdvancedMode_ ? "ON" : "OFF"));
}

void ZenithPolySynthUI::toggleLearningMode() {
    // Learning mode shows tooltips for all controls
    // Currently stubbed - could show persistent tooltips overlay
    DBG("Learning mode toggle - Currently not implemented in widget-based UI");
}

void ZenithPolySynthUI::loadPreset(int index) {
    if (index < 0 || index >= static_cast<int>(presetList_.size())) {
        DBG("Invalid preset index: " + juce::String(index));
        return;
    }
    
    currentPresetIndex_ = index;
    const auto& meta = presetList_[index];
    
    // Update preset bar display
    if (presetBar_) {
        presetBar_->setPresetName(meta.name);
    }
    
    // Load full preset data
    auto preset = ZenithPresetManager::getInstance().loadPreset("ZenithPolySynth", meta.id);
    
    // Apply to processor parameters
    auto& params = processor.getParameters();
    for (const auto& pair : preset.parameters) {
        const juce::String& paramId = pair.first;
        float paramValue = pair.second;  // Normalized [0-1]
        
        if (auto* param = params.getParameter(paramId)) {
            // Set value with host notification
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param)) {
                ranged->beginChangeGesture();
                ranged->setValueNotifyingHost(paramValue);
                ranged->endChangeGesture();
            }
        }
    }
    
    DBG("Loaded preset: " + meta.name + " (" + juce::String(index + 1) + "/" + juce::String(presetList_.size()) + ")");
}

void ZenithPolySynthUI::loadNextPreset() {
    if (presetList_.empty()) return;
    
    int next = (currentPresetIndex_ + 1) % static_cast<int>(presetList_.size());
    loadPreset(next);
}

void ZenithPolySynthUI::loadPrevPreset() {
    if (presetList_.empty()) return;
    
    int size = static_cast<int>(presetList_.size());
    int prev = (currentPresetIndex_ - 1 + size) % size;
    loadPreset(prev);
}

void ZenithPolySynthUI::refreshPresetList() {
    // Get preset metadata list from PresetManager
    presetList_ = ZenithPresetManager::getInstance().getPresetList("ZenithPolySynth");
    
    if (presetList_.empty()) {
        DBG("No presets found for ZenithPolySynth");
        if (presetBar_) {
            presetBar_->setPresetName("No Presets");
        }
        currentPresetIndex_ = -1;
    } else {
        DBG("Found " + juce::String(presetList_.size()) + " presets for ZenithPolySynth");
        // Load first preset by default
        currentPresetIndex_ = 0;
        loadPreset(0);
    }
}

} // namespace zenith

