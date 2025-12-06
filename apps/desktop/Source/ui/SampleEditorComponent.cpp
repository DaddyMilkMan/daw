/*
  ==============================================================================
    SampleEditorComponent.cpp - ZENITH EDISON
    Professional Sample Editor Implementation
  ==============================================================================
*/

#include "SampleEditorComponent.h"
#include "skia/ZenithDesignSystem.h"
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>

namespace zenith {

// Static clipboard
std::unique_ptr<juce::AudioBuffer<float>> SampleEditorComponent::clipboard_ = nullptr;
double SampleEditorComponent::clipboardSampleRate_ = 44100.0;

namespace Colors {
    constexpr SkColor bg = SkColorSetRGB(25, 25, 30);
    constexpr SkColor toolbarBg = SkColorSetRGB(35, 35, 42);
    constexpr SkColor waveform = SkColorSetRGB(90, 160, 220);
    constexpr SkColor waveformOutline = SkColorSetRGB(120, 180, 240);
    constexpr SkColor selection = SkColorSetARGB(60, 100, 150, 255);
    constexpr SkColor selectionBorder = SkColorSetARGB(200, 100, 150, 255);
    constexpr SkColor playhead = SkColorSetRGB(255, 100, 100);
    constexpr SkColor grid = SkColorSetARGB(25, 255, 255, 255);
    constexpr SkColor rulerBg = SkColorSetRGB(40, 40, 48);
    constexpr SkColor rulerText = SkColorSetRGB(150, 150, 160);
    constexpr SkColor buttonActive = SkColorSetRGB(80, 140, 220);
    constexpr SkColor buttonHover = SkColorSetRGB(60, 60, 70);
    constexpr SkColor marker = SkColorSetRGB(255, 200, 50);
    constexpr SkColor overviewBg = SkColorSetRGB(30, 30, 36);
    constexpr SkColor overviewViewport = SkColorSetARGB(80, 255, 255, 255);
}

//==============================================================================
SampleEditorComponent::SampleEditorComponent(Engine& engine, ProjectState& state)
    : engine_(engine), projectState_(state)
{
    setOpaque(true);
    setWantsKeyboardFocus(true);
    
    backgroundColor_ = Colors::bg;
    waveformColor_ = Colors::waveform;
    selectionColor_ = Colors::selection;
    gridColor_ = Colors::grid;
    rulerColor_ = Colors::rulerText;
    playheadColor_ = Colors::playhead;

    projectState_.getState().addListener(this);
    startTimerHz(30); // 30fps playhead updates
}

SampleEditorComponent::~SampleEditorComponent()
{
    stopTimer();
    projectState_.getState().removeListener(this);
}

void SampleEditorComponent::timerCallback()
{
    if (isPlaying_) {
        // Update playhead from engine
        playheadPosition_ += 1.0 / 30.0; // Approximate
        if (audioHandle_ && playheadPosition_ > samplesToTime(audioHandle_->lengthInSamples)) {
            if (isLooping_ && hasSelection()) {
                playheadPosition_ = selection_.getStart();
            } else {
                stop();
            }
        }
        repaint();
    }
}

//==============================================================================
void SampleEditorComponent::setClipToEdit(const juce::String& trackId, const juce::String& clipId)
{
    currentTrackId_ = trackId;
    currentClipId_ = clipId;
    
    auto trackNode = projectState_.getTrack(trackId);
    if (trackNode.isValid()) {
        auto clips = trackNode.getChildWithName("CLIPS");
        clipNode_ = clips.getChildWithProperty("id", clipId);
        
        if (clipNode_.isValid()) {
            juce::File audioFile(clipNode_.getProperty("sourceFile").toString());
            if (audioFile.existsAsFile()) {
                audioHandle_ = engine_.getAudioFilePool().getFile(audioFile);
                if (!audioHandle_)
                    audioHandle_ = engine_.getAudioFilePool().loadFile(audioFile);
                if (audioHandle_) {
                    // Create mutable copy for editing
                    editBuffer_ = std::make_unique<juce::AudioBuffer<float>>(*audioHandle_->buffer.getArrayOfReadPointers(), 
                                                                             audioHandle_->buffer.getNumChannels(),
                                                                             audioHandle_->buffer.getNumSamples());
                    fitToWindow();
                }
            }
        }
    }
    repaint();
}

void SampleEditorComponent::clearClip()
{
    currentTrackId_ = {};
    currentClipId_ = {};
    clipNode_ = {};
    audioHandle_ = nullptr;
    markers_.clear();
    regions_.clear();
    clearSelection();
    repaint();
}

//==============================================================================
void SampleEditorComponent::drawSkia(SkCanvas* canvas)
{
    float w = (float)getWidth();
    float h = (float)getHeight();
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(Colors::bg);
    canvas->drawRect(SkRect::MakeWH(w, h), bgPaint);
    
    // Layout areas
    SkRect toolbarRect = SkRect::MakeXYWH(0, 0, w, toolbarHeight_);
    SkRect overviewRect = SkRect::MakeXYWH(0, toolbarHeight_, w, overviewHeight_);
    SkRect rulerRect = SkRect::MakeXYWH(0, toolbarHeight_ + overviewHeight_, w, rulerHeight_);
    SkRect waveformRect = SkRect::MakeXYWH(0, toolbarHeight_ + overviewHeight_ + rulerHeight_, 
                                            w, h - toolbarHeight_ - overviewHeight_ - rulerHeight_ - scrollbarHeight_);
    SkRect scrollRect = SkRect::MakeXYWH(0, h - scrollbarHeight_, w, scrollbarHeight_);
    
    // Draw components
    drawToolbar(canvas, toolbarRect);
    
    if (!audioHandle_ || !audioHandle_->isValid()) {
        drawEmptyState(canvas, w, h);
        return;
    }
    
    drawOverview(canvas, overviewRect);
    drawRuler(canvas, rulerRect);
    drawGrid(canvas, waveformRect);
    drawRegions(canvas, waveformRect);
    drawWaveform(canvas, waveformRect);
    drawSelection(canvas, waveformRect);
    drawMarkers(canvas, waveformRect);
    drawPlayhead(canvas, waveformRect);
    drawScrollbar(canvas, scrollRect);
}

//==============================================================================
void SampleEditorComponent::drawToolbar(SkCanvas* canvas, const SkRect& bounds)
{
    // Toolbar background
    SkPaint bgPaint;
    bgPaint.setColor(Colors::toolbarBg);
    canvas->drawRect(bounds, bgPaint);
    
    // Bottom border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetARGB(60, 0, 0, 0));
    canvas->drawLine(0, bounds.bottom(), bounds.width(), bounds.bottom(), borderPaint);
    
    float x = 8;
    float btnSize = 28;
    float spacing = 4;
    
    SkFont font(nullptr, 10);
    SkPaint textPaint;
    textPaint.setColor(Colors::rulerText);
    textPaint.setAntiAlias(true);
    
    // Tool buttons group
    auto drawBtn = [&](const char* label, bool active) {
        SkRect btnRect = SkRect::MakeXYWH(x, (bounds.height() - btnSize) / 2, btnSize, btnSize);
        SkPaint btnPaint;
        btnPaint.setColor(active ? Colors::buttonActive : SkColorSetARGB(40, 255, 255, 255));
        btnPaint.setAntiAlias(true);
        canvas->drawRoundRect(btnRect, 4, 4, btnPaint);
        canvas->drawString(label, x + 8, bounds.centerY() + 4, font, textPaint);
        x += btnSize + spacing;
    };
    
    // Tools
    drawBtn("S", currentTool_ == SampleEditorTool::Select);
    drawBtn("P", currentTool_ == SampleEditorTool::Pencil);
    drawBtn("X", currentTool_ == SampleEditorTool::Slice);
    
    x += 12; // Separator
    
    // Transport
    drawBtn(isPlaying_ ? "||" : ">", false);
    drawBtn("[]", false);
    drawBtn("L", isLooping_);
    
    x += 12;
    
    // Edit operations
    canvas->drawString("Cut", x, bounds.centerY() + 4, font, textPaint); x += 30;
    canvas->drawString("Copy", x, bounds.centerY() + 4, font, textPaint); x += 35;
    canvas->drawString("Paste", x, bounds.centerY() + 4, font, textPaint); x += 40;
    
    x += 12;
    
    // Processing
    canvas->drawString("Norm", x, bounds.centerY() + 4, font, textPaint); x += 35;
    canvas->drawString("Rev", x, bounds.centerY() + 4, font, textPaint); x += 30;
    canvas->drawString("Fade", x, bounds.centerY() + 4, font, textPaint); x += 35;
    
    // Right side - zoom info
    if (audioHandle_) {
        char zoomStr[64];
        snprintf(zoomStr, sizeof(zoomStr), "%.1fx  V:%.0f%%", 
                 samplesToTime(audioHandle_->lengthInSamples) / viewWidthSeconds_,
                 verticalZoom_ * 100);
        canvas->drawString(zoomStr, bounds.width() - 100, bounds.centerY() + 4, font, textPaint);
    }
}

void SampleEditorComponent::drawOverview(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint bgPaint;
    bgPaint.setColor(Colors::overviewBg);
    canvas->drawRect(bounds, bgPaint);
    
    if (!audioHandle_) return;
    
    // Draw mini waveform
    const juce::AudioBuffer<float>* bufferPtr = &audioHandle_->buffer;
    if (editBuffer_) bufferPtr = editBuffer_.get();
    auto& buffer = *bufferPtr;
    int numSamples = buffer.getNumSamples();
    float w = bounds.width();
    float h = bounds.height();
    
    SkPaint wavePaint;
    wavePaint.setColor(SkColorSetARGB(100, 90, 160, 220));
    
    float samplesPerPixel = (float)numSamples / w;
    const float* samples = buffer.getReadPointer(0);
    
    for (float x = 0; x < w; x++) {
        int idx = (int)(x * samplesPerPixel);
        if (idx >= numSamples) break;
        float val = std::abs(samples[idx]) * (h / 2) * 0.8f;
        canvas->drawLine(bounds.left() + x, bounds.centerY() - val, 
                        bounds.left() + x, bounds.centerY() + val, wavePaint);
    }
    
    // Viewport indicator
    double totalDuration = samplesToTime(numSamples);
    float vpLeft = (float)(timeOffset_ / totalDuration) * w;
    float vpWidth = (float)(viewWidthSeconds_ / totalDuration) * w;
    
    SkPaint vpPaint;
    vpPaint.setColor(Colors::overviewViewport);
    canvas->drawRect(SkRect::MakeXYWH(bounds.left() + vpLeft, bounds.top(), vpWidth, h), vpPaint);
    
    vpPaint.setStyle(SkPaint::kStroke_Style);
    vpPaint.setColor(SK_ColorWHITE);
    vpPaint.setStrokeWidth(1);
    canvas->drawRect(SkRect::MakeXYWH(bounds.left() + vpLeft, bounds.top(), vpWidth, h), vpPaint);
}

void SampleEditorComponent::drawRuler(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint bgPaint;
    bgPaint.setColor(Colors::rulerBg);
    canvas->drawRect(bounds, bgPaint);
    
    double startTime = timeOffset_;
    double endTime = startTime + viewWidthSeconds_;
    
    double tickInterval = 1.0;
    if (viewWidthSeconds_ > 60) tickInterval = 10.0;
    else if (viewWidthSeconds_ > 30) tickInterval = 5.0;
    else if (viewWidthSeconds_ > 10) tickInterval = 2.0;
    else if (viewWidthSeconds_ < 2) tickInterval = 0.5;
    else if (viewWidthSeconds_ < 0.5) tickInterval = 0.1;
    
    SkFont font(nullptr, 10);
    SkPaint textPaint, tickPaint;
    textPaint.setColor(Colors::rulerText);
    textPaint.setAntiAlias(true);
    tickPaint.setColor(SkColorSetRGB(80, 80, 90));
    
    for (double t = std::floor(startTime / tickInterval) * tickInterval; t < endTime; t += tickInterval) {
        float x = timeToPixels(t, bounds.width());
        if (x < 0 || x > bounds.width()) continue;
        
        canvas->drawLine(x, bounds.bottom() - 6, x, bounds.bottom(), tickPaint);
        
        char timeStr[32];
        int mins = (int)(t / 60);
        if (mins > 0) snprintf(timeStr, 32, "%d:%04.1f", mins, t - mins * 60);
        else snprintf(timeStr, 32, "%.1fs", t);
        canvas->drawString(timeStr, x + 3, bounds.bottom() - 10, font, textPaint);
    }
}

void SampleEditorComponent::drawGrid(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint gridPaint;
    gridPaint.setColor(Colors::grid);
    
    double startTime = timeOffset_;
    double endTime = startTime + viewWidthSeconds_;
    
    for (double t = std::floor(startTime); t < endTime; t += 1.0) {
        float x = timeToPixels(t, bounds.width());
        if (x >= 0 && x <= bounds.width())
            canvas->drawLine(x, bounds.top(), x, bounds.bottom(), gridPaint);
    }
    
    // Center line
    gridPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
    canvas->drawLine(0, bounds.centerY(), bounds.width(), bounds.centerY(), gridPaint);
}

void SampleEditorComponent::drawWaveform(SkCanvas* canvas, const SkRect& bounds)
{
    if (!audioHandle_) return;
    
    const juce::AudioBuffer<float>* bufferPtr = &audioHandle_->buffer;
    if (editBuffer_) bufferPtr = editBuffer_.get();
    auto& buffer = *bufferPtr;
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    if (numSamples == 0) return;
    
    float w = bounds.width();
    float h = bounds.height();
    float channelHeight = h / (float)numChannels;
    
    juce::int64 startSample = timeToSamples(timeOffset_);
    juce::int64 endSample = timeToSamples(timeOffset_ + viewWidthSeconds_);
    startSample = std::max<juce::int64>(0, startSample);
    endSample = std::min<juce::int64>(numSamples, endSample);
    
    if (startSample >= endSample) return;
    
    float samplesPerPixel = (float)(endSample - startSample) / w;
    
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* samples = buffer.getReadPointer(ch);
        float centerY = bounds.top() + ch * channelHeight + channelHeight / 2;
        float amp = (channelHeight / 2) * 0.9f * verticalZoom_;
        
        SkPath path;
        path.moveTo(0, centerY);
        
        for (float x = 0; x < w; x++) {
            juce::int64 idx = startSample + (juce::int64)(x * samplesPerPixel);
            if (idx >= endSample) break;
            
            int step = std::max(1, (int)samplesPerPixel);
            juce::int64 rem = endSample - idx;
            int lim = (int)std::min<juce::int64>(step, rem);
            
            float maxV = samples[idx];
            for (int i = 1; i < lim; i++) {
                float s = samples[idx + i];
                if (s > maxV) maxV = s;
            }
            path.lineTo(x, centerY - maxV * amp);
        }
        
        for (float x = w - 1; x >= 0; x--) {
            juce::int64 idx = startSample + (juce::int64)(x * samplesPerPixel);
            if (idx >= endSample || idx < startSample) continue;
            
            int step = std::max(1, (int)samplesPerPixel);
            juce::int64 rem = endSample - idx;
            int lim = (int)std::min<juce::int64>(step, rem);
            
            float minV = samples[idx];
            for (int i = 1; i < lim; i++) {
                float s = samples[idx + i];
                if (s < minV) minV = s;
            }
            path.lineTo(x, centerY - minV * amp);
        }
        path.close();
        
        SkPaint fill;
        fill.setColor(Colors::waveform);
        fill.setAntiAlias(true);
        canvas->drawPath(path, fill);
        
        SkPaint outline;
        outline.setColor(Colors::waveformOutline);
        outline.setStyle(SkPaint::kStroke_Style);
        outline.setStrokeWidth(0.5f);
        outline.setAntiAlias(true);
        canvas->drawPath(path, outline);
    }
}

void SampleEditorComponent::drawSelection(SkCanvas* canvas, const SkRect& bounds)
{
    if (selection_.isEmpty()) return;
    
    float x1 = timeToPixels(selection_.getStart(), bounds.width());
    float x2 = timeToPixels(selection_.getEnd(), bounds.width());
    
    SkRect selRect = SkRect::MakeLTRB(x1, bounds.top(), x2, bounds.bottom());
    
    SkPaint fill;
    fill.setColor(Colors::selection);
    canvas->drawRect(selRect, fill);
    
    SkPaint border;
    border.setColor(Colors::selectionBorder);
    border.setStyle(SkPaint::kStroke_Style);
    canvas->drawRect(selRect, border);
}

void SampleEditorComponent::drawPlayhead(SkCanvas* canvas, const SkRect& bounds)
{
    float x = timeToPixels(playheadPosition_, bounds.width());
    if (x < 0 || x > bounds.width()) return;
    
    SkPaint paint;
    paint.setColor(Colors::playhead);
    paint.setStrokeWidth(2);
    canvas->drawLine(x, bounds.top(), x, bounds.bottom(), paint);
    
    // Triangle at top
    SkPath tri;
    tri.moveTo(x - 6, bounds.top());
    tri.lineTo(x + 6, bounds.top());
    tri.lineTo(x, bounds.top() + 8);
    tri.close();
    canvas->drawPath(tri, paint);
}

void SampleEditorComponent::drawMarkers(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint paint;
    paint.setColor(Colors::marker);
    paint.setStrokeWidth(1);
    
    SkFont font(nullptr, 9);
    SkPaint textPaint;
    textPaint.setColor(Colors::marker);
    
    for (const auto& m : markers_) {
        float x = timeToPixels(m.timeSeconds, bounds.width());
        if (x < 0 || x > bounds.width()) continue;
        
        canvas->drawLine(x, bounds.top(), x, bounds.bottom(), paint);
        canvas->drawString(m.name.toRawUTF8(), x + 2, bounds.top() + 12, font, textPaint);
    }
}

void SampleEditorComponent::drawRegions(SkCanvas* canvas, const SkRect& bounds)
{
    for (const auto& r : regions_) {
        float x1 = timeToPixels(r.startTime, bounds.width());
        float x2 = timeToPixels(r.endTime, bounds.width());
        
        SkPaint fill;
        fill.setColor(SkColorSetARGB(30, SkColorGetR(r.color), SkColorGetG(r.color), SkColorGetB(r.color)));
        canvas->drawRect(SkRect::MakeLTRB(x1, bounds.top(), x2, bounds.bottom()), fill);
    }
}

void SampleEditorComponent::drawScrollbar(SkCanvas* canvas, const SkRect& bounds)
{
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetRGB(35, 35, 40));
    canvas->drawRect(bounds, bgPaint);
    
    if (!audioHandle_) return;
    
    double total = samplesToTime(audioHandle_->lengthInSamples);
    float thumbX = (float)(timeOffset_ / total) * bounds.width();
    float thumbW = (float)(viewWidthSeconds_ / total) * bounds.width();
    thumbW = std::max(20.0f, thumbW);
    
    SkPaint thumbPaint;
    thumbPaint.setColor(SkColorSetRGB(80, 80, 90));
    canvas->drawRoundRect(SkRect::MakeXYWH(bounds.left() + thumbX, bounds.top() + 2, thumbW, bounds.height() - 4), 4, 4, thumbPaint);
}

void SampleEditorComponent::drawEmptyState(SkCanvas* canvas, float w, float h)
{
    SkFont font(nullptr, 16);
    SkPaint paint;
    paint.setColor(SkColorSetARGB(100, 255, 255, 255));
    canvas->drawString("Double-click an audio clip to edit", w/2 - 120, h/2, font, paint);
}

void SampleEditorComponent::drawBackground(SkCanvas*, float, float, float) {}
void SampleEditorComponent::drawBorder(SkCanvas*, float, float, float) {}
void SampleEditorComponent::drawSpectrogram(SkCanvas*, const SkRect&) {} // TODO

//==============================================================================
// Coordinate conversion
float SampleEditorComponent::timeToPixels(double t, float w) const {
    return (float)((t - timeOffset_) / viewWidthSeconds_) * w;
}
double SampleEditorComponent::pixelsToTime(float p, float w) const {
    return timeOffset_ + (p / w) * viewWidthSeconds_;
}
juce::int64 SampleEditorComponent::timeToSamples(double t) const {
    return audioHandle_ ? (juce::int64)(t * audioHandle_->sampleRate) : 0;
}
double SampleEditorComponent::samplesToTime(juce::int64 s) const {
    return (audioHandle_ && audioHandle_->sampleRate > 0) ? (double)s / audioHandle_->sampleRate : 0;
}

//==============================================================================
// Mouse handling
void SampleEditorComponent::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    lastMouseX_ = e.position.x;
    
    float waveformTop = toolbarHeight_ + overviewHeight_ + rulerHeight_;
    float waveformHeight = getHeight() - waveformTop - scrollbarHeight_;
    
    if (isInToolbar(e.position.y)) {
        // Handle toolbar clicks
        float x = e.position.x;
        float btnSize = 28, spacing = 4, startX = 8;
        int btnIdx = (int)((x - startX) / (btnSize + spacing));
        
        if (btnIdx == 0) setTool(SampleEditorTool::Select);
        else if (btnIdx == 1) setTool(SampleEditorTool::Pencil);
        else if (btnIdx == 2) setTool(SampleEditorTool::Slice);
        else if (btnIdx == 3) { isPlaying_ ? stop() : play(); }
        else if (btnIdx == 4) stop();
        else if (btnIdx == 5) toggleLoop();
    }
    else if (isInOverview(e.position.y)) {
        isDraggingOverview_ = true;
        if (audioHandle_) {
            double total = samplesToTime(audioHandle_->lengthInSamples);
            timeOffset_ = (e.position.x / getWidth()) * total - viewWidthSeconds_ / 2;
            timeOffset_ = std::max(0.0, timeOffset_);
            repaint();
        }
    }
    else if (isInRuler(e.position.y)) {
        // Click to set playhead
        playheadPosition_ = pixelsToTime(e.position.x, (float)getWidth());
        playheadPosition_ = std::max(0.0, playheadPosition_);
        isDraggingPlayhead_ = true;
        repaint();
    }
    else if (isInWaveform(e.position.y)) {
        double t = pixelsToTime(e.position.x, (float)getWidth());
        if (snapToZeroCrossing_) t = snapToNearestZeroCrossing(t);
        
        if (e.mods.isShiftDown() && hasSelection()) {
            // Extend selection
            if (t < selection_.getStart())
                selection_.setStart(t);
            else
                selection_.setEnd(t);
        } else {
            selection_.setStart(t);
            selection_.setEnd(t);
            selectionAnchor_ = t;
            isSelecting_ = true;
        }
        repaint();
    }
}

void SampleEditorComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingOverview_ && audioHandle_) {
        double total = samplesToTime(audioHandle_->lengthInSamples);
        timeOffset_ = (e.position.x / getWidth()) * total - viewWidthSeconds_ / 2;
        timeOffset_ = std::max(0.0, std::min(timeOffset_, total - viewWidthSeconds_));
        repaint();
    }
    else if (isDraggingPlayhead_) {
        playheadPosition_ = pixelsToTime(e.position.x, (float)getWidth());
        playheadPosition_ = std::max(0.0, playheadPosition_);
        repaint();
    }
    else if (isSelecting_) {
        double t = pixelsToTime(e.position.x, (float)getWidth());
        if (snapToZeroCrossing_) t = snapToNearestZeroCrossing(t);
        
        if (t < selectionAnchor_) {
            selection_.setStart(t);
            selection_.setEnd(selectionAnchor_);
        } else {
            selection_.setStart(selectionAnchor_);
            selection_.setEnd(t);
        }
        repaint();
    }
    
    lastMouseX_ = e.position.x;
}

void SampleEditorComponent::mouseUp(const juce::MouseEvent&)
{
    isSelecting_ = false;
    isDraggingPlayhead_ = false;
    isDraggingOverview_ = false;
}

void SampleEditorComponent::mouseMove(const juce::MouseEvent&) {}
void SampleEditorComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (isInWaveform(e.position.y)) {
        double t = pixelsToTime(e.position.x, (float)getWidth());
        addMarker(t, "M" + juce::String(markers_.size() + 1));
        repaint();
    }
}

void SampleEditorComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {
        if (e.mods.isShiftDown()) {
            // Vertical zoom
            zoomVertical(w.deltaY > 0 ? 1.1f : 0.9f);
        } else {
            // Horizontal zoom
            zoomHorizontal(w.deltaY > 0 ? 1.15f : 0.87f, e.position.x);
        }
    } else {
        scrollHorizontal(-w.deltaY * 50);
    }
}

bool SampleEditorComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey) { isPlaying_ ? stop() : play(); return true; }
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) { deleteSelection(); return true; }
    if (key.getModifiers().isCommandDown()) {
        if (key.getKeyCode() == 'X') { cutSelection(); return true; }
        if (key.getKeyCode() == 'C') { copySelection(); return true; }
        if (key.getKeyCode() == 'V') { paste(); return true; }
        if (key.getKeyCode() == 'A') { selectAll(); return true; }
        if (key.getKeyCode() == 'N') { normalize(); return true; }
        if (key.getKeyCode() == 'R') { reverse(); return true; }
    }
    if (key.getKeyCode() == 'L') { toggleLoop(); return true; }
    return false;
}

//==============================================================================
// Zoom/Scroll
void SampleEditorComponent::zoomHorizontal(float factor, float centerX)
{
    double centerT = pixelsToTime(centerX, (float)getWidth());
    viewWidthSeconds_ /= factor;
    viewWidthSeconds_ = juce::jlimit(0.01, 600.0, viewWidthSeconds_);
    timeOffset_ = centerT - (centerX / getWidth()) * viewWidthSeconds_;
    timeOffset_ = std::max(0.0, timeOffset_);
    repaint();
}

void SampleEditorComponent::zoomVertical(float factor)
{
    verticalZoom_ *= factor;
    verticalZoom_ = juce::jlimit(0.1f, 10.0f, verticalZoom_);
    repaint();
}

void SampleEditorComponent::scrollHorizontal(float delta)
{
    timeOffset_ += (delta / getWidth()) * viewWidthSeconds_;
    timeOffset_ = std::max(0.0, timeOffset_);
    repaint();
}

void SampleEditorComponent::fitToWindow()
{
    if (audioHandle_) {
        timeOffset_ = 0;
        viewWidthSeconds_ = samplesToTime(audioHandle_->lengthInSamples) * 1.02;
        repaint();
    }
}

void SampleEditorComponent::zoomToSelection()
{
    if (hasSelection()) {
        timeOffset_ = selection_.getStart() - 0.1;
        viewWidthSeconds_ = selection_.getLength() + 0.2;
        repaint();
    }
}

//==============================================================================
// Selection
void SampleEditorComponent::setSelection(double s, double e) { selection_ = {s, e}; repaint(); }
void SampleEditorComponent::selectAll() { 
    if (audioHandle_) selection_ = {0, samplesToTime(audioHandle_->lengthInSamples)}; 
    repaint();
}
void SampleEditorComponent::clearSelection() { selection_ = {}; repaint(); }

//==============================================================================
// Playback
void SampleEditorComponent::play() { isPlaying_ = true; repaint(); }
void SampleEditorComponent::stop() { isPlaying_ = false; repaint(); }
void SampleEditorComponent::playSelection() { 
    if (hasSelection()) { playheadPosition_ = selection_.getStart(); play(); }
}
void SampleEditorComponent::toggleLoop() { isLooping_ = !isLooping_; repaint(); }
void SampleEditorComponent::setPlayheadPosition(double t) { playheadPosition_ = t; repaint(); }

//==============================================================================
// Editing (stubs - would need CommandAPI integration)
void SampleEditorComponent::cutSelection() { copySelection(); deleteSelection(); }
void SampleEditorComponent::copySelection() {
    if (!hasSelection() || !audioHandle_) return;
    auto s = timeToSamples(selection_.getStart());
    auto e = timeToSamples(selection_.getEnd());
    int len = (int)(e - s);
    clipboard_ = std::make_unique<juce::AudioBuffer<float>>(audioHandle_->buffer.getNumChannels(), len);
    for (int ch = 0; ch < audioHandle_->buffer.getNumChannels(); ch++)
        clipboard_->copyFrom(ch, 0, audioHandle_->buffer, ch, (int)s, len);
    clipboardSampleRate_ = audioHandle_->sampleRate;
    DBG("Copied " + juce::String(len) + " samples");
}
void SampleEditorComponent::paste() { DBG("Paste - needs implementation"); }
void SampleEditorComponent::deleteSelection() { DBG("Delete - needs implementation"); }
void SampleEditorComponent::trimToSelection() { DBG("Trim - needs implementation"); }
void SampleEditorComponent::splitAtCursor() { DBG("Split - needs implementation"); }

//==============================================================================
// Processing - REAL IMPLEMENTATIONS
//==============================================================================

void SampleEditorComponent::normalize(float targetDb)
{
    if (!audioHandle_ || !hasSelection()) return;
    
    if (!editBuffer_) return;
    auto& buffer = *editBuffer_;
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    startSample = std::max<juce::int64>(0, startSample);
    endSample = std::min<juce::int64>(buffer.getNumSamples(), endSample);
    
    // Step 1: Find peak amplitude
    float peakLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        const float* data = buffer.getReadPointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            float absVal = std::abs(data[i]);
            if (absVal > peakLevel) peakLevel = absVal;
        }
    }
    
    if (peakLevel < 0.0001f) return; // Silence or near-silence
    
    // Step 2: Calculate gain to reach target
    // targetDb = 0 means normalize to 1.0 (0dBFS)
    float targetLinear = std::pow(10.0f, targetDb / 20.0f);
    float gain = targetLinear / peakLevel;
    
    // Step 3: Apply gain
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        float* data = buffer.getWritePointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            data[i] *= gain;
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("Normalized selection: peak=" + juce::String(peakLevel) + " gain=" + juce::String(gain));
    repaint();
}

void SampleEditorComponent::reverse()
{
    if (!audioHandle_ || !hasSelection()) return;
    
    if (!editBuffer_) return;
    auto& buffer = *editBuffer_;
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    startSample = std::max<juce::int64>(0, startSample);
    endSample = std::min<juce::int64>(buffer.getNumSamples(), endSample);
    
    // Reverse each channel independently
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        float* data = buffer.getWritePointer(ch);
        juce::int64 left = startSample;
        juce::int64 right = endSample - 1;
        
        while (left < right) {
            std::swap(data[left], data[right]);
            left++;
            right--;
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("Reversed selection");
    repaint();
}

void SampleEditorComponent::fadeIn(double durationSeconds)
{
    if (!audioHandle_) return;
    
    if (!editBuffer_) return;
    auto& buffer = *editBuffer_;
    double startTime = hasSelection() ? selection_.getStart() : 0.0;
    juce::int64 startSample = timeToSamples(startTime);
    juce::int64 fadeSamples = (juce::int64)(durationSeconds * audioHandle_->sampleRate);
    juce::int64 endSample = std::min<juce::int64>(startSample + fadeSamples, buffer.getNumSamples());
    
    // Apply x-squared curve for natural fade (logarithmic perception)
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        float* data = buffer.getWritePointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            float progress = (float)(i - startSample) / (float)(endSample - startSample);
            float gain = progress * progress; // x² curve
            data[i] *= gain;
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("Applied fade in: " + juce::String(durationSeconds) + "s");
    repaint();
}

void SampleEditorComponent::fadeOut(double durationSeconds)
{
    if (!audioHandle_) return;
    
    if (!editBuffer_) return;
    auto& buffer = *editBuffer_;
    double endTime = hasSelection() ? selection_.getEnd() : samplesToTime(buffer.getNumSamples());
    juce::int64 endSample = timeToSamples(endTime);
    juce::int64 fadeSamples = (juce::int64)(durationSeconds * audioHandle_->sampleRate);
    juce::int64 startSample = std::max<juce::int64>(0, endSample - fadeSamples);
    
    // Apply inverse x-squared curve
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        float* data = buffer.getWritePointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            float progress = (float)(i - startSample) / (float)(endSample - startSample);
            float gain = (1.0f - progress) * (1.0f - progress); // inverse x² curve
            data[i] *= gain;
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("Applied fade out: " + juce::String(durationSeconds) + "s");
    repaint();
}

void SampleEditorComponent::adjustGain(float db)
{
    if (!audioHandle_ || !hasSelection()) return;
    
    float gain = std::pow(10.0f, db / 20.0f); // Convert dB to linear
    
    if (!editBuffer_) return;
    auto& buffer = *editBuffer_;
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        float* data = buffer.getWritePointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            data[i] *= gain;
            // Soft clip to prevent harsh distortion
            if (data[i] > 1.0f) data[i] = 1.0f - 1.0f / (data[i] + 1.0f);
            else if (data[i] < -1.0f) data[i] = -1.0f - 1.0f / (data[i] - 1.0f);
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("Adjusted gain: " + juce::String(db) + "dB");
    repaint();
}

void SampleEditorComponent::silenceSelection()
{
    if (!audioHandle_ || !hasSelection()) return;
    
    if (!editBuffer_) return;
    auto& buffer = *editBuffer_;
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        float* data = buffer.getWritePointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            data[i] = 0.0f;
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("Silenced selection");
    repaint();
}

void SampleEditorComponent::removeOffset()
{
    if (!audioHandle_ || !hasSelection()) return;
    
    if (!editBuffer_) return;
    auto& buffer = *editBuffer_;
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    // Calculate DC offset (average of all samples)
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        float* data = buffer.getWritePointer(ch);
        double sum = 0.0;
        for (juce::int64 i = startSample; i < endSample; i++) {
            sum += data[i];
        }
        float dcOffset = (float)(sum / (double)(endSample - startSample));
        
        // Subtract DC offset
        for (juce::int64 i = startSample; i < endSample; i++) {
            data[i] -= dcOffset;
        }
        
        DBG("Removed DC offset ch" + juce::String(ch) + ": " + juce::String(dcOffset));
    }
    
    hasUnsavedChanges_ = true;
    repaint();
}

//==============================================================================
// Advanced Processing
//==============================================================================

void SampleEditorComponent::timeStretch(float ratio)
{
    // Time stretching requires complex DSP (phase vocoder or WSOLA)
    // Stub for now - would integrate with a library like Rubber Band
    DBG("Time stretch ratio: " + juce::String(ratio) + " - requires external library");
}

void SampleEditorComponent::pitchShift(int semitones)
{
    // Pitch shifting requires FFT-based processing
    // Stub - would integrate with Rubber Band or similar
    DBG("Pitch shift: " + juce::String(semitones) + " semitones - requires external library");
}

void SampleEditorComponent::detectTransients(float sensitivity)
{
    if (!audioHandle_) return;
    
    const juce::AudioBuffer<float>* bufferPtr = &audioHandle_->buffer;
    if (editBuffer_) bufferPtr = editBuffer_.get();
    auto& buffer = *bufferPtr;
    int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);
    
    // Energy-based transient detection using sliding window
    constexpr int windowSize = 512;
    constexpr int hopSize = 256;
    
    float prevEnergy = 0.0f;
    float threshold = sensitivity * 0.5f; // Adjust based on sensitivity
    
    clearMarkers();
    
    for (int i = windowSize; i < numSamples - windowSize; i += hopSize) {
        // Calculate energy in current window
        float energy = 0.0f;
        for (int j = 0; j < windowSize; j++) {
            energy += data[i + j] * data[i + j];
        }
        energy = std::sqrt(energy / windowSize); // RMS energy
        
        // Detect sudden increase in energy (transient)
        float energyDelta = energy - prevEnergy;
        if (energyDelta > threshold && energy > 0.05f) {
            // Found a transient
            double time = samplesToTime(i);
            addMarker(time, "T" + juce::String(markers_.size() + 1));
        }
        
        prevEnergy = energy;
    }
    
    DBG("Detected " + juce::String(markers_.size()) + " transients");
    repaint();
}

void SampleEditorComponent::autoSlice(float sensitivity)
{
    // First detect transients
    detectTransients(sensitivity);
    
    // Convert markers to regions
    clearRegions();
    
    if (markers_.empty()) return;
    
    for (size_t i = 0; i < markers_.size(); i++) {
        double start = markers_[i].timeSeconds;
        double end = (i < markers_.size() - 1) ? markers_[i + 1].timeSeconds : samplesToTime(audioHandle_->lengthInSamples);
        addRegion(start, end, "Slice " + juce::String(i + 1));
    }
    
    DBG("Created " + juce::String(regions_.size()) + " slices");
}

void SampleEditorComponent::sliceToMidi()
{
    // Export slices as MIDI notes (would create a new MIDI clip)
    DBG("Slice to MIDI - " + juce::String(regions_.size()) + " regions would be exported");
}

//==============================================================================
// Analysis
//==============================================================================

void SampleEditorComponent::generateWaveformCache()
{
    // Pre-compute min/max peaks at multiple resolutions for fast rendering
    DBG("Generating waveform cache...");
}

float SampleEditorComponent::getRMSLevel() const
{
    if (!audioHandle_ || !hasSelection()) return 0.0f;
    
    const juce::AudioBuffer<float>* bufferPtr = &audioHandle_->buffer;
    if (editBuffer_) bufferPtr = editBuffer_.get();
    auto& buffer = *bufferPtr;
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    double sum = 0.0;
    int count = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        const float* data = buffer.getReadPointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            sum += data[i] * data[i];
            count++;
        }
    }
    
    return count > 0 ? (float)std::sqrt(sum / count) : 0.0f;
}

float SampleEditorComponent::getPeakLevel() const
{
    if (!audioHandle_ || !hasSelection()) return 0.0f;
    
    const juce::AudioBuffer<float>* bufferPtr = &audioHandle_->buffer;
    if (editBuffer_) bufferPtr = editBuffer_.get();
    auto& buffer = *bufferPtr;
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
        const float* data = buffer.getReadPointer(ch);
        for (juce::int64 i = startSample; i < endSample; i++) {
            float absVal = std::abs(data[i]);
            if (absVal > peak) peak = absVal;
        }
    }
    
    return peak;
}

//==============================================================================
// Markers
void SampleEditorComponent::addMarker(double t, const juce::String& name) {
    markers_.push_back({juce::Uuid().toString(), name, t, Colors::marker});
    repaint();
}
void SampleEditorComponent::removeMarker(const juce::String& id) {
    markers_.erase(std::remove_if(markers_.begin(), markers_.end(), [&](auto& m){ return m.id == id; }), markers_.end());
    repaint();
}
void SampleEditorComponent::clearMarkers() { markers_.clear(); repaint(); }

void SampleEditorComponent::addRegion(double s, double e, const juce::String& name) {
    regions_.push_back({juce::Uuid().toString(), name, s, e, SkColorSetRGB(100, 200, 100)});
    repaint();
}
void SampleEditorComponent::removeRegion(const juce::String& id) {
    regions_.erase(std::remove_if(regions_.begin(), regions_.end(), [&](auto& r){ return r.id == id; }), regions_.end());
    repaint();
}
void SampleEditorComponent::clearRegions() { regions_.clear(); repaint(); }

double SampleEditorComponent::snapToNearestZeroCrossing(double t) const {
    if (!audioHandle_) return t;
    juce::int64 sampleIdx = timeToSamples(t);
    const float* data = audioHandle_->buffer.getReadPointer(0);
    int numSamples = audioHandle_->buffer.getNumSamples();
    
    for (int offset = 0; offset < 100; offset++) {
        juce::int64 left = sampleIdx - offset;
        juce::int64 right = sampleIdx + offset;
        
        if (left > 0 && left < numSamples - 1) {
            if ((data[left] >= 0 && data[left + 1] < 0) || (data[left] < 0 && data[left + 1] >= 0))
                return samplesToTime(left);
        }
        if (right > 0 && right < numSamples - 1) {
            if ((data[right] >= 0 && data[right + 1] < 0) || (data[right] < 0 && data[right + 1] >= 0))
                return samplesToTime(right);
        }
    }
    return t;
}

void SampleEditorComponent::resized() {}
void SampleEditorComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& prop) {
    if (tree == clipNode_ && prop.toString() == "sourceFile")
        setClipToEdit(currentTrackId_, currentClipId_);
}


} // namespace zenith
