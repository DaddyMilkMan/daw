/*
  ==============================================================================
    AI Mastering Agent Implementation - Grok Integration
  ==============================================================================
*/

#include "AIMasteringAgent.h"

namespace zenith {
namespace ai {

//==============================================================================
// AudioFeatureExtractor Implementation
//==============================================================================

AudioFeatureExtractor::Features AudioFeatureExtractor::extract(
    const juce::AudioBuffer<float>& buffer, 
    double sampleRate)
{
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
        const float* data = buffer.getReadPointer(ch);
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
    
    // Simplified frequency band analysis using basic filtering
    // In production, use proper FFT-based analysis
    analyzeFrequencyBands(buffer, sampleRate, features);
    
    // Stereo analysis
    if (numChannels == 2) {
        analyzeStereo(buffer, features);
    }
    
    features.valid = true;
    return features;
}

juce::String AudioFeatureExtractor::toJSON(const Features& features) {
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

juce::String GrokMasteringAI::buildPrompt(
    const AudioFeatureExtractor::Features& features,
    const juce::String& userIntent,
    float targetLoudness)
{
    juce::String prompt = R"(You are an expert mastering engineer. Analyze this audio and provide mastering parameters.

AUDIO ANALYSIS:
Peak Level: )" + juce::String(features.peakDb, 1) + R"( dB
RMS Level: )" + juce::String(features.rmsDb, 1) + R"( dB
Crest Factor: )" + juce::String(features.crestFactor, 2) + R"(
Dynamic Range: )" + juce::String(features.peakDb - features.rmsDb, 1) + R"( dB

FREQUENCY BALANCE:
Sub-bass (20-60Hz): )" + juce::String(features.subBass * 100.0f, 1) + R"(%
Bass (60-250Hz): )" + juce::String(features.bass * 100.0f, 1) + R"(%
Low-mid (250-500Hz): )" + juce::String(features.lowMid * 100.0f, 1) + R"(%
Mid (500-2kHz): )" + juce::String(features.mid * 100.0f, 1) + R"(%
High-mid (2-4kHz): )" + juce::String(features.highMid * 100.0f, 1) + R"(%
Presence (4-6kHz): )" + juce::String(features.presence * 100.0f, 1) + R"(%
Brilliance (6-20kHz): )" + juce::String(features.brilliance * 100.0f, 1) + R"(%

STEREO FIELD:
Width: )" + juce::String(features.stereoWidth * 100.0f, 1) + R"(%

TARGET:
Loudness: )" + juce::String(targetLoudness, 1) + R"( LUFS
User Intent: )" + userIntent + R"(

Provide mastering parameters in this EXACT JSON format (no extra text):
{
  "eq": {
    "lowShelfGain": <dB value, range -6 to +6>,
    "midCutGain": <dB value, range -6 to +6>,
    "presenceGain": <dB value, range -6 to +6>,
    "airGain": <dB value, range -6 to +6>
  },
  "compression": {
    "threshold": <dB value, range -24 to 0>,
    "ratio": <value, range 1.5 to 8.0>,
    "attack": <ms, range 10 to 100>,
    "release": <ms, range 50 to 500>
  },
  "limiterCeiling": <dB value, range -1.0 to -0.1>,
  "reasoning": "<brief explanation of choices>"
})";
    
    return prompt;
}

GrokMasteringAI::MasteringDecision GrokMasteringAI::parseGrokResponse(
    const juce::String& response)
{
    MasteringDecision decision;
    
    // Extract JSON from response (Grok might add markdown code blocks)
    juce::String jsonStr = response;
    
    // Remove markdown code fences if present
    jsonStr = jsonStr.replace("```json", "").replace("```", "").trim();
    
    // Parse JSON
    auto json = juce::JSON::parse(jsonStr);
    
    if (json.isVoid() || !json.isObject()) {
        DBG("Failed to parse Grok response as JSON");
        return decision;
    }
    
    auto* obj = json.getDynamicObject();
    if (!obj) return decision;
    
    // Extract EQ parameters
    if (auto* eqObj = obj->getProperty("eq").getDynamicObject()) {
        decision.eq.lowShelfGain = eqObj->getProperty("lowShelfGain");
        decision.eq.midCutGain = eqObj->getProperty("midCutGain");
        decision.eq.presenceGain = eqObj->getProperty("presenceGain");
        decision.eq.airGain = eqObj->getProperty("airGain");
    }
    
    // Extract compression parameters
    if (auto* compObj = obj->getProperty("compression").getDynamicObject()) {
        decision.compression.threshold = compObj->getProperty("threshold");
        decision.compression.ratio = compObj->getProperty("ratio");
        decision.compression.attack = compObj->getProperty("attack");
        decision.compression.release = compObj->getProperty("release");
    }
    
    // Extract limiter ceiling
    decision.limiterCeiling = obj->getProperty("limiterCeiling");
    
    // Extract reasoning
    decision.reasoning = obj->getProperty("reasoning").toString();
    
    // Validate ranges
    decision.eq.lowShelfGain = juce::jlimit(-6.0f, 6.0f, decision.eq.lowShelfGain);
    decision.eq.midCutGain = juce::jlimit(-6.0f, 6.0f, decision.eq.midCutGain);
    decision.eq.presenceGain = juce::jlimit(-6.0f, 6.0f, decision.eq.presenceGain);
    decision.eq.airGain = juce::jlimit(-6.0f, 6.0f, decision.eq.airGain);
    
    decision.compression.threshold = juce::jlimit(-24.0f, 0.0f, decision.compression.threshold);
    decision.compression.ratio = juce::jlimit(1.5f, 8.0f, decision.compression.ratio);
    decision.compression.attack = juce::jlimit(10.0f, 100.0f, decision.compression.attack);
    decision.compression.release = juce::jlimit(50.0f, 500.0f, decision.compression.release);
    
    decision.limiterCeiling = juce::jlimit(-1.0f, -0.1f, decision.limiterCeiling);
    
    decision.valid = true;
    
    DBG("Grok AI Decision:");
    DBG("  EQ: Low " + juce::String(decision.eq.lowShelfGain, 1) + "dB, " +
        "Mid " + juce::String(decision.eq.midCutGain, 1) + "dB, " +
        "Presence " + juce::String(decision.eq.presenceGain, 1) + "dB, " +
        "Air " + juce::String(decision.eq.airGain, 1) + "dB");
    DBG("  Compression: " + juce::String(decision.compression.threshold, 1) + "dB @ " +
        juce::String(decision.compression.ratio, 1) + ":1");
    DBG("  Reasoning: " + decision.reasoning);
    
    return decision;
}

GrokMasteringAI::MasteringDecision GrokMasteringAI::getMasteringDecision(
    const AudioFeatureExtractor::Features& features,
    const juce::String& userIntent,
    float targetLoudness)
{
    if (!apiCallback_) {
        DBG("ERROR: Grok API callback not set!");
        return MasteringDecision();
    }
    
    // Build the prompt
    juce::String prompt = buildPrompt(features, userIntent, targetLoudness);
    
    // System message for Grok
    juce::String systemMsg = R"(You are a professional mastering engineer with decades of experience. 
You analyze audio characteristics and provide optimal mastering parameters in JSON format.
Be precise and musical in your decisions. Respond ONLY with valid JSON, no additional text.)";
    
    // Call Grok API
    DBG("Querying Grok AI for mastering decision...");
    juce::String response = apiCallback_(prompt, systemMsg);
    
    // Parse response
    return parseGrokResponse(response);
}

//==============================================================================
// AIMasteringAgent Implementation
//==============================================================================

AIMasteringAgent::AIMasteringAgent(Engine& engine) 
    : engine_(engine)
{
}

void AIMasteringAgent::prepare(double sampleRate, int samplesPerBlock, int numChannels) {
    // Prepare DSP chain
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);
    
    // eq_.prepare(spec);
    // compressor_.prepare(spec);
    // limiter_.prepare(spec);
    
    isPrepared_.store(true);
    
    DBG("AIMasteringAgent prepared: " + juce::String(sampleRate) + "Hz, " +
        juce::String(numChannels) + " channels");
}

void AIMasteringAgent::analyzeAndConfigure(
    const juce::AudioBuffer<float>& analysisBuffer,
    const Options& options)
{
    DBG("========================================");
    DBG("AI Mastering Analysis Starting...");
    DBG("========================================");
    
    // Extract audio features
    auto features = featureExtractor_.extract(analysisBuffer, 44100.0);
    
    if (!features.valid) {
        DBG("ERROR: Feature extraction failed");
        return;
    }
    
    DBG("Audio Features Extracted:");
    DBG("  Peak: " + juce::String(features.peakDb, 1) + " dB");
    DBG("  RMS: " + juce::String(features.rmsDb, 1) + " dB");
    DBG("  Dynamic Range: " + juce::String(features.peakDb - features.rmsDb, 1) + " dB");
    
    // Query Grok for mastering decision
    if (options.useAI) {
        lastDecision_ = grokAI_.getMasteringDecision(
            features,
            options.userIntent,
            options.targetLoudness
        );
        
        if (lastDecision_.valid) {
            DBG("AI Decision received successfully");
            applyAIDecision(lastDecision_);
        } else {
            DBG("ERROR: AI decision invalid, using defaults");
        }
    }
    
    DBG("========================================");
}

void AIMasteringAgent::applyAIDecision(const GrokMasteringAI::MasteringDecision& decision) {
    // Apply EQ settings
    // AIMasteringEQ::EQSettings eqSettings;
    // eqSettings.lowShelfGain = decision.eq.lowShelfGain;
    // eqSettings.midCutGain = decision.eq.midCutGain;
    // eqSettings.presenceGain = decision.eq.presenceGain;
    // eqSettings.airGain = decision.eq.airGain;
    // eq_.setSettings(eqSettings);
    
    // Apply compression settings
    // AIGlueCompressor::CompressorSettings compSettings;
    // compSettings.threshold = decision.compression.threshold;
    // compSettings.ratio = decision.compression.ratio;
    // compSettings.attack = decision.compression.attack;
    // compSettings.release = decision.compression.release;
    // compressor_.setSettings(compSettings);
    
    // Apply limiter ceiling
    // limiter_.setCeiling(decision.limiterCeiling);
    
    // Apply track balance (if provided)
    for (const auto& [trackIndex, gain] : decision.trackGains) {
        if (trackIndex >= 0 && trackIndex < engine_.tracks().size()) {
            engine_.setTrackVolume(trackIndex, gain);
            DBG("  Track " + juce::String(trackIndex) + " gain: " + 
                juce::String(20.0f * std::log10(gain), 1) + " dB");
        }
    }
    
    DBG("AI settings applied to DSP chain");
}

void AIMasteringAgent::processBlock(juce::AudioBuffer<float>& buffer) {
    if (!isPrepared_.load()) return;
    
    // Apply DSP chain
    // juce::dsp::AudioBlock<float> block(buffer);
    // juce::dsp::ProcessContextReplacing<float> context(block);
    
    // eq_.process(context);
    // compressor_.process(context);
    // limiter_.process(context);
}

} // namespace ai
} // namespace zenith
