/*
  ==============================================================================

    Engine.cpp
    Created: 2025-11-10

    Implementation of the core audio engine.

  ==============================================================================
*/

#include "Engine.h"

//==============================================================================
ZenithEngine::ZenithEngine()
{
    // Create the audio processor graph
    audioGraph = std::make_unique<juce::AudioProcessorGraph>();

    // Add default I/O nodes
    audioInputNode = audioGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode))->nodeID;

    audioOutputNode = audioGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

    midiInputNode = audioGraph->addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode))->nodeID;

    // Register this engine as the audio callback
    deviceManager.addAudioCallback(this);
}

ZenithEngine::~ZenithEngine()
{
    // Unregister audio callback before destroying graph
    deviceManager.removeAudioCallback(this);
    audioGraph = nullptr;
}

//==============================================================================
// Audio Device Management

void ZenithEngine::setAudioDeviceSetup(const juce::AudioDeviceManager::AudioDeviceSetup& setup)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto error = deviceManager.setAudioDeviceSetup(setup, true);

    if (error.isNotEmpty())
    {
        // Handle error (show alert, log, etc.)
        DBG("Audio device setup failed: " + error);
    }
}

//==============================================================================
// Transport Control

void ZenithEngine::play()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    playing.store(true, std::memory_order_release);

    // TODO: Send play command to audio thread via lock-free FIFO
}

void ZenithEngine::stop()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    playing.store(false, std::memory_order_release);

    // TODO: Send stop command to audio thread via lock-free FIFO
}

void ZenithEngine::togglePlayback()
{
    if (isPlaying())
        stop();
    else
        play();
}

void ZenithEngine::setPlayheadPosition(double timeInSeconds)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    playheadPosition.store(timeInSeconds, std::memory_order_release);

    // TODO: Send seek command to audio thread via lock-free FIFO
}

//==============================================================================
// Audio Graph Management

juce::String ZenithEngine::addTrack(const juce::String& trackName, int numChannels)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Generate unique track ID
    auto trackId = juce::Uuid().toString();

    // Add track to project state
    projectState.addTrack(trackId, trackName, numChannels);

    // TODO: Create corresponding node in audio graph
    // For now, this is a stub - you'd create a channel strip processor:
    // - Volume control
    // - Pan control
    // - Mute/Solo
    // - Insert chain
    // - Send chain

    return trackId;
}

void ZenithEngine::removeTrack(const juce::String& trackId)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Remove from project state
    projectState.removeTrack(trackId);

    // TODO: Remove corresponding node from audio graph
}

bool ZenithEngine::loadPlugin(const juce::String& trackId, const juce::File& pluginFile)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // TODO: Implement plugin loading
    // 1. Use AudioPluginFormatManager to scan and load the plugin
    // 2. Create an AudioProcessorGraph::Node
    // 3. Insert into the track's insert chain
    // 4. Update project state

    juce::ignoreUnused(trackId, pluginFile);
    return false;
}

//==============================================================================
// AudioIODeviceCallback Implementation (AUDIO THREAD!)

void ZenithEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    // CALLED ON AUDIO THREAD!
    // Safe to access device info here, but don't allocate or lock!

    if (device != nullptr)
    {
        sampleRate.store(device->getCurrentSampleRate(), std::memory_order_relaxed);
        blockSize.store(device->getCurrentBufferSizeSamples(), std::memory_order_relaxed);
    }

    // Prepare the audio graph for playback
    audioGraph->setPlayConfigDetails(
        device->getActiveInputChannels().countNumberOfSetBits(),
        device->getActiveOutputChannels().countNumberOfSetBits(),
        device->getCurrentSampleRate(),
        device->getCurrentBufferSizeSamples()
    );

    audioGraph->prepareToPlay(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

void ZenithEngine::audioDeviceStopped()
{
    // CALLED ON AUDIO THREAD!
    audioGraph->releaseResources();
}

void ZenithEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    // ============================================================================
    // WARNING: REAL-TIME AUDIO THREAD!
    //
    // NEVER in this function or any function it calls:
    // - Allocate memory (new, malloc, std::vector::push_back, etc.)
    // - Lock mutexes (std::mutex::lock, even try_lock)
    // - Call system functions (file I/O, logging, network, etc.)
    // - Use MessageManager
    // - Take unbounded time
    //
    // Budget: ~2.9ms for 128 samples @ 44.1kHz
    // ============================================================================

    juce::ignoreUnused(context);

    // Clear output first (in case graph doesn't fill it)
    for (int i = 0; i < numOutputChannels; ++i)
    {
        if (outputChannelData[i] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }

    // Process the audio graph
    processAudioGraph(inputChannelData, numInputChannels,
                     outputChannelData, numOutputChannels,
                     numSamples);

    // Update playhead position if playing
    if (playing.load(std::memory_order_acquire))
    {
        updatePlayhead(numSamples);
    }

    // TODO: Push meter data to GUI via lock-free FIFO
    // TODO: Read parameter changes from GUI via lock-free FIFO
}

//==============================================================================
// Private Real-Time Methods

void ZenithEngine::processAudioGraph(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples)
{
    // REAL-TIME SAFE!
    // This runs on the audio thread with hard deadlines.

    // Create audio buffer wrappers (no allocation - just pointers)
    juce::AudioBuffer<float> inputBuffer(const_cast<float**>(inputChannelData),
                                        numInputChannels, numSamples);

    juce::AudioBuffer<float> outputBuffer(outputChannelData,
                                         numOutputChannels, numSamples);

    // Create MIDI buffer (pre-allocated, fixed size)
    juce::MidiBuffer midiBuffer;

    // Process the graph
    audioGraph->processBlock(outputBuffer, midiBuffer);

    // Note: AudioProcessorGraph handles the routing internally
}

void ZenithEngine::updatePlayhead(int numSamples)
{
    // REAL-TIME SAFE!
    // Simple atomic increment, no allocation or locking.

    double currentSampleRate = sampleRate.load(std::memory_order_relaxed);
    double currentPosition = playheadPosition.load(std::memory_order_relaxed);

    // Increment position
    double timeIncrement = numSamples / currentSampleRate;
    playheadPosition.store(currentPosition + timeIncrement, std::memory_order_relaxed);
}
