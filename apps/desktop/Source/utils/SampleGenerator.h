#pragma once
#include <juce_core/juce_core.h>
#include <functional>

namespace zenith {

class SampleGenerator {
public:
  /**
   * Checks for missing sample files referenced by example maps
   * and generates simple placeholders if they are missing.
   *
   * @param threadPool Optional thread pool to run generation asynchronously.
   *                   If nullptr, runs synchronously.
   * @param onComplete Optional callback when generation finishes.
   */
  static void generateMissingSamples(juce::ThreadPool* threadPool = nullptr, std::function<void()> onComplete = nullptr);
};

} // namespace zenith
