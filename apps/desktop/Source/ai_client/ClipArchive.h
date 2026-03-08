/*
  ==============================================================================

    ClipArchive.h
    Clip archival management for dead/unwanted clips

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Clip archival information
*/
struct ClipArchive {
  juce::String trackId;
  juce::String clipId;
  juce::String reason;
  juce::String targetTrackId; // ID of the quarantine track

  // Helper methods
  juce::String getReason() const;
  juce::String toString() const;
  bool isValid() const;
};

//==============================================================================
/**
    Manager for clip archival operations
*/
class ClipArchiveManager {
public:
  ClipArchiveManager();
  ~ClipArchiveManager();

  void addClipToArchive(const ClipArchive& clip);
  void addClipToArchive(const juce::String& trackId,
                        const juce::String& clipId,
                        const juce::String& reason,
                        const juce::String& targetTrackId);

  int getCount() const;
  bool isEmpty() const;
  void clear();

  const std::vector<ClipArchive>& getClips() const;
  std::vector<ClipArchive>& getClips();

  juce::String getSummary() const;

  juce::var toJSON() const;
  bool fromJSON(const juce::var& json);

private:
  std::vector<ClipArchive> clipsToArchive;
};

} // namespace ai
} // namespace zenith
