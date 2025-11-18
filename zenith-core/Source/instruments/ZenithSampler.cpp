#include "instruments/ZenithSampler.h"
#include "instruments/ZenithSamplerEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace zenith {

//==============================================================================
// Patch data structure for async loading
//==============================================================================

struct ZenithSamplerProcessor::PatchData
{
    juce::String patchName;

    struct SampleInfo
    {
        juce::File file;
        int rootNote = 60;
        int lowNote = 0;
        int highNote = 127;
        int lowVelocity = 0;
        int highVelocity = 127;
        float gain = 1.0f;
    };

    std::vector<SampleInfo> samples;

    // Default parameters
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.3f;
    float filterCutoff = 1.0f;
    float filterResonance = 0.0f;
    float tune = 0.0f;
    float gain = 0.8f;
};

//==============================================================================
// ZenithSampler
//==============================================================================

ZenithSamplerProcessor::ZenithSamplerProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize synthesiser with voices
    for (int i = 0; i < 16; ++i)
    {
        synth.addVoice(new ZenithSamplerVoice());
    }
}

ZenithSamplerProcessor::~ZenithSamplerProcessor()
{
    if (loadingThread != nullptr && loadingThread->isThreadRunning())
    {
        loadingThread->stopThread(1000);
    }
}

//==============================================================================
// Parameter layout
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout ZenithSamplerProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Amp Envelope
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f),
        0.01f, "s"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "decay", "Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f),
        0.1f, "s"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sustain", "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.7f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f),
        0.3f, "s"));

    // Filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "filterCutoff", "Filter Cutoff",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "filterResonance", "Filter Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));

    // Global controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "tune", "Tune",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f),
        0.0f, "semitones"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        0.8f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "character", "Character",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    return layout;
}

//==============================================================================
// Audio processing
//==============================================================================

void ZenithSamplerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    // Update all voices with parameter pointers
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<ZenithSamplerVoice*>(synth.getVoice(i)))
        {
            voice->setParameters(
                parameters.getRawParameterValue("attack"),
                parameters.getRawParameterValue("decay"),
                parameters.getRawParameterValue("sustain"),
                parameters.getRawParameterValue("release"),
                parameters.getRawParameterValue("filterCutoff"),
                parameters.getRawParameterValue("filterResonance"),
                parameters.getRawParameterValue("tune"),
                parameters.getRawParameterValue("gain"));
        }
    }
}

void ZenithSamplerProcessor::releaseResources()
{
    // Nothing to release
}

void ZenithSamplerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any unused channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    // Render synthesiser
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply character control (simple saturation)
    float character = *parameters.getRawParameterValue("character");
    if (character > 0.5f)
    {
        float drive = (character - 0.5f) * 4.0f; // 0-2 range
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s)
            {
                float sample = data[s] * (1.0f + drive);
                data[s] = std::tanh(sample); // Soft clipping
            }
        }
    }
}

//==============================================================================
// State save/load
//==============================================================================

void ZenithSamplerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Add current patch name to state
    state.setProperty("currentPatch", currentPatchName, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ZenithSamplerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xmlState);
            parameters.replaceState(state);

            // Restore patch
            juce::String patchName = state.getProperty("currentPatch", "");
            if (patchName.isNotEmpty())
            {
                loadPatchByName(patchName);
            }
        }
    }
}

//==============================================================================
// Patch management
//==============================================================================

bool ZenithSamplerProcessor::loadPatch(const juce::File& patchFile)
{
    if (!patchFile.existsAsFile())
        return false;

    loadPatchAsync(patchFile);
    return true;
}

bool ZenithSamplerProcessor::loadPatchByName(const juce::String& patchName)
{
    auto patchFile = ContentPaths::getInstance()
        .getPatchFile("ZenithSampler", patchName);

    return loadPatch(patchFile);
}

juce::StringArray ZenithSamplerProcessor::getAvailablePatches() const
{
    return ContentPaths::getInstance().getAvailablePatches("ZenithSampler");
}

//==============================================================================
// Async patch loading
//==============================================================================

void ZenithSamplerProcessor::loadPatchAsync(const juce::File& patchFile)
{
    // Stop any existing loading thread
    if (loadingThread != nullptr && loadingThread->isThreadRunning())
    {
        loadingThread->stopThread(1000);
    }

    isLoadingPatch.store(true);

    // Create a loading thread
    class LoadingThread : public juce::Thread
    {
    public:
        LoadingThread(ZenithSampler& owner, const juce::File& file)
            : juce::Thread("PatchLoader"), sampler(owner), patchFile(file)
        {
        }

        void run() override
        {
            auto patchData = std::make_unique<PatchData>();

            if (sampler.parsePatchFile(patchFile, *patchData))
            {
                // Apply on message thread
                juce::MessageManager::callAsync([this, data = std::move(patchData)]() mutable {
                    sampler.applyPatchData(std::move(data));
                    sampler.isLoadingPatch.store(false);
                });
            }
            else
            {
                sampler.isLoadingPatch.store(false);
                DBG("Failed to load patch: " << patchFile.getFullPathName());
            }
        }

    private:
        ZenithSampler& sampler;
        juce::File patchFile;
    };

    loadingThread = std::make_unique<LoadingThread>(*this, patchFile);
    loadingThread->startThread();
}

bool ZenithSamplerProcessor::parsePatchFile(const juce::File& patchFile, PatchData& outData)
{
    // Parse JSON patch file
    auto jsonText = patchFile.loadFileAsString();
    auto json = juce::JSON::parse(jsonText);

    if (!json.isObject())
        return false;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr)
        return false;

    // Read patch name
    outData.patchName = obj->getProperty("name").toString();
    if (outData.patchName.isEmpty())
    {
        outData.patchName = patchFile.getFileNameWithoutExtension();
    }

    // Read default parameters
    if (auto* params = obj->getProperty("parameters").getDynamicObject())
    {
        outData.attack = params->getProperty("attack");
        outData.decay = params->getProperty("decay");
        outData.sustain = params->getProperty("sustain");
        outData.release = params->getProperty("release");
        outData.filterCutoff = params->getProperty("filterCutoff");
        outData.filterResonance = params->getProperty("filterResonance");
        outData.tune = params->getProperty("tune");
        outData.gain = params->getProperty("gain");
    }

    // Read samples
    auto* samplesArray = obj->getProperty("samples").getArray();
    if (samplesArray == nullptr)
        return false;

    auto samplesDir = patchFile.getParentDirectory().getChildFile("Samples");

    for (int i = 0; i < samplesArray->size(); ++i)
    {
        auto* sampleObj = (*samplesArray)[i].getDynamicObject();
        if (sampleObj == nullptr)
            continue;

        PatchData::SampleInfo info;
        info.file = samplesDir.getChildFile(sampleObj->getProperty("file").toString());
        info.rootNote = sampleObj->getProperty("rootNote");
        info.lowNote = sampleObj->getProperty("lowNote");
        info.highNote = sampleObj->getProperty("highNote");
        info.lowVelocity = sampleObj->getProperty("lowVelocity");
        info.highVelocity = sampleObj->getProperty("highVelocity");
        info.gain = sampleObj->getProperty("gain");

        if (info.file.existsAsFile())
        {
            outData.samples.push_back(info);
        }
        else
        {
            DBG("Sample file not found: " << info.file.getFullPathName());
        }
    }

    return !outData.samples.empty();
}

void ZenithSamplerProcessor::applyPatchData(std::unique_ptr<PatchData> patchData)
{
    if (patchData == nullptr)
        return;

    // Clear existing sounds
    synth.clearSounds();

    // Load all samples
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    for (const auto& sampleInfo : patchData->samples)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(sampleInfo.file));

        if (reader != nullptr)
        {
            juce::BigInteger midiNotes;
            midiNotes.setRange(sampleInfo.lowNote, sampleInfo.highNote - sampleInfo.lowNote + 1, true);

            synth.addSound(new ZenithSamplerSound(
                sampleInfo.file.getFileNameWithoutExtension(),
                *reader,
                midiNotes,
                sampleInfo.rootNote,
                sampleInfo.lowVelocity,
                sampleInfo.highVelocity,
                patchData->attack,
                patchData->release,
                10.0)); // Max 10 seconds
        }
    }

    // Update parameters
    *parameters.getRawParameterValue("attack") = patchData->attack;
    *parameters.getRawParameterValue("decay") = patchData->decay;
    *parameters.getRawParameterValue("sustain") = patchData->sustain;
    *parameters.getRawParameterValue("release") = patchData->release;
    *parameters.getRawParameterValue("filterCutoff") = patchData->filterCutoff;
    *parameters.getRawParameterValue("filterResonance") = patchData->filterResonance;
    *parameters.getRawParameterValue("tune") = patchData->tune;
    *parameters.getRawParameterValue("gain") = patchData->gain;

    // Store patch name
    currentPatchName = patchData->patchName;

    DBG("Loaded patch: " << currentPatchName << " with " << synth.getNumSounds() << " samples");
}

//==============================================================================
// Editor
//==============================================================================

juce::AudioProcessorEditor* ZenithSamplerProcessor::createEditor()
{
    return new ZenithSamplerEditor(*this);
}

//==============================================================================
// ZenithSamplerSound
//==============================================================================

ZenithSamplerSound::ZenithSamplerSound(const juce::String& name,
                                       juce::AudioFormatReader& source,
                                       const juce::BigInteger& notes,
                                       int midiNoteForNormalPitch,
                                       int lowVel,
                                       int highVel,
                                       double attackTimeSecs,
                                       double releaseTimeSecs,
                                       double maxSampleLengthSeconds)
    : soundName(name),
      midiNotes(notes),
      rootNote(midiNoteForNormalPitch),
      lowVelocity(lowVel),
      highVelocity(highVel),
      attackTime(attackTimeSecs),
      releaseTime(releaseTimeSecs)
{
    sourceSampleRate = source.sampleRate;

    auto lengthInSamples = (int)std::min((int64)source.lengthInSamples,
                                         (int64)(maxSampleLengthSeconds * sourceSampleRate));

    data = std::make_unique<juce::AudioBuffer<float>>(
        (int)source.numChannels, lengthInSamples + 4);

    source.read(data.get(), 0, lengthInSamples + 4, 0, true, true);
}

ZenithSamplerSound::~ZenithSamplerSound()
{
}

bool ZenithSamplerSound::appliesToNote(int midiNoteNumber)
{
    return midiNotes[midiNoteNumber];
}

bool ZenithSamplerSound::appliesToChannel(int /*midiChannel*/)
{
    return true;
}

//==============================================================================
// ZenithSamplerVoice
//==============================================================================

ZenithSamplerVoice::ZenithSamplerVoice()
{
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

ZenithSamplerVoice::~ZenithSamplerVoice()
{
}

bool ZenithSamplerVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<ZenithSamplerSound*>(sound) != nullptr;
}

void ZenithSamplerVoice::setParameters(float* attack, float* decay, float* sustain, float* release,
                                       float* filterCutoff, float* filterResonance,
                                       float* tune, float* gain)
{
    attackParam = attack;
    decayParam = decay;
    sustainParam = sustain;
    releaseParam = release;
    filterCutoffParam = filterCutoff;
    filterResonanceParam = filterResonance;
    tuneParam = tune;
    gainParam = gain;
}

void ZenithSamplerVoice::startNote(int midiNoteNumber, float vel,
                                  juce::SynthesiserSound* s,
                                  int /*currentPitchWheelPosition*/)
{
    if (auto* sound = dynamic_cast<ZenithSamplerSound*>(s))
    {
        // Check velocity layer
        int midiVelocity = static_cast<int>(vel * 127.0f);
        if (!sound->appliesToVelocity(midiVelocity))
        {
            clearCurrentNote();
            return;
        }

        velocity = vel;

        // Calculate pitch ratio using the sample's actual root note
        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / sound->getSampleRate();

        auto rootCyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(sound->getRootNote());
        auto rootCyclesPerSample = rootCyclesPerSecond / sound->getSampleRate();

        pitchRatio = cyclesPerSample / rootCyclesPerSample;
        sourceSamplePosition = 0.0;

        // Update envelope
        if (attackParam != nullptr && releaseParam != nullptr)
        {
            ampEnvParams.attack = *attackParam;
            ampEnvParams.decay = decayParam ? *decayParam : 0.1f;
            ampEnvParams.sustain = sustainParam ? *sustainParam : 0.7f;
            ampEnvParams.release = *releaseParam;
            ampEnvelope.setParameters(ampEnvParams);
        }

        ampEnvelope.noteOn();

        // Prepare filter
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = getSampleRate();
        spec.maximumBlockSize = 512;
        spec.numChannels = 2;
        filter.prepare(spec);
        filter.reset();
    }
}

void ZenithSamplerVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnvelope.noteOff();
    }
    else
    {
        clearCurrentNote();
        ampEnvelope.reset();
    }
}

void ZenithSamplerVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{
}

void ZenithSamplerVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
}

void ZenithSamplerVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                        int startSample, int numSamples)
{
    if (auto* sound = dynamic_cast<ZenithSamplerSound*>(getCurrentlyPlayingSound().get()))
    {
        auto& data = *sound->getAudioData();
        const int dataLength = data.getNumSamples();

        // Apply tuning
        float tuning = tuneParam ? *tuneParam : 0.0f;
        float pitchMultiplier = std::pow(2.0f, tuning / 12.0f);
        double finalPitchRatio = pitchRatio * pitchMultiplier;

        // Get filter parameters
        float cutoff = filterCutoffParam ? *filterCutoffParam : 1.0f;
        float resonance = filterResonanceParam ? *filterResonanceParam : 0.0f;

        // Map cutoff to Hz (20Hz - 20kHz)
        float cutoffHz = 20.0f + cutoff * cutoff * 19980.0f;
        filter.setCutoffFrequency(cutoffHz);
        filter.setResonance(resonance * 0.9f + 0.1f); // 0.1 - 1.0

        // Get gain
        float gainValue = gainParam ? *gainParam : 0.8f;

        for (int i = 0; i < numSamples; ++i)
        {
            auto pos = (int)sourceSamplePosition;

            if (pos >= dataLength)
            {
                stopNote(0.0f, false);
                break;
            }

            // Linear interpolation
            auto alpha = (float)(sourceSamplePosition - pos);
            auto invAlpha = 1.0f - alpha;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            {
                auto* channelData = data.getReadPointer(ch % data.getNumChannels());
                auto sample = (channelData[pos] * invAlpha + channelData[pos + 1] * alpha);

                // Apply envelope
                sample *= ampEnvelope.getNextSample();

                // Apply velocity
                sample *= velocity;

                // Apply filter
                sample = filter.processSample(ch, sample);

                // Apply gain
                sample *= gainValue;

                // Add to output
                outputBuffer.addSample(ch, startSample + i, sample);
            }

            sourceSamplePosition += finalPitchRatio;

            if (!ampEnvelope.isActive())
            {
                stopNote(0.0f, false);
                break;
            }
        }
    }
}

//==============================================================================
// ZenithSampler (Instrument Wrapper)
//==============================================================================

ZenithSampler::ZenithSampler()
    : InstrumentBase(std::make_unique<ZenithSamplerProcessor>(), createMetadata())
{
    // Map parameter IDs to JUCE indices
    mapParameter("attack", ZenithSamplerProcessor::Attack);
    mapParameter("decay", ZenithSamplerProcessor::Decay);
    mapParameter("sustain", ZenithSamplerProcessor::Sustain);
    mapParameter("release", ZenithSamplerProcessor::Release);
    mapParameter("filter_cutoff", ZenithSamplerProcessor::FilterCutoff);
    mapParameter("filter_resonance", ZenithSamplerProcessor::FilterResonance);
    mapParameter("tune", ZenithSamplerProcessor::Tune);
    mapParameter("gain", ZenithSamplerProcessor::Gain);
    mapParameter("character", ZenithSamplerProcessor::Character);

    // Register presets (patches)
    registerPresets();
}

juce::AudioProcessorValueTreeState* ZenithSampler::getParameterState()
{
    if (auto* proc = dynamic_cast<ZenithSamplerProcessor*>(getAudioProcessor()))
    {
        return &proc->getParameters();
    }
    return nullptr;
}

InstrumentMetadata ZenithSampler::createMetadata()
{
    InstrumentMetadata metadata;
    metadata.instrumentId = "zenith_sampler";
    metadata.name = "Zenith Sampler";
    metadata.category = "Sampler";
    metadata.description = "Multi-sample instrument with envelope, filter, and velocity layers";

    // Envelope parameters
    {
        ParameterMetadata param;
        param.id = "attack";
        param.name = "Attack";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.01f;
        param.minValue = 0.001f;
        param.maxValue = 5.0f;
        param.units = "s";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "decay";
        param.name = "Decay";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.1f;
        param.minValue = 0.001f;
        param.maxValue = 5.0f;
        param.units = "s";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "sustain";
        param.name = "Sustain";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.7f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "release";
        param.name = "Release";
        param.category = "Envelope";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.3f;
        param.minValue = 0.001f;
        param.maxValue = 10.0f;
        param.units = "s";
        metadata.parameters.push_back(param);
    }

    // Filter parameters
    {
        ParameterMetadata param;
        param.id = "filter_cutoff";
        param.name = "Filter Cutoff";
        param.category = "Filter";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 1.0f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "filter_resonance";
        param.name = "Filter Resonance";
        param.category = "Filter";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.0f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }

    // Global parameters
    {
        ParameterMetadata param;
        param.id = "tune";
        param.name = "Tune";
        param.category = "Global";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.0f;
        param.minValue = -1.0f;
        param.maxValue = 1.0f;
        param.units = "semitones";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "gain";
        param.name = "Gain";
        param.category = "Global";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.8f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }
    {
        ParameterMetadata param;
        param.id = "character";
        param.name = "Character";
        param.category = "Global";
        param.type = ParameterMetadata::Type::Float;
        param.defaultValue = 0.0f;
        param.minValue = 0.0f;
        param.maxValue = 1.0f;
        param.units = "%";
        metadata.parameters.push_back(param);
    }

    // Macros for high-level control
    {
        MacroMetadata macro;
        macro.id = "macro_brightness";
        macro.name = "Brightness";
        macro.description = "Controls filter cutoff and character";
        macro.targets.push_back({.parameterId = "filter_cutoff", .amount = 0.8f});
        macro.targets.push_back({.parameterId = "character", .amount = 0.5f});
        metadata.macros.push_back(macro);
    }
    {
        MacroMetadata macro;
        macro.id = "macro_response";
        macro.name = "Response";
        macro.description = "Controls envelope attack and release";
        macro.targets.push_back({.parameterId = "attack", .amount = 0.7f});
        macro.targets.push_back({.parameterId = "release", .amount = 0.7f});
        metadata.macros.push_back(macro);
    }

    return metadata;
}

void ZenithSampler::registerPresets()
{
    // TODO: Load .zpatch files from content directory and register as presets
    // For now, just register a default preset
    std::map<juce::String, float> defaultPreset;
    defaultPreset["attack"] = 0.01f;
    defaultPreset["decay"] = 0.1f;
    defaultPreset["sustain"] = 0.7f;
    defaultPreset["release"] = 0.3f;
    defaultPreset["filter_cutoff"] = 1.0f;
    defaultPreset["filter_resonance"] = 0.0f;
    defaultPreset["tune"] = 0.0f;
    defaultPreset["gain"] = 0.8f;
    defaultPreset["character"] = 0.0f;

    registerPreset("default", "Default", defaultPreset);
}

} // namespace zenith
