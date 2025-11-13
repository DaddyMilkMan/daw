/**
 * @file Vst3Node.h
 * @brief VST3 plugin wrapper implementing IDspNode interface
 *
 * Threading model:
 * - Construction/destruction: MESSAGE THREAD
 * - prepareToPlay / releaseResources: MESSAGE THREAD
 * - processBlock: AUDIO THREAD (RT-safe, delegates to plugin)
 * - Plugin instance owned by Vst3Node
 */

#pragma once

#include "../../../include/engine/IDspNode.h"
#include <JuceHeader.h>
#include <atomic>
#include <memory>

namespace zenith {

/**
 * @brief VST3 plugin host wrapper
 *
 * Wraps a JUCE AudioPluginInstance to implement the IDspNode interface.
 * Allows VST3 plugins to be inserted into the track FX chain.
 *
 * Lifecycle:
 * 1. Create plugin instance externally (via AudioPluginFormatManager)
 * 2. Pass ownership to Vst3Node constructor
 * 3. Vst3Node manages plugin lifetime (prepareToPlay, processBlock, releaseResources)
 * 4. Destruction releases plugin
 *
 * RT Safety:
 * - processBlock delegates to plugin->processBlock (RT-safe if plugin is RT-safe)
 * - No allocations in Vst3Node itself (reuses midi_ buffer)
 * - Bypass check via atomic bool (lock-free)
 */
class Vst3Node : public IDspNode
{
public:
    /**
     * @brief Construct VST3 node with plugin instance
     * @param instance Plugin instance (takes ownership)
     *
     * MESSAGE THREAD only
     * Instance must be fully created and valid
     */
    explicit Vst3Node(std::unique_ptr<juce::AudioPluginInstance> instance);

    ~Vst3Node() override;

    //==========================================================================
    // IDspNode Interface
    //==========================================================================

    void prepareToPlay(double sampleRate, int blockSize, int numChannels) override;
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples) override;
    void releaseResources() override;
    void setBypassed(bool bypassed) override;
    bool isBypassed() const noexcept override;

    //==========================================================================
    // Plugin Access (MESSAGE THREAD)
    //==========================================================================

    /**
     * @brief Get underlying plugin instance
     * @return Pointer to plugin (never null)
     *
     * MESSAGE THREAD only
     * Use for parameter access, editor creation, etc.
     */
    juce::AudioPluginInstance* getPlugin() noexcept { return plugin_.get(); }

    /**
     * @brief Get plugin name
     * @return Plugin display name
     */
    juce::String getPluginName() const;

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    /// Plugin instance (owned, never null after construction)
    std::unique_ptr<juce::AudioPluginInstance> plugin_;

    /// Bypass state (atomic for thread-safe reads)
    std::atomic<bool> bypassed_{false};

    /// MIDI buffer (reused every processBlock call, no RT allocations)
    juce::MidiBuffer midi_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vst3Node)
};

} // namespace zenith
