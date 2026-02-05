/*
  ==============================================================================
    VisualAnalyzer.cpp
    Multi-modal audio analysis implementation
  ==============================================================================
*/

#include "VisualAnalyzer.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace zenith {
namespace ai {

VisualAnalyzer::VisualAnalyzer() 
    : fft(2048), 
      window(2048, juce::dsp::WindowingFunction<float>::hann) {
    fftSize = 2048;
    hopSize = 512;
    minFrequency = 20.0f;
    maxFrequency = 20000.0f;
}

VisualAnalyzer::~VisualAnalyzer() = default;

VisualAnalysisResult VisualAnalyzer::analyzeAudio(const juce::AudioBuffer<float>& audio,
                                                  double sampleRate) {
    VisualAnalysisResult result;
    
    // Extract waveform features
    result.waveform = extractWaveformFeatures(audio, sampleRate);
    
    // Generate spectral heatmap
    result.heatmap = generateSpectralHeatmap(audio, sampleRate);
    
    // Generate AI-readable description
    result.visualDescription = generateVisualDescription(result);
    
    // Extract structured features for AI
    result.visualFeatures = extractVisualFeatures(result);
    
    return result;
}

WaveformFeatures VisualAnalyzer::extractWaveformFeatures(const juce::AudioBuffer<float>& audio,
                                                         double sampleRate) {
    WaveformFeatures features;
    
    // Convert to mono for analysis
    const int numSamples = audio.getNumSamples();
    std::vector<float> monoSignal(numSamples);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        float sum = 0.0f;
        for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
            sum += audio.getSample(ch, sample);
        }
        monoSignal[sample] = sum / audio.getNumChannels();
    }
    
    // Compute RMS envelope
    features.rmsEnvelope = computeRMSEnvelope(audio);
    
    // Find peaks and troughs
    features.peaks = findPeaks(features.rmsEnvelope);
    features.troughs = findTroughs(features.rmsEnvelope);
    
    // Compute zero crossing rate
    features.zeroCrossings = computeZeroCrossingRate(audio);
    
    // Calculate dynamic range and crest factor
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    
    for (float sample : monoSignal) {
        peakLevel = std::max(peakLevel, std::abs(sample));
        rmsLevel += sample * sample;
    }
    
    rmsLevel = std::sqrt(rmsLevel / numSamples);
    features.dynamicRange = peakLevel / rmsLevel;
    features.crestFactor = 20.0f * std::log10(peakLevel / rmsLevel);
    
    // Estimate attack and decay times
    features.attackTime = estimateAttackTime(features.rmsEnvelope, sampleRate);
    features.decayTime = estimateDecayTime(features.rmsEnvelope, sampleRate);
    
    return features;
}

SpectralHeatmap VisualAnalyzer::generateSpectralHeatmap(const juce::AudioBuffer<float>& audio,
                                                        double sampleRate) {
    SpectralHeatmap heatmap;
    heatmap.sampleRate = static_cast<float>(sampleRate);
    heatmap.fftSize = fftSize;
    heatmap.hopSize = hopSize;
    
    // Create heatmap data
    heatmap.frequencyBands = createHeatmapData(audio, sampleRate);
    
    // Generate time axis
    const int numFrames = static_cast<int>(heatmap.frequencyBands.size());
    heatmap.timeAxis.resize(numFrames);
    for (int i = 0; i < numFrames; ++i) {
        heatmap.timeAxis[i] = static_cast<float>(i * hopSize) / sampleRate;
    }
    
    // Generate frequency axis
    const int numFreqBins = fftSize / 2 + 1;
    heatmap.frequencyAxis.resize(numFreqBins);
    for (int i = 0; i < numFreqBins; ++i) {
        heatmap.frequencyAxis[i] = static_cast<float>(i * sampleRate) / fftSize;
    }
    
    return heatmap;
}

std::vector<float> VisualAnalyzer::computeRMSEnvelope(const juce::AudioBuffer<float>& audio) {
    const int windowSize = 1024;
    const int hopSize = 256;
    const int numSamples = audio.getNumSamples();
    const int numWindows = (numSamples - windowSize) / hopSize + 1;
    
    std::vector<float> envelope(numWindows);
    
    for (int win = 0; win < numWindows; ++win) {
        float rms = 0.0f;
        const int startSample = win * hopSize;
        
        for (int ch = 0; ch < audio.getNumChannels(); ++ch) {
            for (int i = 0; i < windowSize; ++i) {
                float sample = audio.getSample(ch, startSample + i);
                rms += sample * sample;
            }
        }
        
        rms = std::sqrt(rms / (windowSize * audio.getNumChannels()));
        envelope[win] = rms;
    }
    
    return envelope;
}

std::vector<float> VisualAnalyzer::findPeaks(const std::vector<float>& signal, float threshold) {
    std::vector<float> peaks;
    
    for (size_t i = 1; i < signal.size() - 1; ++i) {
        if (signal[i] > signal[i-1] && signal[i] > signal[i+1] && signal[i] > threshold) {
            peaks.push_back(static_cast<float>(i));
        }
    }
    
    return peaks;
}

std::vector<float> VisualAnalyzer::findTroughs(const std::vector<float>& signal, float threshold) {
    std::vector<float> troughs;
    
    for (size_t i = 1; i < signal.size() - 1; ++i) {
        if (signal[i] < signal[i-1] && signal[i] < signal[i+1] && signal[i] < threshold) {
            troughs.push_back(static_cast<float>(i));
        }
    }
    
    return troughs;
}

std::vector<float> VisualAnalyzer::computeZeroCrossingRate(const juce::AudioBuffer<float>& audio) {
    const int windowSize = 512;
    const int hopSize = 128;
    const int numSamples = audio.getNumSamples();
    const int numWindows = (numSamples - windowSize) / hopSize + 1;
    
    std::vector<float> zcr(numWindows);
    
    for (int win = 0; win < numWindows; ++win) {
        int crossings = 0;
        const int startSample = win * hopSize;
        
        // Use first channel for ZCR
        for (int i = 1; i < windowSize; ++i) {
            float prev = audio.getSample(0, startSample + i - 1);
            float curr = audio.getSample(0, startSample + i);
            if ((prev >= 0 && curr < 0) || (prev < 0 && curr >= 0)) {
                crossings++;
            }
        }
        
        zcr[win] = static_cast<float>(crossings) / windowSize;
    }
    
    return zcr;
}

float VisualAnalyzer::estimateAttackTime(const std::vector<float>& rmsEnvelope, double sampleRate) {
    if (rmsEnvelope.size() < 10) return 0.0f;
    
    // Find peak
    auto peakIt = std::max_element(rmsEnvelope.begin(), rmsEnvelope.end());
    float peakLevel = *peakIt;
    
    // Find 10% level before peak
    float threshold = peakLevel * 0.1f;
    int attackStart = 0;
    
    for (int i = 0; i < std::distance(rmsEnvelope.begin(), peakIt); ++i) {
        if (rmsEnvelope[i] >= threshold) {
            attackStart = i;
            break;
        }
    }
    
    // Calculate attack time in seconds
    float hopTime = 256.0f / sampleRate;  // Based on RMS window hop size
    return (std::distance(rmsEnvelope.begin(), peakIt) - attackStart) * hopTime;
}

float VisualAnalyzer::estimateDecayTime(const std::vector<float>& rmsEnvelope, double sampleRate) {
    if (rmsEnvelope.size() < 10) return 0.0f;
    
    // Find peak
    auto peakIt = std::max_element(rmsEnvelope.begin(), rmsEnvelope.end());
    float peakLevel = *peakIt;
    
    // Find 10% level after peak
    float threshold = peakLevel * 0.1f;
    int decayEnd = static_cast<int>(rmsEnvelope.size()) - 1;
    
    for (int i = std::distance(rmsEnvelope.begin(), peakIt); i < rmsEnvelope.size(); ++i) {
        if (rmsEnvelope[i] <= threshold) {
            decayEnd = i;
            break;
        }
    }
    
    // Calculate decay time in seconds
    float hopTime = 256.0f / sampleRate;  // Based on RMS window hop size
    return (decayEnd - std::distance(rmsEnvelope.begin(), peakIt)) * hopTime;
}

std::vector<float> VisualAnalyzer::computeSpectrum(const float* channelData, int numSamples) {
    std::vector<float> fftData(fftSize * 2);
    std::vector<float> magnitude(fftSize / 2 + 1);
    
    // Copy and window the data
    for (int i = 0; i < std::min(numSamples, fftSize); ++i) {
        fftData[i] = channelData[i];
    }
    
    // Apply window
    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    
    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    
    // Copy magnitude spectrum
    for (int i = 0; i < fftSize / 2 + 1; ++i) {
        magnitude[i] = fftData[i];
    }
    
    return magnitude;
}

std::vector<std::vector<float>> VisualAnalyzer::createHeatmapData(const juce::AudioBuffer<float>& audio,
                                                                  double sampleRate) {
    std::vector<std::vector<float>> heatmap;
    const int numFrames = (audio.getNumSamples() - fftSize) / hopSize + 1;
    
    heatmap.resize(numFrames);
    
    for (int frame = 0; frame < numFrames; ++frame) {
        const int startSample = frame * hopSize;
        
        // Analyze first channel (or mix down if needed)
        std::vector<float> spectrum = computeSpectrum(audio.getReadPointer(0, startSample), 
                                                     std::min(fftSize, audio.getNumSamples() - startSample));
        
        heatmap[frame] = spectrum;
    }
    
    return heatmap;
}

juce::String VisualAnalyzer::generateVisualDescription(const VisualAnalysisResult& result) {
    juce::String description;
    
    // Waveform characteristics
    description += "Waveform Analysis:\n";
    description += "- Dynamic Range: " + juce::String(result.waveform.dynamicRange, 2) + "\n";
    description += "- Crest Factor: " + juce::String(result.waveform.crestFactor, 1) + " dB\n";
    description += "- Attack Time: " + juce::String(result.waveform.attackTime * 1000, 0) + " ms\n";
    description += "- Decay Time: " + juce::String(result.waveform.decayTime * 1000, 0) + " ms\n";
    
    // Peak analysis
    if (!result.waveform.peaks.empty()) {
        description += "- Peaks detected: " + juce::String(result.waveform.peaks.size()) + "\n";
    }
    
    // Spectral characteristics
    description += "\nSpectral Analysis:\n";
    description += "- FFT Size: " + juce::String(result.heatmap.fftSize) + "\n";
    description += "- Frequency Range: " + juce::String(result.heatmap.frequencyAxis.front(), 1) + 
                  " - " + juce::String(result.heatmap.frequencyAxis.back(), 1) + " Hz\n";
    
    // AI interpretation
    description += "\nAI Interpretation:\n";
    
    if (result.waveform.attackTime < 0.01f) {
        description += "- Fast attack detected (percussive/transient content)\n";
    } else if (result.waveform.attackTime > 0.1f) {
        description += "- Slow attack (pad/synth content)\n";
    }
    
    if (result.waveform.crestFactor > 12.0f) {
        description += "- High crest factor (dynamic material)\n";
    } else if (result.waveform.crestFactor < 6.0f) {
        description += "- Low crest factor (compressed material)\n";
    }
    
    return description;
}

juce::var VisualAnalyzer::extractVisualFeatures(const VisualAnalysisResult& result) {
    auto features = new juce::DynamicObject();
    
    // Waveform features
    auto waveformObj = new juce::DynamicObject();
    waveformObj->setProperty("dynamicRange", result.waveform.dynamicRange);
    waveformObj->setProperty("crestFactor", result.waveform.crestFactor);
    waveformObj->setProperty("attackTime", result.waveform.attackTime);
    waveformObj->setProperty("decayTime", result.waveform.decayTime);
    waveformObj->setProperty("peakCount", static_cast<int>(result.waveform.peaks.size()));
    waveformObj->setProperty("troughCount", static_cast<int>(result.waveform.troughs.size()));
    
    // Convert zero crossing rate to average
    float avgZCR = 0.0f;
    for (float zcr : result.waveform.zeroCrossings) {
        avgZCR += zcr;
    }
    avgZCR /= result.waveform.zeroCrossings.size();
    waveformObj->setProperty("avgZeroCrossingRate", avgZCR);
    
    features->setProperty("waveform", juce::var(waveformObj));
    
    // Spectral features
    auto spectralObj = new juce::DynamicObject();
    spectralObj->setProperty("fftSize", result.heatmap.fftSize);
    spectralObj->setProperty("sampleRate", result.heatmap.sampleRate);
    spectralObj->setProperty("frequencyBins", static_cast<int>(result.heatmap.frequencyAxis.size()));
    spectralObj->setProperty("timeFrames", static_cast<int>(result.heatmap.timeAxis.size()));
    
    // Calculate spectral centroid (brightness)
    float spectralCentroid = 0.0f;
    float totalEnergy = 0.0f;
    
    if (!result.heatmap.frequencyBands.empty() && !result.heatmap.frequencyBands[0].empty()) {
        for (size_t frame = 0; frame < result.heatmap.frequencyBands.size(); ++frame) {
            for (size_t bin = 0; bin < result.heatmap.frequencyBands[frame].size(); ++bin) {
                float magnitude = result.heatmap.frequencyBands[frame][bin];
                float frequency = result.heatmap.frequencyAxis[bin];
                
                spectralCentroid += magnitude * frequency;
                totalEnergy += magnitude;
            }
        }
        
        if (totalEnergy > 0.0f) {
            spectralCentroid /= totalEnergy;
        }
    }
    
    spectralObj->setProperty("spectralCentroid", spectralCentroid);
    features->setProperty("spectral", juce::var(spectralObj));
    
    return juce::var(features);
}

void VisualAnalyzer::setAnalysisResolution(int newFftSize, int newHopSize) {
    this->fftSize = newFftSize;
    this->hopSize = newHopSize;
    fft = juce::dsp::FFT(static_cast<int>(std::log2(newFftSize)));
    new (&window) juce::dsp::WindowingFunction<float>(newFftSize, juce::dsp::WindowingFunction<float>::hann);
}

void VisualAnalyzer::setFrequencyRange(float minFreq, float maxFreq) {
    minFrequency = minFreq;
    maxFrequency = maxFreq;
}

} // namespace ai
} // namespace zenith
