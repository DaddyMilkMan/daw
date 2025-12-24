#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace zenith {

struct TempoPoint {
  double timeBeats;
  double bpm;

  TempoPoint(double beats, double tempoBpm) : timeBeats(beats), bpm(tempoBpm) {}
};

struct TempoMapSnapshot {
  std::vector<TempoPoint> points;
  int timeSigNumerator = 4;
  int timeSigDenominator = 4;

  TempoMapSnapshot() = default;

  void prepare();

  struct CachedPoint {
    double timeBeats;
    double bpm;
    double cumulativeSeconds;
  };

  std::vector<CachedPoint> cachedPoints;
};

class TempoMap {
public:
  TempoMap();
  ~TempoMap();

  void updateFromValueTree(const juce::ValueTree &tempoMapTree);
  void setSingleTempo(double bpm);

  double beatsToSeconds(double beats, double sampleRate) const;
  double secondsToBeats(double seconds, double sampleRate) const;
  int64_t beatsToSamples(double beats, double sampleRate) const;
  double samplesToBeats(int64_t samples, double sampleRate) const;
  double getTempoAt(double beats) const;
  int getTimeSignatureNumerator() const;
  int getTimeSignatureDenominator() const;

private:
  std::atomic<std::shared_ptr<const TempoMapSnapshot>> snapshot_;
  void swapSnapshot(std::shared_ptr<const TempoMapSnapshot> newSnapshot);
  std::shared_ptr<const TempoMapSnapshot> loadSnapshot() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMap)
};

} // namespace zenith
