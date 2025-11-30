#include "ZenithSampler.h"
#include "ZenithSamplerEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace zenith {

//==============================================================================
// Sample bank data structure for async loading
//==============================================================================

struct ZenithSamplerProcessor::SampleBankData {
  juce::String bankName;
  juce::String category; // "drums", "808", "keys", "fx"

  struct SampleRegion {
    juce::File file;
    int rootNote = 60;
    int lowNote = 0;
    int highNote = 127;
    int lowVelocity = 0;
    int highVelocity = 127;
    ZenithSamplerSound::LoopMode loopMode = ZenithSamplerSound::LoopMode::None;
    float gain = 1.0f;
    float tune = 0.0f; // Per-sample tuning in semitones
  };

  std::vector<SampleRegion> regions;

  // Default parameters
  float attack = 0.01f;
  float decay = 0.1f;
  float sustain = 0.7f;
  float release = 0.3f;
  float filterCutoff = 1.0f;
  float filterResonance = 0.0f;
  float sampleStartOffset = 0.0f;
  float pitchFine = 0.0f;
  float pitchSemitones = 0.0f;
  float globalPan = 0.5f;
  float globalGain = 0.8f;
};

//==============================================================================
// ZenithSampler
//==============================================================================

ZenithSamplerProcessor::ZenithSamplerProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout()) {
  // Initialize synthesiser with voices
  for (int i = 0; i < 16; ++i) {
    synth.addVoice(new ZenithSamplerVoice());
  }
}

ZenithSamplerProcessor::~ZenithSamplerProcessor() {
  if (loadingThread != nullptr && loadingThread->isThreadRunning()) {
    loadingThread->stopThread(1000);
  }
}

//==============================================================================
// Parameter layout
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
ZenithSamplerProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // Amp Envelope
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "attack", "Attack",
      juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f, "s"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "decay", "Decay",
      juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f, "s"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sustain", "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "release", "Release",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.3f, "s"));

  // Filter
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filterCutoff", "Filter Cutoff",
      juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "filterResonance", "Filter Resonance",
      juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

  // Sample controls
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "sampleStartOffset", "Sample Start",
      juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

  // Pitch controls
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "pitchFine", "Fine Tune",
      juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f, "cents"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "pitchSemitones", "Pitch",
      juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 0.0f, "semitones"));

  // Global controls
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "globalPan", "Pan", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "globalGain", "Gain", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
      0.8f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "character", "Character", juce::NormalisableRange<float>(0.0f, 1.0f),
      0.5f));

  return layout;
}

//==============================================================================
// Audio processing
//==============================================================================

void ZenithSamplerProcessor::prepareToPlay(double sampleRate,
                                           int samplesPerBlock) {
  synth.setCurrentPlaybackSampleRate(sampleRate);

  // Update all voices with parameter pointers
  for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<ZenithSamplerVoice *>(synth.getVoice(i))) {
      voice->setParameters(parameters.getRawParameterValue("attack"),
                           parameters.getRawParameterValue("decay"),
                           parameters.getRawParameterValue("sustain"),
                           parameters.getRawParameterValue("release"),
                           parameters.getRawParameterValue("filterCutoff"),
                           parameters.getRawParameterValue("filterResonance"),
                           parameters.getRawParameterValue("sampleStartOffset"),
                           parameters.getRawParameterValue("pitchFine"),
                           parameters.getRawParameterValue("pitchSemitones"),
                           parameters.getRawParameterValue("globalPan"),
                           parameters.getRawParameterValue("globalGain"));
    }
  }
}

void ZenithSamplerProcessor::releaseResources() {
  // Nothing to release
}

void ZenithSamplerProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                          juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;

  // Clear any unused channels
  for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels();
       ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  // Render synthesiser
  synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

  // Apply character control (simple saturation)
  float character = *parameters.getRawParameterValue("character");
  if (character > 0.5f) {
    float drive = (character - 0.5f) * 4.0f; // 0-2 range
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      auto *data = buffer.getWritePointer(ch);
      for (int s = 0; s < buffer.getNumSamples(); ++s) {
        float sample = data[s] * (1.0f + drive);
        data[s] = std::tanh(sample); // Soft clipping
      }
    }
  }
}

//==============================================================================
// State save/load
//==============================================================================

void ZenithSamplerProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = parameters.copyState();

  // Add current patch name to state
  state.setProperty("currentPatch", currentPatchName, nullptr);

  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void ZenithSamplerProcessor::setStateInformation(const void *data,
                                                 int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState != nullptr) {
    if (xmlState->hasTagName(parameters.state.getType())) {
      auto state = juce::ValueTree::fromXml(*xmlState);
      parameters.replaceState(state);

      // Restore patch
      juce::String patchName = state.getProperty("currentPatch", "");
      if (patchName.isNotEmpty()) {
        loadSampleBankByName(patchName);
      }
    }
  }
}

//==============================================================================
// Sample bank management
//==============================================================================

bool ZenithSamplerProcessor::loadSampleBank(const juce::File &bankFile) {
  if (!bankFile.existsAsFile())
    return false;

  loadBankAsync(bankFile);
  return true;
}

bool ZenithSamplerProcessor::loadSampleBankByName(
    const juce::String &bankName) {
  auto bankFile =
      ContentPaths::getInstance().getPatchFile("ZenithSampler", bankName);

  return loadSampleBank(bankFile);
}

bool ZenithSamplerProcessor::loadSampleBankByNameSync(
    const juce::String &bankName) {
  auto bankFile =
      ContentPaths::getInstance().getPatchFile("ZenithSampler", bankName);

  if (!bankFile.existsAsFile())
    return false;

  SampleBankData bankData;
  if (parseBankFile(bankFile, bankData)) {
    auto dataPtr = std::make_unique<SampleBankData>(std::move(bankData));
    applyBankData(std::move(dataPtr));
    return true;
  }
  return false;
}

bool ZenithSamplerProcessor::loadSampleBankFromJson(
    const juce::String &jsonString, const juce::String &bankName) {
  loadBankFromJsonAsync(jsonString, bankName);
  return true;
}

juce::StringArray ZenithSamplerProcessor::getAvailableBanks() const {
  return ContentPaths::getInstance().getAvailablePatches("ZenithSampler");
}

bool ZenithSamplerProcessor::loadPatchByName(const juce::String &patchName) {
  // Try to load the patch by name - compatibility alias
  return loadSampleBankByName(patchName);
}

//==============================================================================
// Async bank loading
//==============================================================================

void ZenithSamplerProcessor::loadBankAsync(const juce::File &bankFile) {
  // Stop any existing loading thread
  if (loadingThread != nullptr && loadingThread->isThreadRunning()) {
    loadingThread->stopThread(1000);
  }

  isLoadingPatch.store(true);

  // Create a loading thread
  class LoadingThread : public juce::Thread {
  public:
    LoadingThread(ZenithSamplerProcessor &owner, const juce::File &file)
        : juce::Thread("BankLoader"), processor(owner), bankFile(file) {}

    void run() override {
      auto bankData = std::make_unique<SampleBankData>();

      if (processor.parseBankFile(bankFile, *bankData)) {
        // Apply on message thread
        juce::MessageManager::callAsync(
            [this, data = std::move(bankData)]() mutable {
              processor.applyBankData(std::move(data));
              processor.isLoadingPatch.store(false);
            });
      } else {
        processor.isLoadingPatch.store(false);
        DBG("Failed to load bank: " << bankFile.getFullPathName());
      }
    }

  private:
    ZenithSamplerProcessor &processor;
    juce::File bankFile;
  };

  loadingThread = std::make_unique<LoadingThread>(*this, bankFile);
  loadingThread->startThread();
}

void ZenithSamplerProcessor::loadBankFromJsonAsync(
    const juce::String &jsonString, const juce::String &bankName) {
  // Stop any existing loading thread
  if (loadingThread != nullptr && loadingThread->isThreadRunning()) {
    loadingThread->stopThread(1000);
  }

  isLoadingPatch.store(true);

  // Create a loading thread
  class JsonLoadingThread : public juce::Thread {
  public:
    JsonLoadingThread(ZenithSamplerProcessor &owner, const juce::String &json,
                      const juce::String &name)
        : juce::Thread("JsonBankLoader"), processor(owner), jsonString(json),
          bankName(name) {}

    void run() override {
      auto bankData = std::make_unique<SampleBankData>();
      bankData->bankName = bankName;

      auto json = juce::JSON::parse(jsonString);
      if (json.isObject()) {
        // For built-in banks, samples are in the built-in content directory
        auto baseDir = ContentPaths::getInstance().getInstrumentTypeDirectory(
            "ZenithSampler");

        if (processor.parseBankJson(json, baseDir, *bankData)) {
          // Apply on message thread
          juce::MessageManager::callAsync(
              [this, data = std::move(bankData)]() mutable {
                processor.applyBankData(std::move(data));
                processor.isLoadingPatch.store(false);
              });
          return;
        }
      }

      processor.isLoadingPatch.store(false);
      DBG("Failed to load JSON bank: " << bankName);
    }

  private:
    ZenithSamplerProcessor &processor;
    juce::String jsonString;
    juce::String bankName;
  };

  loadingThread =
      std::make_unique<JsonLoadingThread>(*this, jsonString, bankName);
  loadingThread->startThread();
}

bool ZenithSamplerProcessor::parseBankFile(const juce::File &bankFile,
                                           SampleBankData &outData) {
  // Parse JSON bank file
  auto jsonText = bankFile.loadFileAsString();
  auto json = juce::JSON::parse(jsonText);

  if (!json.isObject())
    return false;

  auto baseDir = bankFile.getParentDirectory();
  return parseBankJson(json, baseDir, outData);
}

bool ZenithSamplerProcessor::parseBankJson(const juce::var &json,
                                           const juce::File &baseDir,
                                           SampleBankData &outData) {
  auto *obj = json.getDynamicObject();
  if (obj == nullptr)
    return false;

  // Read bank metadata
  if (outData.bankName.isEmpty()) {
    outData.bankName = obj->getProperty("name").toString();
  }
  outData.category = obj->getProperty("category").toString();

  // Read default parameters
  if (auto *params = obj->getProperty("parameters").getDynamicObject()) {
    outData.attack = params->getProperty("attack");
    outData.decay = params->getProperty("decay");
    outData.sustain = params->getProperty("sustain");
    outData.release = params->getProperty("release");
    outData.filterCutoff = params->getProperty("filterCutoff");
    outData.filterResonance = params->getProperty("filterResonance");
    outData.sampleStartOffset = params->getProperty("sampleStartOffset");
    outData.pitchFine = params->getProperty("pitchFine");
    outData.pitchSemitones = params->getProperty("pitchSemitones");
    outData.globalPan = params->getProperty("globalPan");
    outData.globalGain = params->getProperty("globalGain");
  }

  // Read sample regions
  auto *regionsArray = obj->getProperty("regions").getArray();
  if (regionsArray == nullptr)
    return false;

  auto samplesDir = baseDir.getChildFile("Samples");

  for (int i = 0; i < regionsArray->size(); ++i) {
    auto *regionObj = (*regionsArray)[i].getDynamicObject();
    if (regionObj == nullptr)
      continue;

    SampleBankData::SampleRegion region;
    region.file =
        samplesDir.getChildFile(regionObj->getProperty("filePath").toString());
    region.rootNote = regionObj->getProperty("rootNote");
    region.lowNote = regionObj->getProperty("lowNote");
    region.highNote = regionObj->getProperty("highNote");
    region.lowVelocity = regionObj->getProperty("lowVel");
    region.highVelocity = regionObj->getProperty("highVel");
    region.gain = regionObj->getProperty("gain");
    region.tune = regionObj->getProperty("tune");

    // Parse loop mode
    auto loopModeStr =
        regionObj->getProperty("loopMode").toString().toLowerCase();
    if (loopModeStr == "forward")
      region.loopMode = ZenithSamplerSound::LoopMode::Forward;
    else if (loopModeStr == "pingpong" || loopModeStr == "ping-pong")
      region.loopMode = ZenithSamplerSound::LoopMode::PingPong;
    else
      region.loopMode = ZenithSamplerSound::LoopMode::None;

    if (region.file.existsAsFile()) {
      outData.regions.push_back(region);
    } else {
      DBG("Sample file not found: " << region.file.getFullPathName());
    }
  }

  return !outData.regions.empty();
}

void ZenithSamplerProcessor::applyBankData(
    std::unique_ptr<SampleBankData> bankData) {
  if (bankData == nullptr)
    return;

  // Clear existing sounds
  synth.clearSounds();

  // Load all sample regions
  if (audioFilePool_ != nullptr) {
    // Use AudioFilePool for RT-safe loading
    for (const auto &region : bankData->regions) {
      juce::String errorMessage;
      auto audioHandle = audioFilePool_->loadFile(region.file, errorMessage);

      if (audioHandle) {
        juce::BigInteger midiNotes;
        midiNotes.setRange(region.lowNote, region.highNote - region.lowNote + 1,
                           true);

        synth.addSound(new ZenithSamplerSound(
            region.file.getFileNameWithoutExtension(), audioHandle, midiNotes,
            region.rootNote, region.lowVelocity, region.highVelocity,
            region.loopMode, region.gain, region.tune));
      } else {
        DBG("Failed to load sample from pool: " << region.file.getFullPathName()
                                                << " - " << errorMessage);
      }
    }
  } else {
    // Fallback: direct loading (for standalone mode)
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    for (const auto &region : bankData->regions) {
      std::unique_ptr<juce::AudioFormatReader> reader(
          formatManager.createReaderFor(region.file));

      if (reader != nullptr) {
        juce::BigInteger midiNotes;
        midiNotes.setRange(region.lowNote, region.highNote - region.lowNote + 1,
                           true);

        synth.addSound(new ZenithSamplerSound(
            region.file.getFileNameWithoutExtension(), *reader, midiNotes,
            region.rootNote, region.lowVelocity, region.highVelocity, 0.01, 0.3,
            10.0, // attack, release, max length
            region.loopMode, region.gain, region.tune));
      }
    }
  }

  // Update parameters
  *parameters.getRawParameterValue("attack") = bankData->attack;
  *parameters.getRawParameterValue("decay") = bankData->decay;
  *parameters.getRawParameterValue("sustain") = bankData->sustain;
  *parameters.getRawParameterValue("release") = bankData->release;
  *parameters.getRawParameterValue("filterCutoff") = bankData->filterCutoff;
  *parameters.getRawParameterValue("filterResonance") =
      bankData->filterResonance;
  *parameters.getRawParameterValue("sampleStartOffset") =
      bankData->sampleStartOffset;
  *parameters.getRawParameterValue("pitchFine") = bankData->pitchFine;
  *parameters.getRawParameterValue("pitchSemitones") = bankData->pitchSemitones;
  *parameters.getRawParameterValue("globalPan") = bankData->globalPan;
  *parameters.getRawParameterValue("globalGain") = bankData->globalGain;

  // Store bank name
  currentPatchName = bankData->bankName;

  DBG("Loaded bank: " << currentPatchName << " (" << bankData->category
                      << ") with " << synth.getNumSounds() << " samples");
}

//==============================================================================
// Editor
//==============================================================================

juce::AudioProcessorEditor *ZenithSamplerProcessor::createEditor() {
  // TODO(zenith-core#1): ZenithSamplerEditor requires additional parameters
  // (ZenithSampler& instrument, ZenithPresetManager& presetManager) For now,
  // return nullptr - editor creation requires refactoring to pass these
  // dependencies
  return nullptr;
}

//==============================================================================
// ZenithSamplerSound
//==============================================================================

ZenithSamplerSound::ZenithSamplerSound(
    const juce::String &name, juce::AudioFormatReader &source,
    const juce::BigInteger &notes, int midiNoteForNormalPitch, int lowVel,
    int highVel, double attackTimeSecs, double releaseTimeSecs,
    double maxSampleLengthSeconds, LoopMode loop, float sampleGain,
    float sampleTune, int choke)
    : soundName(name), midiNotes(notes), rootNote(midiNoteForNormalPitch),
      lowVelocity(lowVel), highVelocity(highVel), loopMode(loop),
      gain(sampleGain), tune(sampleTune), chokeGroup(choke) {
  sourceSampleRate = source.sampleRate;

  auto lengthInSamples =
      (int)std::min((juce::int64)source.lengthInSamples,
                    (juce::int64)(maxSampleLengthSeconds * sourceSampleRate));

  ownedData = std::make_unique<juce::AudioBuffer<float>>(
      (int)source.numChannels, lengthInSamples + 4);

  source.read(ownedData.get(), 0, lengthInSamples + 4, 0, true, true);
  data = ownedData.get();
}

ZenithSamplerSound::ZenithSamplerSound(
    const juce::String &name, AudioFilePool::HandlePtr audioHandle,
    const juce::BigInteger &notes, int midiNoteForNormalPitch, int lowVel,
    int highVel, LoopMode loop, float sampleGain, float sampleTune, int choke)
    : soundName(name), poolHandle(audioHandle), midiNotes(notes),
      rootNote(midiNoteForNormalPitch), lowVelocity(lowVel),
      highVelocity(highVel), loopMode(loop), gain(sampleGain), tune(sampleTune),
      chokeGroup(choke) {
  if (poolHandle) {
    sourceSampleRate = poolHandle->sampleRate;
    // Point to the pool's buffer (const_cast is safe as we only read)
    data = const_cast<juce::AudioBuffer<float> *>(&poolHandle->buffer);
  }
}

ZenithSamplerSound::~ZenithSamplerSound() {}

bool ZenithSamplerSound::appliesToNote(int midiNoteNumber) {
  return midiNotes[midiNoteNumber];
}

bool ZenithSamplerSound::appliesToChannel(int /*midiChannel*/) { return true; }

//==============================================================================
// ZenithSamplerVoice
//==============================================================================

ZenithSamplerVoice::ZenithSamplerVoice() {
  filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

ZenithSamplerVoice::~ZenithSamplerVoice() {}

bool ZenithSamplerVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<ZenithSamplerSound *>(sound) != nullptr;
}

void ZenithSamplerVoice::setParameters(
    std::atomic<float> *attack, std::atomic<float> *decay,
    std::atomic<float> *sustain, std::atomic<float> *release,
    std::atomic<float> *filterCutoff, std::atomic<float> *filterResonance,
    std::atomic<float> *sampleStartOffset, std::atomic<float> *pitchFine,
    std::atomic<float> *pitchSemitones, std::atomic<float> *globalPan,
    std::atomic<float> *globalGain) {
  attackParam = attack;
  decayParam = decay;
  sustainParam = sustain;
  releaseParam = release;
  filterCutoffParam = filterCutoff;
  filterResonanceParam = filterResonance;
  sampleStartOffsetParam = sampleStartOffset;
  pitchFineParam = pitchFine;
  pitchSemitonesParam = pitchSemitones;
  globalPanParam = globalPan;
  globalGainParam = globalGain;
}

void ZenithSamplerVoice::startNote(int midiNoteNumber, float vel,
                                   juce::SynthesiserSound *s,
                                   int /*currentPitchWheelPosition*/) {
  if (auto *sound = dynamic_cast<ZenithSamplerSound *>(s)) {
    // Check velocity layer
    int midiVelocity = static_cast<int>(vel * 127.0f);
    if (!sound->appliesToVelocity(midiVelocity)) {
      clearCurrentNote();
      return;
    }

    velocity = vel;

    // Calculate pitch ratio using the sample's actual root note and per-sample
    // tuning
    auto cyclesPerSecond =
        juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    auto cyclesPerSample = cyclesPerSecond / sound->getSampleRate();

    // Apply per-sample tuning
    float sampleTuneSemitones = sound->getTune();
    auto rootCyclesPerSecond =
        juce::MidiMessage::getMidiNoteInHertz(sound->getRootNote()) *
        std::pow(2.0f, sampleTuneSemitones / 12.0f);
    auto rootCyclesPerSample = rootCyclesPerSecond / sound->getSampleRate();

    pitchRatio = cyclesPerSample / rootCyclesPerSample;

    // Apply sample start offset
    float startOffset = (sampleStartOffsetParam != nullptr)
                            ? static_cast<float>(*sampleStartOffsetParam)
                            : 0.0f;
    auto *audioData = sound->getAudioData();
    if (audioData) {
      sourceSamplePosition = startOffset * audioData->getNumSamples();
    } else {
      sourceSamplePosition = 0.0;
    }

    loopDirection = true; // Reset loop direction for ping-pong

    // Update envelope
    if (attackParam != nullptr && releaseParam != nullptr) {
      ampEnvParams.attack = static_cast<float>(*attackParam);
      ampEnvParams.decay =
          (decayParam != nullptr) ? static_cast<float>(*decayParam) : 0.1f;
      ampEnvParams.sustain =
          (sustainParam != nullptr) ? static_cast<float>(*sustainParam) : 0.7f;
      ampEnvParams.release = static_cast<float>(*releaseParam);
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

void ZenithSamplerVoice::stopNote(float /*velocity*/, bool allowTailOff) {
  if (allowTailOff) {
    ampEnvelope.noteOff();
  } else {
    clearCurrentNote();
    ampEnvelope.reset();
  }
}

void ZenithSamplerVoice::pitchWheelMoved(int /*newPitchWheelValue*/) {}

void ZenithSamplerVoice::controllerMoved(int /*controllerNumber*/,
                                         int /*newControllerValue*/) {}

void ZenithSamplerVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                         int startSample, int numSamples) {
  if (auto *sound = dynamic_cast<ZenithSamplerSound *>(
          getCurrentlyPlayingSound().get())) {
    auto *audioData = sound->getAudioData();
    if (!audioData)
      return;

    auto &data = *audioData;
    const int dataLength = data.getNumSamples();

    // Apply global pitch control (semitones + fine cents)
    float pitchSemitones = (pitchSemitonesParam != nullptr)
                               ? static_cast<float>(*pitchSemitonesParam)
                               : 0.0f;
    float pitchFine = (pitchFineParam != nullptr)
                          ? static_cast<float>(*pitchFineParam)
                          : 0.0f;
    float totalPitchShift = pitchSemitones + (pitchFine / 100.0f);
    float pitchMultiplier = std::pow(2.0f, totalPitchShift / 12.0f);
    double finalPitchRatio = pitchRatio * pitchMultiplier;

    // Issue 10: Fix Sampler Filter Stepping
    // Process in small sub-blocks to update filter parameters
    const int subBlockSize = 32;

    for (int start = 0; start < numSamples; start += subBlockSize) {
      int blockSize = std::min(subBlockSize, numSamples - start);

      // Update filter parameters for this sub-block
      // Get filter parameters
      float cutoff = (filterCutoffParam != nullptr)
                         ? static_cast<float>(*filterCutoffParam)
                         : 1.0f;
      float resonance = (filterResonanceParam != nullptr)
                            ? static_cast<float>(*filterResonanceParam)
                            : 0.0f;

      // Map cutoff to Hz (20Hz - 20kHz)
      float cutoffHz = 20.0f + cutoff * cutoff * 19980.0f;
      filter.setCutoffFrequency(cutoffHz);
      filter.setResonance(resonance * 0.9f + 0.1f); // 0.1 - 1.0

      // Get global controls
      float gainValue = (globalGainParam != nullptr)
                            ? static_cast<float>(*globalGainParam)
                            : 0.8f;
      float pan = (globalPanParam != nullptr)
                      ? static_cast<float>(*globalPanParam)
                      : 0.5f; // 0 = left, 0.5 = center, 1 = right

      // Apply per-sample gain
      gainValue *= sound->getGain();

      // Calculate pan gains (constant power)
      float leftGain = std::cos(pan * juce::MathConstants<float>::halfPi);
      float rightGain = std::sin(pan * juce::MathConstants<float>::halfPi);

      auto loopMode = sound->getLoopMode();

      for (int i = 0; i < blockSize; ++i) {
        int sampleIndex = start + i;
        auto pos = (int)sourceSamplePosition;

        // Handle looping
        if (loopMode == ZenithSamplerSound::LoopMode::Forward &&
            pos >= dataLength - 1) {
          sourceSamplePosition = 0.0;
          pos = 0;
        } else if (loopMode == ZenithSamplerSound::LoopMode::PingPong) {
          if (pos >= dataLength - 1 && loopDirection) {
            loopDirection = false; // Reverse
          } else if (pos <= 0 && !loopDirection) {
            loopDirection = true; // Forward
          }
        } else if (loopMode == ZenithSamplerSound::LoopMode::None &&
                   pos >= dataLength - 1) {
          stopNote(0.0f, false);
          // Break out of inner loop
          i = blockSize;
          // Break out of outer loop
          start = numSamples;
          break;
        }

        // Ensure we're in bounds
        if (pos < 0 || pos >= dataLength - 1) {
          stopNote(0.0f, false);
          i = blockSize;
          start = numSamples;
          break;
        }

        // Issue 4: Fix Sampler Aliasing with Cubic Hermite Interpolation
        auto alpha = (float)(sourceSamplePosition - pos);

        // Get envelope value once per sample
        float envValue = ampEnvelope.getNextSample();

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
          auto *channelData = data.getReadPointer(ch % data.getNumChannels());

          // Safe index access for Hermite (4 points: pos-1, pos, pos+1, pos+2)
          int p0 = std::max(0, pos - 1);
          int p1 = pos;
          int p2 = std::min(dataLength - 1, pos + 1);
          int p3 = std::min(dataLength - 1, pos + 2);

          float y0 = channelData[p0];
          float y1 = channelData[p1];
          float y2 = channelData[p2];
          float y3 = channelData[p3];

          // Cubic Hermite Interpolation
          float c0 = y1;
          float c1 = 0.5f * (y2 - y0);
          float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
          float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

          float sample = ((c3 * alpha + c2) * alpha + c1) * alpha + c0;

          // Apply envelope
          sample *= envValue;

          // Apply velocity
          sample *= velocity;

          // Apply filter
          sample = filter.processSample(ch, sample);

          // Apply gain
          sample *= gainValue;

          // Apply pan (only for stereo output)
          if (outputBuffer.getNumChannels() >= 2) {
            sample *= (ch == 0) ? leftGain : rightGain;
          }

          outputBuffer.addSample(ch, startSample + sampleIndex, sample);
        }

        // Advance sample position
        if (loopMode == ZenithSamplerSound::LoopMode::PingPong &&
            !loopDirection) {
          sourceSamplePosition -= finalPitchRatio;
        } else {
          sourceSamplePosition += finalPitchRatio;
        }

        if (!ampEnvelope.isActive()) {
          stopNote(0.0f, false);
          i = blockSize;
          start = numSamples;
          break;
        }
      }
    }
  }
}

//==============================================================================
// ZenithSampler (Instrument Wrapper)
//==============================================================================

ZenithSampler::ZenithSampler()
    : InstrumentBase(std::make_unique<ZenithSamplerProcessor>(),
                     createMetadata()) {
  // Map parameter IDs to JUCE indices
  mapParameter("attack", ZenithSamplerProcessor::Attack);
  mapParameter("decay", ZenithSamplerProcessor::Decay);
  mapParameter("sustain", ZenithSamplerProcessor::Sustain);
  mapParameter("release", ZenithSamplerProcessor::Release);
  mapParameter("filter_cutoff", ZenithSamplerProcessor::FilterCutoff);
  mapParameter("filter_resonance", ZenithSamplerProcessor::FilterResonance);
  mapParameter("sample_start_offset",
               ZenithSamplerProcessor::SampleStartOffset);
  mapParameter("pitch_fine", ZenithSamplerProcessor::PitchFine);
  mapParameter("pitch_semitones", ZenithSamplerProcessor::PitchSemitones);
  mapParameter("global_pan", ZenithSamplerProcessor::GlobalPan);
  mapParameter("global_gain", ZenithSamplerProcessor::GlobalGain);
  mapParameter("character", ZenithSamplerProcessor::Character);

  // Register presets (sample banks)
  registerPresets();
}

juce::AudioProcessorValueTreeState *ZenithSampler::getParameterState() {
  if (auto *proc =
          dynamic_cast<ZenithSamplerProcessor *>(getAudioProcessor())) {
    return &proc->getParameters();
  }
  return nullptr;
}

InstrumentMetadata ZenithSampler::createMetadata() {
  InstrumentMetadata metadata;
  metadata.instrumentId = "zenith_sampler";
  metadata.name = "Zenith Sampler";
  metadata.category = "sampler";
  metadata.description = "Multi-sample instrument for drums, 808s, pianos, and "
                         "one-shot FX with RT-safe sample loading";
  metadata.tags = {"drums", "808", "keys", "fx", "sampler"};

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

  // Sample controls
  {
    ParameterMetadata param;
    param.id = "sample_start_offset";
    param.name = "Sample Start";
    param.category = "Sample";
    param.type = ParameterMetadata::Type::Float;
    param.defaultValue = 0.0f;
    param.minValue = 0.0f;
    param.maxValue = 1.0f;
    param.units = "%";
    metadata.parameters.push_back(param);
  }

  // Pitch controls
  {
    ParameterMetadata param;
    param.id = "pitch_fine";
    param.name = "Fine Tune";
    param.category = "Pitch";
    param.type = ParameterMetadata::Type::Float;
    param.defaultValue = 0.0f;
    param.minValue = -100.0f;
    param.maxValue = 100.0f;
    param.units = "cents";
    metadata.parameters.push_back(param);
  }
  {
    ParameterMetadata param;
    param.id = "pitch_semitones";
    param.name = "Pitch";
    param.category = "Pitch";
    param.type = ParameterMetadata::Type::Float;
    param.defaultValue = 0.0f;
    param.minValue = -24.0f;
    param.maxValue = 24.0f;
    param.units = "semitones";
    metadata.parameters.push_back(param);
  }

  // Global parameters
  {
    ParameterMetadata param;
    param.id = "global_pan";
    param.name = "Pan";
    param.category = "Global";
    param.type = ParameterMetadata::Type::Float;
    param.defaultValue = 0.5f;
    param.minValue = 0.0f;
    param.maxValue = 1.0f;
    param.units = "";
    metadata.parameters.push_back(param);
  }
  {
    ParameterMetadata param;
    param.id = "global_gain";
    param.name = "Gain";
    param.category = "Global";
    param.type = ParameterMetadata::Type::Float;
    param.defaultValue = 0.8f;
    param.minValue = 0.0f;
    param.maxValue = 2.0f;
    param.units = "";
    metadata.parameters.push_back(param);
  }
  {
    ParameterMetadata param;
    param.id = "character";
    param.name = "Character";
    param.category = "Global";
    param.type = ParameterMetadata::Type::Float;
    param.defaultValue = 0.5f;
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

void ZenithSampler::registerPresets() {
  // Register built-in sample banks as presets
  // Note: These are stubs - actual sample files would need to be in the
  // content directory

  // Built-in bank: 808 Essentials
  const char *bank808Json = R"({
        "name": "808 Essentials",
        "category": "808",
        "parameters": {
            "attack": 0.001,
            "decay": 0.2,
            "sustain": 0.0,
            "release": 0.5,
            "filterCutoff": 0.8,
            "filterResonance": 0.2,
            "sampleStartOffset": 0.0,
            "pitchFine": 0.0,
            "pitchSemitones": 0.0,
            "globalPan": 0.5,
            "globalGain": 0.9
        },
        "regions": [
            { "filePath": "808-kick.wav", "rootNote": 36, "lowNote": 36, "highNote": 36, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
            { "filePath": "808-snare.wav", "rootNote": 38, "lowNote": 38, "highNote": 38, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
            { "filePath": "808-hihat-closed.wav", "rootNote": 42, "lowNote": 42, "highNote": 42, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 0.8, "tune": 0.0 },
            { "filePath": "808-hihat-open.wav", "rootNote": 46, "lowNote": 46, "highNote": 46, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 0.8, "tune": 0.0 },
            { "filePath": "808-bass.wav", "rootNote": 48, "lowNote": 24, "highNote": 72, "lowVel": 0, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 }
        ]
    })";

  // Built-in bank: LoFi Keys
  const char *bankLoFiKeysJson = R"({
        "name": "LoFi Keys",
        "category": "keys",
        "parameters": {
            "attack": 0.05,
            "decay": 0.3,
            "sustain": 0.6,
            "release": 0.8,
            "filterCutoff": 0.7,
            "filterResonance": 0.1,
            "sampleStartOffset": 0.0,
            "pitchFine": 0.0,
            "pitchSemitones": 0.0,
            "globalPan": 0.5,
            "globalGain": 0.85
        },
        "regions": [
            { "filePath": "lofi-piano-C3.wav", "rootNote": 48, "lowNote": 42, "highNote": 53, "lowVel": 0, "highVel": 63, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
            { "filePath": "lofi-piano-C4.wav", "rootNote": 60, "lowNote": 54, "highNote": 65, "lowVel": 0, "highVel": 63, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
            { "filePath": "lofi-piano-C5.wav", "rootNote": 72, "lowNote": 66, "highNote": 84, "lowVel": 0, "highVel": 63, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
            { "filePath": "lofi-piano-C3-hard.wav", "rootNote": 48, "lowNote": 42, "highNote": 53, "lowVel": 64, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
            { "filePath": "lofi-piano-C4-hard.wav", "rootNote": 60, "lowNote": 54, "highNote": 65, "lowVel": 64, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 },
            { "filePath": "lofi-piano-C5-hard.wav", "rootNote": 72, "lowNote": 66, "highNote": 84, "lowVel": 64, "highVel": 127, "loopMode": "forward", "gain": 1.0, "tune": 0.0 }
        ]
    })";

  // Built-in bank: FX Hits
  const char *bankFXJson = R"({
        "name": "FX Hits",
        "category": "fx",
        "parameters": {
            "attack": 0.01,
            "decay": 0.5,
            "sustain": 0.3,
            "release": 1.5,
            "filterCutoff": 1.0,
            "filterResonance": 0.3,
            "sampleStartOffset": 0.0,
            "pitchFine": 0.0,
            "pitchSemitones": 0.0,
            "globalPan": 0.5,
            "globalGain": 0.8
        },
        "regions": [
            { "filePath": "fx-riser.wav", "rootNote": 60, "lowNote": 60, "highNote": 60, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
            { "filePath": "fx-impact.wav", "rootNote": 62, "lowNote": 62, "highNote": 62, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
            { "filePath": "fx-reverse.wav", "rootNote": 64, "lowNote": 64, "highNote": 64, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 },
            { "filePath": "fx-whoosh.wav", "rootNote": 65, "lowNote": 65, "highNote": 65, "lowVel": 0, "highVel": 127, "loopMode": "none", "gain": 1.0, "tune": 0.0 }
        ]
    })";

  // Register default preset with updated parameter names
  std::map<juce::String, float> defaultPreset;
  defaultPreset["attack"] = 0.01f;
  defaultPreset["decay"] = 0.1f;
  defaultPreset["sustain"] = 0.7f;
  defaultPreset["release"] = 0.3f;
  defaultPreset["filter_cutoff"] = 1.0f;
  defaultPreset["filter_resonance"] = 0.0f;
  defaultPreset["sample_start_offset"] = 0.0f;
  defaultPreset["pitch_fine"] = 0.0f;
  defaultPreset["pitch_semitones"] = 0.0f;
  defaultPreset["global_pan"] = 0.5f;
  defaultPreset["global_gain"] = 0.8f;
  defaultPreset["character"] = 0.5f;

  registerPreset("default", "Default", defaultPreset);

  // Note: To actually load these banks, you would need to:
  // 1. Create the sample files in the content directory structure
  // 2. Call loadSampleBankFromJson() with the JSON strings above
  // For example:
  //   auto* proc =
  //   dynamic_cast<ZenithSamplerProcessor*>(getAudioProcessor()); if (proc)
  //   proc->loadSampleBankFromJson(bank808Json, "808 Essentials");
}

} // namespace zenith
