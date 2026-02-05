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
*/

/*
    ==============================================================================
    Original file header:
*/

//     File: TempoMap.cpp
//     Brief: RT-safe tempo map implementation
//*


//==============================================================================
// TempoMapSnapshot Implementation
//==============================================================================

void TempoMapSnapshot::prepare()
{
    cachedPoints.clear();

    if (points.empty())
    {
        // Default: 120 BPM at beat 0
        CachedPoint cached;
        cached.timeBeats = 0.0;
        cached.bpm = 120.0;
        cached.cumulativeSeconds = 0.0;
        cachedPoints.push_back(cached);
        return;
    }

    double cumulativeSeconds = 0.0;

    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& point = points[i];

        CachedPoint cached;
        cached.timeBeats = point.timeBeats;
        cached.bpm = point.bpm;
        cached.cumulativeSeconds = cumulativeSeconds;
        cachedPoints.push_back(cached);

        // Calculate time to next point
        if (i + 1 < points.size())
        {
            double beatDelta = points[i + 1].timeBeats - point.timeBeats;
            double secondsPerBeat = 60.0 / std::max(0.1, point.bpm);
            double timeDelta = beatDelta * secondsPerBeat;
            cumulativeSeconds += timeDelta;
        }
    }
}

//==============================================================================
// TempoMap Implementation
//==============================================================================

TempoMap::TempoMap()
{
    // Create default snapshot: 120 BPM at beat 0
    auto defaultSnapshot = std::make_shared<TempoMapSnapshot>();
    defaultSnapshot->points.emplace_back(0.0, 120.0);
    defaultSnapshot->prepare();

    snapshot_ = defaultSnapshot;
}

TempoMap::~TempoMap()
{
}

void TempoMap::updateFromValueTree(const juce::ValueTree& tempoMapTree)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto newSnapshot = std::make_shared<TempoMapSnapshot>();
    
    // Get time signature from properties (match ProjectState names)
    newSnapshot->timeSigNumerator = tempoMapTree.getProperty("timeSignatureNumerator", 4);
    newSnapshot->timeSigDenominator = tempoMapTree.getProperty("timeSignatureDenominator", 4);

    // Safeguard numerator
    if (newSnapshot->timeSigNumerator <= 0) newSnapshot->timeSigNumerator = 4;

    if (!tempoMapTree.isValid())
    {
        // No tempo map: default to 120 BPM
        newSnapshot->points.emplace_back(0.0, 120.0);
    }
    else
    {
        // Extract tempo points from ValueTree
        for (const auto& pointTree : tempoMapTree)
        {
            if (pointTree.hasType(ProjectState::ID_TEMPO_POINT))
            {
                double timeBeats = pointTree[ProjectState::PROP_TIME_BEATS];
                double bpm = pointTree[ProjectState::PROP_BPM];

                newSnapshot->points.emplace_back(timeBeats, bpm);
            }
        }

        // Ensure we have at least one tempo point
        if (newSnapshot->points.empty())
        {
            newSnapshot->points.emplace_back(0.0, 120.0);
        }

        // Sort by timeBeats (should already be sorted, but be safe)
        std::sort(newSnapshot->points.begin(), newSnapshot->points.end(),
                  [](const TempoPoint& a, const TempoPoint& b) {
                      return a.timeBeats < b.timeBeats;
                  });
    }

    // Prepare cached data
    newSnapshot->prepare();

    // Swap snapshot
    swapSnapshot(newSnapshot);

    DBG("TempoMap: Updated with " + juce::String(newSnapshot->points.size()) + " tempo points");
}

void TempoMap::setSingleTempo(double bpm)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto newSnapshot = std::make_shared<TempoMapSnapshot>();
    newSnapshot->points.emplace_back(0.0, bpm);
    newSnapshot->prepare();

    swapSnapshot(newSnapshot);

    DBG("TempoMap: Set single tempo to " + juce::String(bpm) + " BPM");
}

void TempoMap::swapSnapshot(std::shared_ptr<const TempoMapSnapshot> newSnapshot)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    snapshot_.store(newSnapshot, std::memory_order_release);
}

std::shared_ptr<const TempoMapSnapshot> TempoMap::loadSnapshot() const
{
    // RT-safe: atomic load of shared_ptr
    return snapshot_.load(std::memory_order_acquire);
}

//==============================================================================
// RT-Safe Conversion Methods
//==============================================================================

double TempoMap::beatsToSeconds(double beats, double /* sampleRate */) const
{
    auto snap = loadSnapshot();
    if (!snap || snap->cachedPoints.empty())
        return beats * (60.0 / 120.0);  // Fallback to 120 BPM

    // Find the tempo segment containing this beat
    const auto& cached = snap->cachedPoints;

    // Find the last cached point at or before 'beats'
    size_t index = 0;
    for (size_t i = 0; i < cached.size(); ++i)
    {
        if (cached[i].timeBeats <= beats)
            index = i;
        else
            break;
    }

    // Calculate time from the found point
    const auto& point = cached[index];
    double beatDelta = beats - point.timeBeats;
    double secondsPerBeat = 60.0 / std::max(0.1, point.bpm);
    double timeDelta = beatDelta * secondsPerBeat;

    return point.cumulativeSeconds + timeDelta;
}

double TempoMap::secondsToBeats(double seconds, double /* sampleRate */) const
{
    auto snap = loadSnapshot();
    if (!snap || snap->cachedPoints.empty())
        return seconds / (60.0 / 120.0);  // Fallback to 120 BPM

    const auto& cached = snap->cachedPoints;

    // Find the tempo segment containing this time
    size_t index = 0;
    for (size_t i = 0; i < cached.size(); ++i)
    {
        if (cached[i].cumulativeSeconds <= seconds)
            index = i;
        else
            break;
    }

    // Calculate beats from the found point
    const auto& point = cached[index];
    double timeDelta = seconds - point.cumulativeSeconds;
    double secondsPerBeat = 60.0 / std::max(0.1, point.bpm);
    double beatDelta = timeDelta / secondsPerBeat;

    return point.timeBeats + beatDelta;
}

int64_t TempoMap::beatsToSamples(double beats, double sampleRate) const
{
    double seconds = beatsToSeconds(beats, sampleRate);
    return static_cast<int64_t>(seconds * sampleRate);
}

double TempoMap::samplesToBeats(int64_t samples, double sampleRate) const
{
    if (sampleRate <= 0.0)
        return 0.0;

    double seconds = static_cast<double>(samples) / sampleRate;
    return secondsToBeats(seconds, sampleRate);
}

double TempoMap::getTempoAt(double beats) const
{
    auto snap = loadSnapshot();
    if (!snap || snap->cachedPoints.empty())
        return 120.0;

    // Find the tempo at or before this beat
    size_t index = 0;
    for (size_t i = 0; i < snap->cachedPoints.size(); ++i)
    {
        if (snap->cachedPoints[i].timeBeats <= beats)
            index = i;
        else
            break;
    }

    return snap->cachedPoints[index].bpm;
}

int TempoMap::getTimeSignatureNumerator() const
{
    return loadSnapshot()->timeSigNumerator;
}

int TempoMap::getTimeSignatureDenominator() const
{
    return loadSnapshot()->timeSigDenominator;
}

} // namespace zenith


