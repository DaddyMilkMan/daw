/*
  ==============================================================================

    ZenithUltraSynth.cpp
    Created: [Date] Author: Claude AI
    Implementation of the main Zenith Ultra Synth processor

  ==============================================================================
*/

#include "ZenithUltraSynth.h"
#include "../ZenithPolySynth.h"

namespace Zenith
{

ZenithUltraSynthProcessor::ZenithUltraSynthProcessor()
    : synthesisMode_(SynthesisMode::Subtractive)
    , voiceCount_(64)
    , unisonSize_(1)
    , mpeEnabled_(false)
    , sampleRate_(44100.0)
    , bufferSize_(512)
    , lastCpuUsage_(0.0f)
    , currentPresetId_("")
{
    initializeEngines();
    setVoiceCount(voiceCount_);

    // Initialize MPE
    enableMPE(false);
}

ZenithUltraSynthProcessor::~ZenithUltraSynthProcessor()
{
    clearAllVoices();
    shutdown();
}

void ZenithUltraSynthProcessor::initializeEngines()
{
    // Initialize component engines
    try {
        subtractiveEngine_ = std::make_unique<SubtractiveEngine>();
        subtractiveEngine_->initialize(sampleRate_, bufferSize_);

        physicalModelingEngine_ = std::make_unique<PhysicalModelingEngine>();
        physicalModelingEngine_->initialize(sampleRate_, bufferSize_);

        neuralSynthEngine_ = std::make_unique<NeuralSynthEngine>();
        neuralSynthEngine_->initialize(sampleRate_, bufferSize_);

        wavetableEngine_ = std::make_ptr<AdvancedWavetableEngine>();
        wavetableEngine_->initialize(sampleRate_, bufferSize_);
    }
    catch (const std::exception& e) {
        DBG("ZenithUltraSynthProcessor: Error initializing engines - " << e.what());
    }
}

void ZenithUltraSynthProcessor::setSynthesisMode(SynthesisMode mode)
{
    if (synthesisMode_ != mode)
    {
        synthesisMode_ = mode;

        // Update all voices
        for (int i = 0; i < getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<ZenithUltraSynthVoice*>(getVoice(i)))
            {
                voice->setSynthesisMode(mode);
            }
        }
    }
}

ZenithUltraSynthProcessor::SynthesisMode ZenithUltraSynthProcessor::getSynthesisMode() const
{
    return synthesisMode_;
}

void ZenithUltraSynthProcessor::setVoiceCount(int voices)
{
    voiceCount_ = juce::jlimit(1, maxVoices, voices);

    // Synchronize with voice manager
    setNumVoices(voiceCount_);

    // Update voice parameters
    updateVoiceParameters();
}

int ZenithUltraSynthProcessor::getVoiceCount() const
{
    return voiceCount_;
}

void ZenithUltraSynthProcessor::setUnisonSize(int size)
{
    unisonSize_ = juce::jlimit(1, maxUnison, size);
    updateVoiceParameters();
}

int ZenithUltraSynthProcessor::getUnisonSize() const
{
    return unisonSize_;
}

void ZenithUltraSynthProcessor::loadPreset(const juce::String& presetId)
{
    if (presetDatabase_.contains(presetId))
    {
        currentPresetId_ = presetId;

        // Apply preset parameters
        const auto presetData = presetDatabase_[presetId];

        // Update synthesis mode
        if (presetData.hasProperty("synthesisMode"))
        {
            auto modeString = presetData.getProperty("synthesisMode").toString();
            if (modeString == "Subtractive") synthesisMode_ = SynthesisMode::Subtractive;
            else if (modeString == "PhysicalModeling") synthesisMode_ = SynthesisMode::PhysicalModeling;
            else if (modeString == "Neural") synthesisMode_ = SynthesisMode::Neural;
            else if (modeString == "Wavetable") synthesisMode_ = SynthesisMode::Wavetable;
            else if (modeString == "Hybrid") synthesisMode_ = SynthesisMode::Hybrid;
        }

        // Update voice parameters
        updateVoiceParameters();

        // Update other parameters based on preset
        if (presetData.hasProperty("voiceCount"))
            setVoiceCount(presetData.getProperty("voiceCount"));

        if (presetData.hasProperty("unisonSize"))
            setUnisonSize(presetData.getProperty("unisonSize"));

        // Trigger callback or update UI if needed
        if (onPresetChanged_)
            onPresetChanged_();
    }
}

void ZenithUltraSynthProcessor::savePreset(const juce::String& name)
{
    auto presetId = name.trim().toLowerCase().replace(" ", "_");
    auto presetData = juce::ValueTree("Preset");

    // Store preset metadata
    presetData.setProperty("name", name, nullptr);
    presetData.setProperty("synthesisMode", juce::String(synthesisMode_), nullptr);
    presetData.setProperty("voiceCount", voiceCount_, nullptr);
    presetData.setProperty("unisonSize", unisonSize_, nullptr);
    presetData.setProperty("timestamp", juce::Time::getCurrentTime().toString(), nullptr);

    // Store current parameter states
    // This would be expanded to store all current parameters

    // Add to database
    presetDatabase_[presetId] = presetData;
    currentPresetId_ = presetId;
}

juce::StringArray ZenithUltraSynthProcessor::getPresetList() const
{
    juce::StringArray presets;

    for (const auto& entry : presetDatabase_)
    {
        const auto& data = entry.getValue();
        if (data.hasProperty("name"))
        {
            presets.add(data.getProperty("name").toString());
        }
        else
        {
            presets.add(entry.getKey());
        }
    }

    return presets;
}

void ZenithUltraSynthProcessor::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                               int startSample, int numSamples)
{
    if (numSamples <= 0)
        return;

    // Start CPU monitoring
    cpuMeter_.start();

    // Process modulation
    processGlobalModulation(outputBuffer, numSamples);

    // Render audio from voices
    juce::MPESynthesiser::renderNextBlock(outputBuffer, startSample, numSamples);

    // Update CPU usage
    lastCpuUsage_ = cpuMeter_.stop();
}

void ZenithUltraSynthProcessor::setSampleRate(double newSampleRate)
{
    if (sampleRate_ != newSampleRate)
    {
        sampleRate_ = newSampleRate;

        // Update all engines
        if (subtractiveEngine_) subtractiveEngine_->setSampleRate(sampleRate_);
        if (physicalModelingEngine_) physicalModelingEngine_->setSampleRate(sampleRate_);
        if (neuralSynthEngine_) neuralSynthEngine_->setSampleRate(sampleRate_);
        if (wavetableEngine_) wavetableEngine_->setSampleRate(sampleRate_);

        // Update voices
        for (int i = 0; i < getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<ZenithUltraSynthVoice*>(getVoice(i)))
            {
                voice->setSampleRate(sampleRate_);
            }
        }
    }
}

void ZenithUltraSynthProcessor::enableMPE(bool enabled)
{
    mpeEnabled_ = enabled;

    if (enabled)
    {
        // Initialize MPE zones
        setMPEZone(0, 15);  // 16 channels for MPE
    }
    else
    {
        // Reset to single channel mode
        setMPEZone(0, 0);
    }
}

bool ZenithUltraSynthProcessor::isMPEEnabled() const
{
    return mpeEnabled_;
}

void ZenithUltraSynthProcessor::reset()
{
    // Clear all voices
    clearAllVoices();

    // Reset engines
    if (subtractiveEngine_) subtractiveEngine_->reset();
    if (physicalModelingEngine_) physicalModelingEngine_->reset();
    if (neuralSynthEngine_) neuralSynthEngine_->reset();
    if (wavetableEngine_) wavetableEngine_->reset();

    // Reset parameters
    synthesisMode_ = SynthesisMode::Subtractive;
    voiceCount_ = 64;
    unisonSize_ = 1;

    updateVoiceParameters();
}

void ZenithUltraSynthProcessor::clearAllVoices()
{
    for (int i = getNumVoices() - 1; i >= 0; --i)
    {
        releaseVoice(getVoice(i));
    }
}

void ZenithUltraSynthProcessor::setBufferSize(int bufferSize)
{
    bufferSize_ = bufferSize;

    // Update engines
    if (subtractiveEngine_) subtractiveEngine_->setBufferSize(bufferSize_);
    if (physicalModelingEngine_) physicalModelingEngine_->setBufferSize(bufferSize_);
    if (neuralSynthEngine_) neuralSynthEngine_->setBufferSize(bufferSize);
    if (wavetableEngine_) wavetableEngine_->setBufferSize(bufferSize);
}

int ZenithUltraSynthProcessor::getBufferSize() const
{
    return bufferSize_;
}

float ZenithUltraSynthProcessor::getCpuUsage() const
{
    return lastCpuUsage_;
}

int ZenithUltraSynthProcessor::getActiveVoices() const
{
    return getNumActiveVoices();
}

std::unique_ptr<juce::Component> ZenithUltraSynthProcessor::createEditor()
{
    // Create main editor component
    auto editor = std::make_unique<juce::Component>();

    // Add individual engine editors
    if (wavetableEditor_ == nullptr)
    {
        wavetableEditor_ = std::make_unique<WavetableEditor>();
    }

    if (visualCanvas_ == nullptr)
    {
        visualCanvas_ = std::make_unique<VisualSynthesisCanvas>();
    }

    return editor;
}

WavetableEditor* ZenithUltraSynthProcessor::getWavetableEditor()
{
    if (wavetableEditor_ == nullptr)
    {
        wavetableEditor_ = std::make_unique<WavetableEditor>();
    }

    return wavetableEditor_.get();
}

VisualSynthesisCanvas* ZenithUltraSynthProcessor::getVisualCanvas()
{
    if (visualCanvas_ == nullptr)
    {
        visualCanvas_ = std::make_unique<VisualSynthesisCanvas>();
    }

    return visualCanvas_.get();
}

void ZenithUltraSynthProcessor::updateVoiceParameters()
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ZenithUltraSynthVoice*>(getVoice(i)))
        {
            voice->setSynthesisMode(synthesisMode_);
            voice->setVoiceIndex(i);
            voice->setUnisonDetune(0.01f * (unisonSize_ - 1)); // Small detune for unison
        }
    }
}

void ZenithUltraSynthProcessor::processGlobalModulation(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Process LFOs, envelopes, and other global modulation sources
    // This would be implemented based on the specific modulation requirements

    // Placeholder for modulation processing
    // In a real implementation, this would handle:
    // - Global LFOs
    // - Master envelopes
    // - Modulation matrix
    // - Sidechain processing
    // etc.
}

} // namespace Zenith