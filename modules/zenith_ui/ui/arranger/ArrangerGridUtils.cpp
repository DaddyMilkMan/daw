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

/*
    ==============================================================================
    Original file header:
*/

 * @file ArrangerGridUtils.cpp
 * @brief Implementation of coordinate conversion, grid snapping, and waveform cache utilities
 */



#include <cmath>

namespace zenith {

//==============================================================================
// Layout Constants (must match ArrangerComponent.cpp)
//==============================================================================
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float SECTION_HEIGHT = 24.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT = 80.0f;
static constexpr float TOP_MARGIN = SECTION_HEIGHT + RULER_HEIGHT;

//==============================================================================
// Constructor
//==============================================================================

ArrangerGridUtils::ArrangerGridUtils(ArrangerComponent& owner, Engine& engine, ProjectState& projectState)
    : owner_(owner)
    , engine_(engine)
    , projectState_(projectState)
{
}

//==============================================================================
// Coordinate Conversion
//==============================================================================

float ArrangerGridUtils::beatsToX(double beats) const {
    return HEADER_WIDTH +
           static_cast<float>((beats - owner_.viewStartBeats) * owner_.pixelsPerBeat);
}

double ArrangerGridUtils::xToBeats(float x) const {
    return owner_.viewStartBeats + ((x - HEADER_WIDTH) / owner_.pixelsPerBeat);
}

float ArrangerGridUtils::trackIndexToY(int trackIndex) const {
    return TOP_MARGIN + (trackIndex - owner_.firstVisibleTrackIndex) * TRACK_HEIGHT;
}

int ArrangerGridUtils::yToTrackIndex(float y) const {
    if (y < TOP_MARGIN)
        return -1;

    int index = owner_.firstVisibleTrackIndex +
           static_cast<int>((y - TOP_MARGIN) / TRACK_HEIGHT);

    // Bounds check against actual track count
    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    int trackCount = tracksNode.isValid() ? tracksNode.getNumChildren() : 0;
    if (index >= trackCount)
        return -1;
        
    return index;
}

double ArrangerGridUtils::snapToGrid(double beats) const {
    if (owner_.gridSnapBeats <= 0.0)
        return beats; // No snapping when grid is off
    return std::round(beats / owner_.gridSnapBeats) * owner_.gridSnapBeats;
}

//==============================================================================
// Time Conversion & Formatting
//==============================================================================

double ArrangerGridUtils::samplesToBeats(juce::int64 samples) const {
    double sampleRate = engine_.getSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    double tempo = projectState_.getTempo();
    if (tempo <= 0.0)
        tempo = 120.0;

    double seconds = static_cast<double>(samples) / sampleRate;
    double beatsPerSecond = tempo / 60.0;
    return seconds * beatsPerSecond;
}

juce::int64 ArrangerGridUtils::beatsToSamples(double beats) const {
    double sampleRate = engine_.getSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    double tempo = projectState_.getTempo();
    if (tempo <= 0.0)
        tempo = 120.0;

    double beatsPerSecond = tempo / 60.0;
    if (beatsPerSecond <= 0.0)
        return 0;

    double seconds = beats / beatsPerSecond;
    return static_cast<juce::int64>(seconds * sampleRate);
}

int ArrangerGridUtils::getBeatsPerBar() const {
    return projectState_.getTimeSignatureNumerator();
}

juce::String ArrangerGridUtils::formatBarBeatTick(double beats) const {
    int beatsPerBar = getBeatsPerBar();
    if (beatsPerBar <= 0)
        beatsPerBar = 4;

    int totalBeats = static_cast<int>(beats);
    int bar = (totalBeats / beatsPerBar) + 1;
    int beat = (totalBeats % beatsPerBar) + 1;

    // Tick is the fractional part (0-99 for display)
    double fractional = beats - static_cast<double>(totalBeats);
    int tick = static_cast<int>(fractional * 100.0);

    return juce::String(bar) + "." + juce::String(beat) + "." +
           juce::String(tick).paddedLeft('0', 2);
}

//==============================================================================
// Waveform Cache Management
//==============================================================================

// Helper for background processing
static WaveformCache calculatePeaks(const AudioFilePool::AudioFileHandle& handle, const juce::String& audioFilePath) {
    WaveformCache cache;
    cache.audioFilePath = audioFilePath;
    cache.samplesPerPixel = 512;

    const auto& buffer = handle.buffer;
    int numSamples = static_cast<int>(handle.lengthInSamples);
    int numChannels = handle.numChannels;

    if (numSamples <= 0 || numChannels <= 0) return cache;

    int numPeaks = (numSamples + cache.samplesPerPixel - 1) / cache.samplesPerPixel;
    cache.minPeaks.resize(numPeaks, 0.0f);
    cache.maxPeaks.resize(numPeaks, 0.0f);

    for (int peakIdx = 0; peakIdx < numPeaks; ++peakIdx) {
        int startSample = peakIdx * cache.samplesPerPixel;
        int endSample = juce::jmin(startSample + cache.samplesPerPixel, numSamples);

        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (int s = startSample; s < endSample; ++s) {
            float sample = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                sample += buffer.getSample(ch, s);
            }
            sample /= static_cast<float>(numChannels);

            minVal = juce::jmin(minVal, sample);
            maxVal = juce::jmax(maxVal, sample);
        }

        cache.minPeaks[peakIdx] = minVal;
        cache.maxPeaks[peakIdx] = maxVal;
    }

    cache.isValid = true;
    return cache;
}

void ArrangerGridUtils::buildWaveformCache(const juce::String& audioFilePath) {
    // Check if already cached
    if (waveformCache_.find(audioFilePath) != waveformCache_.end()) {
        return;
    }

    // Get audio file from pool
    auto& pool = engine_.getAudioFilePool();
    juce::File file(audioFilePath);

    auto handle = pool.getFile(file);
    if (handle && handle->isValid()) {
        // If already loaded, calculate immediately (or spawn thread)
        // For UI responsiveness, we spawn thread even for loaded files if they are large
        juce::Thread::launch([this, handle, audioFilePath]() {
            auto cache = calculatePeaks(*handle, audioFilePath);
            juce::MessageManager::callAsync([this, cache = std::move(cache), audioFilePath]() mutable {
                waveformCache_[audioFilePath] = std::move(cache);
                owner_.repaint();
            });
        });
    } else {
        // Async load
        pool.loadFileAsync(file, [this, audioFilePath](AudioFilePool::HandlePtr loadedHandle, juce::String error) {
            if (loadedHandle && loadedHandle->isValid()) {
                juce::Thread::launch([this, loadedHandle, audioFilePath]() {
                    auto cache = calculatePeaks(*loadedHandle, audioFilePath);
                    juce::MessageManager::callAsync([this, cache = std::move(cache), audioFilePath]() mutable {
                        waveformCache_[audioFilePath] = std::move(cache);
                        owner_.repaint();
                    });
                });
            }
        });
    }
}

const WaveformCache* ArrangerGridUtils::getWaveformCache(const juce::String& audioFilePath) const {
    auto it = waveformCache_.find(audioFilePath);
    if (it != waveformCache_.end() && it->second.isValid) {
        return &it->second;
    }
    return nullptr;
}

} // namespace zenith
