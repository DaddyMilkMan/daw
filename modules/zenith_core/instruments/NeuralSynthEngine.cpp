/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    ==============================================================================

    NeuralSynthEngine.cpp
    Created: 2025-02-05
    Author:  Zenith DAW

    Implementation of real-time neural audio synthesis engine.

    ==============================================================================
*/

#include "NeuralSynthEngine.h"
#include "PlatformModelUtils.h"
#include "SIMDHelpers.h"
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <random>
#include <algorithm>

#ifdef ZENITH_USE_ONNX_RUNTIME
#include <cpu_provider_factory.h>
#endif

namespace zenith {

//==============================================================================
// Constructor/Destructor
//==============================================================================

NeuralSynthEngine::NeuralSynthEngine() {
    // Initialize all voices
    for (auto& voice : voices_) {
        voice.clear();
        voice.latent.resize(LatentSpace::latentDim);
    }

    // Allocate inference buffer
    inferenceBuffer_.setSize(2, 2048);

    // Initialize parameter history
    learningHistory_.ensureStorageAllocated(100);

    DBG("NeuralSynthEngine: Initialized");
}

NeuralSynthEngine::~NeuralSynthEngine() {
    release();
}

//==============================================================================
// Initialization
//==============================================================================

bool NeuralSynthEngine::initialize(const juce::File& modelPath,
                                   double sampleRate,
                                   int maxBlockSize) {
    juce::ScopedLock lock(engineLock_);

    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;

    // Validate model file
    if (!modelPath.existsAsFile()) {
        DBG("NeuralSynthEngine: Model file not found: " + modelPath.getFullPathName());
        return false;
    }

    // Load ONNX model
#ifdef ZENITH_USE_ONNX_RUNTIME
    try {
        // Create ONNX Runtime environment
        if (!env_) {
            env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ZenithNeuralSynth");
        }

        // Create session options
        sessionOptions_ = std::make_unique<Ort::SessionOptions>();
        sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        sessionOptions_->SetIntraOpNumThreads(2);
        sessionOptions_->SetInterOpNumThreads(1);
        sessionOptions_->EnableCpuMemArena();

        // Create memory info
        memoryInfo_ = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

        // Load model
#ifdef _WIN32
        std::wstring wPath = modelPath.getFullPathName().toWideCharPointer();
        session_ = std::make_unique<Ort::Session>(*env_, wPath.c_str(), *sessionOptions_);
#else
        std::string sPath = modelPath.getFullPathName().toStdString();
        session_ = std::make_unique<Ort::Session>(*env_, sPath.c_str(), *sessionOptions_);
#endif

        // Get input/output names and shapes
        Ort::AllocatorWithDefaultOptions allocator;
        inputNames_.clear();
        outputNames_.clear();

        for (size_t i = 0; i < session_->GetInputCount(); ++i) {
            auto name = session_->GetInputNameAllocated(i, allocator);
            inputNames_.push_back(name.get());
        }

        for (size_t i = 0; i < session_->GetOutputCount(); ++i) {
            auto name = session_->GetOutputNameAllocated(i, allocator);
            outputNames_.push_back(name.get());
        }

        // Get input shape (assuming [batch, seq_len, features])
        if (!inputNames_.empty()) {
            Ort::TypeInfo typeInfo = session_->GetInputTypeInfo(0);
            auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            inputShape_ = tensorInfo.GetShape();
        }

        modelLoaded_ = true;
        initialized_ = true;

        DBG("NeuralSynthEngine: Model loaded successfully");
        DBG("  Model: " + modelPath.getFileName());
        DBG("  Inputs: " + juce::String((int)inputNames_.size()));
        DBG("  Outputs: " + juce::String((int)outputNames_.size()));

        return true;

    } catch (const Ort::Exception& e) {
        DBG("NeuralSynthEngine: ONNX Runtime error: " + juce::String(e.what()));
        modelLoaded_ = false;
        initialized_ = false;
        return false;
    }
#else
    // ONNX Runtime not available - use DSP fallback
    DBG("NeuralSynthEngine: ONNX Runtime not available, using DSP fallback");
    modelLoaded_ = false;
    initialized_ = true;
    return true;
#endif
}

bool NeuralSynthEngine::initializeWithType(NeuralModelType modelType,
                                          double sampleRate,
                                          int maxBlockSize) {
    auto modelFile = NeuralSynthFactory::findDefaultModel(modelType);
    return initialize(modelFile, sampleRate, maxBlockSize);
}

void NeuralSynthEngine::release() {
    juce::ScopedLock lock(engineLock_);

#ifdef ZENITH_USE_ONNX_RUNTIME
    session_.reset();
    sessionOptions_.reset();
    memoryInfo_.reset();
#endif

    modelLoaded_ = false;
    initialized_ = false;

    // Clear all voices
    for (auto& voice : voices_) {
        voice.clear();
    }

    activeVoiceCount_ = 0;
}

//==============================================================================
// Real-time Synthesis
//==============================================================================

void NeuralSynthEngine::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                       int startSample,
                                       int numSamples,
                                       const juce::MidiBuffer& midiMessages) {
    // Clear output buffer
    outputBuffer.clear(startSample, numSamples);

    if (!initialized_) {
        return;
    }

    // Process MIDI for this block
    processMidi(midiMessages, numSamples);

    // Create temporary buffer for mixing
    juce::AudioBuffer<float> voiceMixBuffer(2, numSamples);
    voiceMixBuffer.clear();

    // Render each active voice
    int activeCount = 0;
    for (auto& voice : voices_) {
        if (voice.active) {
            voiceMixBuffer.clear();
            renderVoice(voice, voiceMixBuffer, numSamples);

            // Mix voice output
            for (int ch = 0; ch < 2; ++ch) {
                outputBuffer.addFrom(ch, startSample,
                                   voiceMixBuffer, ch, 0, numSamples);
            }
            activeCount++;
        }
    }

    activeVoiceCount_ = activeCount;

    // Post-process output
    juce::AudioBuffer<float> processBuffer(outputBuffer.getArrayOfWritePointers(),
                                          2, startSample, numSamples);
    postProcess(processBuffer, numSamples);

    // Update performance metrics
    if (adaptiveQualityEnabled_) {
        updateAdaptiveQuality();
    }
}

void NeuralSynthEngine::noteOn(int midiNote, float velocity, float pitchBend) {
    juce::ScopedLock lock(engineLock_);

    // Find free voice
    int voiceIndex = findFreeVoice();

    if (voiceIndex < 0) {
        stealVoice();
        voiceIndex = findFreeVoice();
    }

    if (voiceIndex >= 0) {
        auto& voice = voices_[voiceIndex];

        // Initialize voice
        voice.active = true;
        voice.midiNote = midiNote;
        voice.frequency = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
        voice.velocity = velocity;
        voice.amplitude = velocity;
        voice.phase = 0.0;
        voice.modPhase = 0.0f;

        // Set up envelope
        voice.envelope.setParameters({
            0.01f,   // attack
            0.1f,    // decay
            0.7f,    // sustain
            0.2f     // release
        });
        voice.envelope.noteOn();

        // Initialize latent space with frequency-dependent encoding
        processLatentSpace(voice, 0);

        DBG("NeuralSynthEngine: Note on - MIDI " + juce::String(midiNote) +
            " (voice " + juce::String(voiceIndex) + ")");
    }
}

void NeuralSynthEngine::noteOff(int midiNote, bool allowTailOff) {
    juce::ScopedLock lock(engineLock_);

    auto* voice = findVoice(midiNote);
    if (voice != nullptr && voice->active) {
        if (allowTailOff) {
            voice->envelope.noteOff();
        } else {
            voice->active = false;
            voice->amplitude = 0.0f;
        }
    }
}

void NeuralSynthEngine::setPressure(int midiNote, float pressure) {
    juce::ScopedLock lock(engineLock_);

    auto* voice = findVoice(midiNote);
    if (voice != nullptr && voice->active) {
        parameters_.mpePressure = pressure;
    }
}

void NeuralSynthEngine::setTimbre(int midiNote, float timbre) {
    juce::ScopedLock lock(engineLock_);

    auto* voice = findVoice(midiNote);
    if (voice != nullptr && voice->active) {
        parameters_.mpeTimbre = timbre;
    }
}

void NeuralSynthEngine::setPitchBend(int midiNote, float pitchBend) {
    juce::ScopedLock lock(engineLock_);

    auto* voice = findVoice(midiNote);
    if (voice != nullptr && voice->active) {
        parameters_.mpePitchBend = pitchBend;
        // Update frequency
        float bendFactor = std::pow(2.0f, pitchBend * 2.0f / 12.0f); // ±2 semitones
        voice->frequency = 440.0f * std::pow(2.0f, (voice->midiNote - 69) / 12.0f) * bendFactor;
    }
}

//==============================================================================
// Parameters
//==============================================================================

void NeuralSynthEngine::setParameters(const NeuralSynthParameters& params) {
    juce::ScopedLock lock(engineLock_);
    parameters_ = params;
}

void NeuralSynthEngine::setParameterValue(int parameterIndex, float value) {
    juce::ScopedLock lock(engineLock_);

    switch (parameterIndex) {
        case 0: parameters_.complexity = value; break;
        case 1: parameters_.temperature = value; break;
        case 2: parameters_.brightness = value; break;
        case 3: parameters_.spectralContrast = value; break;
        case 4: parameters_.modulationDepth = value; break;
        case 5: parameters_.harmonicity = value; break;
        case 6: parameters_.morphPosition = value; break;
        default:
            if (parameterIndex - 7 < 16) {
                parameters_.timbreVector[parameterIndex - 7] = value;
            }
            break;
    }
}

void NeuralSynthEngine::setTimbreVector(const float* vector, int size) {
    juce::ScopedLock lock(engineLock_);

    int copySize = juce::jmin(size, 16);
    juce::FloatVectorOperations::copy(parameters_.timbreVector, vector, copySize);
}

//==============================================================================
// Timbre Transfer
//==============================================================================

std::vector<float> NeuralSynthEngine::extractTimbre(const juce::AudioBuffer<float>& audio) {
    // Perform FFT-based spectral analysis for timbre extraction
    const int fftSize = 2048;
    const int numBins = fftSize / 2 + 1;

    // Compute FFT
    std::vector<float> magnitudeSpectrum(numBins);
    std::vector<float> phaseSpectrum(numBins);

    // Simple spectral centroid and contrast calculation
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    float spectralCentroid = 0.0f;

    for (int i = 0; i < numBins; ++i) {
        float freq = (float)i * sampleRate_ / (float)fftSize;
        weightedSum += freq * magnitudeSpectrum[i];
        magnitudeSum += magnitudeSpectrum[i];
    }

    if (magnitudeSum > 0.0f) {
        spectralCentroid = weightedSum / magnitudeSum;
    }

    // Create timbre embedding (simplified version)
    std::vector<float> timbreVector(16);

    timbreVector[0] = juce::jmap(spectralCentroid, 0.0f, sampleRate_ / 2.0f, 0.0f, 1.0f);
    timbreVector[1] = parameters_.brightness;
    timbreVector[2] = parameters_.spectralContrast;
    timbreVector[3] = parameters_.harmonicity;

    // Fill remaining with spectral features
    for (int i = 4; i < 16; ++i) {
        if (i < numBins) {
            timbreVector[i] = magnitudeSpectrum[i] / (magnitudeSum + 1e-6f);
        } else {
            timbreVector[i] = 0.0f;
        }
    }

    return timbreVector;
}

void NeuralSynthEngine::applyTimbreTransfer(const std::vector<float>& sourceTimbre,
                                           float transferAmount) {
    juce::ScopedLock lock(engineLock_);

    // Interpolate between current and source timbre
    for (int i = 0; i < 16; ++i) {
        if (i < sourceTimbre.size()) {
            parameters_.timbreVector[i] = juce::jmap(transferAmount,
                                                     parameters_.timbreVector[i],
                                                     sourceTimbre[i]);
        }
    }
}

void NeuralSynthEngine::morphTimbres(const std::vector<float>& timbreA,
                                    const std::vector<float>& timbreB,
                                    float morphPosition) {
    juce::ScopedLock lock(engineLock_);

    // Spherical interpolation for smoother morphing
    auto result = slerp(timbreA, timbreB, morphPosition);

    int copySize = juce::jmin((int)result.size(), 16);
    for (int i = 0; i < copySize; ++i) {
        parameters_.timbreVector[i] = result[i];
    }
}

//==============================================================================
// AI-Powered Suggestions
//==============================================================================

NeuralSynthParameters NeuralSynthEngine::getSuggestedParameters(const juce::String& targetDescription) {
    NeuralSynthParameters params = parameters_;

    // Simple rule-based system (in production, would use actual ML model)
    juce::String desc = targetDescription.toLowerCase();

    if (desc.contains("warm") || desc.contains("pad")) {
        params.brightness = 0.3f;
        params.spectralContrast = 0.4f;
        params.harmonicity = 0.8f;
        params.modulationDepth = 0.2f;
    } else if (desc.contains("bright") || desc.contains("lead")) {
        params.brightness = 0.8f;
        params.spectralContrast = 0.7f;
        params.harmonicity = 0.6f;
        params.modulationDepth = 0.5f;
    } else if (desc.contains("bass")) {
        params.brightness = 0.2f;
        params.spectralContrast = 0.3f;
        params.harmonicity = 0.9f;
        params.modulationDepth = 0.3f;
    } else if (desc.contains("pluck") || desc.contains("percussive")) {
        params.brightness = 0.6f;
        params.spectralContrast = 0.8f;
        params.harmonicity = 0.5f;
        params.modulationDepth = 0.1f;
    } else if (desc.contains("ambient") || desc.contains("evolving")) {
        params.brightness = 0.5f;
        params.spectralContrast = 0.3f;
        params.harmonicity = 0.7f;
        params.modulationDepth = 0.6f;
    }

    // Set default envelope based on type
    if (desc.contains("pad") || desc.contains("ambient")) {
        // Slow envelope
    } else if (desc.contains("pluck") || desc.contains("percussive")) {
        // Fast envelope
    }

    return params;
}

void NeuralSynthEngine::provideFeedback(const NeuralSynthParameters& parameters, float reward) {
    juce::ScopedLock lock(engineLock_);

    ParameterHistory history;
    history.params = parameters;
    history.reward = reward;
    history.timestamp = juce::Time::getMillisecondCounter();

    learningHistory_.add(history);

    // Keep only recent history
    if (learningHistory_.size() > 100) {
        learningHistory_.remove(0);
    }
}

NeuralSynthParameters NeuralSynthEngine::exploreVariations(const NeuralSynthParameters& baseParameters,
                                                          float variationAmount) {
    NeuralSynthParameters varied = baseParameters;

    // Add random variations
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-variationAmount, variationAmount);

    varied.brightness = juce::jlimit(0.0f, 1.0f, varied.brightness + dis(gen));
    varied.spectralContrast = juce::jlimit(0.0f, 1.0f, varied.spectralContrast + dis(gen));
    varied.harmonicity = juce::jlimit(0.0f, 1.0f, varied.harmonicity + dis(gen));
    varied.modulationDepth = juce::jlimit(0.0f, 1.0f, varied.modulationDepth + dis(gen));
    varied.temperature = juce::jlimit(0.1f, 2.0f, varied.temperature + dis(gen) * 0.5f);

    return varied;
}

//==============================================================================
// Model Management
//==============================================================================

bool NeuralSynthEngine::loadModel(const juce::File& modelPath) {
    return initialize(modelPath, sampleRate_, maxBlockSize_);
}

juce::String NeuralSynthEngine::getModelInfo() const {
    juce::String info;

    info << "Neural Synthesis Engine\n";
    info << "======================\n";
    info << "Initialized: " << (initialized_ ? "Yes" : "No") << "\n";
    info << "Model Loaded: " << (modelLoaded_ ? "Yes" : "No") << "\n";
    info << "Sample Rate: " << sampleRate_ << "\n";
    info << "Max Block Size: " << maxBlockSize_ << "\n";
    info << "Active Voices: " << activeVoiceCount_ << "\n";
    info << "CPU Usage: " << juce::String(cpuUsage_ * 100.0f, 1) << "%\n";
    info << "Avg Inference Time: " << juce::String(avgInferenceTime_, 2) << "ms\n";

#ifdef ZENITH_USE_ONNX_RUNTIME
    info << "\nONNX Runtime Status:\n";
    if (session_) {
        info << "  Session: Active\n";
        info << "  Inputs: " << (int)inputNames_.size() << "\n";
        info << "  Outputs: " << (int)outputNames_.size() << "\n";
    } else {
        info << "  Session: Inactive\n";
    }
#else
    info << "\nONNX Runtime: Not compiled in\n";
#endif

    return info;
}

bool NeuralSynthEngine::isONNXRuntimeAvailable() {
#ifdef ZENITH_USE_ONNX_RUNTIME
    return true;
#else
    return false;
#endif
}

//==============================================================================
// State Management
//==============================================================================

void NeuralSynthEngine::reset() {
    juce::ScopedLock lock(engineLock_);

    for (auto& voice : voices_) {
        voice.clear();
    }

    activeVoiceCount_ = 0;
    cpuUsage_ = 0.0f;
    avgInferenceTime_ = 0.0f;
}

void NeuralSynthEngine::getState(juce::MemoryBlock& block) {
    // Serialize parameters
    block.append(&parameters_, sizeof(NeuralSynthParameters));

    // Serialize active voices
    for (const auto& voice : voices_) {
        if (voice.active) {
            block.append(&voice, sizeof(NeuralVoice));
        }
    }
}

void NeuralSynthEngine::setState(const void* data, int sizeInBytes) {
    juce::ScopedLock lock(engineLock_);

    const char* ptr = static_cast<const char*>(data);

    // Restore parameters
    if (sizeInBytes >= sizeof(NeuralSynthParameters)) {
        juce::FloatVectorOperations::copy(
            reinterpret_cast<float*>(&parameters_),
            reinterpret_cast<const float*>(ptr),
            sizeof(NeuralSynthParameters) / sizeof(float));
        ptr += sizeof(NeuralSynthParameters);
    }

    // Restore voices
    // (simplified - would need proper versioning in production)
}

//==============================================================================
// Internal Processing
//==============================================================================

void NeuralSynthEngine::processMidi(const juce::MidiBuffer& midiMessages, int numSamples) {
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        const int samplePosition = metadata.samplePosition;

        if (samplePosition >= numSamples) continue;

        switch (msg.getRawData()[0] & 0xf0) {
            case 0x90: // Note on
                if (msg.getVelocity() > 0) {
                    noteOn(msg.getNoteNumber(), msg.getVelocity() / 127.0f);
                } else {
                    noteOff(msg.getNoteNumber(), true);
                }
                break;

            case 0x80: // Note off
                noteOff(msg.getNoteNumber(), true);
                break;

            case 0xA0: // Polyphonic aftertouch (pressure)
                setPressure(msg.getNoteNumber(), msg.getAfterTouchValue() / 127.0f);
                break;

            case 0xE0: // Pitch bend
                setPitchBend(msg.getNoteNumber(),
                           (msg.getPitchWheelValue() - 8192) / 8192.0f);
                break;
        }
    }
}

void NeuralSynthEngine::renderVoice(NeuralVoice& voice,
                                   juce::AudioBuffer<float>& buffer,
                                   int numSamples) {
    if (!voice.active) {
        buffer.clear();
        return;
    }

    // Try neural inference first
    auto result = runInference(voice, numSamples);

    if (result.success) {
        // Use neural output
        int samplesToCopy = juce::jmin(numSamples, result.audio.getNumSamples());

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            juce::FloatVectorOperations::copy(
                buffer.getWritePointer(ch),
                result.audio.getReadPointer(ch),
                samplesToCopy);
        }
    } else {
        // Fall back to DSP synthesis
        generateDSPAudio(voice, buffer, numSamples);
    }

    // Apply envelope
    auto envelopeBuffer = buffer.getArrayOfWritePointers();
    voice.envelope.applyEnvelopeToBuffer(buffer, 0, numSamples);

    // Check if voice finished
    if (!voice.envelope.isActive()) {
        voice.active = false;
    }
}

NeuralInferenceResult NeuralSynthEngine::runInference(const NeuralVoice& voice, int numSamples) {
    NeuralInferenceResult result;

#ifdef ZENITH_USE_ONNX_RUNTIME
    if (!modelLoaded_ || !session_) {
        return result;
    }

    try {
        auto startTime = juce::Time::getMillisecondCounterHiRes();

        // Prepare input tensor
        // Shape: [batch=1, latent_dim]
        std::vector<int64_t> inputShape = {1, LatentSpace::latentDim};

        // Prepare latent vector with voice parameters
        inputTensorData_.resize(LatentSpace::latentDim);

        // Encode frequency, velocity, and MPE controls into latent space
        for (int i = 0; i < LatentSpace::latentDim; ++i) {
            float val = 0.0f;

            // Base latent representation from voice state
            if (i < voice.latent.z.size()) {
                val = voice.latent.z[i];
            }

            // Add timbre vector influence
            if (i < 16) {
                val += parameters_.timbreVector[i] * 0.5f;
            }

            // Add frequency encoding (position encoding style)
            float phase = voice.phase + (i * 0.1f);
            val += std::sin(phase) * 0.1f;

            // Add brightness control
            val *= (0.5f + parameters_.brightness);

            inputTensorData_[i] = val;
        }

        // Create input tensor
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            *memoryInfo_,
            inputTensorData_.data(),
            inputTensorData_.size(),
            inputShape.data(),
            inputShape.size());

        // Run inference
        auto outputTensors = session_->Run(
            Ort::RunOptions{nullptr},
            inputNames_.data(),
            &inputTensor,
            1,
            outputNames_.data(),
            outputNames_.size());

        auto endTime = juce::Time::getMillisecondCounterHiRes();
        result.inferenceTimeMs = endTime - startTime;

        // Process output
        if (outputTensors.size() > 0) {
            auto* outputData = outputTensors[0].GetTensorMutableData<float>();
            auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

            // Output shape should be [batch=1, samples] or [batch=1, channels, samples]
            int outputSamples = (outputShape.size() > 1) ? outputShape[1] : numSamples;

            // Copy to result buffer
            result.audio.setSize(2, numSamples);

            if (outputShape.size() == 3 && outputShape[1] == 2) {
                // Stereo output [1, 2, samples]
                for (int ch = 0; ch < 2; ++ch) {
                    int samples = juce::jmin(numSamples, (int)outputShape[2]);
                    juce::FloatVectorOperations::copy(
                        result.audio.getWritePointer(ch),
                        outputData + (ch * outputShape[2]),
                        samples);
                }
            } else {
                // Mono or interleaved - copy to both channels
                int samples = juce::jmin(numSamples, (int)outputSamples);
                juce::FloatVectorOperations::copy(
                    result.audio.getWritePointer(0),
                    outputData,
                    samples);
                juce::FloatVectorOperations::copy(
                    result.audio.getWritePointer(1),
                    outputData,
                    samples);
            }

            result.success = true;
            result.confidence = 0.8f; // Placeholder - would come from model if available

            // Update timing stats
            inferenceTimes_.add(result.inferenceTimeMs);
            if (inferenceTimes_.size() > 100) {
                inferenceTimes_.remove(0);
            }

            // Calculate average
            double sum = 0.0;
            for (auto time : inferenceTimes_) {
                sum += time;
            }
            avgInferenceTime_ = sum / inferenceTimes_.size();
        }

    } catch (const Ort::Exception& e) {
        DBG("NeuralSynthEngine: Inference error: " + juce::String(e.what()));
        result.success = false;
        result.errorMessage = e.what();
    }
#else
    // ONNX Runtime not available
    result.success = false;
#endif

    return result;
}

void NeuralSynthEngine::generateDSPAudio(NeuralVoice& voice,
                                        juce::AudioBuffer<float>& buffer,
                                        int numSamples) {
    // DSP-based synthesis as fallback
    // Uses additive synthesis with neural-like modulation

    float** out = buffer.getArrayOfWritePointers();

    // Oscillator parameters
    float fundamental = voice.frequency;
    int numHarmonics = 8;
    float brightness = parameters_.brightness;
    float modulation = parameters_.modulationDepth;
    float harmonicity = parameters_.harmonicity;

    for (int i = 0; i < numSamples; ++i) {
        float sample = 0.0f;

        // Generate harmonics with amplitude envelope
        for (int h = 1; h <= numHarmonics; ++h) {
            float harmonicFreq = fundamental * h;

            // Amplitude based on brightness and harmonic number
            float harmonicAmp = 1.0f / std::pow((float)h, brightness + 0.5f);
            harmonicAmp *= harmonicity;

            // Additive synthesis
            float phase = voice.phase * h;
            sample += std::sin(phase) * harmonicAmp;

            // Add inharmonic partials
            if (modulation > 0.0f) {
                float inharmonicFreq = fundamental * (h + 0.5f);
                float modPhase = voice.modPhase * (h + 0.5f);
                sample += std::sin(modPhase) * harmonicAmp * modulation;
            }
        }

        // Normalize
        sample *= 0.3f;

        // Apply amplitude
        sample *= voice.amplitude;

        out[0][i] = sample;
        out[1][i] = sample;

        // Update phases
        double phaseIncrement = (2.0 * juce::MathConstants<double>::pi * fundamental) / sampleRate_;
        voice.phase += phaseIncrement;
        voice.modPhase += phaseIncrement * 1.5; // Slight detune for modulation

        // Wrap phases
        while (voice.phase >= 2.0 * juce::MathConstants<double>::pi) {
            voice.phase -= 2.0 * juce::MathConstants<double>::pi;
        }
        while (voice.modPhase >= 2.0 * juce::MathConstants<double>::pi) {
            voice.modPhase -= 2.0 * juce::MathConstants<double>::pi;
        }
    }
}

void NeuralSynthEngine::processLatentSpace(NeuralVoice& voice, int numSamples) {
    // Initialize latent space based on voice parameters

    // Encode frequency using sinusoidal position encoding
    for (int i = 0; i < LatentSpace::latentDim; ++i) {
        float freq = voice.frequency;

        // Position encoding
        float posEnc = std::sin((i / 256.0f) * juce::MathConstants<float>::pi * freq / 100.0f);

        // Combine with timbre vector
        float timbre = (i < 16) ? parameters_.timbreVector[i] : 0.0f;

        // Set latent value
        voice.latent.z[i] = posEnc * 0.5f + timbre * 0.5f;
        voice.latent.zMean[i] = voice.latent.z[i];
        voice.latent.zLogVar[i] = 0.0f; // Deterministic encoding
    }

    // Apply temperature-based noise
    if (parameters_.temperature > 0.0f) {
        addLatentNoise(voice.latent.z, parameters_.temperature * 0.1f);
    }
}

void NeuralSynthEngine::postProcess(juce::AudioBuffer<float>& buffer, int numSamples) {
    // Apply soft clipping to prevent harsh distortion
    simd::applySoftClip(buffer, 1.2f);

    // Apply subtle saturation
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            // Mild tube-style saturation
            float x = data[i];
            float absx = std::abs(x);
            if (absx > 0.01f) {
                data[i] = x * (1.0f + 0.1f * std::exp(-absx * 10.0f));
            }
        }
    }
}

//==============================================================================
// Voice Management
//==============================================================================

int NeuralSynthEngine::findFreeVoice() const {
    for (int i = 0; i < maxVoices_; ++i) {
        if (!voices_[i].active) {
            return i;
        }
    }
    return -1;
}

NeuralVoice* NeuralSynthEngine::findVoice(int midiNote) {
    for (auto& voice : voices_) {
        if (voice.active && voice.midiNote == midiNote) {
            return &voice;
        }
    }
    return nullptr;
}

void NeuralSynthEngine::stealVoice() {
    // Find oldest voice with lowest amplitude
    int stealIndex = -1;
    float minAmplitude = std::numeric_limits<float>::max();
    juce::uint64 oldestTime = std::numeric_limits<juce::uint64>::max();

    for (int i = 0; i < maxVoices_; ++i) {
        if (voices_[i].active) {
            if (voices_[i].amplitude < minAmplitude) {
                minAmplitude = voices_[i].amplitude;
                stealIndex = i;
            }
        }
    }

    if (stealIndex >= 0) {
        voices_[stealIndex].active = false;
        voices_[stealIndex].clear();
        DBG("NeuralSynthEngine: Stole voice " + juce::String(stealIndex));
    }
}

//==============================================================================
// Neural Network Operations
//==============================================================================

std::vector<float> NeuralSynthEngine::encodeToLatent(const juce::AudioBuffer<float>& audio) {
    // Simplified encoding - in production would use actual encoder model
    std::vector<float> latent(LatentSpace::latentDim, 0.0f);

    const float* left = audio.getReadPointer(0);
    int numSamples = audio.getNumSamples();

    // Compute simple features
    float rms = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        rms += left[i] * left[i];
    }
    rms = std::sqrt(rms / numSamples);

    latent[0] = rms;

    // Fill rest with zero (would be actual encoder output in production)
    return latent;
}

juce::AudioBuffer<float> NeuralSynthEngine::decodeFromLatent(const std::vector<float>& latent) {
    // Simplified decoding - in production would use actual decoder model
    juce::AudioBuffer<float> audio(2, 512);
    audio.clear();

    // Placeholder - actual decoder would generate audio from latent
    return audio;
}

void NeuralSynthEngine::sampleLatent(LatentSpace& latent) {
    // VAE-style reparameterization trick
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dis(0.0f, 1.0f);

    for (size_t i = 0; i < latent.z.size(); ++i) {
        float epsilon = dis(gen);
        latent.z[i] = latent.zMean[i] + std::exp(latent.zLogVar[i] * 0.5f) * epsilon;
    }
}

//==============================================================================
// AI/ML Helper Methods
//==============================================================================

std::vector<float> NeuralSynthEngine::slerp(const std::vector<float>& a,
                                           const std::vector<float>& b,
                                           float t) {
    // Spherical linear interpolation for smoother latent space interpolation
    size_t size = juce::jmin(a.size(), b.size());
    std::vector<float> result(size, 0.0f);

    // Normalize vectors
    std::vector<float> aNorm = a;
    std::vector<float> bNorm = b;
    normalizeLatent(aNorm);
    normalizeLatent(bNorm);

    // Compute dot product
    float dot = 0.0f;
    for (size_t i = 0; i < size; ++i) {
        dot += aNorm[i] * bNorm[i];
    }

    // Clamp for numerical stability
    dot = juce::jlimit(-1.0f, 1.0f, dot);

    // Compute angle
    float theta = std::acos(dot) * t;

    // Compute interpolation
    float sinTheta = std::sin(theta);
    float scaleA = std::sin((1.0f - t) * theta) / sinTheta;
    float scaleB = std::sin(t * theta) / sinTheta;

    for (size_t i = 0; i < size; ++i) {
        result[i] = scaleA * aNorm[i] + scaleB * bNorm[i];
    }

    return result;
}

void NeuralSynthEngine::normalizeLatent(std::vector<float>& latent) {
    float sum = 0.0f;
    for (float val : latent) {
        sum += val * val;
    }

    float norm = std::sqrt(sum + 1e-6f);
    if (norm > 1e-6f) {
        for (float& val : latent) {
            val /= norm;
        }
    }
}

void NeuralSynthEngine::addLatentNoise(std::vector<float>& latent, float amount) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dis(0.0f, amount);

    for (float& val : latent) {
        val += dis(gen);
    }
}

//==============================================================================
// Performance Optimization
//==============================================================================

void NeuralSynthEngine::updateAdaptiveQuality() {
    // Adjust quality based on CPU usage and inference time

    if (cpuUsage_ > parameters_.cpuLimit || avgInferenceTime_ > 10.0) {
        // CPU overload - reduce quality
        if (currentQuality_ != QualityPreset::Low) {
            currentQuality_ = QualityPreset::Low;
            DBG("NeuralSynthEngine: Reducing quality to Low");
        }
    } else if (cpuUsage_ < parameters_.cpuLimit * 0.7 && avgInferenceTime_ < 5.0) {
        // CPU headroom - increase quality
        if (currentQuality_ != QualityPreset::High) {
            currentQuality_ = QualityPreset::High;
            DBG("NeuralSynthEngine: Increasing quality to High");
        }
    } else {
        // Medium quality
        currentQuality_ = QualityPreset::Medium;
    }
}

void NeuralSynthEngine::preComputeValues() {
    // Pre-compute frequently used values
    // This would include lookup tables, etc.
}

//==============================================================================
// NeuralSynthFactory Implementation
//==============================================================================

juce::File NeuralSynthFactory::findDefaultModel(NeuralModelType type) {
    // Try platform-specific model paths
    juce::Array<juce::File> searchPaths;

    // Add platform-specific paths
#ifdef _WIN32
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Zenith").getChildFile("models"));
#elif __APPLE__
    searchPaths.add(juce::File("/Library/Audio/Plug-Ins/VST3/Zenith.models"));
#else
    searchPaths.add(juce::File("/usr/share/zenith/models"));
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile(".zenith").getChildFile("models"));
#endif

    // Add development path
    searchPaths.add(juce::File("/home/micah/Desktop/zenith/daw/models"));

    // Model filenames for each type
    juce::String modelFilename;
    switch (type) {
        case NeuralModelType::WaveNet:
            modelFilename = "wavenet_synthesizer.onnx";
            break;
        case NeuralModelType::DDSP:
            modelFilename = "ddsp_synthesizer.onnx";
            break;
        case NeuralModelType::NSynth:
            modelFilename = "nsynth_encoder.onnx";
            break;
        case NeuralModelType::Rave:
            modelFilename = "rave_generator.onnx";
            break;
        case NeuralModelType::TimbreTransfer:
            modelFilename = "timbre_transfer.onnx";
            break;
        default:
            modelFilename = "neural_synth.onnx";
            break;
    }

    // Search for model file
    for (auto& path : searchPaths) {
        juce::File modelFile = path.getChildFile(modelFilename);
        if (modelFile.existsAsFile()) {
            DBG("NeuralSynthFactory: Found model at " + modelFile.getFullPathName());
            return modelFile;
        }
    }

    DBG("NeuralSynthFactory: Model not found - " + modelFilename);
    return juce::File();
}

bool NeuralSynthFactory::validateModel(const juce::File& modelFile) {
    if (!modelFile.existsAsFile()) {
        return false;
    }

    // Check file size
    auto fileSize = modelFile.getSize();
    if (fileSize < 1024) {
        DBG("NeuralSynthFactory: Model file too small");
        return false;
    }

    // Check ONNX header
    juce::FileInputStream stream(modelFile);
    if (!stream.openedOk()) {
        return false;
    }

    char header[4];
    if (stream.read(header, 4) != 4) {
        return false;
    }

    // ONNX files typically start with specific protobuf markers
    // This is a simplified check
    return true;
}

int NeuralSynthFactory::getRecommendedBufferSize(NeuralModelType type) {
    switch (type) {
        case NeuralModelType::WaveNet:
            return 256;  // WaveNet needs smaller buffers for low latency
        case NeuralModelType::DDSP:
            return 512;  // DDSP works well with medium buffers
        case NeuralModelType::NSynth:
            return 1024; // NSynth can use larger buffers
        case NeuralModelType::Rave:
            return 512;  // RAVE optimized for real-time
        case NeuralModelType::TimbreTransfer:
            return 1024; // Timbre transfer less latency-sensitive
        default:
            return 512;
    }
}

float NeuralSynthFactory::getExpectedLatency(NeuralModelType type) {
    switch (type) {
        case NeuralModelType::WaveNet:
            return 5.0f;  // 5ms typical
        case NeuralModelType::DDSP:
            return 3.0f;  // 3ms typical
        case NeuralModelType::NSynth:
            return 8.0f;  // 8ms typical
        case NeuralModelType::Rave:
            return 4.0f;  // 4ms typical
        case NeuralModelType::TimbreTransfer:
            return 10.0f; // 10ms typical
        default:
            return 5.0f;
    }
}

} // namespace zenith
