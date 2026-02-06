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

    TimbreTransfer.cpp
    Created: 2025-02-05
    Author:  Zenith DAW

    Implementation of real-time timbre extraction and transfer.

    ==============================================================================
*/

#include "TimbreTransfer.h"
#include "SIMDHelpers.h"
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

#ifdef ZENITH_USE_ONNX_RUNTIME
#include <cpu_provider_factory.h>
#endif

namespace zenith {

//==============================================================================
// Construction/Destruction
//==============================================================================

TimbreTransfer::TimbreTransfer()
    : fft_(11), // 2^11 = 2048
      window_(2048, juce::dsp::WindowingFunction<float>::hann) {
    // Initialize analysis history
    analysisHistory_.resize(historySize);

    // Allocate FFT buffer
    fftBuffer_.resize(2048);
    magnitudeBuffer_.resize(1024);
    phaseBuffer_.resize(1024);

    DBG("TimbreTransfer: Initialized");
}

TimbreTransfer::~TimbreTransfer() {
    release();
}

//==============================================================================
// Initialization
//==============================================================================

bool TimbreTransfer::initialize(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;

    // Set preset database path
    presetDatabasePath_ = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Zenith")
        .getChildFile("timbre_presets.xml");

    // Load existing presets
    if (presetDatabasePath_.existsAsFile()) {
        importPresets(presetDatabasePath_);
    }

    // Create built-in presets if none exist
    if (presets_.isEmpty()) {
        presets_ = TimbreTransferFactory::createBuiltInPresets();
    }

    // Initialize MFCC filterbank
    int numMelFilters = 40;
    melFilterbank_.resize(numMelFilters * 513); // 513 frequency bins for 1024-point FFT

    for (int i = 0; i < numMelFilters; ++i) {
        float melCenter = 2595.0f * std::log10(1.0f + (i + 1) * 7000.0f / (numMelFilters * 700.0f));
        float freqCenter = 700.0f * (std::pow(10.0f, melCenter / 2595.0f) - 1.0f);
        int binCenter = (int)(freqCenter * 1024.0f / (sampleRate / 2.0f));

        // Create triangular filter
        for (int j = 0; j < 513; ++j) {
            float freq = j * sampleRate / 2048.0f;
            float weight = 0.0f;

            if (i > 0) {
                float prevCenter = (i - 1 + 1) * sampleRate / numMelFilters;
                float nextCenter = (i + 2) * sampleRate / numMelFilters;

                if (freq >= prevCenter && freq < freqCenter) {
                    weight = (freq - prevCenter) / (freqCenter - prevCenter);
                } else if (freq >= freqCenter && freq < nextCenter) {
                    weight = (nextCenter - freq) / (nextCenter - freqCenter);
                }
            }

            melFilterbank_[i * 513 + j] = weight;
        }
    }

    // Initialize DCT matrix for MFCC
    dctMatrix_.resize(numMelFilters * TimbreAnalysis::numMFCC);
    for (int i = 0; i < TimbreAnalysis::numMFCC; ++i) {
        for (int j = 0; j < numMelFilters; ++j) {
            dctMatrix_[i * numMelFilters + j] = std::cos((juce::MathConstants<float>::pi * i * (j + 0.5f)) / numMelFilters);
        }
    }

    // Allocate processing buffer
    processBuffer_.setSize(2, 2048);

    initialized_ = true;
    DBG("TimbreTransfer: Initialized successfully");

    return true;
}

void TimbreTransfer::release() {
    juce::ScopedLock lock(transferLock_);

#ifdef ZENITH_USE_ONNX_RUNTIME
    session_.reset();
    sessionOptions_.reset();
    env_.reset();
#endif

    neuralModelLoaded_ = false;
    initialized_ = false;
}

//==============================================================================
// Timbre Analysis
//==============================================================================

TimbreAnalysis TimbreTransfer::analyzeTimbre(const juce::AudioBuffer<float>& audio) {
    TimbreAnalysis analysis;

    if (!initialized_) {
        DBG("TimbreTransfer: Not initialized");
        return analysis;
    }

    auto startTime = juce::Time::getMillisecondCounterHiRes();

    // Extract all features
    extractSpectralFeatures(audio, analysis);
    extractMFCCs(audio, analysis);
    extractChroma(audio, analysis);
    extractHarmonicFeatures(audio, analysis);

    // Calculate temporal features
    const float* left = audio.getReadPointer(0);
    int numSamples = audio.getNumSamples();

    float sumSquares = 0.0f;
    int zeroCrossings = 0;
    float prevSample = left[0];

    for (int i = 0; i < numSamples; ++i) {
        float sample = left[i];
        sumSquares += sample * sample;

        if ((prevSample >= 0.0f && sample < 0.0f) || (prevSample < 0.0f && sample >= 0.0f)) {
            zeroCrossings++;
        }
        prevSample = sample;
    }

    analysis.rms = std::sqrt(sumSquares / numSamples);
    analysis.energy = sumSquares;
    analysis.zeroCrossingRate = (float)zeroCrossings / numSamples;

    // Extract neural embedding (if available)
    if (neuralModelLoaded_) {
        analysis.neuralEmbedding = extractEmbedding(audio);
    } else {
        // Use MFCCs as embedding
        analysis.neuralEmbedding = analysis.mfcc;
    }

    auto endTime = juce::Time::getMillisecondCounterHiRes();
    analysis.analysisTime = endTime - startTime;
    analysis.valid = true;

    // Update history
    for (int i = historySize - 1; i > 0; --i) {
        analysisHistory_[i] = analysisHistory_[i - 1];
    }
    analysisHistory_[0] = analysis;

    currentAnalysis_ = analysis;

    return analysis;
}

TimbreAnalysis TimbreTransfer::analyzeTimbreFromFile(const juce::File& audioFile) {
    if (!audioFile.existsAsFile()) {
        DBG("TimbreTransfer: File not found: " + audioFile.getFullPathName());
        return TimbreAnalysis();
    }

    // Create audio format manager and reader
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));

    if (!reader) {
        DBG("TimbreTransfer: Could not create reader for file");
        return TimbreAnalysis();
    }

    // Read audio data
    juce::AudioBuffer<float> audio(reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&audio, 0, (int)reader->lengthInSamples, 0, true, true);

    // Analyze
    return analyzeTimbre(audio);
}

TimbreAnalysis TimbreTransfer::analyzeRealTime(const juce::dsp::AudioBlock<float>& audio) {
    // Convert to AudioBuffer for analysis
    processBuffer_.clear();
    int numSamples = (int)audio.getNumSamples();

    for (size_t ch = 0; ch < audio.getNumChannels() && ch < 2; ++ch) {
        juce::FloatVectorOperations::copy(
            processBuffer_.getWritePointer((int)ch),
            audio.getChannelPointer((int)ch),
            numSamples);
    }

    return analyzeTimbre(processBuffer_);
}

//==============================================================================
// Timbre Transfer
//==============================================================================

juce::AudioBuffer<float> TimbreTransfer::applyTimbreTransfer(
    const juce::AudioBuffer<float>& sourceAudio,
    const TimbreAnalysis& targetTimbre,
    const TimbreTransferParams& params) {

    juce::ScopedLock lock(transferLock_);

    // Select method based on parameters
    switch (params.method) {
        case TimbreTransferParams::Method::Neural:
            if (neuralModelLoaded_) {
                return neuralTransfer(sourceAudio, targetTimbre.neuralEmbedding);
            }
            // Fall through to spectral if neural not available
            [[fallthrough]];

        case TimbreTransferParams::Method::Spectral:
            return spectralTransfer(sourceAudio, targetTimbre, params);

        case TimbreTransferParams::Method::Morphing:
            // Use current analysis as source
            return applyMorphing(sourceAudio, currentAnalysis_, targetTimbre, params.morphPosition);

        case TimbreTransferParams::Method::Hybrid:
        default: {
            // Combine spectral and neural approaches
            auto spectralResult = spectralTransfer(sourceAudio, targetTimbre, params);

            if (neuralModelLoaded_) {
                auto neuralResult = neuralTransfer(sourceAudio, targetTimbre.neuralEmbedding);

                // Mix results
                juce::AudioBuffer<float> mixed(spectralResult);
                int numSamples = juce::jmin(spectralResult.getNumSamples(), neuralResult.getNumSamples());

                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < numSamples; ++i) {
                        float s = spectralResult.getSample(ch, i);
                        float n = neuralResult.getSample(ch, i);
                        mixed.setSample(ch, i, s * 0.6f + n * 0.4f);
                    }
                }

                return mixed;
            }

            return spectralResult;
        }
    }
}

void TimbreTransfer::processRealTime(juce::dsp::AudioBlock<float>& audioBlock,
                                    const TimbreAnalysis& targetTimbre,
                                    const TimbreTransferParams& params) {
    // Convert to buffer
    juce::AudioBuffer<float> buffer(2, (int)audioBlock.getNumSamples());

    for (size_t ch = 0; ch < audioBlock.getNumChannels() && ch < 2; ++ch) {
        juce::FloatVectorOperations::copy(
            buffer.getWritePointer((int)ch),
            audioBlock.getChannelPointer((int)ch),
            (int)audioBlock.getNumSamples());
    }

    // Apply transfer
    auto result = applyTimbreTransfer(buffer, targetTimbre, params);

    // Copy back
    for (size_t ch = 0; ch < audioBlock.getNumChannels() && ch < 2; ++ch) {
        juce::FloatVectorOperations::copy(
            audioBlock.getChannelPointer((int)ch),
            result.getReadPointer((int)ch),
            (int)audioBlock.getNumSamples());
    }
}

juce::AudioBuffer<float> TimbreTransfer::transferBetweenAudio(
    const juce::AudioBuffer<float>& source,
    const juce::AudioBuffer<float>& target) {

    // Analyze target timbre
    auto targetTimbre = analyzeTimbre(target);

    // Apply to source
    TimbreTransferParams params;
    params.method = TimbreTransferParams::Method::Hybrid;

    return applyTimbreTransfer(source, targetTimbre, params);
}

//==============================================================================
// Timbre Morphing
//==============================================================================

TimbreAnalysis TimbreTransfer::morphTimbres(const TimbreAnalysis& timbreA,
                                           const TimbreAnalysis& timbreB,
                                           float morphPosition) {
    TimbreAnalysis morphed;

    // Interpolate spectral features
    size_t minSize = juce::jmin(timbreA.spectralCentroid.size(),
                               timbreB.spectralCentroid.size());
    morphed.spectralCentroid.resize(minSize);

    for (size_t i = 0; i < minSize; ++i) {
        morphed.spectralCentroid[i] = timbreA.spectralCentroid[i] * (1.0f - morphPosition) +
                                      timbreB.spectralCentroid[i] * morphPosition;
    }

    // Interpolate MFCCs
    for (int i = 0; i < TimbreAnalysis::numMFCC; ++i) {
        morphed.mfcc[i] = timbreA.mfcc[i] * (1.0f - morphPosition) +
                         timbreB.mfcc[i] * morphPosition;
    }

    // Interpolate chroma
    for (int i = 0; i < TimbreAnalysis::numChroma; ++i) {
        morphed.chroma[i] = timbreA.chroma[i] * (1.0f - morphPosition) +
                           timbreB.chroma[i] * morphPosition;
    }

    // Interpolate harmonic features
    morphed.harmonicity = timbreA.harmonicity * (1.0f - morphPosition) +
                         timbreB.harmonicity * morphPosition;
    morphed.inharmonicity = timbreA.inharmonicity * (1.0f - morphPosition) +
                           timbreB.inharmonicity * morphPosition;

    // Spherical interpolation for neural embedding
    morphed.neuralEmbedding = slerpTimbres(timbreA.neuralEmbedding,
                                          timbreB.neuralEmbedding,
                                          morphPosition);

    morphed.valid = true;

    return morphed;
}

juce::AudioBuffer<float> TimbreTransfer::applyMorphing(
    const juce::AudioBuffer<float>& audio,
    const TimbreAnalysis& timbreA,
    const TimbreAnalysis& timbreB,
    float morphPosition) {

    // Create morphed timbre
    auto morphedTimbre = morphTimbres(timbreA, timbreB, morphPosition);

    // Apply morphed timbre
    TimbreTransferParams params;
    params.method = TimbreTransferParams::Method::Spectral;
    params.morphPosition = morphPosition;

    return applyTimbreTransfer(audio, morphedTimbre, params);
}

//==============================================================================
// Preset Management
//==============================================================================

void TimbreTransfer::savePreset(const juce::String& name,
                               const TimbreAnalysis& timbre,
                               const juce::String& category) {
    juce::ScopedLock lock(transferLock_);

    TimbrePreset preset;
    preset.name = name;
    preset.category = category;
    preset.timbre = timbre;

    // Check if preset already exists
    for (auto& p : presets_) {
        if (p.name == name) {
            p = preset;
            return;
        }
    }

    presets_.add(preset);

    // Export to file
    exportPresets(presetDatabasePath_);

    DBG("TimbreTransfer: Saved preset '" + name + "'");
}

TimbreAnalysis TimbreTransfer::loadPreset(const juce::String& name) {
    for (const auto& preset : presets_) {
        if (preset.name == name) {
            return preset.timbre;
        }
    }

    DBG("TimbreTransfer: Preset '" + name + "' not found");
    return TimbreAnalysis();
}

juce::Array<TimbrePreset> TimbreTransfer::getPresets() const {
    return presets_;
}

void TimbreTransfer::deletePreset(const juce::String& name) {
    juce::ScopedLock lock(transferLock_);

    for (int i = 0; i < presets_.size(); ++i) {
        if (presets_[i].name == name) {
            presets_.remove(i);
            exportPresets(presetDatabasePath_);
            return;
        }
    }
}

bool TimbreTransfer::exportPresets(const juce::File& outputFile) {
    juce::ScopedLock lock(transferLock_);

    // Create XML
    auto xml = std::make_unique<juce::XmlElement>("TimbrePresets");

    for (const auto& preset : presets_) {
        auto presetXml = xml->createNewChildElement("Preset");
        presetXml->setAttribute("name", preset.name);
        presetXml->setAttribute("category", preset.category);

        // Store timbre features (simplified - would need proper serialization)
        auto timbreXml = presetXml->createNewChildElement("Timbre");
        timbreXml->setAttribute("valid", preset.timbre.valid);
        timbreXml->setAttribute("harmonicity", preset.timbre.harmonicity);
        timbreXml->setAttribute("inharmonicity", preset.timbre.inharmonicity);
        timbreXml->setAttribute("rms", preset.timbre.rms);
    }

    // Write to file
    juce::FileOutputStream stream(outputFile);
    if (stream.openedOk()) {
        stream.setPosition(0);
        stream.truncate();
        stream.writeText(xml->toString(), false, false, nullptr);
        return true;
    }

    return false;
}

bool TimbreTransfer::importPresets(const juce::File& inputFile) {
    juce::ScopedLock lock(transferLock_);

    if (!inputFile.existsAsFile()) {
        return false;
    }

    auto xml = juce::XmlDocument::parse(inputFile);
    if (!xml || xml->getTagName() != "TimbrePresets") {
        return false;
    }

    presets_.clear();

    for (auto* presetXml : xml->getChildWithTagNameIterator("Preset")) {
        TimbrePreset preset;
        preset.name = presetXml->getStringAttribute("name");
        preset.category = presetXml->getStringAttribute("category");

        auto* timbreXml = presetXml->getChildByName("Timbre");
        if (timbreXml) {
            preset.timbre.valid = timbreXml->getBoolAttribute("valid");
            preset.timbre.harmonicity = (float)timbreXml->getDoubleAttribute("harmonicity");
            preset.timbre.inharmonicity = (float)timbreXml->getDoubleAttribute("inharmonicity");
            preset.timbre.rms = (float)timbreXml->getDoubleAttribute("rms");
        }

        presets_.add(preset);
    }

    return true;
}

//==============================================================================
// Neural Network Integration
//==============================================================================

bool TimbreTransfer::loadNeuralModel(const juce::File& modelPath) {
#ifdef ZENITH_USE_ONNX_RUNTIME
    try {
        if (!env_) {
            env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ZenithTimbreTransfer");
        }

        sessionOptions_ = std::make_unique<Ort::SessionOptions>();
        sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        sessionOptions_->SetIntraOpNumThreads(2);

        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        memoryInfo_ = std::make_unique<Ort::MemoryInfo>(memoryInfo);

#ifdef _WIN32
        std::wstring wPath = modelPath.getFullPathName().toWideCharPointer();
        session_ = std::make_unique<Ort::Session>(*env_, wPath.c_str(), *sessionOptions_);
#else
        std::string sPath = modelPath.getFullPathName().toStdString();
        session_ = std::make_unique<Ort::Session>(*env_, sPath.c_str(), *sessionOptions_);
#endif

        neuralModelLoaded_ = true;
        DBG("TimbreTransfer: Neural model loaded successfully");
        return true;

    } catch (const Ort::Exception& e) {
        DBG("TimbreTransfer: ONNX error: " + juce::String(e.what()));
        neuralModelLoaded_ = false;
        return false;
    }
#else
    DBG("TimbreTransfer: ONNX Runtime not available");
    neuralModelLoaded_ = false;
    return false;
#endif
}

juce::AudioBuffer<float> TimbreTransfer::neuralTransfer(
    const juce::AudioBuffer<float>& source,
    const std::vector<float>& targetEmbedding) {

    // Placeholder for neural transfer
    // In production, this would run actual neural network inference

    // For now, return source with spectral modification based on embedding
    auto result = spectralTransfer(source, TimbreAnalysis(), getDefaultParams());

    return result;
}

std::vector<float> TimbreTransfer::extractEmbedding(const juce::AudioBuffer<float>& audio) {
    // Extract MFCC-based embedding
    TimbreAnalysis analysis = analyzeTimbre(audio);
    return analysis.neuralEmbedding;
}

//==============================================================================
// Utilities
//==============================================================================

float TimbreTransfer::calculateSimilarity(const TimbreAnalysis& timbreA,
                                         const TimbreAnalysis& timbreB) {
    if (!timbreA.valid || !timbreB.valid) {
        return 0.0f;
    }

    // Calculate cosine similarity of MFCCs
    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;

    for (int i = 0; i < TimbreAnalysis::numMFCC; ++i) {
        dotProduct += timbreA.mfcc[i] * timbreB.mfcc[i];
        normA += timbreA.mfcc[i] * timbreA.mfcc[i];
        normB += timbreB.mfcc[i] * timbreB.mfcc[i];
    }

    if (normA < 1e-6f || normB < 1e-6f) {
        return 0.0f;
    }

    float similarity = dotProduct / (std::sqrt(normA) * std::sqrt(normB));
    return juce::jlimit(0.0f, 1.0f, similarity);
}

juce::String TimbreTransfer::findMostSimilar(const TimbreAnalysis& query) {
    float maxSimilarity = -1.0f;
    juce::String mostSimilar;

    for (const auto& preset : presets_) {
        float similarity = calculateSimilarity(query, preset.timbre);
        if (similarity > maxSimilarity) {
            maxSimilarity = similarity;
            mostSimilar = preset.name;
        }
    }

    return mostSimilar;
}

TimbreTransferParams TimbreTransfer::getDefaultParams() {
    TimbreTransferParams params;
    params.method = TimbreTransferParams::Method::Hybrid;
    params.transferAmount = 0.5f;
    params.spectralSmoothing = 0.3f;
    params.harmonicMatch = 0.7f;
    params.noiseMatching = 0.5f;
    params.morphPosition = 0.5f;
    params.morphSmoothness = 0.5f;
    params.temperature = 0.7f;
    params.styleStrength = 0.8f;
    params.realTimeMode = true;
    params.fftSize = 2048;
    params.hopSize = 512;

    return params;
}

//==============================================================================
// Internal Analysis
//==============================================================================

void TimbreTransfer::extractSpectralFeatures(const juce::AudioBuffer<float>& audio,
                                            TimbreAnalysis& analysis) {
    // Perform FFT
    performFFT(audio, fftBuffer_);

    // Calculate magnitude spectrum
    for (size_t i = 0; i < magnitudeBuffer_.size(); ++i) {
        magnitudeBuffer_[i] = std::abs(fftBuffer_[i]);
    }

    int numBins = (int)magnitudeBuffer_.size();

    // Calculate spectral centroid
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;

    for (int i = 0; i < numBins; ++i) {
        float freq = (float)i * sampleRate_ / 2048.0f;
        weightedSum += freq * magnitudeBuffer_[i];
        magnitudeSum += magnitudeBuffer_[i];
    }

    float centroid = (magnitudeSum > 0.0f) ? (weightedSum / magnitudeSum) : 0.0f;
    analysis.spectralCentroid.push_back(centroid);

    // Calculate spectral rolloff (85% energy point)
    float energySum = 0.0f;
    float totalEnergy = magnitudeSum;
    float rolloffBin = 0.0f;

    for (int i = 0; i < numBins; ++i) {
        energySum += magnitudeBuffer_[i];
        if (energySum >= 0.85f * totalEnergy) {
            rolloffBin = (float)i;
            break;
        }
    }

    float rolloff = rolloffBin * sampleRate_ / 2048.0f;
    analysis.spectralRolloff.push_back(rolloff);

    // Calculate spectral contrast (difference between peaks and valleys)
    float contrast = 0.0f;
    float peakSum = 0.0f;
    float valleySum = 0.0f;
    int peakCount = 0;
    int valleyCount = 0;

    for (int i = 1; i < numBins - 1; ++i) {
        if (magnitudeBuffer_[i] > magnitudeBuffer_[i - 1] &&
            magnitudeBuffer_[i] > magnitudeBuffer_[i + 1]) {
            peakSum += magnitudeBuffer_[i];
            peakCount++;
        } else if (magnitudeBuffer_[i] < magnitudeBuffer_[i - 1] &&
                   magnitudeBuffer_[i] < magnitudeBuffer_[i + 1]) {
            valleySum += magnitudeBuffer_[i];
            valleyCount++;
        }
    }

    if (peakCount > 0 && valleyCount > 0) {
        float avgPeak = peakSum / peakCount;
        float avgValley = valleySum / valleyCount;
        contrast = (avgPeak - avgValley) / (avgPeak + avgValley + 1e-6f);
    }

    analysis.spectralContrast.push_back(contrast);
}

void TimbreTransfer::extractMFCCs(const juce::AudioBuffer<float>& audio,
                                 TimbreAnalysis& analysis) {
    // Perform FFT
    performFFT(audio, fftBuffer_);

    // Calculate magnitude spectrum
    for (size_t i = 0; i < magnitudeBuffer_.size(); ++i) {
        magnitudeBuffer_[i] = std::abs(fftBuffer_[i]);
    }

    // Apply mel filterbank
    int numMelFilters = 40;
    std::vector<float> melEnergies(numMelFilters, 0.0f);

    for (int i = 0; i < numMelFilters; ++i) {
        for (int j = 0; j < 513; ++j) {
            melEnergies[i] += magnitudeBuffer_[j] * melFilterbank_[i * 513 + j];
        }
    }

    // Log mel energies
    for (auto& energy : melEnergies) {
        energy = std::log(energy + 1e-6f);
    }

    // Apply DCT to get MFCCs
    analysis.mfcc.resize(TimbreAnalysis::numMFCC);

    for (int i = 0; i < TimbreAnalysis::numMFCC; ++i) {
        float mfcc = 0.0f;
        for (int j = 0; j < numMelFilters; ++j) {
            mfcc += melEnergies[j] * dctMatrix_[i * numMelFilters + j];
        }
        analysis.mfcc[i] = mfcc;
    }

    // Use as neural embedding
    analysis.neuralEmbedding = analysis.mfcc;
}

void TimbreTransfer::extractChroma(const juce::AudioBuffer<float>& audio,
                                  TimbreAnalysis& analysis) {
    // Perform FFT
    performFFT(audio, fftBuffer_);

    // Initialize chroma
    analysis.chroma.resize(TimbreAnalysis::numChroma, 0.0f);

    // Map frequency bins to chroma
    for (size_t i = 1; i < fftBuffer_.size() / 2; ++i) {
        float freq = (float)i * sampleRate_ / 2048.0f;

        // Convert to MIDI note number
        float midiNote = 69.0f + 12.0f * std::log2(freq / 440.0f);

        // Get chroma bin (0-11)
        int chromaBin = ((int)std::round(midiNote)) % 12;
        if (chromaBin < 0) chromaBin += 12;

        // Add magnitude
        analysis.chroma[chromaBin] += std::abs(fftBuffer_[i]);
    }

    // Normalize
    float sum = 0.0f;
    for (auto& chroma : analysis.chroma) {
        sum += chroma;
    }
    if (sum > 1e-6f) {
        for (auto& chroma : analysis.chroma) {
            chroma /= sum;
        }
    }
}

void TimbreTransfer::extractHarmonicFeatures(const juce::AudioBuffer<float>& audio,
                                            TimbreAnalysis& analysis) {
    // Perform FFT
    performFFT(audio, fftBuffer_);

    // Find peaks in spectrum
    std::vector<int> peakBins;
    float threshold = 0.1f;

    for (size_t i = 2; i < fftBuffer_.size() / 2 - 2; ++i) {
        float mag = std::abs(fftBuffer_[i]);
        if (mag > threshold &&
            mag > std::abs(fftBuffer_[i - 1]) &&
            mag > std::abs(fftBuffer_[i - 2]) &&
            mag > std::abs(fftBuffer_[i + 1]) &&
            mag > std::abs(fftBuffer_[i + 2])) {
            peakBins.push_back((int)i);
        }
    }

    if (peakBins.size() < 2) {
        analysis.harmonicity = 0.0f;
        analysis.inharmonicity = 0.0f;
        return;
    }

    // Calculate fundamental frequency (lowest peak)
    float fundamentalFreq = peakBins[0] * sampleRate_ / 2048.0f;

    // Calculate harmonicity (how many peaks are at harmonic frequencies)
    int harmonicCount = 0;
    for (int bin : peakBins) {
        float freq = bin * sampleRate_ / 2048.0f;
        float harmonicRatio = freq / fundamentalFreq;
        float nearestHarmonic = std::round(harmonicRatio);
        float deviation = std::abs(harmonicRatio - nearestHarmonic);

        if (deviation < 0.05f) {
            harmonicCount++;
        }
    }

    analysis.harmonicity = (float)harmonicCount / peakBins.size();

    // Calculate inharmonicity (deviation from perfect harmonics)
    float inharmonicitySum = 0.0f;
    for (int bin : peakBins) {
        float freq = bin * sampleRate_ / 2048.0f;
        float harmonicRatio = freq / fundamentalFreq;
        float nearestHarmonic = std::round(harmonicRatio);
        float deviation = std::abs(harmonicRatio - nearestHarmonic);

        inharmonicitySum += deviation;
    }

    analysis.inharmonicity = inharmonicitySum / peakBins.size();

    // Store harmonic peak frequencies
    analysis.harmonicPeaks.clear();
    for (int bin : peakBins) {
        float freq = bin * sampleRate_ / 2048.0f;
        analysis.harmonicPeaks.push_back(freq);
    }
}

//==============================================================================
// Internal Transfer
//==============================================================================

juce::AudioBuffer<float> TimbreTransfer::spectralTransfer(
    const juce::AudioBuffer<float>& source,
    const TimbreAnalysis& targetTimbre,
    const TimbreTransferParams& params) {

    // Copy source to result
    juce::AudioBuffer<float> result(source);
    int numSamples = result.getNumSamples();

    // Extract spectral envelope of source
    auto sourceEnvelope = extractSpectralEnvelope(source);

    // Create target envelope from timbre analysis
    std::vector<float> targetEnvelope(1024);

    if (targetTimbre.spectralCentroid.empty()) {
        // Use flat envelope
        std::fill(targetEnvelope.begin(), targetEnvelope.end(), 1.0f);
    } else {
        // Generate envelope from spectral features
        float centroid = targetTimbre.spectralCentroid[0];
        float rolloff = targetTimbre.spectralRolloff[0];
        float contrast = targetTimbre.spectralContrast[0];

        for (size_t i = 0; i < targetEnvelope.size(); ++i) {
            float freq = (float)i * sampleRate_ / 2048.0f;

            // Create envelope based on centroid and rolloff
            float env = 1.0f;

            if (freq < centroid) {
                env = std::pow(freq / (centroid + 1.0f), 0.5f);
            } else if (freq > rolloff) {
                env = std::exp(-(freq - rolloff) / 1000.0f);
            }

            targetEnvelope[i] = env * (1.0f + contrast);
        }
    }

    // Apply envelope to result
    applySpectralEnvelope(result, targetEnvelope, params.transferAmount);

    return result;
}

std::vector<float> TimbreTransfer::extractSpectralEnvelope(
    const juce::AudioBuffer<float>& audio) {

    performFFT(audio, fftBuffer_);

    std::vector<float> envelope(fftBuffer_.size() / 2);

    for (size_t i = 0; i < envelope.size(); ++i) {
        envelope[i] = std::abs(fftBuffer_[i]);
    }

    return envelope;
}

void TimbreTransfer::applySpectralEnvelope(juce::AudioBuffer<float>& audio,
                                          const std::vector<float>& envelope,
                                          float amount) {
    // Perform FFT
    performFFT(audio, fftBuffer_);

    // Apply envelope
    for (size_t i = 0; i < envelope.size() && i < fftBuffer_.size() / 2; ++i) {
        float currentMag = std::abs(fftBuffer_[i]);
        float targetMag = envelope[i];
        float newMag = currentMag * (1.0f - amount) + targetMag * amount;

        float phase = std::arg(fftBuffer_[i]);
        fftBuffer_[i] = std::polar(newMag, phase);
        if (i > 0) {
            fftBuffer_[fftBuffer_.size() - i] = std::polar(newMag, -phase);
        }
    }

    // Perform IFFT
    performIFFT(fftBuffer_, audio);
}

//==============================================================================
// Neural Network Processing
//==============================================================================

std::vector<float> TimbreTransfer::runNeuralInference(
    const std::vector<float>& inputFeatures) {

#ifdef ZENITH_USE_ONNX_RUNTIME
    if (!neuralModelLoaded_ || !session_) {
        return inputFeatures;
    }

    try {
        // Create input tensor
        std::vector<int64_t> inputShape = {1, (int64_t)inputFeatures.size()};

        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            *memoryInfo_,
            const_cast<float*>(inputFeatures.data()),
            inputFeatures.size(),
            inputShape.data(),
            inputShape.size());

        // Run inference
        std::vector<const char*> inputNames = {"input"};
        std::vector<const char*> outputNames = {"output"};

        auto outputTensors = session_->Run(
            Ort::RunOptions{nullptr},
            inputNames.data(),
            &inputTensor,
            1,
            outputNames.data(),
            outputNames.size());

        // Extract output
        if (outputTensors.size() > 0) {
            auto* outputData = outputTensors[0].GetTensorMutableData<float>();
            auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
            size_t outputSize = outputShape[1];

            std::vector<float> result(outputSize);
            juce::FloatVectorOperations::copy(result.data(), outputData, (int)outputSize);

            return result;
        }

    } catch (const Ort::Exception& e) {
        DBG("TimbreTransfer: Neural inference error: " + juce::String(e.what()));
    }
#endif

    return inputFeatures;
}

//==============================================================================
// Morphing
//==============================================================================

std::vector<float> TimbreTransfer::interpolateFeatures(
    const std::vector<float>& a,
    const std::vector<float>& b,
    float t) {

    size_t size = juce::jmin(a.size(), b.size());
    std::vector<float> result(size);

    for (size_t i = 0; i < size; ++i) {
        result[i] = a[i] * (1.0f - t) + b[i] * t;
    }

    return result;
}

std::vector<float> TimbreTransfer::slerpTimbres(
    const std::vector<float>& a,
    const std::vector<float>& b,
    float t) {

    // Normalize inputs
    std::vector<float> aNorm = a;
    std::vector<float> bNorm = b;

    float normA = 0.0f;
    float normB = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    normA = std::sqrt(normA) + 1e-6f;
    normB = std::sqrt(normB) + 1e-6f;

    for (size_t i = 0; i < a.size(); ++i) {
        aNorm[i] /= normA;
        bNorm[i] /= normB;
    }

    // Calculate dot product
    float dot = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += aNorm[i] * bNorm[i];
    }

    dot = juce::jlimit(-1.0f, 1.0f, dot);

    // Calculate interpolation
    float theta = std::acos(dot) * t;
    float sinTheta = std::sin(theta);

    if (sinTheta < 1e-6f) {
        // Linear interpolation for nearly parallel vectors
        return interpolateFeatures(a, b, t);
    }

    float scaleA = std::sin((1.0f - t) * theta) / sinTheta;
    float scaleB = std::sin(t * theta) / sinTheta;

    std::vector<float> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result[i] = scaleA * aNorm[i] + scaleB * bNorm[i];
    }

    return result;
}

//==============================================================================
// FFT Processing
//==============================================================================

void TimbreTransfer::performFFT(const juce::AudioBuffer<float>& audio,
                               std::vector<std::complex<float>>& fftResult) {
    // Convert to mono
    int numSamples = juce::jmin(audio.getNumSamples(), 2048);
    std::vector<float> mono(2048, 0.0f);

    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        const float* channelData = audio.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            mono[i] += channelData[i];
        }
    }

    // Normalize
    for (auto& sample : mono) {
        sample /= (float)audio.getNumChannels();
    }

    // Apply window
    window_.multiplyWithWindowingTable(mono.data(), 2048);

    // Perform FFT
    fft_.perform(mono.data(), fftResult.data());
}

void TimbreTransfer::performIFFT(const std::vector<std::complex<float>>& fftData,
                                juce::AudioBuffer<float>& audio) {
    std::vector<float> output(2048);

    // Perform IFFT
    fft_.perform(fftData.data(), output.data(), true);

    // Apply window
    window_.multiplyWithWindowingTable(output.data(), 2048);

    // Copy to audio buffer
    int numSamples = juce::jmin(audio.getNumSamples(), 2048);

    for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
        float* channelData = audio.getWritePointer(ch);
        juce::FloatVectorOperations::copy(channelData, output.data(), numSamples);
    }
}

//==============================================================================
// TimbreTransferFactory Implementation
//==============================================================================

TimbrePreset TimbreTransferFactory::createPresetFromFile(const juce::File& audioFile,
                                                        const juce::String& name) {
    TimbrePreset preset;
    preset.name = name;
    preset.filepath = audioFile.getFullPathName();

    // Create temporary analyzer
    TimbreTransfer analyzer;
    if (analyzer.initialize(44100.0, 512)) {
        preset.timbre = analyzer.analyzeTimbreFromFile(audioFile);
    }

    return preset;
}

juce::Array<TimbrePreset> TimbreTransferFactory::createBuiltInPresets() {
    juce::Array<TimbrePreset> presets;

    // Create procedural presets
    juce::StringArray timbreTypes = {
        "warm_pad", "bright_lead", "deep_bass", "pluck",
        "ambient", "brass", "string", "wind"
    };

    for (const auto& type : timbreTypes) {
        TimbrePreset preset;
        preset.name = type;
        preset.category = "Built-in";
        preset.timbre = createProceduralTimbre(type);
        preset.description = "Procedurally generated " + type + " timbre";
        presets.add(preset);
    }

    return presets;
}

TimbreAnalysis TimbreTransferFactory::createProceduralTimbre(const juce::String& type) {
    TimbreAnalysis analysis;
    analysis.valid = true;

    // Generate timbre features based on type
    if (type.contains("warm") || type.contains("pad")) {
        analysis.harmonicity = 0.9f;
        analysis.inharmonicity = 0.1f;
        analysis.spectralCentroid.push_back(1000.0f);
        analysis.spectralRolloff.push_back(4000.0f);
        analysis.spectralContrast.push_back(0.3f);
    } else if (type.contains("bright") || type.contains("lead")) {
        analysis.harmonicity = 0.7f;
        analysis.inharmonicity = 0.2f;
        analysis.spectralCentroid.push_back(3000.0f);
        analysis.spectralRolloff.push_back(8000.0f);
        analysis.spectralContrast.push_back(0.8f);
    } else if (type.contains("bass")) {
        analysis.harmonicity = 0.95f;
        analysis.inharmonicity = 0.05f;
        analysis.spectralCentroid.push_back(200.0f);
        analysis.spectralRolloff.push_back(1000.0f);
        analysis.spectralContrast.push_back(0.4f);
    } else if (type.contains("pluck")) {
        analysis.harmonicity = 0.6f;
        analysis.inharmonicity = 0.3f;
        analysis.spectralCentroid.push_back(2000.0f);
        analysis.spectralRolloff.push_back(6000.0f);
        analysis.spectralContrast.push_back(0.9f);
    } else {
        // Default
        analysis.harmonicity = 0.7f;
        analysis.inharmonicity = 0.2f;
        analysis.spectralCentroid.push_back(1500.0f);
        analysis.spectralRolloff.push_back(5000.0f);
        analysis.spectralContrast.push_back(0.5f);
    }

    // Initialize MFCCs
    analysis.mfcc.resize(TimbreAnalysis::numMFCC, 0.0f);
    analysis.neuralEmbedding = analysis.mfcc;

    // Initialize chroma
    analysis.chroma.resize(TimbreAnalysis::numChroma, 1.0f / 12.0f);

    return analysis;
}

std::vector<float> TimbreTransferFactory::generateSpectralEnvelope(
    const juce::String& type,
    int fftSize) {

    std::vector<float> envelope(fftSize / 2);

    float centroid = 1000.0f;
    float rolloff = 5000.0f;

    if (type.contains("bright")) {
        centroid = 3000.0f;
        rolloff = 8000.0f;
    } else if (type.contains("warm")) {
        centroid = 800.0f;
        rolloff = 3000.0f;
    } else if (type.contains("bass")) {
        centroid = 150.0f;
        rolloff = 1000.0f;
    }

    for (int i = 0; i < fftSize / 2; ++i) {
        float freq = (float)i * 44100.0f / fftSize;

        if (freq < centroid) {
            envelope[i] = std::pow(freq / centroid, 0.5f);
        } else if (freq < rolloff) {
            envelope[i] = 1.0f - 0.5f * (freq - centroid) / (rolloff - centroid);
        } else {
            envelope[i] = 0.5f * std::exp(-(freq - rolloff) / 2000.0f);
        }
    }

    return envelope;
}

} // namespace zenith
