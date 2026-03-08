/*
  ==============================================================================

    AudioGlitchDetector.cpp
    Implementation of real-time audio glitch detection

  ==============================================================================
*/

#include "AudioGlitchDetector.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
// AudioGlitchDetector Implementation
//==============================================================================

AudioGlitchDetector::AudioGlitchDetector() {
    std::cout << "AudioGlitchDetector: Initialized" << std::endl;
}

AudioGlitchDetector::~AudioGlitchDetector() {
    std::cout << "AudioGlitchDetector: Shut down ("
              << statistics_.totalGlitches.load() << " glitches detected)" << std::endl;
}

//==============================================================================
std::vector<GlitchEvent> AudioGlitchDetector::analyzeBuffer(
    const juce::AudioBuffer<float>& buffer,
    double sampleRate)
{
    std::vector<GlitchEvent> allGlitches;

    // Run all enabled detectors
    if (config_.enableDiscontinuityDetection) {
        auto glitches = detectDiscontinuities(buffer);
        allGlitches.insert(allGlitches.end(), glitches.begin(), glitches.end());
    }

    if (config_.enableNaNInfinityDetection) {
        auto glitches = detectNaNInfinity(buffer);
        allGlitches.insert(allGlitches.end(), glitches.begin(), glitches.end());
    }

    if (config_.enableDCOffsetDetection) {
        auto glitches = detectDCOffset(buffer);
        allGlitches.insert(allGlitches.end(), glitches.begin(), glitches.end());
    }

    if (config_.enableClippingDetection) {
        auto glitches = detectClipping(buffer);
        allGlitches.insert(allGlitches.end(), glitches.begin(), glitches.end());
    }

    if (config_.enableSuddenLevelChangeDetection) {
        auto glitches = detectSuddenLevelChanges(buffer);
        allGlitches.insert(allGlitches.end(), glitches.begin(), glitches.end());
    }

    if (config_.enableDropoutDetection) {
        auto glitches = detectDropouts(buffer);
        allGlitches.insert(allGlitches.end(), glitches.begin(), glitches.end());
    }

    return allGlitches;
}

//==============================================================================
std::vector<GlitchEvent> AudioGlitchDetector::processBuffer(
    const juce::AudioBuffer<float>& buffer)
{
    return analyzeBuffer(buffer, 48000.0);
}

//==============================================================================
std::vector<GlitchEvent> AudioGlitchDetector::getRecentGlitches() const {
    return glitchHistory_;
}

//==============================================================================
void AudioGlitchDetector::clearHistory() {
    glitchHistory_.clear();
}

//==============================================================================
void AudioGlitchDetector::resetStatistics() {
    statistics_.totalGlitches.store(0);
    statistics_.discontinuities.store(0);
    statistics_.nanValues.store(0);
    statistics_.infinityValues.store(0);
    statistics_.dcOffsets.store(0);
    statistics_.clips.store(0);
    statistics_.dropouts.store(0);

    previousSamples_.clear();
    previousLevels_.clear();

    std::cout << "AudioGlitchDetector: Statistics reset" << std::endl;
}

//==============================================================================
// Private Methods
//==============================================================================

std::vector<GlitchEvent> AudioGlitchDetector::detectDiscontinuities(
    const juce::AudioBuffer<float>& buffer)
{
    std::vector<GlitchEvent> glitches;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    // Resize previous samples if channel count changed
    if (static_cast<int>(previousSamples_.size()) != channels) {
        previousSamples_.resize(channels, 0.0f);
    }

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);
        float& prevSample = previousSamples_[channel];

        for (int i = 0; i < samples; ++i) {
            float diff = std::abs(data[i] - prevSample);

            if (diff > config_.discontinuityThreshold) {
                GlitchEvent glitch;
                glitch.type = GlitchType::Discontinuity;
                glitch.description = "Sample change: " +
                                   juce::String(diff, 3) + " (threshold: " +
                                   juce::String(config_.discontinuityThreshold, 3) + ")";
                glitch.channel = channel;
                glitch.samplePosition = i;
                glitch.value = data[i];
                glitch.severity = juce::jmin(10.0, diff / config_.discontinuityThreshold * 2.0);

                glitches.push_back(glitch);
                recordGlitch(glitch);
            }

            prevSample = data[i];
        }
    }

    return glitches;
}

std::vector<GlitchEvent> AudioGlitchDetector::detectNaNInfinity(
    const juce::AudioBuffer<float>& buffer)
{
    std::vector<GlitchEvent> glitches;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);

        for (int i = 0; i < samples; ++i) {
            if (std::isnan(data[i])) {
                GlitchEvent glitch;
                glitch.type = GlitchType::NaN;
                glitch.description = "Not-a-Number detected";
                glitch.channel = channel;
                glitch.samplePosition = i;
                glitch.value = 0.0;
                glitch.severity = 10.0;

                glitches.push_back(glitch);
                recordGlitch(glitch);
            } else if (std::isinf(data[i])) {
                GlitchEvent glitch;
                glitch.type = GlitchType::Infinity;
                glitch.description = "Infinite value detected";
                glitch.channel = channel;
                glitch.samplePosition = i;
                glitch.value = data[i] > 0 ? 1e30 : -1e30;
                glitch.severity = 10.0;

                glitches.push_back(glitch);
                recordGlitch(glitch);
            }
        }
    }

    return glitches;
}

std::vector<GlitchEvent> AudioGlitchDetector::detectDCOffset(
    const juce::AudioBuffer<float>& buffer)
{
    std::vector<GlitchEvent> glitches;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);

        // Calculate average
        double sum = 0.0;
        for (int i = 0; i < samples; ++i) {
            sum += data[i];
        }
        double average = sum / samples;

        if (std::abs(average) > config_.dcOffsetThreshold) {
            GlitchEvent glitch;
            glitch.type = GlitchType::DCOffset;
            glitch.description = "DC offset: " + juce::String(average * 100, 2) + "%";
            glitch.channel = channel;
            glitch.samplePosition = 0;
            glitch.value = average;
            glitch.severity = juce::jmin(10.0, std::abs(average) / config_.dcOffsetThreshold * 3.0);

            glitches.push_back(glitch);
            recordGlitch(glitch);
        }
    }

    return glitches;
}

std::vector<GlitchEvent> AudioGlitchDetector::detectClipping(
    const juce::AudioBuffer<float>& buffer)
{
    std::vector<GlitchEvent> glitches;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);

        for (int i = 0; i < samples; ++i) {
            if (std::abs(data[i]) > config_.clippingThreshold) {
                GlitchEvent glitch;
                glitch.type = GlitchType::Clipping;
                glitch.description = "Clipping at " + juce::String(std::abs(data[i]) * 100, 1) + "%";
                glitch.channel = channel;
                glitch.samplePosition = i;
                glitch.value = data[i];
                glitch.severity = juce::jmin(10.0, (std::abs(data[i]) - config_.clippingThreshold) * 50.0);

                glitches.push_back(glitch);
                recordGlitch(glitch);
            }
        }
    }

    return glitches;
}

std::vector<GlitchEvent> AudioGlitchDetector::detectSuddenLevelChanges(
    const juce::AudioBuffer<float>& buffer)
{
    std::vector<GlitchEvent> glitches;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    // Resize previous levels if needed
    if (static_cast<int>(previousLevels_.size()) != channels) {
        previousLevels_.resize(channels, -100.0);
    }

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);
        double& prevLevel = previousLevels_[channel];

        for (int i = 0; i < samples; ++i) {
            // Calculate current level in dB
            double level = 20.0 * std::log10(std::max(1e-10, static_cast<double>(std::abs(data[i]))));

            double levelChange = std::abs(level - prevLevel);

            if (levelChange > config_.suddenLevelChangeThreshold) {
                GlitchEvent glitch;
                glitch.type = GlitchType::SuddenLevelChange;
                glitch.description = "Level change: " + juce::String(levelChange, 1) + " dB";
                glitch.channel = channel;
                glitch.samplePosition = i;
                glitch.value = data[i];
                glitch.severity = juce::jmin(10.0, levelChange / config_.suddenLevelChangeThreshold * 3.0);

                glitches.push_back(glitch);
                recordGlitch(glitch);
            }

            prevLevel = level;
        }
    }

    return glitches;
}

std::vector<GlitchEvent> AudioGlitchDetector::detectDropouts(
    const juce::AudioBuffer<float>& buffer)
{
    std::vector<GlitchEvent> glitches;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);

        // Calculate RMS
        double sumSquares = 0.0;
        for (int i = 0; i < samples; ++i) {
            sumSquares += data[i] * data[i];
        }
        double rms = std::sqrt(sumSquares / samples);

        // Convert to dB
        double levelDb = 20.0 * std::log10(std::max(1e-10, rms));

        if (levelDb < config_.dropoutThreshold) {
            GlitchEvent glitch;
            glitch.type = GlitchType::Dropout;
            glitch.description = "Signal dropout: " + juce::String(levelDb, 1) + " dB";
            glitch.channel = channel;
            glitch.samplePosition = 0;
            glitch.value = rms;
            glitch.severity = juce::jmin(10.0, (config_.dropoutThreshold - levelDb) / 20.0);

            glitches.push_back(glitch);
            recordGlitch(glitch);
        }
    }

    return glitches;
}

void AudioGlitchDetector::recordGlitch(const GlitchEvent& event) {
    // Update statistics
    statistics_.totalGlitches++;

    switch (event.type) {
        case GlitchType::Discontinuity:
            statistics_.discontinuities++;
            break;
        case GlitchType::NaN:
            statistics_.nanValues++;
            break;
        case GlitchType::Infinity:
            statistics_.infinityValues++;
            break;
        case GlitchType::DCOffset:
            statistics_.dcOffsets++;
            break;
        case GlitchType::Clipping:
            statistics_.clips++;
            break;
        case GlitchType::Dropout:
            statistics_.dropouts++;
            break;
        default:
            break;
    }

    // Add to history
    glitchHistory_.push_back(event);

    // Limit history size
    if (static_cast<int>(glitchHistory_.size()) > maxHistorySize) {
        glitchHistory_.erase(glitchHistory_.begin());
    }

    // Log critical glitches
    if (event.severity >= 8.0) {
        std::cerr << "AudioGlitchDetector: " << event.toString() << std::endl;
    }
}

} // namespace zenith
