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

==============================================================================
// WAVETABLE EDITOR IMPLEMENTATION
//==============================================================================
*/

#include "WavetableEditor.h"
#include "WavetableLoader.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// WAVETABLE EDITOR
//==============================================================================

ZenithWavetableEditor::ZenithWavetableEditor() {
    frames_.resize(1);
    frames_[0].fill(0.0f);
}

//==============================================================================
// FRAME MANAGEMENT
//==============================================================================

void ZenithWavetableEditor::clear() {
    for (auto& frame : frames_) {
        frame.fill(0.0f);
    }
    notifyChanged();
}

void ZenithWavetableEditor::setNumFrames(int numFrames) {
    numFrames_ = juce::jlimit(1, MAX_FRAMES, numFrames);
    frames_.resize(numFrames_);
    for (auto& frame : frames_) {
        frame.fill(0.0f);
    }
    currentFrame_ = juce::jmin(currentFrame_, numFrames_ - 1);
    notifyChanged();
}

void ZenithWavetableEditor::setCurrentFrame(int frame) {
    currentFrame_ = juce::jlimit(0, numFrames_ - 1, frame);
}

//==============================================================================
// WAVEFORM GENERATION
//==============================================================================

void ZenithWavetableEditor::generateWaveform(BasicWaveform type) {
    saveUndoState();

    auto& frame = frames_[currentFrame_];

    for (int i = 0; i < SINGLE_CYCLE_SIZE; ++i) {
        float phase = static_cast<float>(i) / static_cast<float>(SINGLE_CYCLE_SIZE);
        float twoPiPhase = phase * juce::MathConstants<float>::twoPi;

        switch (type) {
            case BasicWaveform::Sine:
                frame[i] = std::sin(twoPiPhase);
                break;

            case BasicWaveform::Square:
                frame[i] = (phase < 0.5f) ? 1.0f : -1.0f;
                break;

            case BasicWaveform::Saw:
                frame[i] = 2.0f * phase - 1.0f;
                break;

            case BasicWaveform::Triangle:
                frame[i] = 2.0f * std::abs(2.0f * phase - 1.0f) - 1.0f;
                break;

            case BasicWaveform::Noise: {
                static juce::Random rng;
                frame[i] = rng.nextFloat() * 2.0f - 1.0f;
                break;
            }
        }
    }

    notifyChanged();
}

void ZenithWavetableEditor::generateFromHarmonics(const float* amplitudes, int numHarmonics) {
    generateFromHarmonics([&](int harmonic) -> float {
        if (harmonic >= numHarmonics) return 0.0f;
        return amplitudes[harmonic];
    });
}

void ZenithWavetableEditor::generateFromHarmonics(const std::function<float(int)>& amplitudeFunc) {
    saveUndoState();
    auto& frame = frames_[currentFrame_];

    // Clear frame
    frame.fill(0.0f);

    // Add harmonics
    constexpr int maxHarmonics = 64;
    for (int h = 1; h < maxHarmonics; ++h) {
        float amp = amplitudeFunc(h);
        if (amp == 0.0f) continue;

        for (int i = 0; i < SINGLE_CYCLE_SIZE; ++i) {
            float phase = static_cast<float>(i) / static_cast<float>(SINGLE_CYCLE_SIZE);
            float twoPiPhase = phase * juce::MathConstants<float>::twoPi * h;
            frame[i] += amp * std::sin(twoPiPhase);
        }
    }

    // Normalize
    normalize(true);

    notifyChanged();
}

//==============================================================================
// WAVEFORM PROCESSING
//==============================================================================

void ZenithWavetableEditor::normalize(bool peak) {
    auto& frame = frames_[currentFrame_];

    // Find min/max
    float minVal = *std::min_element(frame.begin(), frame.end());
    float maxVal = *std::max_element(frame.begin(), frame.end());

    if (peak) {
        // Peak normalize to ±1
        float absMax = std::max(std::abs(minVal), std::abs(maxVal));
        if (absMax > 0.0001f) {
            float scale = 1.0f / absMax;
            for (auto& sample : frame) {
                sample *= scale;
            }
        }
    } else {
        // RMS normalize
        float sum = 0.0f;
        for (auto s : frame) sum += s * s;
        float rms = std::sqrt(sum / SINGLE_CYCLE_SIZE);
        if (rms > 0.0001f) {
            float scale = 0.707f / rms; // -3dB
            for (auto& sample : frame) {
                sample *= scale;
            }
        }
    }

    notifyChanged();
}

void ZenithWavetableEditor::alignPhase() {
    saveUndoState();
    auto& frame = frames_[currentFrame_];

    // Find zero crossing near start
    int zeroCross = 0;
    for (int i = 1; i < SINGLE_CYCLE_SIZE / 4; ++i) {
        if ((frame[i-1] <= 0.0f && frame[i] > 0.0f) ||
            (frame[i-1] >= 0.0f && frame[i] < 0.0f)) {
            zeroCross = i;
            break;
        }
    }

    // Rotate waveform so zero crossing is at start
    if (zeroCross > 0) {
        std::rotate(frame.begin(), frame.begin() + zeroCross, frame.end());
    }

    notifyChanged();
}

void ZenithWavetableEditor::smooth(int strength) {
    saveUndoState();
    auto& frame = frames_[currentFrame_];

    std::array<float, SINGLE_CYCLE_SIZE> temp = frame;
    float kernel = static_cast<float>(strength) / 10.0f;

    for (int i = 0; i < SINGLE_CYCLE_SIZE; ++i) {
        int i1 = (i - 1 + SINGLE_CYCLE_SIZE) % SINGLE_CYCLE_SIZE;
        int i2 = i;
        int i3 = (i + 1) % SINGLE_CYCLE_SIZE;

        frame[i] = temp[i2] * (1.0f - kernel) +
                    (temp[i1] + temp[i3]) * kernel * 0.5f;
    }

    notifyChanged();
}

void ZenithWavetableEditor::removeDC() {
    saveUndoState();
    auto& frame = frames_[currentFrame_];

    // Calculate DC offset
    float sum = 0.0f;
    for (auto s : frame) sum += s;
    float dcOffset = sum / SINGLE_CYCLE_SIZE;

    // Remove DC
    for (auto& s : frame) {
        s -= dcOffset;
    }

    notifyChanged();
}

void ZenithWavetableEditor::applyCrossfade(int samples) {
    saveUndoState();
    auto& frame = frames_[currentFrame_];
    samples = juce::jmin(samples, SINGLE_CYCLE_SIZE / 8);

    // Fade in at start
    for (int i = 0; i < samples; ++i) {
        float gain = static_cast<float>(i) / static_cast<float>(samples);
        frame[i] *= gain;
    }

    // Fade out at end
    for (int i = 0; i < samples; ++i) {
        int idx = SINGLE_CYCLE_SIZE - 1 - i;
        float gain = static_cast<float>(i) / static_cast<float>(samples);
        frame[idx] *= gain;
    }

    notifyChanged();
}

//==============================================================================
// IMPORT/EXPORT
//==============================================================================

juce::Result ZenithWavetableEditor::importFromFile(const juce::File& file) {
    WavetableLoader loader;
    WavetableData wtData;

    auto result = loader.loadWavetable(file, wtData);
    if (!result.wasOk()) {
        return result;
    }

    // Convert to editor format
    setNumFrames(wtData.getNumFrames());

    for (int i = 0; i < numFrames_; ++i) {
        const auto& srcFrame = wtData.getFrame(i);
        auto& dstFrame = frames_[i];

        for (int s = 0; s < SINGLE_CYCLE_SIZE; ++s) {
            dstFrame[s] = srcFrame[s];
        }
    }

    notifyChanged();
    return juce::Result::success();
}

void ZenithWavetableEditor::importFromBuffer(const float* data, int numSamples) {
    saveUndoState();
    auto& frame = frames_[currentFrame_];

    if (numSamples == SINGLE_CYCLE_SIZE) {
        // Direct copy
        std::copy(data, data + numSamples, frame.begin());
    } else {
        // Resample
        for (int i = 0; i < SINGLE_CYCLE_SIZE; ++i) {
            float srcPos = (static_cast<float>(i) / SINGLE_CYCLE_SIZE) * numSamples;
            int srcIdx = static_cast<int>(srcPos);
            float frac = srcPos - srcIdx;
            int nextIdx = (srcIdx + 1) % numSamples;

            frame[i] = lerp(data[srcIdx], data[nextIdx], frac);
        }
    }

    notifyChanged();
}

juce::Result ZenithWavetableEditor::exportCurrentFrame(const juce::File& file) const {
    // Create a single-cycle WAV file
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterTo(file, 44100, 1, 16, {})
    );

    if (!writer) {
        return juce::Result::fail("Failed to create WAV writer");
    }

    // Write frame as audio
    juce::AudioBuffer<float> buffer(1, SINGLE_CYCLE_SIZE);
    auto& frame = frames_[currentFrame_];
    juce::FloatVectorOperations::copy(frame.data(), buffer.getWritePointer(0), SINGLE_CYCLE_SIZE);

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, SINGLE_CYCLE_SIZE)) {
        return juce::Result::fail("Failed to write audio data");
    }

    return juce::Result::success();
}

juce::Result ZenithWavetableEditor::exportWavetable(const juce::File& file) const {
    // Export as multi-frame WAV (Serum-compatible)
    juce::WavAudioFormat wavFormat;
    int totalSamples = SINGLE_CYCLE_SIZE * numFrames_;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterTo(file, 44100, 1, 16, {})
    );

    if (!writer) {
        return juce::Result::fail("Failed to create WAV writer");
    }

    // Write all frames
    juce::AudioBuffer<float> buffer(1, totalSamples);

    for (int frame = 0; frame < numFrames_; ++frame) {
        int offset = frame * SINGLE_CYCLE_SIZE;
        const auto& srcFrame = frames_[frame];
        juce::FloatVectorOperations::copy(srcFrame.data(),
            buffer.getWritePointer(0, offset), SINGLE_CYCLE_SIZE);
    }

    if (!writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples)) {
        return juce::Result::fail("Failed to write audio data");
    }

    return juce::Result::success();
}

WavetableData ZenithWavetableEditor::exportWavetableData() const {
    WavetableData wtData;

    for (int i = 0; i < numFrames_; ++i) {
        WavetableFrame frame;
        for (int s = 0; s < SINGLE_CYCLE_SIZE; ++s) {
            frame[s] = frames_[i][s];
        }
        wtData.addFrame(frame);
    }

    wtData.setNumFrames(numFrames_);
    return wtData;
}

//==============================================================================
// FRAME EDITING
//==============================================================================

const float* ZenithWavetableEditor::getFrameData(int frame) const {
    return frames_[juce::jlimit(0, numFrames_ - 1, frame)].data();
}

float* ZenithWavetableEditor::getFrameData(int frame) {
    return frames_[juce::jlimit(0, numFrames_ - 1, frame)].data();
}

const float* ZenithWavetableEditor::getCurrentFrameData() const {
    return frames_[currentFrame_].data();
}

float* ZenithWavetableEditor::getCurrentFrameData() {
    return frames_[currentFrame_].data();
}

void ZenithWavetableEditor::setFrameData(int frame, const float* data) {
    saveUndoState();
    int idx = juce::jlimit(0, numFrames_ - 1, frame);
    std::copy(data, data + SINGLE_CYCLE_SIZE, frames_[idx].begin());
    notifyChanged();
}

//==============================================================================
// MORPHING
//==============================================================================

void ZenithWavetableEditor::morphFrames(const float* frameA, const float* frameB,
                                       float* output, float position) {
    position = juce::jlimit(0.0f, 1.0f, position);

    for (int i = 0; i < SINGLE_CYCLE_SIZE; ++i) {
        output[i] = lerp(frameA[i], frameB[i], position);
    }
}

float ZenithWavetableEditor::getInterpolatedSample(float phase, float framePosition) const {
    framePosition = juce::jlimit(0.0f, 1.0f, framePosition);

    float exactFrame = framePosition * (numFrames_ - 1);
    int frame1 = static_cast<int>(exactFrame);
    int frame2 = juce::jmin(frame1 + 1, numFrames_ - 1);
    float frameFrac = exactFrame - static_cast<float>(frame1);

    const auto& f1 = frames_[frame1];
    const auto& f2 = frames_[frame2];

    // Get sample from each frame
    float samplePos = phase * SINGLE_CYCLE_SIZE;
    int idx1 = static_cast<int>(samplePos) % SINGLE_CYCLE_SIZE;
    int idx2 = (idx1 + 1) % SINGLE_CYCLE_SIZE;
    float sampleFrac = samplePos - static_cast<float>(idx1);

    float s1 = lerp(f1[idx1], f1[idx2], sampleFrac);
    float s2 = lerp(f2[idx1], f2[idx2], sampleFrac);

    return lerp(s1, s2, frameFrac);
}

//==============================================================================
// ANALYSIS
//==============================================================================

ZenithWavetableEditor::WaveformStats ZenithWavetableEditor::analyzeFrame(int frame) const {
    WaveformStats stats;
    const auto& data = frames_[juce::jlimit(0, numFrames_ - 1, frame)];

    // Min/max
    stats.min = *std::min_element(data.begin(), data.end());
    stats.max = *std::max_element(data.begin(), data.end());
    stats.peakToPeak = stats.max - stats.min;

    // RMS
    float sum = 0.0f;
    for (auto s : data) sum += s * s;
    stats.rms = std::sqrt(sum / SINGLE_CYCLE_SIZE);

    // DC offset
    float avg = 0.0f;
    for (auto s : data) avg += s;
    stats.dcOffset = avg / SINGLE_CYCLE_SIZE;

    // Zero crossings
    stats.zeroCrossings = 0.0f;
    for (int i = 1; i < SINGLE_CYCLE_SIZE; ++i) {
        if ((data[i-1] <= 0.0f && data[i] > 0.0f) ||
            (data[i-1] >= 0.0f && data[i] < 0.0f)) {
            stats.zeroCrossings += 1.0f;
        }
    }

    return stats;
}

void ZenithWavetableEditor::getHarmonics(int frame, float* amplitudes, int numHarmonics) const {
    const auto& data = frames_[juce::jlimit(0, numFrames_ - 1, frame)];

    // Simple FFT/DFT for harmonic analysis
    for (int h = 0; h < numHarmonics; ++h) {
        float real = 0.0f;
        float imag = 0.0f;

        for (int i = 0; i < SINGLE_CYCLE_SIZE; ++i) {
            float phase = static_cast<float>(i) / static_cast<float>(SINGLE_CYCLE_SIZE);
            float angle = phase * juce::MathConstants<float>::twoPi * h;
            real += data[i] * std::cos(angle);
            imag -= data[i] * std::sin(angle);
        }

        // Magnitude
        amplitudes[h] = std::sqrt(real * real + imag * imag) / SINGLE_CYCLE_SIZE;
    }
}

//==============================================================================
// UNDO/REDO
//==============================================================================

void ZenithWavetableEditor::saveUndoState() {
    // Copy current state to undo stack
    undoStack_.push_back(frames_[currentFrame_]);

    // Limit undo size
    if (undoStack_.size() > MAX_UNDO) {
        undoStack_.erase(undoStack_.begin());
    }

    // Clear redo stack
    redoStack_.clear();
    undoIndex_ = static_cast<int>(undoStack_.size()) - 1;
}

void ZenithWavetableEditor::undo() {
    if (!canUndo()) return;

    // Save current to redo
    redoStack_.push_back(frames_[currentFrame_]);

    // Restore from undo
    frames_[currentFrame_] = undoStack_.back();
    undoStack_.pop_back();

    notifyChanged();
}

void ZenithWavetableEditor::redo() {
    if (!canRedo()) return;

    // Save current to undo
    undoStack_.push_back(frames_[currentFrame_]);

    // Restore from redo
    frames_[currentFrame_] = redoStack_.back();
    redoStack_.pop_back();

    notifyChanged();
}

bool ZenithWavetableEditor::canUndo() const {
    return !undoStack_.empty();
}

bool ZenithWavetableEditor::canRedo() const {
    return !redoStack_.empty();
}

//==============================================================================
// INTERNAL HELPERS
//==============================================================================

void ZenithWavetableEditor::notifyChanged() {
    if (changeCallback_) {
        changeCallback_();
    }
}

float ZenithWavetableEditor::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

//==============================================================================
// WAVETABLE PRESETS
//==============================================================================

juce::StringArray ZenithWavetablePresets::getPresetNames() {
    return {
        "Sine",
        "Square",
        "Saw",
        "Triangle",
        "Supersaw",
        "Supersquare",
        "Morbid",
        "Vocal",
        "Bells"
    };
}

void ZenithWavetablePresets::loadPreset(const juce::String& name, ZenithWavetableEditor& editor) {
    WavetableData wtData = createPreset(name);
    editor.setNumFrames(wtData.getNumFrames());

    for (int i = 0; i < wtData.getNumFrames(); ++i) {
        const auto& frame = wtData.getFrame(i);
        float* dst = editor.getFrameData(i);
        for (int s = 0; s < ZenithWavetableEditor::SINGLE_CYCLE_SIZE; ++s) {
            dst[s] = frame[s];
        }
    }
}

WavetableData ZenithWavetablePresets::createPreset(const juce::String& name) {
    WavetableData wtData;

    if (name == "Sine") createSine(wtData);
    else if (name == "Square") createSquare(wtData);
    else if (name == "Saw") createSaw(wtData);
    else if (name == "Triangle") createTriangle(wtData);
    else if (name == "Supersaw") createSupersaw(wtData);
    else if (name == "Supersquare") createSupersquare(wtData);
    else if (name == "Morbid") createMorbid(wtData);
    else if (name == "Vocal") createVocal(wtData);
    else if (name == "Bells") createBells(wtData);

    return wtData;
}

void ZenithWavetablePresets::createSine(WavetableData& wt) {
    WavetableFrame frame;
    for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
        float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
        frame[i] = std::sin(phase * juce::MathConstants<float>::twoPi);
    }
    wt.addFrame(frame);
    wt.setName("Sine");
}

void ZenithWavetablePresets::createSquare(WavetableData& wt) {
    WavetableFrame frame;
    for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
        float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
        frame[i] = (phase < 0.5f) ? 0.7f : -0.7f; // Slightly reduced for headroom
    }
    wt.addFrame(frame);
    wt.setName("Square");
}

void ZenithWavetablePresets::createSaw(WavetableData& wt) {
    WavetableFrame frame;
    for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
        float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
        frame[i] = 2.0f * phase - 1.0f;
    }
    wt.addFrame(frame);
    wt.setName("Saw");
}

void ZenithWavetablePresets::createTriangle(WavetableData& wt) {
    WavetableFrame frame;
    for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
        float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
        frame[i] = 2.0f * std::abs(2.0f * phase - 1.0f) - 1.0f;
    }
    wt.addFrame(frame);
    wt.setName("Triangle");
}

void ZenithWavetablePresets::createSupersaw(WavetableData& wt) {
    // Create 8-frame morph from thin to thick
    for (int frame = 0; frame < 8; ++frame) {
        WavetableFrame wtFrame;
        int voices = 1 + frame;
        float gain = 1.0f / std::sqrt(static_cast<float>(voices));

        for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
            float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
            float sample = 0.0f;

            for (int v = 0; v < voices; ++v) {
                float detune = static_cast<float>(v - voices/2) * 0.05f;
                float detuneRatio = std::pow(2.0f, detune / 12.0f);
                float sawPhase = phase * detuneRatio;
                sample += (2.0f * std::fmod(sawPhase, 1.0f) - 1.0f) * gain;
            }

            wtFrame[i] = sample;
        }
        wt.addFrame(wtFrame);
    }
    wt.setName("Supersaw");
    wt.setNumFrames(8);
}

void ZenithWavetablePresets::createSupersquare(WavetableData& wt) {
    for (int frame = 0; frame < 8; ++frame) {
        WavetableFrame wtFrame;
        int voices = 1 + frame;
        float gain = 1.0f / std::sqrt(static_cast<float>(voices));

        for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
            float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
            float sample = 0.0f;

            for (int v = 0; v < voices; ++v) {
                float detune = static_cast<float>(v - voices/2) * 0.05f;
                float detuneRatio = std::pow(2.0f, detune / 12.0f);
                float sqPhase = phase * detuneRatio;
                float pulse = (std::fmod(sqPhase, 1.0f) < 0.5f) ? 0.7f : -0.7f;
                sample += pulse * gain;
            }

            wtFrame[i] = sample;
        }
        wt.addFrame(wtFrame);
    }
    wt.setName("Supersquare");
    wt.setNumFrames(8);
}

void ZenithWavetablePresets::createMorbid(WavetableData& wt) {
    // Serum-style "Morbid" - dark, detuned saw stack
    for (int frame = 0; frame < 16; ++frame) {
        WavetableFrame wtFrame;
        float detuneAmt = static_cast<float>(frame) / 15.0f * 0.5f;

        for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
            float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;

            // Stack of 3 detuned saws with different phase relationships
            float s1 = 2.0f * std::fmod(phase, 1.0f) - 1.0f;
            float s2 = 2.0f * std::fmod(phase * 1.003f, 1.0f) - 1.0f;
            float s3 = 2.0f * std::fmod(phase * 0.997f, 1.0f) - 1.0f;

            wtFrame[i] = (s1 + s2 * 0.5f + s3 * 0.25f) * 0.57f;
        }
        wt.addFrame(wtFrame);
    }
    wt.setName("Morbid");
    wt.setNumFrames(16);
}

void ZenithWavetablePresets::createVocal(WavetableData& wt) {
    // Formant-based vowel morph
    const char* vowels = "AEIOU";
    for (int v = 0; v < 5; ++v) {
        WavetableFrame wtFrame;

        // Formant frequencies for each vowel
        float f1, f2, f3;
        float a1, a2, a3;

        switch (v) {
            case 0: // A
                f1 = 800; f2 = 1150; f3 = 2800;
                a1 = 1.0f; a2 = 0.5f; a3 = 0.2f;
                break;
            case 1: // E
                f1 = 400; f2 = 2200; f3 = 2800;
                a1 = 1.0f; a2 = 0.3f; a3 = 0.15f;
                break;
            case 2: // I
                f1 = 350; f2 = 2200; f3 = 3000;
                a1 = 1.0f; a2 = 0.2f; a3 = 0.1f;
                break;
            case 3: // O
                f1 = 500; f2 = 1000; f3 = 2800;
                a1 = 1.0f; a2 = 0.6f; a3 = 0.2f;
                break;
            case 4: // U
                f1 = 350; f2 = 800; f3 = 2200;
                a1 = 1.0f; a2 = 0.4f; a3 = 0.15f;
                break;
        }

        for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
            float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
            float twoPiPhase = phase * juce::MathConstants<float>::twoPi;

            float sample = a1 * std::sin(twoPiPhase * f1 / 440.0f)
                       + a2 * std::sin(twoPiPhase * f2 / 440.0f)
                       + a3 * std::sin(twoPiPhase * f3 / 440.0f);

            wtFrame[i] = sample / (a1 + a2 + a3);
        }
        wt.addFrame(wtFrame);
    }
    wt.setName("Vocal");
    wt.setNumFrames(5);
}

void ZenithWavetablePresets::createBells(WavetableData& wt) {
    // FM bell tone
    for (int frame = 0; frame < 8; ++frame) {
        WavetableFrame wtFrame;
        float modIndex = 0.5f + static_cast<float>(frame) / 7.0f * 4.0f;

        for (int i = 0; i < WavetableFrame::SAMPLES_PER_FRAME; ++i) {
            float phase = static_cast<float>(i) / WavetableFrame::SAMPLES_PER_FRAME;
            float twoPiPhase = phase * juce::MathConstants<float>::twoPi;

            // FM synthesis: carrier modulated by modulator
            float carrier = std::sin(twoPiPhase);
            float modulator = std::sin(twoPiPhase * 2.0f); // 2:1 ratio
            float sample = std::sin(twoPiPhase + modIndex * modulator);

            wtFrame[i] = sample;
        }
        wt.addFrame(wtFrame);
    }
    wt.setName("Bells");
    wt.setNumFrames(8);
}

} // namespace zenith
