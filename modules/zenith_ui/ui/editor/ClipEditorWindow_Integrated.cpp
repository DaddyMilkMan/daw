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

#include "ClipEditorWindow.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace zenith::ui {

//==============================================================================
// Async Waveform Loader
//==============================================================================

class WaveformLoader : public juce::ThreadPoolJob {
public:
    WaveformLoader(const juce::File& file, 
                   std::function<void(std::vector<float>)> onComplete)
        : ThreadPoolJob("WaveformLoader"),
          audioFile_(file),
          onComplete_(std::move(onComplete)) {}
    
    JobStatus runJob() override {
        auto samples = loadWaveformData();
        
        if (onComplete_ && !shouldExit()) {
            juce::MessageManager::callAsync([samples, cb = std::move(onComplete_)]() {
                cb(samples);
            });
        }
        
        return jobHasFinished;
    }
    
private:
    juce::File audioFile_;
    std::function<void(std::vector<float>)> onComplete_;
    
    std::vector<float> loadWaveformData() {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(audioFile_)
        );
        
        if (!reader) {
            return {};
        }
        
        // Read audio data
        const int numSamples = static_cast<int>(reader->lengthInSamples);
        const int numChannels = static_cast<int>(reader->numChannels);
        
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        reader->read(&buffer, 0, numSamples, 0, true, true);
        
        // Generate overview (downsample for display)
        const int targetPoints = 2000;
        const int samplesPerPoint = std::max(1, numSamples / targetPoints);
        
        std::vector<float> waveform;
        waveform.reserve(targetPoints);
        
        for (int i = 0; i < targetPoints; ++i) {
            int startSample = i * samplesPerPoint;
            int endSample = std::min(startSample + samplesPerPoint, numSamples);
            
            float maxValue = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                auto* channelData = buffer.getReadPointer(ch);
                for (int s = startSample; s < endSample; ++s) {
                    maxValue = std::max(maxValue, std::abs(channelData[s]));
                }
            }
            waveform.push_back(maxValue);
        }
        
        return waveform;
    }
};

//==============================================================================
// ContentComponent - UPDATED with Waveform
//==============================================================================

ClipEditorWindow::ContentComponent::ContentComponent(
    zenith::ProjectState& projectState,
    const juce::String& clipId,
    const juce::String& trackId)
    : projectState_(projectState)
    , clipId_(clipId)
    , trackId_(trackId)
    , threadPool_(1) {  // Single thread for waveform loading

    // Load clip data from project state
    auto [trackTree, clipTree] = projectState_.findClip(clipId_);
    if (clipTree.isValid()) {
        clipName_ = clipTree.getProperty(zenith::ProjectState::PROP_NAME, "Untitled Clip").toString();
        clipType_ = clipTree.getProperty(zenith::ProjectState::PROP_TYPE, "audio").toString();
        startBeats_ = clipTree.getProperty(zenith::ProjectState::PROP_START_BEATS, 0.0);
        lengthBeats_ = clipTree.getProperty(zenith::ProjectState::PROP_LENGTH_BEATS, 4.0);

        if (clipType_ == "audio") {
            audioFilePath_ = clipTree.getProperty(zenith::ProjectState::PROP_AUDIO_FILE, "").toString();
            loadWaveformAsync();
        }
    }
    
    setSize(800, 400);
}

ClipEditorWindow::ContentComponent::~ContentComponent() {
    threadPool_.removeAllJobs(true, 1000);
}

void ClipEditorWindow::ContentComponent::loadWaveformAsync() {
    if (audioFilePath_.isEmpty()) return;
    
    juce::File audioFile(audioFilePath_);
    if (!audioFile.existsAsFile()) {
        waveformState_ = WaveformState::Error;
        errorMessage_ = "Audio file not found";
        repaint();
        return;
    }
    
    waveformState_ = WaveformState::Loading;
    repaint();
    
    // Launch async load
    auto* job = new WaveformLoader(audioFile, [this](std::vector<float> samples) {
        waveformData_ = std::move(samples);
        waveformState_ = waveformData_.empty() ? WaveformState::Error : WaveformState::Loaded;
        if (waveformState_ == WaveformState::Error) {
            errorMessage_ = "Failed to load waveform";
        }
        repaint();
    });
    
    threadPool_.addJob(job, true);
}

void ClipEditorWindow::ContentComponent::resized() {
    // Layout child components if any
}

#ifdef ZENITH_USE_SKIA
void ClipEditorWindow::ContentComponent::drawSkia(SkCanvas* canvas) {
    using namespace design;

    auto width = static_cast<float>(getWidth());
    auto height = static_cast<float>(getHeight());

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(colors::background::dark));
    canvas->drawRect(SkRect::MakeWH(width, height), bgPaint);

    // Header bar
    SkPaint headerPaint;
    headerPaint.setColor(toSkColor(colors::surface::overlay));
    canvas->drawRect(SkRect::MakeWH(width, 40.0f), headerPaint);

    // Clip name in header
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::primary));
    textPaint.setAntiAlias(true);

    SkFont headerFont = typography::getSkFont(typography::FONT_SM, FontWeight::Medium);
    canvas->drawSimpleText(clipName_.toUTF8(), clipName_.length(),
                          headerFont, 12.0f, 26.0f, textPaint);
    
    // Clip type indicator
    SkFont typeFont = typography::getSkFont(typography::FONT_XS);
    SkPaint typePaint;
    typePaint.setColor(toSkColor(colors::text::tertiary));
    typePaint.setAntiAlias(true);
    juce::String typeLabel = "[" + clipType_.toUpperCase() + "]";
    canvas->drawSimpleText(typeLabel.toUTF8(), typeLabel.length(),
                          typeFont, width - 80.0f, 26.0f, typePaint);

    // Editor area content
    if (clipType_ == "audio") {
        drawAudioClipEditor(canvas, width, height);
    } else if (clipType_ == "midi") {
        drawMidiClipEditor(canvas, width, height);
    }
}

void ClipEditorWindow::ContentComponent::drawAudioClipEditor(SkCanvas* canvas, 
                                                              float width, 
                                                              float height) {
    using namespace design;

    const float headerHeight = 40.0f;
    const float padding = 20.0f;
    const float waveformTop = headerHeight + padding;
    const float waveformHeight = height - headerHeight - padding * 2;
    const float centerY = waveformTop + waveformHeight / 2;

    // Draw waveform area background
    SkRect waveRect = SkRect::MakeXYWH(padding, waveformTop, 
                                       width - padding * 2, waveformHeight);
    SkPaint waveBgPaint;
    waveBgPaint.setColor(toSkColor(colors::surface::elevated));
    waveBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(waveRect, 8.0f, 8.0f, waveBgPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(toSkColor(colors::border::default_color));
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(waveRect, 8.0f, 8.0f, borderPaint);

    // State-based rendering
    switch (waveformState_) {
        case WaveformState::Loading:
            drawLoadingState(canvas, waveRect);
            break;
            
        case WaveformState::Loaded:
            drawWaveform(canvas, waveRect, waveformData_);
            break;
            
        case WaveformState::Error:
            drawErrorState(canvas, waveRect, errorMessage_);
            break;
            
        case WaveformState::Empty:
            drawEmptyState(canvas, waveRect);
            break;
    }

    // Center line
    SkPaint centerLinePaint;
    centerLinePaint.setColor(toSkColor(colors::text::tertiary).withAlpha(0.3f));
    centerLinePaint.setStrokeWidth(1.0f);
    canvas->drawLine(padding, centerY, width - padding, centerY, centerLinePaint);

    // Time ruler at bottom
    drawTimeRuler(canvas, padding, height - padding + 10, width - padding * 2);
}

void ClipEditorWindow::ContentComponent::drawWaveform(SkCanvas* canvas,
                                                       const SkRect& bounds,
                                                       const std::vector<float>& samples) {
    using namespace design;
    
    if (samples.empty()) return;
    
    const float centerY = bounds.centerY();
    const float height = bounds.height() * 0.8f;
    const float width = bounds.width();
    const float x = bounds.left();
    
    // Create waveform path
    SkPath waveformPath;
    const size_t numPoints = samples.size();
    const float stepX = width / static_cast<float>(numPoints);
    
    // Top half
    waveformPath.moveTo(x, centerY);
    for (size_t i = 0; i < numPoints; ++i) {
        float sample = samples[i];
        float xPos = x + i * stepX;
        float yPos = centerY - sample * height / 2.0f;
        waveformPath.lineTo(xPos, yPos);
    }
    waveformPath.lineTo(x + width, centerY);
    
    // Bottom half
    for (int i = static_cast<int>(numPoints) - 1; i >= 0; --i) {
        float sample = samples[i];
        float xPos = x + i * stepX;
        float yPos = centerY + sample * height / 2.0f;
        waveformPath.lineTo(xPos, yPos);
    }
    waveformPath.close();
    
    // Fill with gradient
    SkPaint fillPaint;
    SkPoint gradPts[2] = {{0, bounds.top()}, {0, bounds.bottom()}};
    SkColor gradColors[2] = {
        toSkColor(colors::accent::primary).withAlpha(0.8f),
        toSkColor(colors::accent::secondary).withAlpha(0.4f)
    };
    fillPaint.setShader(SkGradientShader::MakeLinear(
        gradPts, gradColors, nullptr, 2, SkTileMode::kClamp
    ));
    fillPaint.setAntiAlias(true);
    canvas->drawPath(waveformPath, fillPaint);
    
    // Outline
    SkPaint outlinePaint;
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(1.0f);
    outlinePaint.setColor(toSkColor(colors::accent::primary));
    outlinePaint.setAntiAlias(true);
    
    SkPath topPath;
    topPath.moveTo(x, centerY);
    for (size_t i = 0; i < numPoints; ++i) {
        float sample = samples[i];
        float xPos = x + i * stepX;
        float yPos = centerY - sample * height / 2.0f;
        topPath.lineTo(xPos, yPos);
    }
    canvas->drawPath(topPath, outlinePaint);
    
    SkPath bottomPath;
    bottomPath.moveTo(x, centerY);
    for (size_t i = 0; i < numPoints; ++i) {
        float sample = samples[i];
        float xPos = x + i * stepX;
        float yPos = centerY + sample * height / 2.0f;
        bottomPath.lineTo(xPos, yPos);
    }
    canvas->drawPath(bottomPath, outlinePaint);
}

void ClipEditorWindow::ContentComponent::drawLoadingState(SkCanvas* canvas,
                                                           const SkRect& bounds) {
    using namespace design;
    
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::tertiary));
    textPaint.setAntiAlias(true);
    SkFont font = typography::getSkFont(typography::FONT_SM);
    
    const char* loadingText = "Loading waveform...";
    canvas->drawSimpleText(loadingText, strlen(loadingText),
                          font, bounds.centerX() - 60, bounds.centerY(), textPaint);
    
    // Draw animated spinner
    static float angle = 0.0f;
    angle += 0.1f;
    
    SkPaint spinnerPaint;
    spinnerPaint.setStyle(SkPaint::kStroke_Style);
    spinnerPaint.setStrokeWidth(2.0f);
    spinnerPaint.setColor(toSkColor(colors::accent::primary));
    spinnerPaint.setAntiAlias(true);
    
    float cx = bounds.centerX();
    float cy = bounds.centerY() + 30;
    float radius = 10.0f;
    
    SkRect oval(cx - radius, cy - radius, cx + radius, cy + radius);
    canvas->drawArc(oval, angle * 180.0f / 3.14159f, 270.0f, false, spinnerPaint);
    
    repaint();  // Keep animating
}

void ClipEditorWindow::ContentComponent::drawErrorState(SkCanvas* canvas,
                                                         const SkRect& bounds,
                                                         const juce::String& message) {
    using namespace design;
    
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::feedback::error));
    textPaint.setAntiAlias(true);
    SkFont font = typography::getSkFont(typography::FONT_SM);
    
    canvas->drawSimpleText("Error:", 6, font, bounds.centerX() - 30, bounds.centerY() - 10, textPaint);
    
    textPaint.setColor(toSkColor(colors::text::tertiary));
    canvas->drawSimpleText(message.toUTF8(), message.length(),
                          font, bounds.centerX() - message.length() * 3, 
                          bounds.centerY() + 10, textPaint);
}

void ClipEditorWindow::ContentComponent::drawEmptyState(SkCanvas* canvas,
                                                         const SkRect& bounds) {
    using namespace design;
    
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::tertiary));
    textPaint.setAntiAlias(true);
    SkFont font = typography::getSkFont(typography::FONT_SM);
    
    const char* emptyText = "No audio file loaded";
    canvas->drawSimpleText(emptyText, strlen(emptyText),
                          font, bounds.centerX() - 60, bounds.centerY(), textPaint);
}

void ClipEditorWindow::ContentComponent::drawTimeRuler(SkCanvas* canvas,
                                                        float x, float y, 
                                                        float width) {
    using namespace design;
    
    SkPaint linePaint;
    linePaint.setColor(toSkColor(colors::border::default_color));
    linePaint.setStrokeWidth(1.0f);
    canvas->drawLine(x, y, x + width, y, linePaint);
    
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::tertiary));
    textPaint.setAntiAlias(true);
    SkFont font = typography::getSkFont(typography::FONT_XS);
    
    // Draw beat markers
    const int numBeats = static_cast<int>(lengthBeats_);
    const float beatWidth = width / lengthBeats_;
    
    for (int i = 0; i <= numBeats; ++i) {
        float xPos = x + i * beatWidth;
        
        // Tick mark
        canvas->drawLine(xPos, y, xPos, y + 5, linePaint);
        
        // Label every 4 beats
        if (i % 4 == 0) {
            juce::String label = juce::String(i + 1);
            canvas->drawSimpleText(label.toUTF8(), label.length(),
                                  font, xPos - 3, y + 15, textPaint);
        }
    }
}

void ClipEditorWindow::ContentComponent::drawMidiClipEditor(SkCanvas* canvas, 
                                                             float width, 
                                                             float height) {
    using namespace design;

    // Piano roll background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(colors::surface::elevated));
    canvas->drawRect(SkRect::MakeXYWH(20.0f, 60.0f, width - 40.0f, height - 100.0f), bgPaint);
    
    // Draw placeholder text
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::tertiary));
    textPaint.setAntiAlias(true);
    
    SkFont labelFont = typography::getSkFont(typography::FONT_SM, FontWeight::Regular);
    canvas->drawSimpleText("MIDI Clip Editor - Piano Roll", 30, 
                          labelFont, width / 2.0f - 80.0f, height / 2.0f, textPaint);
}
#endif

} // namespace zenith::ui
