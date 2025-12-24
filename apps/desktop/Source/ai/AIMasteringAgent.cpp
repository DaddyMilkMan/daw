/*
  ==============================================================================
    AIMasteringAgent.cpp
  ==============================================================================
*/

#include "AIMasteringAgent.h"
#include "../network/SecureKeyStore.h"
#include "../engine/Engine.h"
#include "../engine/Track.h"

namespace zenith {
namespace ai {

//==============================================================================
// AudioFeatureExtractor Implementation
//==============================================================================

AudioFeatureExtractor::Features
AudioFeatureExtractor::extract(const juce::AudioBuffer<float> &buffer,
                               double sampleRate) {
  Features features;

  if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0) {
    return features;
  }

  const int numSamples = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();

  // Calculate peak and RMS
  float peak = 0.0f;
  float sumSquares = 0.0f;

  for (int ch = 0; ch < numChannels; ++ch) {
    const float *data = buffer.getReadPointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      float absVal = std::abs(data[i]);
      peak = std::max(peak, absVal);
      sumSquares += data[i] * data[i];
    }
  }

  float rms = std::sqrt(sumSquares / (numSamples * numChannels));

  features.peakDb = 20.0f * std::log10(peak + 1e-10f);
  features.rmsDb = 20.0f * std::log10(rms + 1e-10f);
  features.crestFactor = peak / (rms + 1e-10f);

  // Frequency band analysis
  analyzeFrequencyBands(buffer, sampleRate, features);

  // Stereo analysis
  if (numChannels == 2) {
    analyzeStereo(buffer, features);
  }

  features.valid = true;
  return features;
}

void AudioFeatureExtractor::analyzeFrequencyBands(
    const juce::AudioBuffer<float> &buffer, double sampleRate,
    Features &features) {
  // Simplified frequency analysis using filtering
  // In production, use FFT for better accuracy

  const int numSamples = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();

  if (numSamples == 0)
    return;

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(numSamples);
  spec.numChannels = static_cast<juce::uint32>(numChannels);

  // Create filters for each band
  auto calcBandEnergy = [&](float lowFreq, float highFreq) -> float {
    juce::AudioBuffer<float> tempBuf(buffer);
    juce::dsp::AudioBlock<float> block(tempBuf);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Simple bandpass approximation
    juce::dsp::StateVariableTPTFilter<float> filter;
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    filter.setCutoffFrequency((lowFreq + highFreq) * 0.5f);
    filter.process(context);

    // Calculate energy
    float energy = 0.0f;
    for (int ch = 0; ch < tempBuf.getNumChannels(); ++ch) {
      const float *data = tempBuf.getReadPointer(ch);
      for (int i = 0; i < tempBuf.getNumSamples(); ++i) {
        energy += data[i] * data[i];
      }
    }
    return energy / (tempBuf.getNumSamples() * tempBuf.getNumChannels());
  };

  features.subBass = calcBandEnergy(20.0f, 60.0f);
  features.bass = calcBandEnergy(60.0f, 250.0f);
  features.lowMid = calcBandEnergy(250.0f, 500.0f);
  features.mid = calcBandEnergy(500.0f, 2000.0f);
  features.highMid = calcBandEnergy(2000.0f, 4000.0f);
  features.presence = calcBandEnergy(4000.0f, 6000.0f);
  features.brilliance = calcBandEnergy(6000.0f, 20000.0f);

  // Normalize to 0-1 range
  float totalEnergy = features.subBass + features.bass + features.lowMid +
                      features.mid + features.highMid + features.presence +
                      features.brilliance;

  if (totalEnergy > 0.0001f) {
    features.subBass /= totalEnergy;
    features.bass /= totalEnergy;
    features.lowMid /= totalEnergy;
    features.mid /= totalEnergy;
    features.highMid /= totalEnergy;
    features.presence /= totalEnergy;
    features.brilliance /= totalEnergy;
  }
}

void AudioFeatureExtractor::analyzeStereo(
    const juce::AudioBuffer<float> &buffer, Features &features) {
  const int numSamples = buffer.getNumSamples();
  const float *left = buffer.getReadPointer(0);
  const float *right = buffer.getReadPointer(1);

  float correlation = 0.0f;
  float leftSum = 0.0f, rightSum = 0.0f;

  for (int i = 0; i < numSamples; ++i) {
    correlation += left[i] * right[i];
    leftSum += left[i] * left[i];
    rightSum += right[i] * right[i];
  }

  float denom = std::sqrt(leftSum * rightSum);
  if (denom > 0.0001f) {
    float corr = correlation / denom;
    features.stereoWidth = 1.0f - corr; // 0 = mono, 1 = wide stereo
  }
}

juce::String AudioFeatureExtractor::toJSON(const Features &features) {
  juce::DynamicObject::Ptr obj = new juce::DynamicObject();

  obj->setProperty("peakDb", features.peakDb);
  obj->setProperty("rmsDb", features.rmsDb);
  obj->setProperty("crestFactor", features.crestFactor);
  obj->setProperty("subBass", features.subBass);
  obj->setProperty("bass", features.bass);
  obj->setProperty("lowMid", features.lowMid);
  obj->setProperty("mid", features.mid);
  obj->setProperty("highMid", features.highMid);
  obj->setProperty("presence", features.presence);
  obj->setProperty("brilliance", features.brilliance);
  obj->setProperty("stereoWidth", features.stereoWidth);

  return juce::JSON::toString(obj.get(), true);
}

//==============================================================================
// GrokMasteringAI Implementation
//==============================================================================

GrokMasteringAI::GrokMasteringAI() {
  grokClient_ = std::make_unique<GrokAPIClient>();

  // Attempt to load API key from SecureKeyStore if not already set by env var
  if (!grokClient_->hasAPIKey()) {
    juce::String key;
    if (SecureKeyStore::retrieveKey(SecureKeyStore::GrokAPIKey, key)) {
      grokClient_->setAPIKey(key);
      DBG("GrokMasteringAI: API key loaded from SecureKeyStore");
    } else {
      DBG("GrokMasteringAI: No API key found in SecureKeyStore or Env Var");
    }
  }

  DBG("GrokMasteringAI initialized with Grok 4.1 reasoning model");
}

juce::String
GrokMasteringAI::buildPrompt(const AudioFeatureExtractor::Features &features,
                             const juce::String &userIntent,
                             float targetLoudness) {
  float dynamicRange = features.peakDb - features.rmsDb;

  juce::String prompt =
      R"(You are a Grammy-winning mastering engineer with 30 years of experience. Analyze this audio and provide professional mastering parameters.

AUDIO ANALYSIS:
═══════════════════════════════════════════════════════════════
Peak Level: )" +
      juce::String(features.peakDb, 1) + R"( dB
RMS Level: )" +
      juce::String(features.rmsDb, 1) + R"( dB
Dynamic Range: )" +
      juce::String(dynamicRange, 1) + R"( dB
Crest Factor: )" +
      juce::String(features.crestFactor, 2) + R"(

FREQUENCY BALANCE:
─────────────────────────────────────────────────────────────
Sub-bass (20-60Hz):     )" +
      juce::String(features.subBass * 100.0f, 1) + R"(%
Bass (60-250Hz):        )" +
      juce::String(features.bass * 100.0f, 1) + R"(%
Low-mid (250-500Hz):    )" +
      juce::String(features.lowMid * 100.0f, 1) + R"(%
Mid (500-2kHz):         )" +
      juce::String(features.mid * 100.0f, 1) + R"(%
High-mid (2-4kHz):      )" +
      juce::String(features.highMid * 100.0f, 1) + R"(%
Presence (4-6kHz):      )" +
      juce::String(features.presence * 100.0f, 1) + R"(%
Brilliance (6-20kHz):   )" +
      juce::String(features.brilliance * 100.0f, 1) + R"(%

STEREO FIELD:
─────────────────────────────────────────────────────────────
Width: )" +
      juce::String(features.stereoWidth * 100.0f, 1) + R"(%

TARGET SPECIFICATION:
═══════════════════════════════════════════════════════════════
Target Loudness: )" +
      juce::String(targetLoudness, 1) + R"( LUFS (streaming standard)
User Intent: ")" +
      userIntent + R"("

MASTERING GUIDELINES:
─────────────────────────────────────────────────────────────
1. Analyze frequency balance - which bands need adjustment?
2. Consider dynamic range - how much compression is appropriate?
3. Determine EQ moves to enhance clarity and balance
4. Set compression to glue the mix without over-squashing
5. Choose limiter ceiling based on target loudness and headroom

Respond with ONLY this JSON structure (no markdown, no extra text):
{
  "eq": {
    "lowShelfGain": <dB value from -6 to +6>,
    "midCutGain": <dB value from -6 to +6>,
    "presenceGain": <dB value from -6 to +6>,
    "airGain": <dB value from -6 to +6>
  },
  "compression": {
    "threshold": <dB value from -24 to 0>,
    "ratio": <value from 1.5 to 8.0>,
    "attack": <ms from 10 to 100>,
    "release": <ms from 50 to 500>
  },
  "limiterCeiling": <dB value from -1.0 to -0.1>,
  "reasoning": "<brief professional explanation of your choices>"
})";

  return prompt;
}

GrokMasteringAI::MasteringDecision
GrokMasteringAI::parseGrokResponse(const juce::String &response) {
  MasteringDecision decision;

  // Remove markdown code fences if present
  juce::String jsonStr = response.trim();
  jsonStr = jsonStr.replace("```json", "").replace("```", "").trim();

  // Parse JSON
  auto json = juce::JSON::parse(jsonStr);

  if (json.isVoid() || !json.isObject()) {
    DBG("Failed to parse Grok response as JSON");
    DBG("Response was: " + response.substring(0, 200));
    return decision;
  }

  auto *obj = json.getDynamicObject();
  if (!obj)
    return decision;

  // Extract EQ parameters
  if (auto *eqObj = obj->getProperty("eq").getDynamicObject()) {
    decision.eq.lowShelfGain = eqObj->getProperty("lowShelfGain");
    decision.eq.midCutGain = eqObj->getProperty("midCutGain");
    decision.eq.presenceGain = eqObj->getProperty("presenceGain");
    decision.eq.airGain = eqObj->getProperty("airGain");
  }

  // Extract compression parameters
  if (auto *compObj = obj->getProperty("compression").getDynamicObject()) {
    decision.compression.threshold = compObj->getProperty("threshold");
    decision.compression.ratio = compObj->getProperty("ratio");
    decision.compression.attack = compObj->getProperty("attack");
    decision.compression.release = compObj->getProperty("release");
  }

  // Extract limiter ceiling
  decision.limiterCeiling = obj->getProperty("limiterCeiling");

  // Extract reasoning
  decision.reasoning = obj->getProperty("reasoning").toString();

  // Validate and clamp ranges
  decision.eq.lowShelfGain =
      juce::jlimit(-6.0f, 6.0f, decision.eq.lowShelfGain);
  decision.eq.midCutGain = juce::jlimit(-6.0f, 6.0f, decision.eq.midCutGain);
  decision.eq.presenceGain =
      juce::jlimit(-6.0f, 6.0f, decision.eq.presenceGain);
  decision.eq.airGain = juce::jlimit(-6.0f, 6.0f, decision.eq.airGain);

  decision.compression.threshold =
      juce::jlimit(-24.0f, 0.0f, decision.compression.threshold);
  decision.compression.ratio =
      juce::jlimit(1.5f, 8.0f, decision.compression.ratio);
  decision.compression.attack =
      juce::jlimit(10.0f, 100.0f, decision.compression.attack);
  decision.compression.release =
      juce::jlimit(50.0f, 500.0f, decision.compression.release);

  decision.limiterCeiling = juce::jlimit(-1.0f, -0.1f, decision.limiterCeiling);

  decision.valid = true;

  DBG("╔════════════════════════════════════════════════════════════╗");
  DBG("║            GROK AI MASTERING DECISION                      ║");
  DBG("╠════════════════════════════════════════════════════════════╣");
  DBG("║ EQ Settings:                                               ║");
  DBG("║   Low Shelf:  " + juce::String(decision.eq.lowShelfGain, 1) + " dB");
  DBG("║   Mid Cut:    " + juce::String(decision.eq.midCutGain, 1) + " dB");
  DBG("║   Presence:   " + juce::String(decision.eq.presenceGain, 1) + " dB");
  DBG("║   Air:        " + juce::String(decision.eq.airGain, 1) + " dB");
  DBG("║                                                            ║");
  DBG("║ Compression:                                               ║");
  DBG("║   Threshold:  " + juce::String(decision.compression.threshold, 1) +
      " dB");
  DBG("║   Ratio:      " + juce::String(decision.compression.ratio, 1) + ":1");
  DBG("║   Attack:     " + juce::String(decision.compression.attack, 0) +
      " ms");
  DBG("║   Release:    " + juce::String(decision.compression.release, 0) +
      " ms");
  DBG("║                                                            ║");
  DBG("║ Limiter:      " + juce::String(decision.limiterCeiling, 2) +
      " dB ceiling");
  DBG("╠════════════════════════════════════════════════════════════╣");
  DBG("║ AI Reasoning:                                              ║");
  DBG("║ " + decision.reasoning);
  DBG("╚════════════════════════════════════════════════════════════╝");

  return decision;
}

GrokMasteringAI::MasteringDecision GrokMasteringAI::getMasteringDecision(
    const AudioFeatureExtractor::Features &features,
    const juce::String &userIntent, float targetLoudness) {
  if (!grokClient_) {
    DBG("ERROR: Grok API client not initialized!");
    return MasteringDecision();
  }

  // Build the prompt
  juce::String prompt = buildPrompt(features, userIntent, targetLoudness);

  // System message for Grok
  juce::String systemMsg =
      R"(You are a professional mastering engineer with decades of experience at top studios. 
You analyze audio characteristics and provide optimal mastering parameters in JSON format.
Be precise, musical, and conservative in your decisions. Respond ONLY with valid JSON, no additional text or markdown.)";

  // Call Grok API
  DBG("Querying Grok 4.1 for mastering decision...");
  juce::String response = grokClient_->callGrok(prompt, systemMsg);

  // Parse response
  return parseGrokResponse(response);
}

//==============================================================================
// AIMasteringAgent Implementation
//==============================================================================

AIMasteringAgent::AIMasteringAgent(Engine &engine) : engine_(engine) {
  DBG("═══════════════════════════════════════════════════════════════");
  DBG("  AI MASTERING AGENT - Powered by Grok 4.1 Reasoning");
  DBG("═══════════════════════════════════════════════════════════════");
}

AIMasteringAgent::~AIMasteringAgent() { DBG("AI Mastering Agent destroyed"); }

void AIMasteringAgent::prepare(double sampleRate, int samplesPerBlock,
                               int numChannels) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
  spec.numChannels = static_cast<juce::uint32>(numChannels);

  sampleRate_ = sampleRate;

  eq_.prepare(spec);
  compressor_.prepare(spec);
  limiter_.prepare(spec);

  isPrepared_.store(true);

  DBG("AI Mastering Agent prepared:");
  DBG("  Sample Rate: " + juce::String(sampleRate) + " Hz");
  DBG("  Block Size: " + juce::String(samplesPerBlock));
  DBG("  Channels: " + juce::String(numChannels));
}

void AIMasteringAgent::analyzeAndConfigure(
    const juce::AudioBuffer<float> &analysisBuffer, const Options &options) {
  DBG("");
  DBG("╔════════════════════════════════════════════════════════════╗");
  DBG("║          AI MASTERING ANALYSIS STARTING...                 ║");
  DBG("╚════════════════════════════════════════════════════════════╝");

  // Extract audio features
  auto features = featureExtractor_.extract(analysisBuffer, sampleRate_);

  if (!features.valid) {
    DBG("ERROR: Feature extraction failed");
    return;
  }

  DBG("Audio Features Extracted:");
  DBG("  Peak: " + juce::String(features.peakDb, 1) + " dB");
  DBG("  RMS: " + juce::String(features.rmsDb, 1) + " dB");
  DBG("  Dynamic Range: " + juce::String(features.peakDb - features.rmsDb, 1) +
      " dB");
  DBG("  Stereo Width: " + juce::String(features.stereoWidth * 100.0f, 0) +
      "%");

  // Auto-balance tracks if requested
  if (options.autoBalance) {
    balanceTracks(options);
  }

  // Query Grok for mastering decision
  if (options.useAI) {
    GrokMasteringAI::MasteringDecision decision = grokAI_.getMasteringDecision(
        features, options.userIntent, options.targetLoudness);

    if (decision.valid) {
      const juce::ScopedLock lock(decisionLock_);
      lastDecision_ = decision;
      applyAIDecision(decision);
      isConfigured_.store(true);
      DBG("✓ AI mastering configuration applied successfully");
    } else {
      DBG("ERROR: AI decision invalid, mastering not configured");
    }
  }

  DBG("╔════════════════════════════════════════════════════════════╗");
  DBG("║          AI MASTERING ANALYSIS COMPLETE                    ║");
  DBG("╚════════════════════════════════════════════════════════════╝");
  DBG("");
}

void AIMasteringAgent::applyAIDecision(
    const GrokMasteringAI::MasteringDecision &decision) {
  // Apply EQ settings
  if (decision.valid) {
    MasteringEQ::Settings eqSettings;
    eqSettings.lowShelfGain = decision.eq.lowShelfGain;
    eqSettings.midCutGain = decision.eq.midCutGain;
    eqSettings.presenceGain = decision.eq.presenceGain;
    eqSettings.airGain = decision.eq.airGain;
    eqSettings.enabled = true;
    eq_.setSettings(eqSettings);

    // Apply compression settings
    MasteringCompressor::Settings compSettings;
    compSettings.threshold = decision.compression.threshold;
    compSettings.ratio = decision.compression.ratio;
    compSettings.attack = decision.compression.attack;
    compSettings.release = decision.compression.release;
    compSettings.enabled = true;
    compressor_.setSettings(compSettings);

    // Apply limiter ceiling
    limiter_.setCeiling(decision.limiterCeiling);

    DBG("DSP chain configured with AI parameters");
  }
}

void AIMasteringAgent::balanceTracks(const Options &options) {
  auto &tracks = engine_.tracks();
  if (tracks.empty()) {
    DBG("No tracks to balance");
    return;
  }

  DBG("Auto-balancing " + juce::String(tracks.size()) + " tracks...");

  // Find peak levels
  float maxLevel = 0.0f;
  for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
    if (!tracks[i])
      continue;

    float level = engine_.getTrackLevel(i);
    if (std::isfinite(level) && level > 0.0f) {
      maxLevel = std::max(maxLevel, level);
    }
  }

  if (maxLevel < 0.0001f) {
    DBG("All tracks silent, skipping balance");
    return;
  }

  // Balance to -18dBFS (0.125 linear)
  const float targetPeak = 0.125f;
  for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
    if (!tracks[i])
      continue;

    float level = engine_.getTrackLevel(i);
    if (level > 0.0001f) {
      float gain = (targetPeak / maxLevel);
      gain = juce::jlimit(0.25f, 4.0f, gain);

      engine_.setTrackVolume(i, gain);
      DBG("  Track " + juce::String(i) + ": " +
          juce::String(20.0f * std::log10(gain), 1) + " dB");
    }
  }
}

void AIMasteringAgent::processBlock(juce::AudioBuffer<float> &buffer) {
  if (!isPrepared_.load() || !isConfigured_.load() || bypassed_.load()) {
    return;
  }

  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);

  // Apply DSP chain
  eq_.process(context);
  compressor_.process(context);
  limiter_.process(context);
}

void AIMasteringAgent::reset() {
  eq_.reset();
  compressor_.reset();
  limiter_.reset();
  isConfigured_.store(false);
}

void AIMasteringAgent::normalizeLoudness(juce::AudioBuffer<float> &buffer,
                                         float targetLufs) {
  float rms = 0.0f;
  int numSamples = buffer.getNumSamples();
  int numChannels = buffer.getNumChannels();
  for (int ch = 0; ch < numChannels; ++ch) {
    const float *data = buffer.getReadPointer(ch);
    for (int i = 0; i < numSamples; ++i) {
      rms += data[i] * data[i];
    }
  }
  rms = std::sqrt(rms / (numSamples * numChannels));
  float currentLufs = 20.0f * std::log10(rms + 1e-10f) - 10.0f;
  float gainDb = targetLufs - currentLufs;
  gainDb = juce::jlimit(-12.0f, 12.0f, gainDb);
  float gainLinear = juce::Decibels::decibelsToGain(gainDb);
  buffer.applyGain(gainLinear);
  DBG("AIMasteringAgent: Normalized loudness by " + juce::String(gainDb, 1) +
      "dB");
}

} // namespace ai
} // namespace zenith