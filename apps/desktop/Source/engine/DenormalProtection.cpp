/*
  ==============================================================================

    DenormalProtection.cpp
    Implementation of denormal number detection and flushing

  ==============================================================================
*/

#include "DenormalProtection.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>

#if JUCE_WINDOWS
#include <intrin.h>
#elif JUCE_MAC || JUCE_LINUX
#include <fenv.h>
#endif

namespace zenith {

//==============================================================================
// DenormalProtection Implementation
//==============================================================================

DenormalProtection::DenormalProtection() {
    // Try to enable hardware denormals
    enableHardwareDenormals();

    std::cout << "DenormalProtection: Initialized" << std::endl;
}

DenormalProtection::~DenormalProtection() {
    std::cout << "DenormalProtection: Shut down ("
              << statistics_.denormalsDetected.load() << " detected, "
              << statistics_.denormalsFlushed.load() << " flushed)" << std::endl;
}

//==============================================================================
bool DenormalProtection::isDenormal(float value) {
    // Denormal numbers have very small magnitude
    // Check if exponent is zero (subnormal/denormal)
    union { float f; uint32_t i; } u;
    u.f = value;
    return (u.i & 0x7F800000) == 0 && (u.i & 0x007FFFFF) != 0;
}

bool DenormalProtection::isDenormal(double value) {
    union { double d; uint64_t i; } u;
    u.d = value;
    return (u.i & 0x7FF0000000000000ULL) == 0 && (u.i & 0x000FFFFFFFFFFFFFULL) != 0;
}

//==============================================================================
float DenormalProtection::flushDenormal(float value) {
    if (isDenormal(value)) {
        return 0.0f;
    }
    return value;
}

double DenormalProtection::flushDenormal(double value) {
    if (isDenormal(value)) {
        return 0.0;
    }
    return value;
}

//==============================================================================
int DenormalProtection::cleanBuffer(
    juce::AudioBuffer<float>& buffer,
    const DenormalProtectionConfig& config)
{
    if (!config.enableAutoFlush) {
        return 0;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    int flushedCount = 0;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        float* data = buffer.getWritePointer(channel);

        for (int i = 0; i < samples; ++i) {
            if (std::abs(data[i]) < config.denormalThreshold) {
                if (isDenormal(data[i])) {
                    statistics_.denormalsDetected++;
                    data[i] = 0.0f;
                    flushedCount++;
                    statistics_.denormalsFlushed++;
                }
            }
        }
    }

    if (flushedCount > 0 && config.enableStatistics) {
        statistics_.buffersCleaned++;

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> flushTime = endTime - startTime;
        statistics_.totalFlushTime += flushTime.count();
    }

    return flushedCount;
}

//==============================================================================
int DenormalProtection::cleanBuffer(
    juce::AudioBuffer<double>& buffer,
    const DenormalProtectionConfig& config)
{
    if (!config.enableAutoFlush) {
        return 0;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    int flushedCount = 0;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        double* data = buffer.getWritePointer(channel);

        for (int i = 0; i < samples; ++i) {
            if (std::abs(data[i]) < config.denormalThreshold) {
                if (isDenormal(data[i])) {
                    statistics_.denormalsDetected++;
                    data[i] = 0.0;
                    flushedCount++;
                    statistics_.denormalsFlushed++;
                }
            }
        }
    }

    if (flushedCount > 0 && config.enableStatistics) {
        statistics_.buffersCleaned++;

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> flushTime = endTime - startTime;
        statistics_.totalFlushTime += flushTime.count();
    }

    return flushedCount;
}

//==============================================================================
int DenormalProtection::countDenormals(const juce::AudioBuffer<float>& buffer) const {
    int count = 0;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel) {
        const float* data = buffer.getReadPointer(channel);

        for (int i = 0; i < samples; ++i) {
            if (isDenormal(data[i])) {
                count++;
            }
        }
    }

    return count;
}

//==============================================================================
void DenormalProtection::resetStatistics() {
    statistics_.denormalsDetected.store(0);
    statistics_.denormalsFlushed.store(0);
    statistics_.buffersCleaned.store(0);
    statistics_.totalFlushTime.store(0.0);

    std::cout << "DenormalProtection: Statistics reset" << std::endl;
}

//==============================================================================
bool DenormalProtection::enableHardwareDenormals() {
    // Try to enable CPU-level FTZ (Flush To Zero) and DAZ (Denormals Are Zero)

#if JUCE_WINDOWS
    // MSVC/SSE
    unsigned int controlWord = _MM_GET_DENORMALS_ZERO_MODE();
    if (!controlWord) {
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        return true;
    }
    return false;

#elif JUCE_MAC || JUCE_LINUX
    // POSIX - try using fenv.h
    // This may not be available on all platforms
    #ifdef FE_DFL_DISABLE_SSE_DENORMS_WORKAROUND
    fenv_t fe;
    fegetenv(&fe);
    fe.__x87.__denormal = _MM_DENORMALS_ZERO_ON;
    fesetenv(&fe);
    return true;
    #else
    return false;
    #endif

#else
    return false;
#endif
}

} // namespace zenith
