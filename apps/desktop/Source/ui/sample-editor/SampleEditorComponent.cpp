/*
  ==============================================================================
    SampleEditorComponent.cpp - ZENITH EDISON
    Professional Sample Editor Implementation
  ==============================================================================
*/

#include "SampleEditorComponent.h"
#include "SampleEditorActions.h"
#include "ZenithDesignSystem.h"
#include <include/core/SkFont.h>
#include <include/core/SkRRect.h>
#include <include/effects/SkGradientShader.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../../dsp/TimeStretcher.h"
#include "../../dsp/SpectralProcessor.h"

namespace {
    // Thread-safe FIFO for incoming audio
    class ScopedFifoWriter {
    public:
        ScopedFifoWriter(juce::AbstractFifo& fifo, int numSamples) : fifo_(fifo) {
            fifo_.prepareToWrite(numSamples, start1, size1, start2, size2);
        }
        
        ~ScopedFifoWriter() {
            if (size1 + size2 > 0)
                fifo_.finishedWrite(size1 + size2);
        }
        
        int start1, size1, start2, size2;
        
    private:
        juce::AbstractFifo& fifo_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedFifoWriter)
    };
}

namespace zenith {

// Static clipboard
std::unique_ptr<juce::AudioBuffer<float>> SampleEditorComponent::clipboard_ =
    nullptr;
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
} // namespace Colors

//==============================================================================
SampleEditorComponent::SampleEditorComponent(Engine &engine,
                                             ProjectState &state)
    : engine_(engine), projectState_(state) {
  // NOTE: Do NOT use setOpaque(true) with Skia components!
  // Skia rendering bypasses JUCE's paint() method, and setOpaque(true)
  // tells JUCE the component will fill all pixels via paint(), which
  // causes black screens when drawing is done via drawSkia() instead.
  setOpaque(false);
  setWantsKeyboardFocus(true);

  backgroundColor_ = Colors::bg;
  waveformColor_ = Colors::waveform;
  selectionColor_ = Colors::selection;
  gridColor_ = Colors::grid;
  rulerColor_ = Colors::rulerText;
  playheadColor_ = Colors::playhead;

  projectState_.getState().addListener(this);
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(30); // 30fps playhead updates
}

SampleEditorComponent::~SampleEditorComponent() {
  stopTimer();
  projectState_.getState().removeListener(this);
}

void SampleEditorComponent::timerCallback() {


  if (isPlaying_) {
    // Update playhead from engine
    playheadPosition_ += 1.0 / 30.0; // Approximate
    if (audioHandle_ &&
        playheadPosition_ > samplesToTime(audioHandle_->lengthInSamples)) {
      if (isLooping_ && hasSelection()) {
        playheadPosition_ = selection_.getStart();
      } else {
        stop();
      }
    }
    repaint();
  }

  if (isRecording_) {
    // Drain FIFO to record buffer
    int numReady = incomingFifo_->getNumReady();
    if (numReady > 0) {
      if (!recordBuffer_) {
        // Should have been allocated in startRecording
        incomingFifo_.reset();
        return;
      }

      int start1, size1, start2, size2;
      incomingFifo_->prepareToRead(numReady, start1, size1, start2, size2);

      // Append to recordBuffer_
      int currentCapacity = recordBuffer_->getNumSamples();
      int requiredCapacity = recordWritePos_ + size1 + size2;
      
      // Grow buffer if needed (amortized doubling)
      if (currentCapacity < requiredCapacity) {
        int newCapacity = std::max(requiredCapacity, currentCapacity * 2);
        newCapacity = std::max(newCapacity, 4096); // Min size
        recordBuffer_->setSize(recordBuffer_->getNumChannels(), newCapacity, true, true, true);
      }
      
      // Copy data from ring buffer
      if (size1 > 0)
        recordBuffer_->copyFrom(0, recordWritePos_, incomingBuffer_, 0, start1, size1);
      if (size2 > 0)
        recordBuffer_->copyFrom(0, recordWritePos_ + size1, incomingBuffer_, 0, start2, size2);

      incomingFifo_->finishedRead(size1 + size2);
      recordWritePos_ += (size1 + size2);
      
      repaint();
    }
  }
}

//==============================================================================
void SampleEditorComponent::setClipToEdit(const juce::String &trackId,
                                          const juce::String &clipId) {
  currentTrackId_ = trackId;
  currentClipId_ = clipId;

  auto trackNode = projectState_.getTrack(trackId);
  if (trackNode.isValid()) {
    auto clips = trackNode.getChildWithName("CLIPS");
    clipNode_ = clips.getChildWithProperty("id", clipId);

    if (clipNode_.isValid()) {
      juce::File audioFile(clipNode_.getProperty("sourceFile").toString());
      if (audioFile.existsAsFile()) {
        // Try to get from cache first
        audioHandle_ = engine_.getAudioFilePool().getFile(audioFile);
        
        auto initBuffer = [this]() {
            if (!audioHandle_) return;
            // Create mutable copy for editing
            int numChannels = audioHandle_->buffer.getNumChannels();
            int numSamples = audioHandle_->buffer.getNumSamples();

            editBuffer_ = std::make_unique<juce::AudioBuffer<float>>(numChannels,
                                                                     numSamples);

            for (int i = 0; i < numChannels; ++i) {
              editBuffer_->copyFrom(i, 0, audioHandle_->buffer, i, 0, numSamples);
            }

            fitToWindow();
            repaint();
        };

        if (audioHandle_) {
            initBuffer();
        } else {
            // Async load to prevent UI freeze
            engine_.getAudioFilePool().loadFileAsync(audioFile, 
                [this, trackId, clipId, initBuffer](AudioFilePool::HandlePtr handle, juce::String error) {
                    // Check if we are still editing the same clip
                    if (handle && currentTrackId_ == trackId && currentClipId_ == clipId) {
                        audioHandle_ = handle;
                        initBuffer();
                    } else if (!handle) {
                        DBG("SampleEditor: Failed to load " << error);
                    }
                });
        }
      }
    }
  }
  repaint();
}

void SampleEditorComponent::clearClip() {
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
void SampleEditorComponent::drawSkia(SkCanvas *canvas) {
  float w = (float)getWidth();
  float h = (float)getHeight();

  // Background
  SkPaint bgPaint;
  bgPaint.setColor(Colors::bg);
  canvas->drawRect(SkRect::MakeWH(w, h), bgPaint);

  // Layout areas
  SkRect toolbarRect = SkRect::MakeXYWH(0, 0, w, toolbarHeight_);
  SkRect overviewRect = SkRect::MakeXYWH(0, toolbarHeight_, w, overviewHeight_);
  SkRect rulerRect =
      SkRect::MakeXYWH(0, toolbarHeight_ + overviewHeight_, w, rulerHeight_);
  SkRect waveformRect = SkRect::MakeXYWH(
      0, toolbarHeight_ + overviewHeight_ + rulerHeight_, w,
      h - toolbarHeight_ - overviewHeight_ - rulerHeight_ - scrollbarHeight_);
  SkRect scrollRect =
      SkRect::MakeXYWH(0, h - scrollbarHeight_, w, scrollbarHeight_);

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
  
  // Draw waveform or spectrogram based on view mode
  if (viewMode_ == WaveformViewMode::Spectrogram || viewMode_ == WaveformViewMode::Combined) {
    drawSpectrogram(canvas, waveformRect);
  }
  if (viewMode_ == WaveformViewMode::Waveform || viewMode_ == WaveformViewMode::Combined) {
    drawWaveform(canvas, waveformRect);
  }
  
  drawSelection(canvas, waveformRect);
  drawMarkers(canvas, waveformRect);
  drawWarpMarkers(canvas, waveformRect);
  drawPlayhead(canvas, waveformRect);
  drawScrollbar(canvas, scrollRect);
}

//==============================================================================
void SampleEditorComponent::drawToolbar(SkCanvas *canvas,
                                        const SkRect &bounds) {
  // Toolbar background
  SkPaint bgPaint;
  bgPaint.setColor(Colors::toolbarBg);
  canvas->drawRect(bounds, bgPaint);

  // Bottom border
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(60, 0, 0, 0));
  canvas->drawLine(0, bounds.bottom(), bounds.width(), bounds.bottom(),
                   borderPaint);

  float x = 8;
  float btnSize = 28;
  float spacing = 4;

  SkFont font(nullptr, 10);
  SkPaint textPaint;
  textPaint.setColor(Colors::rulerText);
  textPaint.setAntiAlias(true);

  // Tool buttons group
  auto drawBtn = [&](const char *label, bool active) {
    SkRect btnRect =
        SkRect::MakeXYWH(x, (bounds.height() - btnSize) / 2, btnSize, btnSize);
    SkPaint btnPaint;
    btnPaint.setColor(active ? Colors::buttonActive
                             : SkColorSetARGB(40, 255, 255, 255));
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
  canvas->drawString("Cut", x, bounds.centerY() + 4, font, textPaint);
  x += 30;
  canvas->drawString("Copy", x, bounds.centerY() + 4, font, textPaint);
  x += 35;
  canvas->drawString("Paste", x, bounds.centerY() + 4, font, textPaint);
  x += 40;

  x += 12;

  // Processing
  canvas->drawString("Norm", x, bounds.centerY() + 4, font, textPaint);
  x += 35;
  canvas->drawString("Rev", x, bounds.centerY() + 4, font, textPaint);
  x += 30;
  canvas->drawString("Fade", x, bounds.centerY() + 4, font, textPaint);
  x += 35;

  // Right side - zoom info
  if (audioHandle_) {
    juce::String zoomStr = juce::String::formatted("%.1fx  V:%.0f%%",
             samplesToTime(audioHandle_->lengthInSamples) / viewWidthSeconds_,
             verticalZoom_ * 100);
    canvas->drawString(zoomStr.toStdString().c_str(), bounds.width() - 100, bounds.centerY() + 4,
                       font, textPaint);
  }
}

void SampleEditorComponent::drawOverview(SkCanvas *canvas,
                                         const SkRect &bounds) {
  SkPaint bgPaint;
  bgPaint.setColor(Colors::overviewBg);
  canvas->drawRect(bounds, bgPaint);

  if (!audioHandle_)
    return;

  // Draw mini waveform
  const juce::AudioBuffer<float> *bufferPtr = &audioHandle_->buffer;
  if (editBuffer_)
    bufferPtr = editBuffer_.get();
  auto &buffer = *bufferPtr;
  int numSamples = buffer.getNumSamples();
  float w = bounds.width();
  float h = bounds.height();

  SkPaint wavePaint;
  wavePaint.setColor(SkColorSetARGB(100, 90, 160, 220));

  float samplesPerPixel = (float)numSamples / w;
  const float *samples = buffer.getReadPointer(0);

  for (float x = 0; x < w; x++) {
    int idx = (int)(x * samplesPerPixel);
    if (idx >= numSamples)
      break;
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
  canvas->drawRect(
      SkRect::MakeXYWH(bounds.left() + vpLeft, bounds.top(), vpWidth, h),
      vpPaint);

  vpPaint.setStyle(SkPaint::kStroke_Style);
  vpPaint.setColor(SK_ColorWHITE);
  vpPaint.setStrokeWidth(1);
  canvas->drawRect(
      SkRect::MakeXYWH(bounds.left() + vpLeft, bounds.top(), vpWidth, h),
      vpPaint);
}

void SampleEditorComponent::drawRuler(SkCanvas *canvas, const SkRect &bounds) {
  SkPaint bgPaint;
  bgPaint.setColor(Colors::rulerBg);
  canvas->drawRect(bounds, bgPaint);

  double startTime = timeOffset_;
  double endTime = startTime + viewWidthSeconds_;

  double tickInterval = 1.0;
  if (viewWidthSeconds_ > 60)
    tickInterval = 10.0;
  else if (viewWidthSeconds_ > 30)
    tickInterval = 5.0;
  else if (viewWidthSeconds_ > 10)
    tickInterval = 2.0;
  else if (viewWidthSeconds_ < 2)
    tickInterval = 0.5;
  else if (viewWidthSeconds_ < 0.5)
    tickInterval = 0.1;

  SkFont font(nullptr, 10);
  SkPaint textPaint, tickPaint;
  textPaint.setColor(Colors::rulerText);
  textPaint.setAntiAlias(true);
  tickPaint.setColor(SkColorSetRGB(80, 80, 90));

  for (double t = std::floor(startTime / tickInterval) * tickInterval;
       t < endTime; t += tickInterval) {
    float x = timeToPixels(t, bounds.width());
    if (x < 0 || x > bounds.width())
      continue;

    canvas->drawLine(x, bounds.bottom() - 6, x, bounds.bottom(), tickPaint);

    juce::String timeStr;
    int mins = (int)(t / 60);
    if (mins > 0)
      timeStr = juce::String::formatted("%d:%04.1f", mins, t - mins * 60);
    else
      timeStr = juce::String::formatted("%.1fs", t);
    canvas->drawString(timeStr.toStdString().c_str(), x + 3, bounds.bottom() - 10, font, textPaint);
  }
}

void SampleEditorComponent::drawGrid(SkCanvas *canvas, const SkRect &bounds) {
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
  canvas->drawLine(0, bounds.centerY(), bounds.width(), bounds.centerY(),
                   gridPaint);
}

void SampleEditorComponent::drawWaveform(SkCanvas *canvas,
                                         const SkRect &bounds) {
  if (!audioHandle_)
    return;

  const juce::AudioBuffer<float> *bufferPtr = &audioHandle_->buffer;
  if (editBuffer_)
    bufferPtr = editBuffer_.get();
  auto &buffer = *bufferPtr;
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  if (numSamples == 0)
    return;

  float w = bounds.width();
  float h = bounds.height();
  float channelHeight = h / (float)numChannels;

  juce::int64 startSample = timeToSamples(timeOffset_);
  juce::int64 endSample = timeToSamples(timeOffset_ + viewWidthSeconds_);
  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(numSamples, endSample);

  if (startSample >= endSample)
    return;

  float samplesPerPixel = (float)(endSample - startSample) / w;

  for (int ch = 0; ch < numChannels; ++ch) {
    const float *samples = buffer.getReadPointer(ch);
    float centerY = bounds.top() + ch * channelHeight + channelHeight / 2;
    float amp = (channelHeight / 2) * 0.9f * verticalZoom_;

    SkPath path;
    path.moveTo(0, centerY);

    for (float x = 0; x < w; x++) {
      juce::int64 idx = startSample + (juce::int64)(x * samplesPerPixel);
      if (idx >= endSample)
        break;

      int step = std::max(1, (int)samplesPerPixel);
      juce::int64 rem = endSample - idx;
      int lim = (int)std::min<juce::int64>(step, rem);

      float maxV = samples[idx];
      for (int i = 1; i < lim; i++) {
        float s = samples[idx + i];
        if (s > maxV)
          maxV = s;
      }
      path.lineTo(x, centerY - maxV * amp);
    }

    for (float x = w - 1; x >= 0; x--) {
      juce::int64 idx = startSample + (juce::int64)(x * samplesPerPixel);
      if (idx >= endSample || idx < startSample)
        continue;

      int step = std::max(1, (int)samplesPerPixel);
      juce::int64 rem = endSample - idx;
      int lim = (int)std::min<juce::int64>(step, rem);

      float minV = samples[idx];
      for (int i = 1; i < lim; i++) {
        float s = samples[idx + i];
        if (s < minV)
          minV = s;
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

void SampleEditorComponent::drawSelection(SkCanvas *canvas,
                                          const SkRect &bounds) {
  if (selection_.isEmpty())
    return;

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

void SampleEditorComponent::drawPlayhead(SkCanvas *canvas,
                                         const SkRect &bounds) {
  float x = timeToPixels(playheadPosition_, bounds.width());
  if (x < 0 || x > bounds.width())
    return;

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

void SampleEditorComponent::drawMarkers(SkCanvas *canvas,
                                        const SkRect &bounds) {
  SkPaint paint;
  paint.setColor(Colors::marker);
  paint.setStrokeWidth(1);

  SkFont font(nullptr, 9);
  SkPaint textPaint;
  textPaint.setColor(Colors::marker);

  for (const auto &m : markers_) {
    float x = timeToPixels(m.timeSeconds, bounds.width());
    if (x < 0 || x > bounds.width())
      continue;

    canvas->drawLine(x, bounds.top(), x, bounds.bottom(), paint);
    canvas->drawString(m.name.toRawUTF8(), x + 2, bounds.top() + 12, font,
                       textPaint);
  }
}

void SampleEditorComponent::drawRegions(SkCanvas *canvas,
                                        const SkRect &bounds) {
  for (const auto &r : regions_) {
    float x1 = timeToPixels(r.startTime, bounds.width());
    float x2 = timeToPixels(r.endTime, bounds.width());

    SkPaint fill;
    fill.setColor(SkColorSetARGB(30, SkColorGetR(r.color), SkColorGetG(r.color),
                                 SkColorGetB(r.color)));
    canvas->drawRect(SkRect::MakeLTRB(x1, bounds.top(), x2, bounds.bottom()),
                     fill);
  }
}

void SampleEditorComponent::drawWarpMarkers(SkCanvas *canvas,
                                            const SkRect &bounds) {
  if (warpMarkers_.empty())
    return;

  SkPaint linePaint;
  linePaint.setColor(SkColorSetRGB(0, 200, 255)); // Cyan for warp markers
  linePaint.setStrokeWidth(2);
  linePaint.setAntiAlias(true);

  SkPaint handlePaint;
  handlePaint.setColor(SkColorSetRGB(0, 220, 255));
  handlePaint.setAntiAlias(true);

  SkPaint textPaint;
  textPaint.setColor(SkColorSetRGB(0, 200, 255));
  textPaint.setAntiAlias(true);

  SkFont font(nullptr, 9);

  for (size_t i = 0; i < warpMarkers_.size(); ++i) {
    const auto &m = warpMarkers_[i];
    
    // Draw at warped time position (where it will be after stretching)
    float x = timeToPixels(m.warpedTime, bounds.width());
    if (x < 0 || x > bounds.width())
      continue;

    // Draw vertical line
    canvas->drawLine(x, bounds.top(), x, bounds.bottom(), linePaint);

    // Draw diamond handle at top
    SkPath diamond;
    float handleSize = 6.0f;
    diamond.moveTo(x, bounds.top());
    diamond.lineTo(x + handleSize, bounds.top() + handleSize);
    diamond.lineTo(x, bounds.top() + handleSize * 2);
    diamond.lineTo(x - handleSize, bounds.top() + handleSize);
    diamond.close();
    canvas->drawPath(diamond, handlePaint);

    // Draw label showing time offset
    double offset = m.warpedTime - m.originalTime;
    juce::String label = (offset >= 0 ? "+" : "") + 
                         juce::String(offset * 1000.0, 1) + "ms";
    canvas->drawString(label.toStdString().c_str(), x + 4, 
                       bounds.top() + handleSize * 2 + 10, font, textPaint);
  }
}

void SampleEditorComponent::drawScrollbar(SkCanvas *canvas,
                                          const SkRect &bounds) {
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(35, 35, 40));
  canvas->drawRect(bounds, bgPaint);

  if (!audioHandle_)
    return;

  double total = samplesToTime(audioHandle_->lengthInSamples);
  float thumbX = (float)(timeOffset_ / total) * bounds.width();
  float thumbW = (float)(viewWidthSeconds_ / total) * bounds.width();
  thumbW = std::max(20.0f, thumbW);

  SkPaint thumbPaint;
  thumbPaint.setColor(SkColorSetRGB(80, 80, 90));
  canvas->drawRoundRect(SkRect::MakeXYWH(bounds.left() + thumbX,
                                         bounds.top() + 2, thumbW,
                                         bounds.height() - 4),
                        4, 4, thumbPaint);
}

void SampleEditorComponent::drawEmptyState(SkCanvas *canvas, float w, float h) {
  SkFont font(nullptr, 16);
  SkPaint paint;
  paint.setColor(SkColorSetARGB(100, 255, 255, 255));
  canvas->drawString("Double-click an audio clip to edit", w / 2 - 120, h / 2,
                     font, paint);
}

void SampleEditorComponent::drawBackground(SkCanvas *canvas, float w, float h,
                                           float radius) {
  SkPaint bgPaint;
  bgPaint.setColor(Colors::bg);
  bgPaint.setAntiAlias(true);

  if (radius > 0) {
    canvas->drawRoundRect(SkRect::MakeWH(w, h), radius, radius, bgPaint);
  } else {
    canvas->drawRect(SkRect::MakeWH(w, h), bgPaint);
  }
}

void SampleEditorComponent::drawBorder(SkCanvas *canvas, float w, float h,
                                       float radius) {
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(60, 255, 255, 255));
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setAntiAlias(true);

  if (radius > 0) {
    canvas->drawRoundRect(SkRect::MakeWH(w, h), radius, radius, borderPaint);
  } else {
    canvas->drawRect(SkRect::MakeWH(w, h), borderPaint);
  }
}

void SampleEditorComponent::drawSpectrogram(SkCanvas *canvas,
                                            const SkRect &bounds) {
  if (!audioHandle_)
    return;

  const juce::AudioBuffer<float> *bufferPtr = &audioHandle_->buffer;
  if (editBuffer_)
    bufferPtr = editBuffer_.get();
  auto &buffer = *bufferPtr;
  int numSamples = buffer.getNumSamples();
  const float *data = buffer.getReadPointer(0);

  if (numSamples == 0)
    return;

  float w = bounds.width();
  float h = bounds.height();

  // Calculate visible range
  juce::int64 startSample = timeToSamples(timeOffset_);
  juce::int64 endSample = timeToSamples(timeOffset_ + viewWidthSeconds_);
  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(numSamples, endSample);

  if (startSample >= endSample)
    return;

  // Simple spectrogram using energy bands (basic FFT approximation)
  // For real FFT, integrate a library like FFTW or use juce::dsp::FFT
  constexpr int numBands = 32;    // Frequency bands
  constexpr int windowSize = 512; // Analysis window

  float samplesPerPixel = (float)(endSample - startSample) / w;
  float bandHeight = h / numBands;

  // Color gradient for intensity
  auto getSpectrogramColor = [](float intensity) -> SkColor {
    // Blue -> Cyan -> Green -> Yellow -> Red
    intensity = juce::jlimit(0.0f, 1.0f, intensity);
    if (intensity < 0.25f) {
      float t = intensity / 0.25f;
      return SkColorSetRGB(0, (int)(t * 50), (int)(50 + t * 150));
    } else if (intensity < 0.5f) {
      float t = (intensity - 0.25f) / 0.25f;
      return SkColorSetRGB(0, (int)(50 + t * 200), (int)(200 - t * 100));
    } else if (intensity < 0.75f) {
      float t = (intensity - 0.5f) / 0.25f;
      return SkColorSetRGB((int)(t * 255), 255, (int)(100 - t * 100));
    } else {
      float t = (intensity - 0.75f) / 0.25f;
      return SkColorSetRGB(255, (int)(255 - t * 255), 0);
    }
  };

  SkPaint paint;
  paint.setAntiAlias(false);

  // Draw spectrogram columns
  for (float x = 0; x < w; x += 2.0f) { // Step by 2 pixels for performance
    juce::int64 samplePos = startSample + (juce::int64)(x * samplesPerPixel);
    if (samplePos + windowSize >= numSamples)
      break;

    // Calculate energy in frequency bands using simple bandpass energy
    std::array<float, numBands> bandEnergies = {};

    for (int b = 0; b < numBands; b++) {
      // Approximate: divide window into bands and measure energy
      int bandStart = (b * windowSize) / numBands;
      int bandEnd = ((b + 1) * windowSize) / numBands;

      float energy = 0.0f;
      for (int i = bandStart; i < bandEnd && samplePos + i < numSamples; i++) {
        float sample = data[samplePos + i];
        energy += sample * sample;
      }
      bandEnergies[b] = std::sqrt(energy / (bandEnd - bandStart));
    }

    // Draw vertical column of bands
    for (int b = 0; b < numBands; b++) {
      // Map band index to y (low freq at bottom)
      float y = bounds.bottom() - (b + 1) * bandHeight;
      float intensity = bandEnergies[b] * 3.0f; // Amplify for visibility

      paint.setColor(getSpectrogramColor(intensity));
      canvas->drawRect(SkRect::MakeXYWH(bounds.left() + x, y, 2.0f, bandHeight),
                       paint);
    }
  }
}

//==============================================================================
// Coordinate conversion
void SampleEditorComponent::pushUndoState(const juce::String& transactionName) {
    projectState_.getUndoManager().beginNewTransaction(transactionName);
}

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
  return (audioHandle_ && audioHandle_->sampleRate > 0)
             ? (double)s / audioHandle_->sampleRate
             : 0;
}

//==============================================================================
// Mouse handling
void SampleEditorComponent::mouseDown(const juce::MouseEvent &e) {
  grabKeyboardFocus();
  lastMouseX_ = e.position.x;

  float waveformTop = toolbarHeight_ + overviewHeight_ + rulerHeight_;
  float waveformHeight = getHeight() - waveformTop - scrollbarHeight_;

  if (isInToolbar(e.position.y)) {
    // Handle toolbar clicks
    float x = e.position.x;
    float btnSize = 28, spacing = 4, startX = 8;
    int btnIdx = (int)((x - startX) / (btnSize + spacing));

    if (btnIdx == 0)
      setTool(SampleEditorTool::Select);
    else if (btnIdx == 1)
      setTool(SampleEditorTool::Pencil);
    else if (btnIdx == 2)
      setTool(SampleEditorTool::Slice);
    else if (btnIdx == 3) {
      isPlaying_ ? stop() : play();
    } else if (btnIdx == 4)
      stop();
    else if (btnIdx == 5)
      toggleLoop();
  } else if (isInOverview(e.position.y)) {
    isDraggingOverview_ = true;
    if (audioHandle_) {
      double total = samplesToTime(audioHandle_->lengthInSamples);
      timeOffset_ = (e.position.x / getWidth()) * total - viewWidthSeconds_ / 2;
      timeOffset_ = std::max(0.0, timeOffset_);
      repaint();
    }
  } else if (isInRuler(e.position.y)) {
    // Click to set playhead
    playheadPosition_ = pixelsToTime(e.position.x, (float)getWidth());
    playheadPosition_ = std::max(0.0, playheadPosition_);
    isDraggingPlayhead_ = true;
    repaint();
  } else if (isInWaveform(e.position.y)) {
    double t = pixelsToTime(e.position.x, (float)getWidth());
    if (snapToZeroCrossing_)
      t = snapToNearestZeroCrossing(t);

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

void SampleEditorComponent::mouseDrag(const juce::MouseEvent &e) {
  if (isDraggingOverview_ && audioHandle_) {
    double total = samplesToTime(audioHandle_->lengthInSamples);
    timeOffset_ = (e.position.x / getWidth()) * total - viewWidthSeconds_ / 2;
    timeOffset_ =
        std::max(0.0, std::min(timeOffset_, total - viewWidthSeconds_));
    repaint();
  } else if (isDraggingPlayhead_) {
    playheadPosition_ = pixelsToTime(e.position.x, (float)getWidth());
    playheadPosition_ = std::max(0.0, playheadPosition_);
    repaint();
  } else if (isSelecting_) {
    double t = pixelsToTime(e.position.x, (float)getWidth());
    if (snapToZeroCrossing_)
      t = snapToNearestZeroCrossing(t);

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

void SampleEditorComponent::mouseUp(const juce::MouseEvent &) {
  isSelecting_ = false;
  isDraggingPlayhead_ = false;
  isDraggingOverview_ = false;
}

void SampleEditorComponent::mouseMove(const juce::MouseEvent &e) {
  // Update cursor based on region
  if (isInToolbar(e.position.y)) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
  } else if (isInOverview(e.position.y)) {
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
  } else if (isInRuler(e.position.y)) {
    setMouseCursor(juce::MouseCursor::IBeamCursor);
  } else if (isInWaveform(e.position.y)) {
    // Tool-specific cursor
    switch (currentTool_) {
    case SampleEditorTool::Select:
      setMouseCursor(juce::MouseCursor::NormalCursor);
      break;
    case SampleEditorTool::Pencil:
      setMouseCursor(juce::MouseCursor::CrosshairCursor);
      break;
    case SampleEditorTool::Slice:
      setMouseCursor(juce::MouseCursor::CrosshairCursor);
      break;
    case SampleEditorTool::Zoom:
      setMouseCursor(juce::MouseCursor::PointingHandCursor);
      break;
    case SampleEditorTool::Scrub:
      setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
      break;
    }
  } else if (isInScrollbar(e.position.y)) {
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
  }

  lastMouseX_ = e.position.x;
}

void SampleEditorComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  if (isInWaveform(e.position.y)) {
    double t = pixelsToTime(e.position.x, (float)getWidth());
    addMarker(t, "M" + juce::String(markers_.size() + 1));
    repaint();
  }
}

void SampleEditorComponent::mouseWheelMove(const juce::MouseEvent &e,
                                           const juce::MouseWheelDetails &w) {
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

bool SampleEditorComponent::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::spaceKey) {
    isPlaying_ ? stop() : play();
    return true;
  }
  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    deleteSelection();
    return true;
  }
  if (key.getModifiers().isCommandDown()) {
    if (key.getKeyCode() == 'X') {
      cutSelection();
      return true;
    }
    if (key.getKeyCode() == 'C') {
      copySelection();
      return true;
    }
    if (key.getKeyCode() == 'V') {
      paste();
      return true;
    }
    if (key.getKeyCode() == 'A') {
      selectAll();
      return true;
    }
    if (key.getKeyCode() == 'N') {
      normalize();
      return true;
    }
    if (key.getKeyCode() == 'R') {
      reverse();
      return true;
    }
  }
  if (key.getKeyCode() == 'L') {
    toggleLoop();
    return true;
  }
  return false;
}

//==============================================================================
// Zoom/Scroll
void SampleEditorComponent::zoomHorizontal(float factor, float centerX) {
  double centerT = pixelsToTime(centerX, (float)getWidth());
  viewWidthSeconds_ /= factor;
  viewWidthSeconds_ = juce::jlimit(0.01, 600.0, viewWidthSeconds_);
  timeOffset_ = centerT - (centerX / getWidth()) * viewWidthSeconds_;
  timeOffset_ = std::max(0.0, timeOffset_);
  repaint();
}

void SampleEditorComponent::zoomVertical(float factor) {
  verticalZoom_ *= factor;
  verticalZoom_ = juce::jlimit(0.1f, 10.0f, verticalZoom_);
  repaint();
}

void SampleEditorComponent::scrollHorizontal(float delta) {
  timeOffset_ += (delta / getWidth()) * viewWidthSeconds_;
  timeOffset_ = std::max(0.0, timeOffset_);
  repaint();
}

void SampleEditorComponent::fitToWindow() {
  if (audioHandle_) {
    timeOffset_ = 0;
    viewWidthSeconds_ = samplesToTime(audioHandle_->lengthInSamples) * 1.02;
    repaint();
  }
}

void SampleEditorComponent::zoomToSelection() {
  if (hasSelection()) {
    timeOffset_ = selection_.getStart() - 0.1;
    viewWidthSeconds_ = selection_.getLength() + 0.2;
    repaint();
  }
}

//==============================================================================
// Selection
void SampleEditorComponent::setSelection(double s, double e) {
  selection_ = {s, e};
  repaint();
}
void SampleEditorComponent::selectAll() {
  if (audioHandle_)
    selection_ = {0, samplesToTime(audioHandle_->lengthInSamples)};
  repaint();
}
void SampleEditorComponent::clearSelection() {
  selection_ = {};
  repaint();
}

//==============================================================================
// Playback
void SampleEditorComponent::play() {
  isPlaying_ = true;
  repaint();
}
void SampleEditorComponent::stop() {
  isPlaying_ = false;
  repaint();
}
void SampleEditorComponent::playSelection() {
  if (hasSelection()) {
    playheadPosition_ = selection_.getStart();
    play();
  }
}
void SampleEditorComponent::toggleLoop() {
  isLooping_ = !isLooping_;
  repaint();
}
void SampleEditorComponent::setPlayheadPosition(double t) {
  playheadPosition_ = t;
  repaint();
}

//==============================================================================
// Editing Operations
//==============================================================================
void SampleEditorComponent::cutSelection() {
  copySelection();
  deleteSelection();
}
void SampleEditorComponent::copySelection() {
  if (!hasSelection() || !audioHandle_)
    return;
  auto s = timeToSamples(selection_.getStart());
  auto e = timeToSamples(selection_.getEnd());
  int len = (int)(e - s);
  clipboard_ = std::make_unique<juce::AudioBuffer<float>>(
      audioHandle_->buffer.getNumChannels(), len);
  for (int ch = 0; ch < audioHandle_->buffer.getNumChannels(); ch++)
    clipboard_->copyFrom(ch, 0, audioHandle_->buffer, ch, (int)s, len);
  clipboardSampleRate_ = audioHandle_->sampleRate;
  DBG("Copied " + juce::String(len) + " samples");
}
void SampleEditorComponent::paste() {
  if (!clipboard_ || clipboard_->getNumSamples() == 0 || !audioHandle_)
    return;
  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);

  auto &buffer = *editBuffer_;
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();
  int clipboardSamples = clipboard_->getNumSamples();

  // Insert position: playhead or start of selection
  juce::int64 insertPos = hasSelection() ? timeToSamples(selection_.getStart())
                                         : timeToSamples(playheadPosition_);
  insertPos =
      std::max<juce::int64>(0, std::min<juce::int64>(numSamples, insertPos));

  // Create new buffer with space for clipboard content
  int newNumSamples = numSamples + clipboardSamples;
  juce::AudioBuffer<float> stateAfter(numChannels, newNumSamples);

  // Copy audio before insert point
  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, 0, buffer, ch, 0, (int)insertPos);
  }

  // Copy clipboard content
  int cbChannels = std::min(numChannels, clipboard_->getNumChannels());
  for (int ch = 0; ch < cbChannels; ch++) {
    stateAfter.copyFrom(ch, (int)insertPos, *clipboard_, ch, 0,
                        clipboardSamples);
  }

  // Copy audio after insert point
  int remaining = numSamples - (int)insertPos;
  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, (int)insertPos + clipboardSamples, buffer, ch,
                        (int)insertPos, remaining);
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Paste Audio"));

  DBG("Pasted " + juce::String(clipboardSamples) + " samples at position " +
      juce::String(insertPos));
}

void SampleEditorComponent::deleteSelection() {
  if (!hasSelection() || !audioHandle_)
    return;
  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);

  auto &buffer = *editBuffer_;
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());
  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(numSamples, endSample);
  int deleteLength = (int)(endSample - startSample);

  if (deleteLength <= 0)
    return;

  // Create new buffer without the deleted section
  int newNumSamples = numSamples - deleteLength;
  if (newNumSamples <= 0) {
    juce::AudioBuffer<float> empty(numChannels, 0);
    projectState_.getUndoManager().perform(
        new SampleEditAction(this, stateBefore, empty, "Delete Selection"));
    clearSelection();
    return;
  }

  juce::AudioBuffer<float> stateAfter(numChannels, newNumSamples);

  // Copy audio before selection
  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, 0, buffer, ch, 0, (int)startSample);
  }

  // Copy audio after selection
  int afterSelectionStart = (int)endSample;
  int afterSelectionLength = numSamples - afterSelectionStart;
  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, (int)startSample, buffer, ch, afterSelectionStart,
                        afterSelectionLength);
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Delete Selection"));

  clearSelection();
  DBG("Deleted " + juce::String(deleteLength) + " samples");
}

void SampleEditorComponent::trimToSelection() {
  if (!hasSelection() || !audioHandle_)
    return;
  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);

  auto &buffer = *editBuffer_;
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());
  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(numSamples, endSample);
  int trimLength = (int)(endSample - startSample);

  if (trimLength <= 0)
    return;

  // Create new buffer containing only the selection
  juce::AudioBuffer<float> stateAfter(numChannels, trimLength);

  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, 0, buffer, ch, (int)startSample, trimLength);
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Trim to Selection"));

  clearSelection();
  timeOffset_ = 0;
  DBG("Trimmed to " + juce::String(trimLength) + " samples");
}

void SampleEditorComponent::splitAtCursor() {
  if (!audioHandle_)
    return;

  // Add a marker at the playhead position for splitting
  double splitTime = playheadPosition_;
  if (snapToZeroCrossing_) {
    splitTime = snapToNearestZeroCrossing(splitTime);
  }

  // Add a split marker
  addMarker(splitTime, "Split");

  // Also create a region from start to split point
  if (splitTime > 0 &&
      splitTime < samplesToTime(audioHandle_->lengthInSamples)) {
    addRegion(0, splitTime, "Region 1");
    addRegion(splitTime, samplesToTime(audioHandle_->lengthInSamples),
              "Region 2");
  }

  DBG("Split at " + juce::String(splitTime) + " seconds");
  repaint();
}

//==============================================================================
// Processing - REAL IMPLEMENTATIONS
//==============================================================================

void SampleEditorComponent::normalize(float targetDb) {
  if (!audioHandle_ || !hasSelection())
    return;

  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);
  
  // Clone to work on stateAfter
  juce::AudioBuffer<float> stateAfter(*editBuffer_);

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());

  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(stateAfter.getNumSamples(), endSample);

  // Step 1: Find peak amplitude
  float peakLevel = 0.0f;
  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    const float *data = stateAfter.getReadPointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      float absVal = std::abs(data[i]);
      if (absVal > peakLevel)
        peakLevel = absVal;
    }
  }

  if (peakLevel < 0.0001f)
    return; // Silence or near-silence

  // Step 2: Calculate gain to reach target
  // targetDb = 0 means normalize to 1.0 (0dBFS)
  float targetLinear = std::pow(10.0f, targetDb / 20.0f);
  float gain = targetLinear / peakLevel;

  // Step 3: Apply gain
  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    float *data = stateAfter.getWritePointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      data[i] *= gain;
    }
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Normalize"));

  DBG("Normalized selection: peak=" + juce::String(peakLevel) +
      " gain=" + juce::String(gain));
}

void SampleEditorComponent::reverse() {
  if (!audioHandle_ || !hasSelection())
    return;

  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);
  
  // Clone to work on stateAfter
  juce::AudioBuffer<float> stateAfter(*editBuffer_);

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());

  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(stateAfter.getNumSamples(), endSample);

  // Reverse each channel independently
  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    float *data = stateAfter.getWritePointer(ch);
    juce::int64 left = startSample;
    juce::int64 right = endSample - 1;

    while (left < right) {
      std::swap(data[left], data[right]);
      left++;
      right--;
    }
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Reverse Audio"));

  DBG("Reversed selection");
}

void SampleEditorComponent::fadeIn(double durationSeconds) {
  if (!audioHandle_)
    return;

  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);
  juce::AudioBuffer<float> stateAfter(*editBuffer_);

  double startTime = hasSelection() ? selection_.getStart() : 0.0;
  juce::int64 startSample = timeToSamples(startTime);
  juce::int64 fadeSamples =
      (juce::int64)(durationSeconds * audioHandle_->sampleRate);
  juce::int64 endSample =
      std::min<juce::int64>(startSample + fadeSamples, stateAfter.getNumSamples());

  // Apply x-squared curve for natural fade (logarithmic perception)
  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    float *data = stateAfter.getWritePointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      float progress =
          (float)(i - startSample) / (float)(endSample - startSample);
      float gain = progress * progress; // x² curve
      data[i] *= gain;
    }
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Fade In"));

  DBG("Applied fade in: " + juce::String(durationSeconds) + "s");
}

void SampleEditorComponent::fadeOut(double durationSeconds) {
  if (!audioHandle_)
    return;

  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);
  juce::AudioBuffer<float> stateAfter(*editBuffer_);

  double endTime = hasSelection() ? selection_.getEnd()
                                  : samplesToTime(stateAfter.getNumSamples());
  juce::int64 endSample = timeToSamples(endTime);
  juce::int64 fadeSamples =
      (juce::int64)(durationSeconds * audioHandle_->sampleRate);
  juce::int64 startSample = std::max<juce::int64>(0, endSample - fadeSamples);

  // Apply inverse x-squared curve
  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    float *data = stateAfter.getWritePointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      float progress =
          (float)(i - startSample) / (float)(endSample - startSample);
      float gain = (1.0f - progress) * (1.0f - progress); // inverse x² curve
      data[i] *= gain;
    }
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Fade Out"));

  DBG("Applied fade out: " + juce::String(durationSeconds) + "s");
}

void SampleEditorComponent::adjustGain(float db) {
  if (!audioHandle_ || !hasSelection())
    return;

  float gain = std::pow(10.0f, db / 20.0f); // Convert dB to linear

  if (!editBuffer_)
    return;

  juce::AudioBuffer<float> stateBefore(*editBuffer_);
  juce::AudioBuffer<float> stateAfter(*editBuffer_);

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());

  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    float *data = stateAfter.getWritePointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      data[i] *= gain;
      // Soft clip to prevent harsh distortion
      if (data[i] > 1.0f)
        data[i] = 1.0f - 1.0f / (data[i] + 1.0f);
      else if (data[i] < -1.0f)
        data[i] = -1.0f - 1.0f / (data[i] - 1.0f);
    }
  }

  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Adjust Gain"));

  DBG("Adjusted gain: " + juce::String(db) + "dB");
}

void SampleEditorComponent::silenceSelection() {
  if (!audioHandle_ || !hasSelection())
    return;

  if (!editBuffer_)
    return;

  juce::AudioBuffer<float> stateBefore(*editBuffer_);
  juce::AudioBuffer<float> stateAfter(*editBuffer_);

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());

  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    float *data = stateAfter.getWritePointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      data[i] = 0.0f;
    }
  }

  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Silence Selection"));

  DBG("Silenced selection");
}

void SampleEditorComponent::removeOffset() {
  if (!audioHandle_ || !hasSelection())
    return;

  if (!editBuffer_)
    return;

  juce::AudioBuffer<float> stateBefore(*editBuffer_);
  juce::AudioBuffer<float> stateAfter(*editBuffer_);

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());

  // Calculate DC offset (average of all samples)
  for (int ch = 0; ch < stateAfter.getNumChannels(); ch++) {
    float *data = stateAfter.getWritePointer(ch);
    double sum = 0.0;
    for (juce::int64 i = startSample; i < endSample; i++) {
      sum += data[i];
    }
    float dcOffset = (float)(sum / (double)(endSample - startSample));

    // Subtract DC offset
    for (juce::int64 i = startSample; i < endSample; i++) {
      data[i] -= dcOffset;
    }

    DBG("Removed DC offset ch" + juce::String(ch) + ": " +
        juce::String(dcOffset));
  }

  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Remove DC Offset"));
}

//==============================================================================
// Advanced Processing
//==============================================================================

void SampleEditorComponent::timeStretch(float ratio) {
  if (!audioHandle_ || !hasSelection() || ratio <= 0.0f)
    return;
  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);

  auto &buffer = *editBuffer_;
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());
  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(numSamples, endSample);
  int selectionLength = (int)(endSample - startSample);

  if (selectionLength <= 0)
    return;

  // Extract selection
  juce::AudioBuffer<float> selectionBuffer(numChannels, selectionLength);
  for (int ch = 0; ch < numChannels; ++ch) {
      selectionBuffer.copyFrom(ch, 0, buffer, ch, (int)startSample, selectionLength);
  }

  // Process with Phase Vocoder (Pitch-Invariant)
  zenith::dsp::TimeStretcher stretcher;
  juce::AudioBuffer<float> stretchedSelection = stretcher.process(selectionBuffer, ratio);
  
  int newSelectionLength = stretchedSelection.getNumSamples();

  // Create new buffer with stretched selection
  int newTotalLength = numSamples - selectionLength + newSelectionLength;
  juce::AudioBuffer<float> stateAfter(numChannels, newTotalLength);

  // Copy before selection
  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, 0, buffer, ch, 0, (int)startSample);
  }

  // Copy stretched selection
  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, (int)startSample, stretchedSelection, ch, 0,
                        newSelectionLength);
  }

  // Copy after selection
  int afterLen = numSamples - (int)endSample;
  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, (int)startSample + newSelectionLength, buffer, ch,
                        (int)endSample, afterLen);
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Time Stretch"));

  DBG("Time stretch ratio: " + juce::String(ratio) + " (Phase Vocoder)");
}

void SampleEditorComponent::pitchShift(int semitones) {
  if (!audioHandle_ || !hasSelection() || semitones == 0)
    return;
  if (!editBuffer_)
    return;

  // 1. Snapshot State Before
  juce::AudioBuffer<float> stateBefore(*editBuffer_);

  // Calculate the ratio: down = slower playback = higher pitch when played at
  // normal speed Each semitone is 2^(1/12) ratio
  float ratio = std::pow(2.0f, -semitones / 12.0f);

  auto &buffer = *editBuffer_;
  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());
  startSample = std::max<juce::int64>(0, startSample);
  endSample = std::min<juce::int64>(numSamples, endSample);
  int selectionLength = (int)(endSample - startSample);

  if (selectionLength <= 0)
    return;

  // Resample the selection
  int newSelectionLength = (int)(selectionLength * ratio);
  if (newSelectionLength <= 0)
    return;

  juce::AudioBuffer<float> resampledSelection(numChannels, newSelectionLength);

  for (int ch = 0; ch < numChannels; ch++) {
    const float *srcData = buffer.getReadPointer(ch);
    float *dstData = resampledSelection.getWritePointer(ch);

    for (int i = 0; i < newSelectionLength; i++) {
      float srcPos = (float)i / ratio + (float)startSample;
      int srcIdx = (int)srcPos;
      float frac = srcPos - srcIdx;

      if (srcIdx >= 0 && srcIdx < numSamples - 1) {
        dstData[i] =
            srcData[srcIdx] * (1.0f - frac) + srcData[srcIdx + 1] * frac;
      } else if (srcIdx >= 0 && srcIdx < numSamples) {
        dstData[i] = srcData[srcIdx];
      } else {
        dstData[i] = 0.0f;
      }
    }
  }

  // Create new buffer with pitch-shifted selection
  int newTotalLength = numSamples - selectionLength + newSelectionLength;
  juce::AudioBuffer<float> stateAfter(numChannels, newTotalLength);

  for (int ch = 0; ch < numChannels; ch++) {
    stateAfter.copyFrom(ch, 0, buffer, ch, 0, (int)startSample);
    stateAfter.copyFrom(ch, (int)startSample, resampledSelection, ch, 0,
                        newSelectionLength);
    stateAfter.copyFrom(ch, (int)startSample + newSelectionLength, buffer, ch,
                        (int)endSample, numSamples - (int)endSample);
  }

  // 2. Perform Action via Global UndoManager
  projectState_.getUndoManager().perform(
      new SampleEditAction(this, stateBefore, stateAfter, "Pitch Shift"));

  DBG("Pitch shift: " + juce::String(semitones) +
      " semitones (resampling method - changes tempo)");
}

void SampleEditorComponent::detectTransients(float sensitivity) {
  if (!audioHandle_)
    return;

  const juce::AudioBuffer<float> *bufferPtr = &audioHandle_->buffer;
  if (editBuffer_)
    bufferPtr = editBuffer_.get();
  auto &buffer = *bufferPtr;
  int numSamples = buffer.getNumSamples();
  const float *data = buffer.getReadPointer(0);

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

void SampleEditorComponent::autoSlice(float sensitivity) {
  // First detect transients
  detectTransients(sensitivity);

  // Convert markers to regions
  clearRegions();

  if (markers_.empty())
    return;

  for (size_t i = 0; i < markers_.size(); i++) {
    double start = markers_[i].timeSeconds;
    double end = (i < markers_.size() - 1)
                     ? markers_[i + 1].timeSeconds
                     : samplesToTime(audioHandle_->lengthInSamples);
    addRegion(start, end, "Slice " + juce::String(i + 1));
  }

  DBG("Created " + juce::String(regions_.size()) + " slices");
}

void SampleEditorComponent::sliceToMidi() {
    if (regions_.empty() || !audioHandle_) {
        DBG("[SampleEditor] No slices to export - run autoSlice first");
        return;
    }
    
    // Find the track that owns this clip
    auto trackNode = projectState_.getTrack(currentTrackId_);
    if (!trackNode.isValid()) {
        DBG("[SampleEditor] Invalid track for MIDI export");
        return;
    }
    
    // Get audio data for energy calculation
    const juce::AudioBuffer<float>* bufferPtr = &audioHandle_->buffer;
    if (editBuffer_)
        bufferPtr = editBuffer_.get();
    
    // Create a new MIDI clip in the same track
    juce::String midiClipId = juce::Uuid().toString();
    
    // Build MIDI notes from slices
    // Map slices to MIDI notes starting at C1 (MIDI note 36)
    int baseNote = 36;
    
    DBG("[SampleEditor] Creating MIDI clip from " + juce::String(regions_.size()) + " slices");
    
    // Calculate total duration
    double totalDuration = samplesToTime(audioHandle_->lengthInSamples);
    
    // Create MIDI clip in project state
    auto clipsNode = trackNode.getChildWithName("CLIPS");
    if (!clipsNode.isValid()) {
        clipsNode = juce::ValueTree("CLIPS");
        trackNode.addChild(clipsNode, -1, nullptr);
    }
    
    // Create the MIDI clip
    juce::ValueTree midiClip("CLIP");
    midiClip.setProperty("id", midiClipId, nullptr);
    midiClip.setProperty("name", "Sliced MIDI", nullptr);
    midiClip.setProperty("type", "midi", nullptr);
    midiClip.setProperty("position", 0.0, nullptr);
    midiClip.setProperty("length", totalDuration, nullptr);
    
    // Add MIDI notes container
    juce::ValueTree notesNode("NOTES");
    
    int noteNumber = baseNote;
    for (size_t i = 0; i < regions_.size() && noteNumber < 128; ++i) {
        const auto& region = regions_[i];
        double startTime = region.startTime;
        double duration = region.endTime - region.startTime;
        
        // Calculate velocity from slice energy (RMS)
        juce::int64 startSample = timeToSamples(startTime);
        juce::int64 endSample = timeToSamples(region.endTime);
        float rms = 0.0f;
        int count = 0;
        
        for (int ch = 0; ch < bufferPtr->getNumChannels(); ++ch) {
            const float* data = bufferPtr->getReadPointer(ch);
            for (juce::int64 s = startSample; s < endSample && s < bufferPtr->getNumSamples(); ++s) {
                rms += data[s] * data[s];
                count++;
            }
        }
        
        if (count > 0) {
            rms = std::sqrt(rms / count);
        }
        
        // Map RMS (0-0.5 typical) to velocity (40-127)
        int velocity = static_cast<int>(juce::jlimit(40.0f, 127.0f, 40.0f + rms * 200.0f));
        
        // Create MIDI note
        juce::ValueTree noteNode("NOTE");
        noteNode.setProperty("id", juce::Uuid().toString(), nullptr);
        noteNode.setProperty("note", noteNumber, nullptr);
        noteNode.setProperty("start", startTime, nullptr);
        noteNode.setProperty("length", duration, nullptr);
        noteNode.setProperty("velocity", velocity, nullptr);
        
        notesNode.addChild(noteNode, -1, nullptr);
        
        DBG("[SampleEditor] Slice " + juce::String(i) + 
            " -> MIDI note " + juce::String(noteNumber) + 
            " vel=" + juce::String(velocity));
        
        noteNumber++;
    }
    
    midiClip.addChild(notesNode, -1, nullptr);
    clipsNode.addChild(midiClip, -1, nullptr);
    
    DBG("[SampleEditor] Created MIDI clip '" + midiClipId + 
        "' with " + juce::String(regions_.size()) + " notes");
}

//==============================================================================
// Analysis
//==============================================================================

void SampleEditorComponent::generateWaveformCache() {
  // Pre-compute min/max peaks at multiple resolutions for fast rendering
  DBG("Generating waveform cache...");
}

float SampleEditorComponent::getRMSLevel() const {
  if (!audioHandle_ || !hasSelection())
    return 0.0f;

  const juce::AudioBuffer<float> *bufferPtr = &audioHandle_->buffer;
  if (editBuffer_)
    bufferPtr = editBuffer_.get();
  auto &buffer = *bufferPtr;
  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());

  double sum = 0.0;
  int count = 0;

  for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
    const float *data = buffer.getReadPointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      sum += data[i] * data[i];
      count++;
    }
  }

  return count > 0 ? (float)std::sqrt(sum / count) : 0.0f;
}

float SampleEditorComponent::getPeakLevel() const {
  if (!audioHandle_ || !hasSelection())
    return 0.0f;

  const juce::AudioBuffer<float> *bufferPtr = &audioHandle_->buffer;
  if (editBuffer_)
    bufferPtr = editBuffer_.get();
  auto &buffer = *bufferPtr;
  juce::int64 startSample = timeToSamples(selection_.getStart());
  juce::int64 endSample = timeToSamples(selection_.getEnd());

  float peak = 0.0f;

  for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
    const float *data = buffer.getReadPointer(ch);
    for (juce::int64 i = startSample; i < endSample; i++) {
      float absVal = std::abs(data[i]);
      if (absVal > peak)
        peak = absVal;
    }
  }

  return peak;
}

//==============================================================================
// Markers
void SampleEditorComponent::addMarker(double t, const juce::String &name) {
  markers_.push_back({juce::Uuid().toString(), name, t, Colors::marker});
  repaint();
}
void SampleEditorComponent::removeMarker(const juce::String &id) {
  markers_.erase(std::remove_if(markers_.begin(), markers_.end(),
                                [&](auto &m) { return m.id == id; }),
                 markers_.end());
  repaint();
}
void SampleEditorComponent::clearMarkers() {
  markers_.clear();
  repaint();
}

void SampleEditorComponent::addRegion(double s, double e,
                                      const juce::String &name) {
  regions_.push_back(
      {juce::Uuid().toString(), name, s, e, SkColorSetRGB(100, 200, 100)});
  repaint();
}
void SampleEditorComponent::removeRegion(const juce::String &id) {
  regions_.erase(std::remove_if(regions_.begin(), regions_.end(),
                                [&](auto &r) { return r.id == id; }),
                 regions_.end());
  repaint();
}
void SampleEditorComponent::clearRegions() {
  regions_.clear();
  repaint();
}

double SampleEditorComponent::snapToNearestZeroCrossing(double t) const {
  if (!audioHandle_)
    return t;
  juce::int64 sampleIdx = timeToSamples(t);
  const float *data = audioHandle_->buffer.getReadPointer(0);
  int numSamples = audioHandle_->buffer.getNumSamples();

  for (int offset = 0; offset < 100; offset++) {
    juce::int64 left = sampleIdx - offset;
    juce::int64 right = sampleIdx + offset;

    if (left > 0 && left < numSamples - 1) {
      if ((data[left] >= 0 && data[left + 1] < 0) ||
          (data[left] < 0 && data[left + 1] >= 0))
        return samplesToTime(left);
    }
    if (right > 0 && right < numSamples - 1) {
      if ((data[right] >= 0 && data[right + 1] < 0) ||
          (data[right] < 0 && data[right + 1] >= 0))
        return samplesToTime(right);
    }
  }
  return t;
}

void SampleEditorComponent::resized() {
  // Regenerate waveform cache when size changes for smooth rendering
  if (audioHandle_ && getWidth() > 0) {
    generateWaveformCache();
  }
}

void SampleEditorComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &prop) {
  if (tree == clipNode_ && prop.toString() == "sourceFile")
    setClipToEdit(currentTrackId_, currentClipId_);
}

void SampleEditorComponent::setEditBuffer(const juce::AudioBuffer<float>& newBuffer) {
    editBuffer_ = std::make_unique<juce::AudioBuffer<float>>(newBuffer);
    hasUnsavedChanges_ = true;
    repaint();
}

//==============================================================================
// Additional Implementations (UI Helpers and Feature Functions)
//==============================================================================

void SampleEditorComponent::drawToolbarButton(SkCanvas *canvas,
                                              const SkRect &bounds,
                                              const char *icon,
                                              const char *tooltip,
                                              bool isActive, bool isEnabled) {
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);

  if (isActive) {
    bgPaint.setColor(Colors::buttonActive);
  } else if (!isEnabled) {
    bgPaint.setColor(SkColorSetA(Colors::buttonHover, 50));
  } else {
    bgPaint.setColor(Colors::buttonHover);
  }

  canvas->drawRoundRect(bounds, 4.0f, 4.0f, bgPaint);

  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(isEnabled ? SK_ColorWHITE
                               : SkColorSetA(SK_ColorWHITE, 100));

  SkFont font = design::getSkFont(16.0f, design::FontWeight::Regular);

  float textWidth = font.measureText(icon, strlen(icon), SkTextEncoding::kUTF8);
  float x = bounds.centerX() - textWidth / 2.0f;
  float y = bounds.centerY() + 6.0f;

  canvas->drawString(icon, x, y, font, textPaint);
}

// Recording
// Recording
void SampleEditorComponent::startRecording() {
  if (isRecording_) return;

  auto* device = engine_.getDeviceManager().getCurrentAudioDevice();
  if (device) {
      audioDeviceAboutToStart(device);
      engine_.getDeviceManager().addAudioCallback(this);
  }

  // Initialize record buffer (start with 1 minute approx)
  double sampleRate = device ? device->getCurrentSampleRate() : 44100.0;
  int initialSamples = (int)(sampleRate * 60.0);
  
  int numChans = device ? device->getActiveInputChannels().countNumberOfSetBits() : 2;
  if (numChans == 0) numChans = 2;

  recordBuffer_ = std::make_unique<juce::AudioBuffer<float>>(numChans, initialSamples);
  recordBuffer_->clear();
  recordWritePos_ = 0;

  // Ensure FIFO is initialized even without a device (for testing/manual callback injection)
  if (!incomingFifo_) {
      int ringBufferSize = (int)(sampleRate * 5.0);
      incomingBuffer_.setSize(numChans, ringBufferSize);
      incomingFifo_ = std::make_unique<juce::AbstractFifo>(ringBufferSize);
  }

  isRecording_ = true;
  if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimer(50); // Start timer to drain FIFO
  repaint();
}

void SampleEditorComponent::stopRecording() {
  if (!isRecording_) return;

  engine_.getDeviceManager().removeAudioCallback(this);
  isRecording_ = false;
  
  // Drain any remaining samples from FIFO
  if (incomingFifo_ && recordBuffer_) {
      int numReady = incomingFifo_->getNumReady();
      if (numReady > 0) {
          int start1, size1, start2, size2;
          incomingFifo_->prepareToRead(numReady, start1, size1, start2, size2);

          // Append to recordBuffer_
          int currentCapacity = recordBuffer_->getNumSamples();
          int requiredCapacity = recordWritePos_ + size1 + size2;
          
          if (currentCapacity < requiredCapacity) {
            int newCapacity = std::max(requiredCapacity, currentCapacity * 2);
            newCapacity = std::max(newCapacity, 4096); 
            recordBuffer_->setSize(recordBuffer_->getNumChannels(), newCapacity, true, true, true);
          }
          
          if (size1 > 0)
            recordBuffer_->copyFrom(0, recordWritePos_, incomingBuffer_, 0, start1, size1);
          if (size2 > 0)
            recordBuffer_->copyFrom(0, recordWritePos_ + size1, incomingBuffer_, 0, start2, size2);

          incomingFifo_->finishedRead(size1 + size2);
          recordWritePos_ += (size1 + size2);
      }
  }
  
  // Trim and finalize
  if (recordBuffer_ && recordWritePos_ > 0) {
      recordBuffer_->setSize(recordBuffer_->getNumChannels(), recordWritePos_, true, true, true);
      
      // Move to edit buffer
      editBuffer_ = std::move(recordBuffer_);
      hasUnsavedChanges_ = true;
      
      // Update UI
      if (editBuffer_) {
          selection_ = juce::Range<double>(0.0, samplesToTime(editBuffer_->getNumSamples()));
          if (audioHandle_) {
               // Update zoom based on new length? 
               // For now just fit
          }
          fitToWindow();
          generateWaveformCache();
      }
  }

  repaint();
}

// Save/Export
void SampleEditorComponent::saveToFile() {
  if (!audioHandle_ || !audioHandle_->sourceFile.exists())
    return;
  saveAsNewFile(audioHandle_->sourceFile);
}

void SampleEditorComponent::saveAsNewFile(const juce::File &targetFile) {
  if ((!editBuffer_ && !audioHandle_) || targetFile == juce::File())
    return;

  const juce::AudioBuffer<float> *bufferToSave =
      editBuffer_ ? editBuffer_.get() : &audioHandle_->buffer;
  double sampleRate = audioHandle_ ? audioHandle_->sampleRate : 44100.0;

  targetFile.deleteFile();
  juce::WavAudioFormat format;
  
  std::unique_ptr<juce::FileOutputStream> stream(new juce::FileOutputStream(targetFile));
  if (!stream->openedOk()) {
    DBG("[SampleEditor] Failed to open file for writing: " + targetFile.getFullPathName());
    return;
  }
  
  std::unique_ptr<juce::AudioFormatWriter> writer(
      format.createWriterFor(stream.release(), sampleRate, (int)bufferToSave->getNumChannels(), 24, {}, 0));

  if (writer) {
    writer->writeFromAudioSampleBuffer(*bufferToSave, 0,
                                       bufferToSave->getNumSamples());
    if (audioHandle_ && targetFile == audioHandle_->sourceFile)
      hasUnsavedChanges_ = false;
  } else {
    DBG("[SampleEditor] Failed to create audio format writer");
  }
}

void SampleEditorComponent::exportSelection(const juce::File &targetFile) {
  if (!hasSelection() || targetFile == juce::File())
    return;

  const juce::AudioBuffer<float> *bufferToSave =
      editBuffer_ ? editBuffer_.get() : &audioHandle_->buffer;

  double sampleRate = audioHandle_ ? audioHandle_->sampleRate : 44100.0;
  int startSample = (int)timeToSamples(selection_.getStart());
  int endSample = (int)timeToSamples(selection_.getEnd());
  int numSamples = endSample - startSample;

  if (numSamples <= 0)
    return;

  targetFile.deleteFile();
  juce::WavAudioFormat format;
  
  std::unique_ptr<juce::OutputStream> fileStream(new juce::FileOutputStream(targetFile));
  std::unique_ptr<juce::AudioFormatWriter> writer(
      format.createWriterFor(fileStream.release(), sampleRate, (int)bufferToSave->getNumChannels(), 24, {}, 0));

  if (writer) {
    writer->writeFromAudioSampleBuffer(*bufferToSave, startSample, numSamples);
  }
}

// Warp Markers - Advanced time-stretching with beat preservation
void SampleEditorComponent::addWarpMarker(double originalTime,
                                          double warpedTime) {
    WarpMarker marker;
    marker.originalTime = originalTime;
    marker.warpedTime = warpedTime;
    
    // Insert in sorted order by original time
    auto it = std::lower_bound(warpMarkers_.begin(), warpMarkers_.end(), marker,
        [](const WarpMarker& a, const WarpMarker& b) {
            return a.originalTime < b.originalTime;
        });
    
    warpMarkers_.insert(it, marker);
    
    DBG("[SampleEditor] Added warp marker: original=" + juce::String(originalTime) + 
        "s, warped=" + juce::String(warpedTime) + "s");
    repaint();
}

void SampleEditorComponent::removeWarpMarker(int index) {
    if (index >= 0 && index < static_cast<int>(warpMarkers_.size())) {
        warpMarkers_.erase(warpMarkers_.begin() + index);
        DBG("[SampleEditor] Removed warp marker at index " + juce::String(index));
        repaint();
    }
}

void SampleEditorComponent::clearWarpMarkers() {
    warpMarkers_.clear();
    DBG("[SampleEditor] Cleared all warp markers");
    repaint();
}

void SampleEditorComponent::quantizeToGrid(double gridSize) {
    if (!audioHandle_ || markers_.empty())
        return;
    
    // Convert transient markers to warp markers quantized to grid
    clearWarpMarkers();
    
    for (const auto& marker : markers_) {
        double originalTime = marker.timeSeconds;
        // Quantize to nearest grid position
        double quantizedTime = std::round(originalTime / gridSize) * gridSize;
        
        if (std::abs(quantizedTime - originalTime) > 0.001) {
            addWarpMarker(originalTime, quantizedTime);
        }
    }
    
    DBG("[SampleEditor] Quantized " + juce::String(warpMarkers_.size()) + 
        " transients to grid (size=" + juce::String(gridSize) + "s)");
}

// Pencil Tool - Direct waveform drawing
void SampleEditorComponent::enablePencilTool(bool enable) {
    pencilToolEnabled_ = enable;
    if (enable) {
        setTool(SampleEditorTool::Pencil);
    }
}

void SampleEditorComponent::pencilDraw(float x, float y) {
    if (!pencilToolEnabled_ || !editBuffer_ || !audioHandle_)
        return;
    
    float waveformTop = toolbarHeight_ + overviewHeight_ + rulerHeight_;
    float waveformHeight = getHeight() - waveformTop - scrollbarHeight_;
    
    // Convert x position to sample index
    double time = pixelsToTime(x, static_cast<float>(getWidth()));
    juce::int64 sampleIdx = timeToSamples(time);
    
    if (sampleIdx < 0 || sampleIdx >= editBuffer_->getNumSamples())
        return;
    
    // Convert y position to amplitude (-1 to 1)
    float relativeY = (y - waveformTop) / waveformHeight;
    float numChannels = static_cast<float>(editBuffer_->getNumChannels());
    int channel = std::min(static_cast<int>(relativeY * numChannels), 
                           editBuffer_->getNumChannels() - 1);
    
    float channelHeight = waveformHeight / numChannels;
    float channelTop = waveformTop + channel * channelHeight;
    float channelCenter = channelTop + channelHeight / 2.0f;
    
    // Calculate amplitude from y position
    float amplitude = (channelCenter - y) / (channelHeight / 2.0f * verticalZoom_);
    amplitude = juce::jlimit(-1.0f, 1.0f, amplitude);
    
    // Write sample
    editBuffer_->setSample(channel, static_cast<int>(sampleIdx), amplitude);
    hasUnsavedChanges_ = true;
    
    repaint();
}

void SampleEditorComponent::smoothSelection(int windowSize) {
    if (!hasSelection() || !editBuffer_)
        return;
    
    pushUndoState("Smooth Selection");
    
    windowSize = std::max(3, windowSize | 1); // Ensure odd and at least 3
    int halfWindow = windowSize / 2;
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    startSample = std::max<juce::int64>(halfWindow, startSample);
    endSample = std::min<juce::int64>(editBuffer_->getNumSamples() - halfWindow, endSample);
    
    // Apply moving average filter to each channel
    for (int ch = 0; ch < editBuffer_->getNumChannels(); ++ch) {
        const float* src = editBuffer_->getReadPointer(ch);
        
        // Create temporary buffer for smoothed values
        std::vector<float> smoothed(static_cast<size_t>(endSample - startSample));
        
        for (juce::int64 i = startSample; i < endSample; ++i) {
            float sum = 0.0f;
            for (int j = -halfWindow; j <= halfWindow; ++j) {
                sum += src[i + j];
            }
            smoothed[static_cast<size_t>(i - startSample)] = sum / static_cast<float>(windowSize);
        }
        
        // Copy back
        float* dst = editBuffer_->getWritePointer(ch);
        for (juce::int64 i = startSample; i < endSample; ++i) {
            dst[i] = smoothed[static_cast<size_t>(i - startSample)];
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Smoothed selection with window size " + juce::String(windowSize));
    repaint();
}

// Envelopes - Automation within sample editor
void SampleEditorComponent::addVolumeEnvelopePoint(double time, float volume) {
    DBG("[SampleEditor] Volume envelope: Feature planned for v1.1");
}
void SampleEditorComponent::addPanEnvelopePoint(double time, float pan) {
    DBG("[SampleEditor] Pan envelope: Feature planned for v1.1");
}
void SampleEditorComponent::applyVolumeEnvelope() {
    DBG("[SampleEditor] Apply volume envelope: Feature planned for v1.1");
}
void SampleEditorComponent::applyPanEnvelope() {
    DBG("[SampleEditor] Apply pan envelope: Feature planned for v1.1");
}
void SampleEditorComponent::clearEnvelopes() {
    DBG("[SampleEditor] Clear envelopes: Feature planned for v1.1");
}

// Noise Reduction - Spectral processing
bool SampleEditorComponent::hasNoiseProfile() const {
    return spectralProcessor_ && spectralProcessor_->hasNoiseProfile();
}

void SampleEditorComponent::captureNoiseProfile() {
    if (!hasSelection() || !audioHandle_) {
        DBG("[SampleEditor] No selection for noise profile capture");
        return;
    }
    
    if (!editBuffer_)
        return;
    
    // Create spectral processor if needed
    if (!spectralProcessor_) {
        spectralProcessor_ = std::make_unique<dsp::SpectralProcessor>();
    }
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    spectralProcessor_->captureNoiseProfile(*editBuffer_, 
                                           static_cast<int>(startSample),
                                           static_cast<int>(endSample),
                                           audioHandle_->sampleRate);
    
    DBG("[SampleEditor] Noise profile captured from selection");
    repaint();
}

void SampleEditorComponent::applyNoiseReduction(float strength) {
    if (!spectralProcessor_ || !spectralProcessor_->hasNoiseProfile()) {
        DBG("[SampleEditor] No noise profile captured - select noise region first");
        return;
    }
    
    if (!hasSelection() || !editBuffer_)
        return;
    
    pushUndoState("Noise Reduction");
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    spectralProcessor_->applyNoiseReduction(*editBuffer_,
                                           static_cast<int>(startSample),
                                           static_cast<int>(endSample),
                                           strength);
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied noise reduction (strength=" + juce::String(strength) + ")");
    repaint();
}

// EQ & Filters - In-editor processing using SpectralProcessor
void SampleEditorComponent::applyEQ(
    const std::vector<std::pair<float, float>> &bands) {
    if (!hasSelection() || !editBuffer_ || !audioHandle_)
        return;
    
    if (!spectralProcessor_) {
        spectralProcessor_ = std::make_unique<dsp::SpectralProcessor>();
    }
    
    pushUndoState("Apply EQ");
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    // Apply each band as frequency gain adjustment
    for (const auto& band : bands) {
        float frequency = band.first;
        float gainDb = band.second;
        
        // Apply a narrow band around the frequency (1/3 octave)
        float lowHz = frequency / 1.26f;  // ~1/3 octave below
        float highHz = frequency * 1.26f; // ~1/3 octave above
        
        spectralProcessor_->applyFrequencyGain(*editBuffer_,
                                              static_cast<int>(startSample),
                                              static_cast<int>(endSample),
                                              lowHz, highHz, gainDb,
                                              audioHandle_->sampleRate);
    }
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied EQ with " + juce::String((int)bands.size()) + " bands");
    repaint();
}

void SampleEditorComponent::applyHighPassFilter(float cutoffHz) {
    if (!hasSelection() || !editBuffer_ || !audioHandle_)
        return;
    
    if (!spectralProcessor_) {
        spectralProcessor_ = std::make_unique<dsp::SpectralProcessor>();
    }
    
    pushUndoState("High-Pass Filter");
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    spectralProcessor_->applyHighPassFilter(*editBuffer_,
                                           static_cast<int>(startSample),
                                           static_cast<int>(endSample),
                                           cutoffHz,
                                           audioHandle_->sampleRate);
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied high-pass filter at " + juce::String(cutoffHz) + "Hz");
    repaint();
}

void SampleEditorComponent::applyLowPassFilter(float cutoffHz) {
    if (!hasSelection() || !editBuffer_ || !audioHandle_)
        return;
    
    if (!spectralProcessor_) {
        spectralProcessor_ = std::make_unique<dsp::SpectralProcessor>();
    }
    
    pushUndoState("Low-Pass Filter");
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    spectralProcessor_->applyLowPassFilter(*editBuffer_,
                                          static_cast<int>(startSample),
                                          static_cast<int>(endSample),
                                          cutoffHz,
                                          audioHandle_->sampleRate);
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied low-pass filter at " + juce::String(cutoffHz) + "Hz");
    repaint();
}

void SampleEditorComponent::applyBandPassFilter(float lowHz, float highHz) {
    if (!hasSelection() || !editBuffer_ || !audioHandle_)
        return;
    
    if (!spectralProcessor_) {
        spectralProcessor_ = std::make_unique<dsp::SpectralProcessor>();
    }
    
    pushUndoState("Band-Pass Filter");
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    spectralProcessor_->applyBandPassFilter(*editBuffer_,
                                           static_cast<int>(startSample),
                                           static_cast<int>(endSample),
                                           lowHz, highHz,
                                           audioHandle_->sampleRate);
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied band-pass filter " + juce::String(lowHz) + "-" + juce::String(highHz) + "Hz");
    repaint();
}

// Effects - Destructive processing
void SampleEditorComponent::applyConvolutionReverb(
    const juce::File &impulseResponse) {
    if (!impulseResponse.existsAsFile()) {
        DBG("[SampleEditor] IR file not found: " + impulseResponse.getFullPathName());
        return;
    }
    
    if (!hasSelection() || !editBuffer_ || !audioHandle_)
        return;
    
    pushUndoState("Convolution Reverb");
    
    // Load impulse response
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> irReader(
        formatManager.createReaderFor(impulseResponse));
    
    if (!irReader) {
        DBG("[SampleEditor] Could not read IR file");
        return;
    }
    
    juce::AudioBuffer<float> irBuffer(static_cast<int>(irReader->numChannels),
                                      static_cast<int>(irReader->lengthInSamples));
    irReader->read(&irBuffer, 0, static_cast<int>(irReader->lengthInSamples), 0, true, true);
    
    // Use JUCE Convolution processor
    juce::dsp::Convolution convolution;
    convolution.loadImpulseResponse(std::move(irBuffer),
                                   irReader->sampleRate,
                                   juce::dsp::Convolution::Stereo::yes,
                                   juce::dsp::Convolution::Trim::yes,
                                   juce::dsp::Convolution::Normalise::yes);
    
    // Process the selection
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = audioHandle_->sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = static_cast<uint32_t>(editBuffer_->getNumChannels());
    convolution.prepare(spec);
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    int numSamples = static_cast<int>(endSample - startSample);
    
    // Create temp buffer for processing
    juce::AudioBuffer<float> processBuffer(editBuffer_->getNumChannels(), numSamples);
    for (int ch = 0; ch < editBuffer_->getNumChannels(); ++ch) {
        processBuffer.copyFrom(ch, 0, *editBuffer_, ch, static_cast<int>(startSample), numSamples);
    }
    
    // Process in blocks
    for (int pos = 0; pos < numSamples; pos += 512) {
        int blockSize = std::min(512, numSamples - pos);
        juce::dsp::AudioBlock<float> block(processBuffer.getArrayOfWritePointers(),
                                          static_cast<size_t>(processBuffer.getNumChannels()),
                                          static_cast<size_t>(pos),
                                          static_cast<size_t>(blockSize));
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolution.process(context);
    }
    
    // Copy back
    for (int ch = 0; ch < editBuffer_->getNumChannels(); ++ch) {
        editBuffer_->copyFrom(ch, static_cast<int>(startSample), processBuffer, ch, 0, numSamples);
    }
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied convolution reverb with " + impulseResponse.getFileName());
    repaint();
}

void SampleEditorComponent::applySimpleReverb(float roomSize, float damping,
                                              float wetLevel) {
    if (!hasSelection() || !editBuffer_ || !audioHandle_)
        return;
    
    pushUndoState("Simple Reverb");
    
    // Use JUCE Reverb
    juce::Reverb reverb;
    juce::Reverb::Parameters params;
    params.roomSize = roomSize;
    params.damping = damping;
    params.wetLevel = wetLevel;
    params.dryLevel = 1.0f - wetLevel;
    params.width = 1.0f;
    reverb.setParameters(params);
    reverb.setSampleRate(audioHandle_->sampleRate);
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    int numSamples = static_cast<int>(endSample - startSample);
    
    // Process blocks
    int blockSize = 512;
    for (int pos = 0; pos < numSamples; pos += blockSize) {
        int remaining = std::min(blockSize, numSamples - pos);
        int bufPos = static_cast<int>(startSample) + pos;
        
        if (editBuffer_->getNumChannels() >= 2) {
            reverb.processStereo(editBuffer_->getWritePointer(0, bufPos),
                                editBuffer_->getWritePointer(1, bufPos),
                                remaining);
        } else {
            reverb.processMono(editBuffer_->getWritePointer(0, bufPos), remaining);
        }
    }
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied simple reverb (room=" + juce::String(roomSize) + 
        ", damp=" + juce::String(damping) + ", wet=" + juce::String(wetLevel) + ")");
    repaint();
}

void SampleEditorComponent::applyBlur(float amount) {
    if (!hasSelection() || !editBuffer_ || !audioHandle_)
        return;
    
    if (!spectralProcessor_) {
        spectralProcessor_ = std::make_unique<dsp::SpectralProcessor>();
    }
    
    pushUndoState("Spectral Blur");
    
    juce::int64 startSample = timeToSamples(selection_.getStart());
    juce::int64 endSample = timeToSamples(selection_.getEnd());
    
    spectralProcessor_->applySpectralBlur(*editBuffer_,
                                         static_cast<int>(startSample),
                                         static_cast<int>(endSample),
                                         amount);
    
    hasUnsavedChanges_ = true;
    DBG("[SampleEditor] Applied spectral blur (amount=" + juce::String(amount) + ")");
    repaint();
}

// Stereo Tools
void SampleEditorComponent::convertToMono() {
  if (!editBuffer_ && !audioHandle_)
    return;

  pushUndoState("Convert to Mono");

  const juce::AudioBuffer<float> *src =
      editBuffer_ ? editBuffer_.get() : &audioHandle_->buffer;
  if (src->getNumChannels() == 1)
    return;

  auto newBuffer =
      std::make_unique<juce::AudioBuffer<float>>(1, src->getNumSamples());

  const float *L = src->getReadPointer(0);
  const float *R = src->getReadPointer(1);
  float *D = newBuffer->getWritePointer(0);

  for (int i = 0; i < src->getNumSamples(); ++i) {
    D[i] = (L[i] + R[i]) * 0.5f;
  }

  editBuffer_ = std::move(newBuffer);
  hasUnsavedChanges_ = true;
  repaint();
}
void SampleEditorComponent::convertToStereo() {
    if (!editBuffer_ && !audioHandle_)
        return;
    
    const juce::AudioBuffer<float> *src =
        editBuffer_ ? editBuffer_.get() : &audioHandle_->buffer;
    if (src->getNumChannels() >= 2) {
        DBG("[SampleEditor] Already stereo");
        return;
    }
    
    pushUndoState("Convert to Stereo");
    
    auto newBuffer = std::make_unique<juce::AudioBuffer<float>>(2, src->getNumSamples());
    const float *mono = src->getReadPointer(0);
    
    // Duplicate mono to both channels
    for (int i = 0; i < src->getNumSamples(); ++i) {
        newBuffer->setSample(0, i, mono[i]);
        newBuffer->setSample(1, i, mono[i]);
    }
    
    editBuffer_ = std::move(newBuffer);
    hasUnsavedChanges_ = true;
    repaint();
}
void SampleEditorComponent::swapChannels() {
    if (!editBuffer_ && !audioHandle_)
        return;
    
    juce::AudioBuffer<float> *buf = editBuffer_ ? editBuffer_.get() : nullptr;
    if (!buf || buf->getNumChannels() < 2) {
        DBG("[SampleEditor] Need stereo audio to swap channels");
        return;
    }
    
    pushUndoState("Swap Channels");
    
    for (int i = 0; i < buf->getNumSamples(); ++i) {
        float L = buf->getSample(0, i);
        float R = buf->getSample(1, i);
        buf->setSample(0, i, R);
        buf->setSample(1, i, L);
    }
    
    hasUnsavedChanges_ = true;
    repaint();
}
void SampleEditorComponent::adjustStereoWidth(float width) {
    DBG("[SampleEditor] Stereo width adjustment (width=" + juce::String(width) + 
        "): Feature planned. Requires M/S encoding.");
}
void SampleEditorComponent::extractCenter() {
    DBG("[SampleEditor] Extract center: Feature planned. Requires M/S decoding with "
        "phase cancellation - keep Mid, discard Side.");
}
void SampleEditorComponent::extractSides() {
    DBG("[SampleEditor] Extract sides: Feature planned. Requires M/S decoding - "
        "keep Side, discard Mid.");
}

//==============================================================================
// Audio Device Callbacks
void SampleEditorComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (!device) return;
    auto sampleRate = device->getCurrentSampleRate();
    // auto bufferSize = device->getCurrentBufferSizeSamples(); // Unused but available
    auto numInputChannels = device->getActiveInputChannels().countNumberOfSetBits();
    
    if (numInputChannels <= 0) numInputChannels = 2;
    
    // Ring buffer size: 5 seconds
    int ringBufferSize = (int)(sampleRate * 5.0);
    incomingBuffer_.setSize(numInputChannels, ringBufferSize);
    incomingFifo_ = std::make_unique<juce::AbstractFifo>(ringBufferSize);
}

void SampleEditorComponent::audioDeviceStopped()
{
    incomingFifo_.reset();
    incomingBuffer_.setSize(0, 0);
}

void SampleEditorComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                             int numInputChannels,
                                                             float* const* outputChannelData,
                                                             int numOutputChannels,
                                                             int numSamples,
                                                             const juce::AudioIODeviceCallbackContext& context)
{
    if (!isRecording_ || !incomingFifo_) return;
    
    int internalChans = incomingBuffer_.getNumChannels();
    int minChans = std::min(numInputChannels, internalChans);
    
    // RAII managed write - finishedWrite() called automatically in destructor
    ScopedFifoWriter writer(*incomingFifo_, numSamples);
    
    if (writer.size1 > 0) {
        for (int ch = 0; ch < minChans; ++ch) {
            if (inputChannelData[ch])
                incomingBuffer_.copyFrom(ch, writer.start1, inputChannelData[ch], writer.size1);
        }
    }
    
    if (writer.size2 > 0) {
        for (int ch = 0; ch < minChans; ++ch) {
            if (inputChannelData[ch])
                incomingBuffer_.copyFrom(ch, writer.start2, inputChannelData[ch] + writer.size1, writer.size2);
        }
    }
}

} // namespace zenith
