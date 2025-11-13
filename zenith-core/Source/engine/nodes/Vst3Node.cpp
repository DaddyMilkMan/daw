/**
 * @file Vst3Node.cpp
 * @brief VST3 plugin wrapper implementation
 */

#include "Vst3Node.h"

namespace zenith {

Vst3Node::Vst3Node(std::unique_ptr<juce::AudioPluginInstance> instance)
    : plugin_(std::move(instance))
{
    jassert(plugin_ != nullptr); // Plugin must be valid
}

Vst3Node::~Vst3Node()
{
    // Plugin automatically released via unique_ptr
}

//==============================================================================
// IDspNode Interface
//==============================================================================

void Vst3Node::prepareToPlay(double sampleRate, int blockSize, int numChannels)
{
    juce::ignoreUnused(numChannels);

    if (!plugin_)
        return;

    // Configure plugin for playback
    plugin_->setPlayHead(nullptr); // TODO: Wire Engine playhead later
    plugin_->prepareToPlay(sampleRate, blockSize);

    // Pre-allocate MIDI buffer space (avoid RT allocations)
    midi_.ensureSize(blockSize * 4); // Conservative estimate
    midi_.clear();
}

void Vst3Node::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Early exit if bypassed
    if (bypassed_.load(std::memory_order_acquire))
        return;

    if (!plugin_)
        return;

    // Clear MIDI buffer (no MIDI events in Phase 1)
    midi_.clear();

    // Delegate to plugin (RT-safe if plugin is RT-safe)
    plugin_->processBlock(buffer, midi_);

    juce::ignoreUnused(numSamples); // Plugin uses buffer.getNumSamples()
}

void Vst3Node::releaseResources()
{
    if (!plugin_)
        return;

    plugin_->releaseResources();
    midi_.clear();
}

void Vst3Node::setBypassed(bool bypassed)
{
    bypassed_.store(bypassed, std::memory_order_release);
}

bool Vst3Node::isBypassed() const noexcept
{
    return bypassed_.load(std::memory_order_acquire);
}

//==============================================================================
// Plugin Access
//==============================================================================

juce::String Vst3Node::getPluginName() const
{
    if (!plugin_)
        return "Invalid Plugin";

    return plugin_->getName();
}

} // namespace zenith
