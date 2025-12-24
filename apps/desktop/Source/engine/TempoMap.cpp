#include "TempoMap.h"
#include "ProjectState.h"
#include "RealTimeGarbageCollector.h"
#include <algorithm>
#include <cstdint>

namespace zenith {

void TempoMapSnapshot::prepare() {
  cachedPoints.clear();
  if (points.empty()) {
    CachedPoint cached;
    cached.timeBeats = 0.0;
    cached.bpm = 120.0;
    cached.cumulativeSeconds = 0.0;
    cachedPoints.push_back(cached);
    return;
  }

  double cumulativeSeconds = 0.0;
  for (size_t i = 0; i < points.size(); ++i) {
    const auto &point = points[i];
    CachedPoint cached;
    cached.timeBeats = point.timeBeats;
    cached.bpm = point.bpm;
    cached.cumulativeSeconds = cumulativeSeconds;
    cachedPoints.push_back(cached);

    if (i + 1 < points.size()) {
      double beatDelta = points[i + 1].timeBeats - point.timeBeats;
      double secondsPerBeat = 60.0 / point.bpm;
      double timeDelta = beatDelta * secondsPerBeat;
      cumulativeSeconds += timeDelta;
    }
  }
}

TempoMap::TempoMap() {
  auto defaultSnapshot = std::make_shared<TempoMapSnapshot>();
  defaultSnapshot->points.emplace_back(0.0, 120.0);
  defaultSnapshot->prepare();
  snapshot_.store(defaultSnapshot, std::memory_order_release);
}

TempoMap::~TempoMap() {}

void TempoMap::updateFromValueTree(const juce::ValueTree &tempoMapTree) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  auto newSnapshot = std::make_shared<TempoMapSnapshot>();
  newSnapshot->timeSigNumerator = tempoMapTree.getProperty("timeSigNum", 4);
  newSnapshot->timeSigDenominator = tempoMapTree.getProperty("timeSigDen", 4);

  if (!tempoMapTree.isValid()) {
    newSnapshot->points.emplace_back(0.0, 120.0);
  } else {
    for (const auto &pointTree : tempoMapTree) {
      if (pointTree.hasType(ProjectState::ID_TEMPO_POINT)) {
        double timeBeats = pointTree[ProjectState::PROP_TIME_BEATS];
        double bpm = pointTree[ProjectState::PROP_BPM];
        newSnapshot->points.emplace_back(timeBeats, bpm);
      }
    }
    if (newSnapshot->points.empty()) {
      newSnapshot->points.emplace_back(0.0, 120.0);
    }
    std::sort(newSnapshot->points.begin(), newSnapshot->points.end(),
              [](const TempoPoint &a, const TempoPoint &b) {
                return a.timeBeats < b.timeBeats;
              });
  }

  newSnapshot->prepare();
  swapSnapshot(newSnapshot);
}

void TempoMap::setSingleTempo(double bpm) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  auto newSnapshot = std::make_shared<TempoMapSnapshot>();
  newSnapshot->points.emplace_back(0.0, bpm);
  newSnapshot->prepare();
  swapSnapshot(newSnapshot);
}

void TempoMap::swapSnapshot(
    std::shared_ptr<const TempoMapSnapshot> newSnapshot) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  auto oldSnapshot = snapshot_.load(std::memory_order_acquire);
  snapshot_.store(newSnapshot, std::memory_order_release);
  if (oldSnapshot) {
    RealTimeGarbageCollector::getInstance().push(oldSnapshot);
  }
}

std::shared_ptr<const TempoMapSnapshot> TempoMap::loadSnapshot() const {
  return snapshot_.load(std::memory_order_acquire);
}

double TempoMap::beatsToSeconds(double beats, double /* sampleRate */) const {
  auto snap = loadSnapshot();
  if (!snap || snap->cachedPoints.empty())
    return beats * 0.5;
  const auto &cached = snap->cachedPoints;
  size_t index = 0;
  for (size_t i = 0; i < cached.size(); ++i) {
    if (cached[i].timeBeats <= beats)
      index = i;
    else
      break;
  }
  const auto &point = cached[index];
  double beatDelta = beats - point.timeBeats;
  double secondsPerBeat = 60.0 / point.bpm;
  return point.cumulativeSeconds + (beatDelta * secondsPerBeat);
}

double TempoMap::secondsToBeats(double seconds, double /* sampleRate */) const {
  auto snap = loadSnapshot();
  if (!snap || snap->cachedPoints.empty())
    return seconds * 2.0;
  const auto &cached = snap->cachedPoints;
  size_t index = 0;
  for (size_t i = 0; i < cached.size(); ++i) {
    if (cached[i].cumulativeSeconds <= seconds)
      index = i;
    else
      break;
  }
  const auto &point = cached[index];
  double timeDelta = seconds - point.cumulativeSeconds;
  double secondsPerBeat = 60.0 / point.bpm;
  return point.timeBeats + (timeDelta / secondsPerBeat);
}

int64_t TempoMap::beatsToSamples(double beats, double sampleRate) const {
  return static_cast<int64_t>(beatsToSeconds(beats, sampleRate) * sampleRate);
}

double TempoMap::samplesToBeats(int64_t samples, double sampleRate) const {
  if (sampleRate <= 0.0)
    return 0.0;
  return secondsToBeats(static_cast<double>(samples) / sampleRate, sampleRate);
}

double TempoMap::getTempoAt(double beats) const {
  auto snap = loadSnapshot();
  if (!snap || snap->cachedPoints.empty())
    return 120.0;
  size_t index = 0;
  for (size_t i = 0; i < snap->cachedPoints.size(); ++i) {
    if (snap->cachedPoints[i].timeBeats <= beats)
      index = i;
    else
      break;
  }
  return snap->cachedPoints[index].bpm;
}

int TempoMap::getTimeSignatureNumerator() const {
  return loadSnapshot()->timeSigNumerator;
}

int TempoMap::getTimeSignatureDenominator() const {
  return loadSnapshot()->timeSigDenominator;
}

} // namespace zenith
