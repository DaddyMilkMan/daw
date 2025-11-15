/**
 * @file TempoMap.cpp
 * @brief RT-safe tempo map implementation
 */

#include "../include/TempoMap.h"
#include <algorithm>
#include <cmath>

//==============================================================================
TempoMap::TempoMap()
    : sampleRate_(44100.0)
{
    // Create default segment: 120 BPM at beat 0
    TempoSegment defaultSegment;
    defaultSegment.startBeats = 0.0;
    defaultSegment.bpm = 120.0;
    defaultSegment.startSamples = 0;
    defaultSegment.timeSig.numerator = 4;
    defaultSegment.timeSig.denominator = 4;
    defaultSegment.secondsPerBeat = 60.0 / 120.0;
    defaultSegment.samplesPerBeat = sampleRate_ * defaultSegment.secondsPerBeat;

    segments_.push_back(defaultSegment);
}

TempoMap::TempoMap(const std::vector<TempoPoint>& tempoPoints, double sampleRate)
    : sampleRate_(sampleRate)
{
    jassert(sampleRate > 0.0);
    jassert(!tempoPoints.empty());

    // Reserve space to avoid reallocations
    segments_.reserve(tempoPoints.size());

    // Build segments with precomputed values
    juce::int64 currentSamples = 0;

    for (size_t i = 0; i < tempoPoints.size(); ++i)
    {
        const auto& pt = tempoPoints[i];

        TempoSegment segment;
        segment.startBeats = pt.timeBeats;
        segment.bpm = pt.bpm;
        segment.startSamples = currentSamples;
        segment.timeSig.numerator = pt.timeSigNum;
        segment.timeSig.denominator = pt.timeSigDen;

        // Precompute conversion factors
        segment.secondsPerBeat = 60.0 / segment.bpm;
        segment.samplesPerBeat = sampleRate_ * segment.secondsPerBeat;

        segments_.push_back(segment);

        // Calculate sample offset to next segment (if there is one)
        if (i + 1 < tempoPoints.size())
        {
            const auto& nextPt = tempoPoints[i + 1];
            double beatDuration = nextPt.timeBeats - pt.timeBeats;
            juce::int64 sampleDuration = static_cast<juce::int64>(std::round(beatDuration * segment.samplesPerBeat));
            currentSamples += sampleDuration;
        }
    }
}

//==============================================================================
double TempoMap::samplesToBeats(juce::int64 samplePos) const
{
    if (segments_.empty())
        return 0.0;

    if (samplePos <= 0)
        return 0.0;

    // Find the segment containing this sample position
    int segmentIdx = findSegmentForSamples(samplePos);
    const auto& segment = segments_[segmentIdx];

    // Calculate beats within this segment
    juce::int64 samplesIntoSegment = samplePos - segment.startSamples;
    double beatsIntoSegment = static_cast<double>(samplesIntoSegment) / segment.samplesPerBeat;

    return segment.startBeats + beatsIntoSegment;
}

juce::int64 TempoMap::beatsToSamples(double beats) const
{
    if (segments_.empty())
        return 0;

    if (beats <= 0.0)
        return 0;

    // Find the segment containing this beat position
    int segmentIdx = findSegmentForBeats(beats);
    const auto& segment = segments_[segmentIdx];

    // Calculate samples within this segment
    double beatsIntoSegment = beats - segment.startBeats;
    juce::int64 samplesIntoSegment = static_cast<juce::int64>(std::round(beatsIntoSegment * segment.samplesPerBeat));

    return segment.startSamples + samplesIntoSegment;
}

double TempoMap::getTempoAtBeats(double beats) const
{
    if (segments_.empty())
        return 120.0;

    int segmentIdx = findSegmentForBeats(beats);
    return segments_[segmentIdx].bpm;
}

TimeSignature TempoMap::getTimeSignatureAtBeats(double beats) const
{
    if (segments_.empty())
        return TimeSignature{4, 4};

    int segmentIdx = findSegmentForBeats(beats);
    return segments_[segmentIdx].timeSig;
}

//==============================================================================
int TempoMap::findSegmentForBeats(double beats) const
{
    if (segments_.empty())
        return 0;

    if (beats <= segments_[0].startBeats)
        return 0;

    // Binary search for the segment
    // We want the last segment where startBeats <= beats
    int left = 0;
    int right = static_cast<int>(segments_.size()) - 1;
    int result = 0;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        if (segments_[mid].startBeats <= beats)
        {
            result = mid;
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return result;
}

int TempoMap::findSegmentForSamples(juce::int64 samplePos) const
{
    if (segments_.empty())
        return 0;

    if (samplePos <= segments_[0].startSamples)
        return 0;

    // Binary search for the segment
    // We want the last segment where startSamples <= samplePos
    int left = 0;
    int right = static_cast<int>(segments_.size()) - 1;
    int result = 0;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        if (segments_[mid].startSamples <= samplePos)
        {
            result = mid;
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return result;
}
