/*
  ==============================================================================

    ChannelMapper.cpp
    Implementation of safe audio channel mapping

  ==============================================================================
*/

#include "ChannelMapper.h"
#include <iostream>
#include <algorithm>

namespace zenith {

//==============================================================================
// ChannelMapper Implementation
//==============================================================================

ChannelMapper::ChannelMapper() {
    std::cout << "ChannelMapper: Initialized" << std::endl;
}

ChannelMapper::~ChannelMapper() {
    std::cout << "ChannelMapper: Shut down ("
              << statistics_.successfulMappings << " successful, "
              << statistics_.failedMappings << " failed)" << std::endl;
}

//==============================================================================
std::vector<ChannelMappingIssue> ChannelMapper::mapBuffer(
    const juce::AudioBuffer<float>& inputBuffer,
    const juce::AudioChannelSet& inputLayout,
    juce::AudioBuffer<float>& outputBuffer,
    const juce::AudioChannelSet& outputLayout)
{
    // Create standard mapping config
    ChannelMappingConfig config = createStandardMapping(inputLayout, outputLayout);

    // Use config-based mapping
    return mapBuffer(inputBuffer, outputBuffer, config);
}

//==============================================================================
std::vector<ChannelMappingIssue> ChannelMapper::mapBuffer(
    const juce::AudioBuffer<float>& inputBuffer,
    juce::AudioBuffer<float>& outputBuffer,
    const ChannelMappingConfig& config)
{
    std::vector<ChannelMappingIssue> issues;

    statistics_.totalMappings++;

    // Validate if enabled
    if (config.validateBeforeMapping) {
        auto validationIssues = validateConfig(config);
        issues.insert(issues.end(), validationIssues.begin(), validationIssues.end());

        if (!validationIssues.empty()) {
            statistics_.failedMappings++;
            return issues;  // Don't proceed if validation fails
        }
    }

    // Perform mapping
    auto mappingIssues = performMapping(inputBuffer, outputBuffer, config);
    issues.insert(issues.end(), mappingIssues.begin(), mappingIssues.end());

    if (issues.empty()) {
        statistics_.successfulMappings++;
    } else {
        statistics_.failedMappings++;
    }

    return issues;
}

//==============================================================================
bool ChannelMapper::isValidLayout(const juce::AudioChannelSet& layout) {
    // Check if layout is recognized
    if (layout == juce::AudioChannelSet::disabled()) {
        return false;
    }

    if (layout == juce::AudioChannelSet::canonical()) {
        return false;  // Canonical is not a specific layout
    }

    // Check channel count
    int channelCount = layout.size();
    if (channelCount <= 0 || channelCount > 64) {  // Reasonable limits
        return false;
    }

    return true;
}

//==============================================================================
std::vector<ChannelMappingIssue> ChannelMapper::validateConfig(
    const ChannelMappingConfig& config) const
{
    std::vector<ChannelMappingIssue> issues;

    // Validate input layout
    if (!isValidLayout(config.inputLayout)) {
        ChannelMappingIssue issue;
        issue.error = ChannelMappingError::InvalidInputLayout;
        issue.description = "Input layout is invalid: " +
                           getLayoutDescription(config.inputLayout);
        issue.inputChannels = config.inputLayout.size();
        issue.severity = 9;
        issues.push_back(issue);
    }

    // Validate output layout
    if (!isValidLayout(config.outputLayout)) {
        ChannelMappingIssue issue;
        issue.error = ChannelMappingError::InvalidOutputLayout;
        issue.description = "Output layout is invalid: " +
                           getLayoutDescription(config.outputLayout);
        issue.outputChannels = config.outputLayout.size();
        issue.severity = 9;
        issues.push_back(issue);
    }

    // Validate mappings
    for (const auto& mapping : config.mappings) {
        int inputChannel = mapping.first;
        int outputChannel = mapping.second;

        // Check channel bounds
        if (inputChannel < 0 || inputChannel >= config.inputLayout.size()) {
            ChannelMappingIssue issue;
            issue.error = ChannelMappingError::InvalidMapping;
            issue.description = "Input channel " + juce::String(inputChannel) +
                               " out of range (0-" +
                               juce::String(config.inputLayout.size() - 1) + ")";
            issue.inputChannels = config.inputLayout.size();
            issue.severity = 8;
            issues.push_back(issue);
        }

        if (outputChannel < 0 || outputChannel >= config.outputLayout.size()) {
            ChannelMappingIssue issue;
            issue.error = ChannelMappingError::InvalidMapping;
            issue.description = "Output channel " + juce::String(outputChannel) +
                               " out of range (0-" +
                               juce::String(config.outputLayout.size() - 1) + ")";
            issue.outputChannels = config.outputLayout.size();
            issue.severity = 8;
            issues.push_back(issue);
        }
    }

    return issues;
}

//==============================================================================
ChannelMappingConfig ChannelMapper::createStandardMapping(
    const juce::AudioChannelSet& inputLayout,
    const juce::AudioChannelSet& outputLayout)
{
    ChannelMappingConfig config;
    config.inputLayout = inputLayout;
    config.outputLayout = outputLayout;
    config.validateBeforeMapping = true;
    config.allowUpmix = true;
    config.allowDownmix = true;
    config.strictMode = false;

    int inputChannels = inputLayout.size();
    int outputChannels = outputLayout.size();

    // Create standard mappings
    if (inputChannels == outputChannels) {
        // Direct mapping (same number of channels)
        for (int i = 0; i < inputChannels; ++i) {
            config.mappings.push_back({i, i});
        }
    } else if (inputChannels == 1 && outputChannels == 2) {
        // Mono to stereo (duplicate to both channels)
        config.mappings.push_back({0, 0});
        config.mappings.push_back({0, 1});
    } else if (inputChannels == 2 && outputChannels == 1) {
        // Stereo to mono (mix both channels)
        config.mappings.push_back({0, 0});
        config.mappings.push_back({1, 0});
    } else if (inputChannels < outputChannels) {
        // Upmix: map inputs to outputs, fill rest with first channel
        for (int i = 0; i < inputChannels; ++i) {
            config.mappings.push_back({i, i});
        }
        for (int i = inputChannels; i < outputChannels; ++i) {
            config.mappings.push_back({0, i});  // Fill with channel 0
        }
    } else {
        // Downmix: mix inputs to outputs
        int ratio = inputChannels / outputChannels;
        for (int outCh = 0; outCh < outputChannels; ++outCh) {
            for (int inCh = outCh * ratio;
                 inCh < juce::jmin((outCh + 1) * ratio, inputChannels); ++inCh) {
                config.mappings.push_back({inCh, outCh});
            }
        }
    }

    return config;
}

//==============================================================================
juce::String ChannelMapper::getLayoutDescription(const juce::AudioChannelSet& layout) {
    if (layout == juce::AudioChannelSet::mono()) return "Mono";
    if (layout == juce::AudioChannelSet::stereo()) return "Stereo";
    if (layout == juce::AudioChannelSet::createLCR()) return "LCR";
    if (layout == juce::AudioChannelSet::createLCRS()) return "LCRS";
    if (layout == juce::AudioChannelSet::create5point0()) return "5.0";
    if (layout == juce::AudioChannelSet::create5point1()) return "5.1";
    if (layout == juce::AudioChannelSet::create7point0()) return "7.0";
    if (layout == juce::AudioChannelSet::create7point1()) return "7.1";
    if (layout == juce::AudioChannelSet::create7point1SDDS()) return "7.1 SDDS";
    if (layout == juce::AudioChannelSet::createDolby()) return "Dolby";
    if (layout == juce::AudioChannelSet::createQuad()) return "Quadraphonic";
    if (layout == juce::AudioChannelSet::createAmbisonic(1)) return "Ambisonic 1st Order";
    if (layout == juce::AudioChannelSet::createAmbisonic(2)) return "Ambisonic 2nd Order";

    return "Custom (" + juce::String(layout.size()) + " channels)";
}

//==============================================================================
int ChannelMapper::getRequiredOutputChannels(
    const juce::AudioChannelSet& inputLayout,
    const juce::AudioChannelSet& outputLayout)
{
    return outputLayout.size();
}

//==============================================================================
bool ChannelMapper::needsUpmix(
    const juce::AudioChannelSet& inputLayout,
    const juce::AudioChannelSet& outputLayout)
{
    return inputLayout.size() < outputLayout.size();
}

//==============================================================================
bool ChannelMapper::needsDownmix(
    const juce::AudioChannelSet& inputLayout,
    const juce::AudioChannelSet& outputLayout)
{
    return inputLayout.size() > outputLayout.size();
}

//==============================================================================
// Private Methods
//==============================================================================

std::vector<ChannelMappingIssue> ChannelMapper::performMapping(
    const juce::AudioBuffer<float>& inputBuffer,
    juce::AudioBuffer<float>& outputBuffer,
    const ChannelMappingConfig& config)
{
    std::vector<ChannelMappingIssue> issues;

    int inputChannels = config.inputLayout.size();
    int outputChannels = config.outputLayout.size();
    int numSamples = inputBuffer.getNumSamples();

    // Prepare output buffer
    outputBuffer.setSize(outputChannels, numSamples, false, true, false);
    outputBuffer.clear();

    // Detect mapping type
    if (inputChannels == 1 && outputChannels == 2 &&
        config.mappings.size() == 2 &&
        config.mappings[0] == std::make_pair(0, 0) &&
        config.mappings[1] == std::make_pair(0, 1)) {
        // Mono to stereo
        mapMonoToStereo(inputBuffer, outputBuffer);
        statistics_.upmixes++;
    }
    else if (inputChannels == 2 && outputChannels == 1 &&
             config.mappings.size() == 2 &&
             config.mappings[0] == std::make_pair(0, 0) &&
             config.mappings[1] == std::make_pair(1, 0)) {
        // Stereo to mono
        mapStereoToMono(inputBuffer, outputBuffer);
        statistics_.downmixes++;
    }
    else if (inputChannels == outputChannels) {
        // Direct mapping
        mapDirect(inputBuffer, outputBuffer, config.mappings);
        statistics_.directMappings++;
    }
    else {
        // Custom mapping
        mapDirect(inputBuffer, outputBuffer, config.mappings);

        if (inputChannels < outputChannels) {
            statistics_.upmixes++;
        } else if (inputChannels > outputChannels) {
            statistics_.downmixes++;
        } else {
            statistics_.directMappings++;
        }
    }

    return issues;
}

void ChannelMapper::mapMonoToStereo(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& output)
{
    int numSamples = input.getNumSamples();

    // Copy mono channel to both stereo channels
    for (int sample = 0; sample < numSamples; ++sample) {
        float value = input.getSample(0, sample);
        output.setSample(0, sample, value);
        output.setSample(1, sample, value);
    }
}

void ChannelMapper::mapStereoToMono(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& output)
{
    int numSamples = input.getNumSamples();

    // Mix stereo channels to mono
    for (int sample = 0; sample < numSamples; ++sample) {
        float left = input.getSample(0, sample);
        float right = input.getSample(1, sample);
        output.setSample(0, sample, (left + right) * 0.5f);
    }
}

void ChannelMapper::mapDirect(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& output,
    const std::vector<std::pair<int, int>>& mappings)
{
    int numSamples = input.getNumSamples();

    // Apply mappings (mix inputs to outputs)
    std::vector<float> accumulators(output.getNumChannels(), 0.0f);

    for (int sample = 0; sample < numSamples; ++sample) {
        // Clear accumulators
        std::fill(accumulators.begin(), accumulators.end(), 0.0f);

        // Accumulate mapped inputs
        for (const auto& mapping : mappings) {
            int inputChannel = mapping.first;
            int outputChannel = mapping.second;
            accumulators[outputChannel] += input.getSample(inputChannel, sample);
        }

        // Write to outputs
        for (int channel = 0; channel < output.getNumChannels(); ++channel) {
            output.setSample(channel, sample, accumulators[channel]);
        }
    }
}

void ChannelMapper::mapAmbient(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& output,
    const juce::AudioChannelSet& inputLayout,
    const juce::AudioChannelSet& outputLayout)
{
    // Placeholder for ambisonic decoding
    // This would require proper ambisonic decoder implementation
    // For now, use direct mapping
    std::vector<std::pair<int, int>> mappings;
    for (int i = 0; i < juce::jmin(input.getNumChannels(), output.getNumChannels()); ++i) {
        mappings.push_back({i, i});
    }
    mapDirect(input, output, mappings);
}

} // namespace zenith
