/*
namespace zenith {

// ============================================================================
// CONSTRUCTION
// ============================================================================

SkiaKnob::SkiaKnob(const juce::String& name) {
    logKnob("Constructor start");
    // Default size
    setSize(60, 80);
    logKnob("setSize done");
    
    // Accessibility
    setDescription(name.isEmpty() ? "Knob" : name);
    setWantsKeyboardFocus(true);
    
    // Initial history
    valueHistory_.push(value_);
    logKnob("Constructor done");
}

SkiaKnob::~SkiaKnob() {
}

// ============================================================================
// APPEARANCE
// ============================================================================

void SkiaKnob::setStyle(Style style) {
    if (style_ != style) {
        style_ = style;
        markDirty();
    }
}

void SkiaKnob::setRotationRange(float degrees) {
    rotationRange_ = degrees;
    markDirty();
}

void SkiaKnob::setValueColoring(bool enabled) {
    valueColoring_ = enabled;
    markDirty();
}

// ============================================================================
// VALUE CONTROL
// ============================================================================

void SkiaKnob::setValue(float value) {
    float clampedValue = juce::jlimit(0.0f, 1.0f, value);
    
    if (std::abs(value_ - clampedValue) > 0.0001f) {
        value_ = clampedValue;
        
        if (onValueChange) {
            onValueChange(value_);
        }
        
        markDirty();
    }
}

void SkiaKnob::setDefaultValue(float value) {
    defaultValue_ = juce::jlimit(0.0f, 1.0f, value);
}

void SkiaKnob::setDisplayRange(float min, float max) {
    displayMin_ = min;
    displayMax_ = max;
    markDirty(); // For label update
}
void SkiaKnob::setSnapToIncrement(bool snap, float increment) {
    snapEnabled_ = snap;
    snapIncrement_ = increment;
}

void SkiaKnob::setSnapToValue(bool enabled, float snapValue, float tolerance) {
    snapEnabled_ = enabled;
    snapIncrement_ = snapValue; // Using snapIncrement_ to store the snap value for simplicity in this context, though semantics differ slightly
    snapTolerance_ = tolerance;
}

// ============================================================================
// INTERACTION
// ============================================================================

void SkiaKnob::mouseDown(const juce::MouseEvent& e) {
    // Context Menu (Right Click)
    if (e.mods.isPopupMenu()) {
        showContextMenu();
        return;
    }
    
    // Fine control
    if (e.mods.isShiftDown() || e.mods.isRightButtonDown()) {
        isFineControl_ = true;
    } else {
        isFineControl_ = false;
    }
    
    isDragging_ = true;
    dragStartValue_ = value_;
    dragStartY_ = e.y;
    
    // Push current value to history before change
    valueHistory_.push(value_);
    
    if (onDragStart) {
        onDragStart();
    }
    
    // Focus for keyboard control
    grabKeyboardFocus();
}

void SkiaKnob::mouseDrag(const juce::MouseEvent& e) {
    if (!isDragging_) return;
    
    float sensitivity = dragSensitivity_ * (isFineControl_ ? 0.1f : 1.0f);
    float delta = (dragStartY_ - e.y) / 200.0f * sensitivity;
    
    float newValue = juce::jlimit(0.0f, 1.0f, dragStartValue_ + delta);
    
    if (snapEnabled_) {
        newValue = std::round(newValue / snapIncrement_) * snapIncrement_;
    }
    
    setValue(newValue);
}

void SkiaKnob::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (isDragging_) {
        isDragging_ = false;
        if (onDragEnd) {
            onDragEnd();
        }
    }
}

void SkiaKnob::mouseDoubleClick(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (doubleClickReset_) {
        resetToDefault();
    }
}

void SkiaKnob::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    float delta = wheel.deltaY * 0.1f;
    if (e.mods.isShiftDown()) delta *= 0.1f;
    
    setValue(juce::jlimit(0.0f, 1.0f, value_ + delta));
}

void SkiaKnob::onHoverEnter() {
    animateTo("scale", 1.05f, design::animation::DURATION_FAST);
    animateTo("glow", 1.0f, design::animation::DURATION_FAST);
}

void SkiaKnob::onHoverExit() {
    animateTo("scale", 1.0f, design::animation::DURATION_FAST);
    animateTo("glow", 0.0f, design::animation::DURATION_FAST);
}

bool SkiaKnob::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    // Undo: Ctrl + Z
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
        if (valueHistory_.canUndo()) {
            float val = valueHistory_.undo();
            setValue(val);
            return true;
        }
    }
    
    // Redo: Ctrl + Y or Ctrl + Shift + Z
    if (key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)) {
        if (valueHistory_.canRedo()) {
            float val = valueHistory_.redo();
            setValue(val);
            return true;
        }
    }
    
    // Call base class for context menu shortcut
    return SkiaComponent::keyPressed(key, origin);
}

// ============================================================================
// CONTEXT MENU & UNDO/REDO
// ============================================================================

void SkiaKnob::resetToDefault() {
    valueHistory_.push(value_); // Save before reset
    setValue(defaultValue_);
}

void SkiaKnob::copyValue() {
    juce::SystemClipboard::copyTextToClipboard(juce::String(value_));
}

void SkiaKnob::pasteValue() {
    juce::String text = juce::SystemClipboard::getTextFromClipboard();
    float val = text.getFloatValue();
    if (val >= 0.0f && val <= 1.0f) { // Simple validation
        valueHistory_.push(value_);
        setValue(val);
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void SkiaKnob::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    
    // Calculate radius (leave room for label)
    float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.35f;
    
    // Apply hover scale
    float scale = getAnimatedValue("scale");
    if (scale > 0.0f) {
        canvas->translate(cx, cy);
        canvas->scale(scale, scale);
        canvas->translate(-cx, -cy);
    }
    
    // Draw Arc
    float startAngle = -rotationRange_ / 2.0f - 90.0f;
    float endAngle = startAngle + (value_ * rotationRange_);
    juce::ignoreUnused(endAngle); // Used for dot calculation
    
    // Background track
    SkPaint trackPaint;
    trackPaint.setStyle(SkPaint::kStroke_Style);
    trackPaint.setStrokeWidth(2.5f); // Thinner for pro look (was 4.0f)
    trackPaint.setColor(design::withAlpha(design::colors::BG_LIGHT, 0.3f));
    trackPaint.setAntiAlias(true);
    trackPaint.setStrokeCap(SkPaint::kRound_Cap);
    
    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    canvas->drawArc(arcRect, startAngle, rotationRange_, false, trackPaint);
    
    // Value arc
    SkPaint valuePaint;
    valuePaint.setStyle(SkPaint::kStroke_Style);
    valuePaint.setStrokeWidth(2.5f); // Match track width
    valuePaint.setAntiAlias(true);
    valuePaint.setStrokeCap(SkPaint::kRound_Cap);
    
    // Color
    SkColor color = design::colors::CYAN;
    if (valueColoring_) {
        // Gradient from Blue to Cyan
        // Simple interpolation for now
        color = design::interpolateColor(design::colors::BLUE, design::colors::CYAN, value_);
    }
    valuePaint.setColor(color);
    
    // Glow
    if (isGlowEnabled() || isHovered()) {
        SkPaint glowPaint = valuePaint;
        glowPaint.setStrokeWidth(5.0f); // Reduced from 8.0f
        glowPaint.setColor(design::withAlpha(color, 0.4f * getAnimatedValue("glow")));
        canvas->drawArc(arcRect, startAngle, value_ * rotationRange_, false, glowPaint);
    }
    
    canvas->drawArc(arcRect, startAngle, value_ * rotationRange_, false, valuePaint);
    
    // Dot indicator
    if (style_ == Style::Dot || style_ == Style::ArcAndDot) {
        float angleRad = (endAngle) * (3.14159f / 180.0f);
        float dotX = cx + std::cos(angleRad) * radius;
        float dotY = cy + std::sin(angleRad) * radius;
        
        SkPaint dotPaint;
        dotPaint.setColor(SK_ColorWHITE);
        dotPaint.setAntiAlias(true);
        canvas->drawCircle(dotX, dotY, 3.0f, dotPaint);
    }
    
    // Label
    if (labelPosition_ != LabelPosition::None) {
        SkFont font;
        font.setSize(12.0f);
        
        juce::String labelText = juce::String(displayMin_ + value_ * (displayMax_ - displayMin_), 1);
        
        SkPaint textPaint;
        textPaint.setColor(design::colors::TEXT_SECONDARY);
        
        float textY = cy;
        if (labelPosition_ == LabelPosition::Below) textY += radius + 15.0f;
        if (labelPosition_ == LabelPosition::Above) textY -= radius + 15.0f;
        
        // Simple center text (Skia text centering is manual)
        float width = font.measureText(labelText.toRawUTF8(), labelText.length(), SkTextEncoding::kUTF8);
        canvas->drawString(labelText.toRawUTF8(), cx - width / 2.0f, textY, font, textPaint);
    }
}

} // namespace zenith
