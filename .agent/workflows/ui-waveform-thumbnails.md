---
description: Implement beautiful waveform and MIDI thumbnails for arranger clips
---

# UI Fix #5: Waveform & MIDI Clip Thumbnails

## MISSION
The arranger has clip bounds, logic for selection/drag/resize, but **the clips render as BLANK RECTANGLES**. A professional DAW shows:
- Audio clips: Waveform preview
- MIDI clips: Note bar visualization ("piano roll blob")

Current state: You build a waveform cache but never render it. Fix this.

## PRE-TASK RESEARCH (MANDATORY)

1. **Web Search**: "audio waveform rendering peak data reduction algorithm"
   - Understand min/max peak pairs for efficient waveform display
   - RMS vs Peak visualization differences
   
2. **Web Search**: "Skia draw polyline fill waveform"
   - Best way to render thousands of sample points efficiently
   
3. **Web Search**: "Ableton Live MIDI clip preview rendering"
   - See how professional DAWs display MIDI notes in arrangement view
   
4. **Web Search**: "audio thumbnail cache JUCE AudioThumbnail"
   - JUCE has built-in AudioThumbnail - can we use it with Skia?
   
5. **Web Search**: "gradient waveform visualization audio"
   - Understand gradient/color techniques for waveforms

## THE PROBLEM

In `ArrangerComponent.cpp`:
```cpp
void ArrangerComponent::buildWaveformCache(const juce::String& audioFilePath) {
    // This populates waveformCache_ (peak data)
    // ...
}
```

But in `drawSkia()`:
```cpp
// ???
// WHERE IS THE CODE THAT DRAWS THE WAVEFORM???
```

You cache the data but never render it!

## IMPLEMENTATION STEPS

### Step 1: Design WaveformThumbnail Data Structure
Create `Source/ui/skia/WaveformThumbnail.h`:

```cpp
#pragma once
#include <core/SkPath.h>
#include <core/SkCanvas.h>
#include <vector>

namespace zenith {

struct WaveformPeaks {
    std::vector<float> minPeaks;  // Negative peaks
    std::vector<float> maxPeaks;  // Positive peaks
    int samplesPerPeak = 512;     // Resolution
    double sampleRate = 44100.0;
    double lengthSeconds = 0.0;
};

class WaveformThumbnail {
public:
    void setSource(const juce::File& audioFile);
    void setSource(const float* samples, int numSamples, double sampleRate);
    
    // Render waveform into given bounds
    void draw(SkCanvas* canvas, const SkRect& bounds, 
              double startSeconds, double endSeconds,
              SkColor fillColor, SkColor strokeColor) const;
    
    // Optimized path generation
    SkPath generatePath(const SkRect& bounds, 
                        double startSeconds, double endSeconds) const;
    
    bool isValid() const { return !peaks_.maxPeaks.empty(); }
    double getLengthSeconds() const { return peaks_.lengthSeconds; }

private:
    WaveformPeaks peaks_;
    void buildPeaks(const float* samples, int numSamples, double sampleRate);
};

} // namespace zenith
```

### Step 2: Implement Peak Building
`WaveformThumbnail.cpp`:

```cpp
void WaveformThumbnail::buildPeaks(const float* samples, int numSamples, 
                                    double sampleRate) {
    peaks_.sampleRate = sampleRate;
    peaks_.samplesPerPeak = 512;  // ~86 peaks per second at 44.1kHz
    
    int numPeaks = (numSamples + peaks_.samplesPerPeak - 1) / peaks_.samplesPerPeak;
    peaks_.minPeaks.resize(numPeaks);
    peaks_.maxPeaks.resize(numPeaks);
    
    for (int p = 0; p < numPeaks; ++p) {
        int start = p * peaks_.samplesPerPeak;
        int end = std::min(start + peaks_.samplesPerPeak, numSamples);
        
        float minVal = 1.0f, maxVal = -1.0f;
        for (int s = start; s < end; ++s) {
            float sample = samples[s];
            minVal = std::min(minVal, sample);
            maxVal = std::max(maxVal, sample);
        }
        
        peaks_.minPeaks[p] = minVal;
        peaks_.maxPeaks[p] = maxVal;
    }
    
    peaks_.lengthSeconds = numSamples / sampleRate;
}
```

### Step 3: Implement Waveform Rendering
```cpp
void WaveformThumbnail::draw(SkCanvas* canvas, const SkRect& bounds,
                              double startSeconds, double endSeconds,
                              SkColor fillColor, SkColor strokeColor) const {
    if (!isValid()) return;
    
    SkPath path = generatePath(bounds, startSeconds, endSeconds);
    
    // Fill (gradient from center outward)
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    
    SkPoint pts[] = {{bounds.centerX(), bounds.centerY()}, 
                     {bounds.centerX(), bounds.top()}};
    SkColor colors[] = {SkColorSetA(fillColor, 180), SkColorSetA(fillColor, 60)};
    fillPaint.setShader(SkGradientShader::MakeLinear(
        pts, colors, nullptr, 2, SkTileMode::kMirror));
    
    canvas->drawPath(path, fillPaint);
    
    // Stroke (outline)
    SkPaint strokePaint;
    strokePaint.setAntiAlias(true);
    strokePaint.setStyle(SkPaint::kStroke_Style);
    strokePaint.setStrokeWidth(0.5f);
    strokePaint.setColor(strokeColor);
    canvas->drawPath(path, strokePaint);
}

SkPath WaveformThumbnail::generatePath(const SkRect& bounds,
                                        double startSeconds, 
                                        double endSeconds) const {
    SkPath path;
    
    float width = bounds.width();
    float height = bounds.height();
    float centerY = bounds.centerY();
    
    // Calculate which peaks are visible
    double secondsPerPeak = peaks_.samplesPerPeak / peaks_.sampleRate;
    int startPeak = std::max(0, (int)(startSeconds / secondsPerPeak));
    int endPeak = std::min((int)peaks_.maxPeaks.size(), 
                           (int)(endSeconds / secondsPerPeak) + 1);
    
    if (endPeak <= startPeak) return path;
    
    int numVisiblePeaks = endPeak - startPeak;
    float pixelsPerPeak = width / numVisiblePeaks;
    
    // Draw top half (max peaks)
    path.moveTo(bounds.left(), centerY);
    for (int i = 0; i < numVisiblePeaks; ++i) {
        float x = bounds.left() + i * pixelsPerPeak;
        float y = centerY - peaks_.maxPeaks[startPeak + i] * (height * 0.45f);
        if (i == 0) path.moveTo(x, y);
        else path.lineTo(x, y);
    }
    
    // Draw bottom half (min peaks, reversed)
    for (int i = numVisiblePeaks - 1; i >= 0; --i) {
        float x = bounds.left() + i * pixelsPerPeak;
        float y = centerY - peaks_.minPeaks[startPeak + i] * (height * 0.45f);
        path.lineTo(x, y);
    }
    
    path.close();
    return path;
}
```

### Step 4: Implement MIDI Thumbnail
`Source/ui/skia/MidiThumbnail.h`:

```cpp
class MidiThumbnail {
public:
    struct Note {
        int pitch;          // 0-127
        double startBeats;
        double lengthBeats;
        int velocity;       // For opacity
    };
    
    void setNotes(const std::vector<Note>& notes);
    
    void draw(SkCanvas* canvas, const SkRect& bounds,
              double startBeats, double endBeats,
              SkColor noteColor) const;

private:
    std::vector<Note> notes_;
    int lowestNote_ = 127;
    int highestNote_ = 0;
};
```

Implementation:
```cpp
void MidiThumbnail::draw(SkCanvas* canvas, const SkRect& bounds,
                          double startBeats, double endBeats,
                          SkColor noteColor) const {
    if (notes_.empty()) return;
    
    float width = bounds.width();
    float height = bounds.height();
    
    int noteRange = std::max(1, highestNote_ - lowestNote_ + 1);
    float noteHeight = std::min(height / noteRange, 4.0f);  // Max 4px per note
    float beatsToPixels = width / (endBeats - startBeats);
    
    SkPaint paint;
    paint.setAntiAlias(true);
    
    for (const auto& note : notes_) {
        // Skip notes outside visible range
        if (note.startBeats + note.lengthBeats < startBeats ||
            note.startBeats > endBeats) continue;
        
        float x = bounds.left() + (note.startBeats - startBeats) * beatsToPixels;
        float w = note.lengthBeats * beatsToPixels;
        float y = bounds.bottom() - 
                  ((note.pitch - lowestNote_) / (float)noteRange) * height -
                  noteHeight;
        
        // Clamp to bounds
        x = std::max(x, bounds.left());
        w = std::min(x + w, bounds.right()) - x;
        
        // Velocity affects opacity
        int alpha = 100 + (note.velocity * 155 / 127);
        paint.setColor(SkColorSetA(noteColor, alpha));
        
        SkRect noteRect = SkRect::MakeXYWH(x, y, w, noteHeight);
        canvas->drawRoundRect(noteRect, 1.0f, 1.0f, paint);
    }
}
```

### Step 5: Integrate into ArrangerComponent
In `ArrangerComponent::drawClip()`:

```cpp
void ArrangerComponent::drawClip(SkCanvas* canvas, const ClipView& clip) {
    // 1. Clip background (glass panel)
    GlassmorphicPanel::draw(canvas, toSkRect(clip.bounds), 
                            clip.isSelected ? GlassmorphicPanel::Style::ActiveGlow
                                           : GlassmorphicPanel::Style::Subtle);
    
    // 2. Content (Waveform or MIDI)
    SkRect contentBounds = toSkRect(clip.bounds).makeInset(4.0f, 4.0f);
    
    if (clip.isMidi) {
        // MIDI thumbnail
        MidiThumbnail thumbnail;
        thumbnail.setNotes(clip.noteBlobs);  // Convert from your MidiNoteBlob
        thumbnail.draw(canvas, contentBounds, 
                       clip.startBeats, clip.startBeats + clip.lengthBeats,
                       clip.isSelected ? design::colors::CYAN 
                                       : design::colors::VIOLET);
    } else {
        // Audio waveform
        if (auto* waveform = getWaveformThumbnail(clip.audioFilePath)) {
            double startSec = beatsToSeconds(clip.startBeats);
            double endSec = beatsToSeconds(clip.startBeats + clip.lengthBeats);
            waveform->draw(canvas, contentBounds, startSec, endSec,
                           clip.isSelected ? design::colors::CYAN 
                                           : design::colors::NEON_GREEN,
                           design::colors::TEXT_PRIMARY);
        }
    }
    
    // 3. Clip name label
    SkPaint textPaint;
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    SkFont font = design::getSkFont(design::typography::FONT_SM);
    canvas->drawString(clip.name.toStdString().c_str(),
                       clip.bounds.getX() + 6.0f,
                       clip.bounds.getY() + 14.0f,
                       font, textPaint);
    
    // 4. Selection glow
    if (clip.isSelected) {
        NeonGlow::drawGlowOutline(canvas, toSkRect(clip.bounds),
                                   design::colors::CYAN,
                                   NeonGlow::Intensity::Strong, 4.0f);
    }
}
```

### Step 6: Waveform Cache Management
Create `WaveformCacheManager`:
```cpp
class WaveformCacheManager {
public:
    static WaveformCacheManager& getInstance();
    
    // Get or build thumbnail (async-safe)
    WaveformThumbnail* getThumbnail(const juce::String& filePath);
    
    // Build in background thread
    void requestThumbnail(const juce::String& filePath);
    
    // Memory management
    void trimCache(size_t maxBytes);
    
private:
    std::unordered_map<juce::String, std::unique_ptr<WaveformThumbnail>> cache_;
    std::mutex cacheMutex_;
    juce::ThreadPool backgroundPool_{1};
};
```

## VERIFICATION CHECKLIST
- [ ] Audio clips show waveform preview
- [ ] MIDI clips show note bars
- [ ] Zooming updates waveform resolution appropriately
- [ ] Waveforms load asynchronously (no UI freeze)
- [ ] Selected clips have distinct visual treatment
- [ ] Performance: 50+ clips on screen at 60fps
- [ ] Build succeeds with no errors

## ACCEPTANCE CRITERIA
1. Import a WAV file to a track
2. The clip should show the waveform immediately (or within 1 second)
3. Zoom in - waveform should reveal more detail
4. Create a MIDI clip with notes - notes should be visible as colored bars

Screenshot before/after. Empty rectangles = FAIL. Visible waveforms = PASS.

## PERFORMANCE NOTES
- Cache waveform peaks, not raw samples
- Use LOD (level of detail) for different zoom levels
- Build thumbnails on background thread
- Limit cache memory (LRU eviction)
