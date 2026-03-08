/*
  ==============================================================================

    SampleConsolidation.h
    Sample deduplication and consolidation

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <set>
#include <vector>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Sample consolidation information
*/
struct SampleConsolidation {
  juce::File sourceFile;
  juce::File destFile;
  juce::String clipId;

  // Helper methods
  juce::String toString() const;
  bool isValid() const;
  bool needsCopy() const;
  juce::int64 getSourceFileSize() const;
  juce::int64 getEstimatedDiskSpaceSaved() const;
};

//==============================================================================
/**
    Manager for sample consolidation operations
*/
class SampleConsolidationManager {
public:
  SampleConsolidationManager();
  ~SampleConsolidationManager();

  void addSampleToConsolidate(const SampleConsolidation& sample);
  void addSampleToConsolidate(const juce::File& sourceFile,
                               const juce::File& destFile,
                               const juce::String& clipId);

  int getCount() const;
  bool isEmpty() const;
  void clear();

  const std::vector<SampleConsolidation>& getSamples() const;
  std::vector<SampleConsolidation>& getSamples();

  juce::int64 getTotalSourceSize() const;
  juce::String getTotalSourceSizeString() const;
  juce::String getSummary() const;

  juce::Result execute(juce::ThreadPool* threadPool = nullptr);

  juce::var toJSON() const;
  bool fromJSON(const juce::var& json);

private:
  static juce::String formatFileSize(juce::int64 bytes);

  std::vector<SampleConsolidation> samplesToConsolidate;
};

} // namespace ai
} // namespace zenith
