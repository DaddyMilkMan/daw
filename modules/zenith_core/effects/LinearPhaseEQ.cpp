/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "LinearPhaseEQ.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace effects {

//==============================================================================
// LinearPhaseEQ Implementation
//==============================================================================

LinearPhaseEQ::LinearPhaseEQ()
{
    // Setup default 8-band EQ
    setupDefaultBands();

    // Register parameters
    addParameter({"phaseMode", "Phase Mode", 0.0f, 1.0f, 0.0f, false, 0.0f});
    addParameter({"filterOrder", "Filter Order", 0.0f, 3.0f, 2.0f, false, 0.0f});
    addParameter({"spectrumEnabled", "Spectrum Analyzer", 0.0f, 1.0f, 1.0f, false, 0.0f});
    addParameter({"matchEQAmount", "Match EQ Amount", 0.0f, 1.0f, 0.5f, true, 0.0f});
}

LinearPhaseEQ::~LinearPhaseEQ()
{
}

//==============================================================================
void LinearPhaseEQ::setupDefaultBands()
{
    numBands_ = 8;

    // Band 0: Low shelf
    bands_[0].type = FilterType::Lowshelf;
    bands_[0].frequency = 80.0f;
    bands_[0].gain = 0.0f;
    bands_[0].q = 0.7f;

    // Band 1: Low-mid bell
    bands_[1].type = FilterType::Bell;
    bands_[1].frequency = 200.0f;
    bands_[1].gain = 0.0f;
    bands_[1].q = 1.0f;

    // Band 2: Low-mid bell
    bands_[2].type = FilterType::Bell;
    bands_[2].frequency = 500.0f;
    bands_[2].gain = 0.0f;
    bands_[2].q = 1.0f;

    // Band 3: Mid bell
    bands_[3].type = FilterType::Bell;
    bands_[3].frequency = 1000.0f;
    bands_[3].gain = 0.0f;
    bands_[3].q = 1.0f;

    // Band 4: Mid-high bell
    bands_[4].type = FilterType::Bell;
    bands_[4].frequency = 2000.0f;
    bands_[4].gain = 0.0f;
    bands_[4].q = 1.0f;

    // Band 5: Mid-high bell
    bands_[5].type = FilterType::Bell;
    bands_[5].frequency = 4000.0f;
    bands_[5].gain = 0.0f;
    bands_[5].q = 1.0f;

    // Band 6: High-mid bell
    bands_[6].type = FilterType::Bell;
    bands_[6].frequency = 6000.0f;
    bands_[6].gain = 0.0f;
    bands_[6].q = 1.0f;

    // Band 7: High shelf
    bands_[7].type = FilterType::Highshelf;
    bands_[7].frequency = 12000.0f;
    bands_[7].gain = 0.0f;
    bands_[7].q = 0.7f;
}

void LinearPhaseEQ::prepare(double sampleRate, int maxSamplesPerBlock)
{
    sampleRate_ = sampleRate;
    maxSamplesPerBlock_ = maxSamplesPerBlock;

    // Prepare convolution engine for linear phase FIR filters
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(maxSamplesPerBlock), 2};
    convolution_.prepare(spec);

    // Prepare IIR filters for minimum phase mode
    for (int i = 0; i < maxBands; ++i)
    {
        iirFilters_[i].filter.prepare(spec);
    }

    // Prepare FFT for spectrum analyzer
    fftBuffer_.resize(spectrumSize * 2, 0.0f);

    // Design filters
    designFilters();
}

void LinearPhaseEQ::reset()
{
    convolution_.reset();

    for (int i = 0; i < maxBands; ++i)
    {
        iirFilters_[i].filter.reset();
    }

    std::fill(spectrumData_.begin(), spectrumData_.end(), 0.0f);
}

void LinearPhaseEQ::process(juce::AudioBuffer<float>& buffer,
                            const juce::AudioBuffer<float>* sidechain)
{
    (void)sidechain;

    // Update spectrum analyzer if enabled
    if (spectrumAnalyzerEnabled_.load())
    {
        updateSpectrumAnalyzer(buffer);
    }

    // Process Match EQ if active
    if (matchingEQ_.load())
    {
        processMatchEQ(buffer);
    }

    // Check if any bands are soloed
    bool anySolo = false;
    for (int i = 0; i < numBands_; ++i)
    {
        if (bands_[i].solo)
        {
            anySolo = true;
            break;
        }
    }

    // Process EQ based on phase mode
    if (phaseMode_ == PhaseMode::Linear)
    {
        // Use linear phase FIR filters via convolution
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolution_.process(context);
    }
    else
    {
        // Use minimum phase IIR filters (cascaded biquads)
        for (int band = 0; band < numBands_; ++band)
        {
            // Skip disabled/muted bands
            if (!bands_[band].enabled || bands_[band].solo == false && anySolo)
                continue;

            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            iirFilters_[band].filter.process(context);
        }
    }
}

//==============================================================================
void LinearPhaseEQ::setPhaseMode(PhaseMode mode)
{
    phaseMode_ = mode;
    designFilters();
}

void LinearPhaseEQ::setFilterOrder(int order)
{
    // Validate: must be power of 2 between 512 and 4096
    int validOrders[] = {512, 1024, 2048, 4096};
    for (int o : validOrders)
    {
        if (order == o)
        {
            filterOrder_ = order;
            designFilters();
            return;
        }
    }
}

void LinearPhaseEQ::setBand(int index, const Band& band)
{
    if (index >= 0 && index < maxBands)
    {
        bands_[index] = band;
        designFilters();
    }
}

LinearPhaseEQ::Band LinearPhaseEQ::getBand(int index) const
{
    if (index >= 0 && index < maxBands)
        return bands_[index];
    return Band{};
}

void LinearPhaseEQ::setNumBands(int num)
{
    numBands_ = juce::jlimit(1, maxBands, num);
    designFilters();
}

void LinearPhaseEQ::setBandEnabled(int index, bool enabled)
{
    if (index >= 0 && index < maxBands)
        bands_[index].enabled = enabled;
}

bool LinearPhaseEQ::isBandEnabled(int index) const
{
    if (index >= 0 && index < maxBands)
        return bands_[index].enabled;
    return false;
}

void LinearPhaseEQ::setBandSolo(int index, bool solo)
{
    if (index >= 0 && index < maxBands)
        bands_[index].solo = solo;
}

bool LinearPhaseEQ::isBandSolo(int index) const
{
    if (index >= 0 && index < maxBands)
        return bands_[index].solo;
    return false;
}

void LinearPhaseEQ::setBandType(int index, FilterType type)
{
    if (index >= 0 && index < maxBands)
    {
        bands_[index].type = type;
        designFilters();
    }
}

void LinearPhaseEQ::setBandFrequency(int index, float frequency)
{
    if (index >= 0 && index < maxBands)
    {
        bands_[index].frequency = juce::jlimit(20.0f, 20000.0f, frequency);
        designFilters();
    }
}

void LinearPhaseEQ::setBandGain(int index, float gain)
{
    if (index >= 0 && index < maxBands)
    {
        bands_[index].gain = juce::jlimit(-24.0f, 24.0f, gain);
        designFilters();
    }
}

void LinearPhaseEQ::setBandQ(int index, float q)
{
    if (index >= 0 && index < maxBands)
    {
        bands_[index].q = juce::jlimit(0.1f, 100.0f, q);
        designFilters();
    }
}

void LinearPhaseEQ::setSpectrumAnalyzerEnabled(bool enabled)
{
    spectrumAnalyzerEnabled_.store(enabled);
}

void LinearPhaseEQ::startMatchEQ()
{
    matchingEQ_.store(true);
    haveTargetSpectrum_ = false;
}

void LinearPhaseEQ::stopMatchEQ()
{
    matchingEQ_.store(false);
    haveTargetSpectrum_ = false;
}

void LinearPhaseEQ::setMatchEQAmount(float amount)
{
    matchEQAmount_.store(juce::jlimit(0.0f, 1.0f, amount));
}

//==============================================================================
void LinearPhaseEQ::designFilters()
{
    if (phaseMode_ == PhaseMode::Linear)
    {
        designLinearPhaseFilter();
    }
    else
    {
        designMinimumPhaseFilter();
    }
}

void LinearPhaseEQ::designLinearPhaseFilter()
{
    // For linear phase EQ, we create a combined FIR filter
    // This is a simplified implementation - a full implementation would
    // use frequency sampling or Parks-McClellan algorithm

    int firSize = filterOrder_;
    std::vector<float> combinedCoeffs(firSize, 0.0f);

    // Combine all enabled bands into a single frequency response
    // For simplicity, this is a basic implementation
    // A production version would use proper FIR design algorithms

    // Clear all filters first
    for (int i = 0; i < maxBands; ++i)
    {
        firFilters_[i].coefficients.clear();
    }

    // Design FIR for each enabled band
    for (int band = 0; band < numBands_; ++band)
    {
        if (!bands_[band].enabled)
            continue;

        designFIRFilter(band);
    }

    // Load combined filter into convolution engine
    // In a full implementation, we would sum all band responses
    // For now, use a simple approach: combine in frequency domain
    std::vector<float> impulseResponse(firSize, 0.0f);

    // Create basic impulse response (simplified - would be full FIR design in production)
    // This is a placeholder - real implementation would design proper FIR filters
    float beta = 6.0f;  // Kaiser window parameter
    for (int i = 0; i < firSize; ++i)
    {
        float x = static_cast<float>(i) / static_cast<float>(firSize - 1) - 0.5f;
        float window = kaiserWindow(x, beta);
        impulseResponse[i] = window;
    }

    // Normalize
    float sum = 0.0f;
    for (float sample : impulseResponse)
        sum += sample;
    if (sum > 0.0f)
    {
        for (float& sample : impulseResponse)
            sample /= sum;
    }

    // Load into convolution engine
    convolution_.loadImpulseResponse(
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalise::no,
        static_cast<int>(sampleRate_),
        impulseResponse.data(),
        static_cast<int>(impulseResponse.size())
    );
}

void LinearPhaseEQ::designMinimumPhaseFilter()
{
    // For minimum phase, use IIR biquad filters
    juce::dsp::ProcessSpec spec{sampleRate_, static_cast<juce::uint32>(maxSamplesPerBlock_), 2};

    for (int band = 0; band < numBands_; ++band)
    {
        auto& filter = iirFilters_[band].filter;
        const auto& b = bands_[band];

        switch (b.type)
        {
            case FilterType::Lowcut:
                *filter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate_, b.frequency, b.q);
                break;

            case FilterType::Highcut:
                *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate_, b.frequency, b.q);
                break;

            case FilterType::Bell:
                *filter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate_, b.frequency, b.q, juce::Decibels::decibelsToGain(b.gain));
                break;

            case FilterType::Lowshelf:
                *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate_, b.frequency, b.q, juce::Decibels::decibelsToGain(b.gain));
                break;

            case FilterType::Highshelf:
                *filter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate_, b.frequency, b.q, juce::Decibels::decibelsToGain(b.gain));
                break;

            case FilterType::Bandpass:
                *filter.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate_, b.frequency, b.q);
                break;

            case FilterType::Notch:
                *filter.state = *juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate_, b.frequency, b.q);
                break;
        }
    }
}

void LinearPhaseEQ::designFIRFilter(int bandIndex)
{
    const auto& band = bands_[bandIndex];
    int firSize = filterOrder_;
    std::vector<float> coeffs(firSize);

    // Simplified FIR design using frequency sampling
    // In production, would use proper algorithms like:
    // - Frequency sampling method
    // - Parks-McClellan (Remez exchange)
    // - Least squares design

    float beta = 8.0f;  // Kaiser window parameter for sidelobe control
    int center = firSize / 2;

    for (int i = 0; i < firSize; ++i)
    {
        float x = static_cast<float>(i - center) / static_cast<float>(center);
        float window = kaiserWindow(x, beta);

        // Basic bell/boost/cut shape (simplified)
        float freqResponse = 1.0f;
        float freq = static_cast<float>(i) / static_cast<float>(firSize) * sampleRate_ / 2.0f;

        // Create frequency response based on filter type
        float freqNorm = freq / band.frequency;
        float bandwidth = 1.0f / band.q;

        switch (band.type)
        {
            case FilterType::Bell:
                if (std::abs(std::log2(freqNorm)) < std::log2(band.q))
                    freqResponse = juce::Decibels::decibelsToGain(band.gain);
                break;

            case FilterType::Lowshelf:
                if (freq < band.frequency)
                    freqResponse = juce::Decibels::decibelsToGain(band.gain);
                else if (freq < band.frequency * 2.0f)
                    freqResponse = 1.0f;
                break;

            case FilterType::Highshelf:
                if (freq > band.frequency)
                    freqResponse = juce::Decibels::decibelsToGain(band.gain);
                else if (freq > band.frequency / 2.0f)
                    freqResponse = 1.0f;
                break;

            case FilterType::Lowcut:
                freqResponse = (freq > band.frequency) ? 1.0f : 0.0f;
                break;

            case FilterType::Highcut:
                freqResponse = (freq < band.frequency) ? 1.0f : 0.0f;
                break;

            default:
                freqResponse = 1.0f;
                break;
        }

        coeffs[i] = window * freqResponse;
    }

    firFilters_[bandIndex].coefficients = coeffs;
}

float LinearPhaseEQ::kaiserWindow(float x, float beta)
{
    // Kaiser window function
    float alpha = std::cosh(beta);
    float pi_x = juce::MathConstants<float>::pi * x;
    return std::cosh(beta * std::sqrt(1.0f - x * x)) / alpha;
}

float LinearPhaseEQ::blackmanWindow(float x)
{
    // Blackman window
    float pi_x = juce::MathConstants<float>::pi * x;
    return 0.42f - 0.5f * std::cos(pi_x) + 0.08f * std::cos(2.0f * pi_x);
}

void LinearPhaseEQ::updateSpectrumAnalyzer(const juce::AudioBuffer<float>& buffer)
{
    // Perform FFT on buffer
    const int numSamples = buffer.getNumSamples();
    int fftSize = static_cast<int>(fftBuffer_.size() / 2);

    // Copy and window the data
    std::fill(fftBuffer_.begin(), fftBuffer_.end(), 0.0f);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const float* data = buffer.getReadPointer(channel);
        for (int i = 0; i < juce::jmin(numSamples, fftSize); ++i)
        {
            // Apply Hann window
            float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * i / fftSize));
            fftBuffer_[i * 2] += data[i] * window * 0.5f;  // Average channels
        }
    }

    // Perform FFT
    fft_.performRealOnlyForwardTransform(fftBuffer_.data());

    // Convert to magnitude spectrum (dB)
    for (int i = 0; i < spectrumSize; ++i)
    {
        float real = fftBuffer_[i * 2];
        float imag = fftBuffer_[i * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);
        float dB = juce::Decibels::gainToDecibels(magnitude + 1e-6f);
        spectrumData_[i] = juce::jlimit(-100.0f, 0.0f, dB);
    }
}

void LinearPhaseEQ::processMatchEQ(const juce::AudioBuffer<float>& buffer)
{
    // Analyze current spectrum
    std::array<float, matchSpectrumSize> currentSpectrum;
    analyzeSpectrum(buffer, currentSpectrum.data());

    // If we don't have a target spectrum yet, store this as target
    if (!haveTargetSpectrum_)
    {
        std::copy(currentSpectrum.begin(), currentSpectrum.end(), targetSpectrum_.begin());
        haveTargetSpectrum_ = true;
        return;
    }

    // Calculate match curve and apply EQ
    calculateMatchCurve(targetSpectrum_.data(), currentSpectrum.data());

    // Apply matching to bands
    float matchAmount = matchEQAmount_.load();

    // This is simplified - real implementation would adjust band gains
    // based on spectral difference
    for (int band = 0; band < numBands_; ++band)
    {
        float targetGain = matchedSpectrum_[band * matchSpectrumSize / numBands_];
        float currentGain = currentSpectrum[band * matchSpectrumSize / numBands_];
        float correction = (targetGain - currentGain) * matchAmount;

        // Apply correction to band (with smoothing)
        float newGain = juce::jlimit(-24.0f, 24.0f, bands_[band].gain + correction);
        bands_[band].gain = newGain;
    }

    // Re-design filters with new gains
    designFilters();
}

void LinearPhaseEQ::analyzeSpectrum(const juce::AudioBuffer<float>& buffer, float* spectrum)
{
    // Analyze spectrum and store in array
    // This is a simplified version using FFT
    int fftSize = static_cast<int>(fftBuffer_.size() / 2);

    // Copy data
    std::fill(fftBuffer_.begin(), fftBuffer_.end(), 0.0f);
    const float* data = buffer.getReadPointer(0);

    for (int i = 0; i < juce::jmin(buffer.getNumSamples(), fftSize); ++i)
    {
        float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * i / fftSize));
        fftBuffer_[i * 2] = data[i] * window;
    }

    // Perform FFT
    fft_.performRealOnlyForwardTransform(fftBuffer_.data());

    // Convert to spectrum (downsampled to matchSpectrumSize)
    for (int i = 0; i < matchSpectrumSize; ++i)
    {
        int fftIndex = i * fftSize / matchSpectrumSize;
        float real = fftBuffer_[fftIndex * 2];
        float imag = fftBuffer_[fftIndex * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);
        spectrum[i] = juce::Decibels::gainToDecibels(magnitude + 1e-6f);
    }
}

void LinearPhaseEQ::calculateMatchCurve(const float* targetSpectrum, const float* currentSpectrum)
{
    // Calculate the difference spectrum
    for (int i = 0; i < matchSpectrumSize; ++i)
    {
        float difference = targetSpectrum[i] - currentSpectrum[i];
        matchedSpectrum_[i] = currentSpectrum[i] + difference * matchEQAmount_.load();
    }
}

//==============================================================================
juce::ValueTree LinearPhaseEQ::getState() const
{
    juce::ValueTree state("LinearPhaseEQ");

    state.setProperty("phaseMode", static_cast<int>(phaseMode_), nullptr);
    state.setProperty("filterOrder", filterOrder_, nullptr);
    state.setProperty("numBands", numBands_, nullptr);
    state.setProperty("spectrumEnabled", spectrumAnalyzerEnabled_.load(), nullptr);

    // Save each band
    for (int i = 0; i < numBands_; ++i)
    {
        juce::ValueTree bandState("Band");
        bandState.setProperty("index", i, nullptr);
        bandState.setProperty("type", static_cast<int>(bands_[i].type), nullptr);
        bandState.setProperty("frequency", bands_[i].frequency, nullptr);
        bandState.setProperty("gain", bands_[i].gain, nullptr);
        bandState.setProperty("q", bands_[i].q, nullptr);
        bandState.setProperty("enabled", bands_[i].enabled, nullptr);
        state.addChild(bandState, -1, nullptr);
    }

    return state;
}

void LinearPhaseEQ::setState(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType() != juce::String("LinearPhaseEQ"))
        return;

    phaseMode_ = static_cast<PhaseMode>(state.getProperty("phaseMode", 0));
    filterOrder_ = state.getProperty("filterOrder", 2048);
    numBands_ = state.getProperty("numBands", 8);
    spectrumAnalyzerEnabled_.store(state.getProperty("spectrumEnabled", true));

    // Load bands
    int bandIndex = 0;
    for (const auto& child : state)
    {
        if (child.getType() == juce::String("Band"))
        {
            int index = child.getProperty("index", bandIndex);
            if (index >= 0 && index < maxBands)
            {
                bands_[index].type = static_cast<FilterType>(child.getProperty("type", 0).toString().getIntValue());
                bands_[index].frequency = child.getProperty("frequency", 1000.0f);
                bands_[index].gain = child.getProperty("gain", 0.0f);
                bands_[index].q = child.getProperty("q", 1.0f);
                bands_[index].enabled = child.getProperty("enabled", true);
            }
            bandIndex++;
        }
    }

    designFilters();
}

//==============================================================================
juce::String LinearPhaseEQ::exportEQCurve() const
{
    // Export EQ curve as XML
    juce::String xml = "<?xml version=\"1.0\"?>\n<LinearPhaseEQ>\n";
    xml += "  <phaseMode>" + juce::String(static_cast<int>(phaseMode_)) + "</phaseMode>\n";
    xml += "  <filterOrder>" + juce::String(filterOrder_) + "</filterOrder>\n";
    xml += "  <numBands>" + juce::String(numBands_) + "</numBands>\n";
    xml += "  <bands>\n";

    for (int i = 0; i < numBands_; ++i)
    {
        xml += "    <band index=\"" + juce::String(i) + "\">\n";
        xml += "      <type>" + juce::String(static_cast<int>(bands_[i].type)) + "</type>\n";
        xml += "      <frequency>" + juce::String(bands_[i].frequency) + "</frequency>\n";
        xml += "      <gain>" + juce::String(bands_[i].gain) + "</gain>\n";
        xml += "      <q>" + juce::String(bands_[i].q) + "</q>\n";
        xml += "      <enabled>" + juce::String(bands_[i].enabled ? "1" : "0") + "</enabled>\n";
        xml += "    </band>\n";
    }

    xml += "  </bands>\n</LinearPhaseEQ>";
    return xml;
}

bool LinearPhaseEQ::importEQCurve(const juce::String& xml)
{
    // Parse XML and load settings
    // This is a simplified implementation
    juce::XmlElement* element = juce::XmlDocument::parse(xml).getDocumentElement();

    if (!element || element->getTagName() != "LinearPhaseEQ")
        return false;

    // Load settings from XML
    // In a full implementation, would parse all properties
    delete element;

    designFilters();
    return true;
}

//==============================================================================
// Presets
void LinearPhaseEQ::loadPreset(const juce::String& presetName)
{
    if (presetName == "Vocal Presence")
    {
        // Presence boost for vocals
        numBands_ = 4;
        bands_[0].type = FilterType::Highcut;
        bands_[0].frequency = 100.0f;
        bands_[0].gain = 0.0f;
        bands_[0].q = 0.7f;

        bands_[1].type = FilterType::Bell;
        bands_[1].frequency = 2000.0f;
        bands_[1].gain = 3.0f;
        bands_[1].q = 2.0f;

        bands_[2].type = FilterType::Highshelf;
        bands_[2].frequency = 8000.0f;
        bands_[2].gain = 2.0f;
        bands_[2].q = 0.7f;

        bands_[3].type = FilterType::Highcut;
        bands_[3].frequency = 12000.0f;
        bands_[3].gain = -0.0f;
        bands_[3].q = 0.7f;
    }
    else if (presetName == "Bass Boost")
    {
        numBands_ = 3;
        bands_[0].type = FilterType::Lowshelf;
        bands_[0].frequency = 80.0f;
        bands_[0].gain = 6.0f;
        bands_[0].q = 0.7f;

        bands_[1].type = FilterType::Bell;
        bands_[1].frequency = 200.0f;
        bands_[1].gain = -2.0f;
        bands_[1].q = 2.0f;

        bands_[2].type = FilterType::Highshelf;
        bands_[2].frequency = 2000.0f;
        bands_[2].gain = 1.0f;
        bands_[2].q = 0.7f;
    }
    else if (presetName == "Bright Airy")
    {
        numBands_ = 3;
        bands_[0].type = FilterType::Lowshelf;
        bands_[0].frequency = 1000.0f;
        bands_[0].gain = 2.0f;
        bands_[0].q = 0.7f;

        bands_[1].type = FilterType::Bell;
        bands_[1].frequency = 4000.0f;
        bands_[1].gain = 3.0f;
        bands_[1].q = 1.5f;

        bands_[2].type = FilterType::Highshelf;
        bands_[2].frequency = 10000.0f;
        bands_[2].gain = 5.0f;
        bands_[2].q = 0.7f;
    }
    else if (presetName == "Vintage Warm")
    {
        numBands_ = 3;
        bands_[0].type = FilterType::Lowshelf;
        bands_[0].frequency = 200.0f;
        bands_[0].gain = 3.0f;
        bands_[0].q = 0.7f;

        bands_[1].type = FilterType::Bell;
        bands_[1].frequency = 1000.0f;
        bands_[1].gain = -1.5f;
        bands_[1].q = 1.0f;

        bands_[2].type = FilterType::Highshelf;
        bands_[2].frequency = 4000.0f;
        bands_[2].gain = -2.0f;
        bands_[2].q = 0.7f;
    }

    designFilters();
}

juce::StringArray LinearPhaseEQ::getPresetNames()
{
    return {
        "Vocal Presence",
        "Bass Boost",
        "Bright Airy",
        "Vintage Warm"
    };
}

} // namespace effects
} // namespace zenith
