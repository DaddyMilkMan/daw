#pragma once
#include <juce_core/juce_core.h>

namespace zenith {

class SampleGenerator {
public:
  /**
   * Checks for missing sample files referenced by example maps
   * and generates simple placeholders if they are missing.
   */
  static void generateMissingSamples();

private:
  static void createWavFile(const juce::File &file, float freq,
                            float durationSecs);
};

} // namespace zenith
