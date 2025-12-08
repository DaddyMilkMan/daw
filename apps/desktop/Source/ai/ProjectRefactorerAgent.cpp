/*
  ==============================================================================

    ProjectRefactorerAgent.cpp
    Created: 2025-12-07
    Author:  Zenith DAW

    "The Refactorer" - Project Organization Agent Implementation

  ==============================================================================
*/

#include "ProjectRefactorerAgent.h"
#include "../engine/AudioFilePool.h"
#include <algorithm>
#include <regex>

namespace zenith {
namespace ai {

//==============================================================================
// Constructor / Destructor
//==============================================================================

ProjectRefactorerAgent::ProjectRefactorerAgent(Engine &engine,
                                               ProjectState &projectState)
    : engine_(engine), projectState_(projectState) {
  DBG("ProjectRefactorerAgent: Initialized");
}

ProjectRefactorerAgent::~ProjectRefactorerAgent() { cancel(); }

//==============================================================================
// Configuration
//==============================================================================

void ProjectRefactorerAgent::setOptions(const Options &options) {
  options_ = options;
}

//==============================================================================
// Analysis
//==============================================================================

RefactorPlan ProjectRefactorerAgent::analyzeQuick() {
  DBG("ProjectRefactorerAgent: Starting quick analysis...");

  issues_.clear();
  currentPlan_ = RefactorPlan();
  categoryCounters_.clear();

  // Run all detection passes
  if (options_.renameGenericTracks)
    detectGenericNames();

  if (options_.removeDeadClips)
    detectDeadClips();

  if (options_.consolidateSamples)
    detectScatteredSamples();

  if (options_.assignTrackColors)
    detectMissingColors();

  if (options_.createTrackGroups)
    detectGroupableEntities();

  // Build the plan from detected issues
  buildRefactorPlan();

  DBG("ProjectRefactorerAgent: Quick analysis complete - found " +
      juce::String(issues_.size()) + " issues");

  return currentPlan_;
}

void ProjectRefactorerAgent::analyze(
    std::function<void(const RefactorProgress &)> onProgress,
    std::function<void(const RefactorPlan &)> onComplete) {
  if (analyzing_.load()) {
    DBG("ProjectRefactorerAgent: Already analyzing, ignoring request");
    return;
  }

  analyzing_.store(true);
  cancelled_.store(false);

  // Start async analysis
  juce::Thread::launch([this, onProgress, onComplete]() {
    RefactorProgress progress;
    progress.totalSteps = 5;

    // Quick analysis first
    progress.currentStep = 1;
    progress.currentOperation = "Detecting generic track names...";
    progress.progressPercent = 10.0f;
    if (onProgress) {
      juce::MessageManager::callAsync(
          [onProgress, progress]() { onProgress(progress); });
    }

    if (cancelled_.load()) {
      analyzing_.store(false);
      return;
    }

    issues_.clear();
    currentPlan_ = RefactorPlan();
    categoryCounters_.clear();

    // Step 1: Generic names
    if (options_.renameGenericTracks)
      detectGenericNames();

    progress.currentStep = 2;
    progress.currentOperation = "Analyzing clips for dead code...";
    progress.progressPercent = 30.0f;
    if (onProgress) {
      juce::MessageManager::callAsync(
          [onProgress, progress]() { onProgress(progress); });
    }

    if (cancelled_.load()) {
      analyzing_.store(false);
      return;
    }

    // Step 2: Dead clips
    if (options_.removeDeadClips)
      detectDeadClips();

    progress.currentStep = 3;
    progress.currentOperation = "Finding scattered samples...";
    progress.progressPercent = 50.0f;
    if (onProgress) {
      juce::MessageManager::callAsync(
          [onProgress, progress]() { onProgress(progress); });
    }

    if (cancelled_.load()) {
      analyzing_.store(false);
      return;
    }

    // Step 3: Scattered samples
    if (options_.consolidateSamples)
      detectScatteredSamples();

    progress.currentStep = 4;
    progress.currentOperation = "Detecting track organization issues...";
    progress.progressPercent = 70.0f;
    if (onProgress) {
      juce::MessageManager::callAsync(
          [onProgress, progress]() { onProgress(progress); });
    }

    if (cancelled_.load()) {
      analyzing_.store(false);
      return;
    }

    // Step 4: Colors and groups
    if (options_.assignTrackColors)
      detectMissingColors();

    if (options_.createTrackGroups)
      detectGroupableEntities();

    progress.currentStep = 5;
    progress.currentOperation = "Building refactoring plan...";
    progress.progressPercent = 90.0f;
    if (onProgress) {
      juce::MessageManager::callAsync(
          [onProgress, progress]() { onProgress(progress); });
    }

    if (cancelled_.load()) {
      analyzing_.store(false);
      return;
    }

    // Step 5: Build plan
    buildRefactorPlan();

    progress.currentStep = 5;
    progress.currentOperation = "Analysis complete!";
    progress.progressPercent = 100.0f;

    analyzing_.store(false);

    if (onComplete) {
      juce::MessageManager::callAsync(
          [this, onComplete]() { onComplete(currentPlan_); });
    }
  });
}

//==============================================================================
// Detection Methods
//==============================================================================

void ProjectRefactorerAgent::detectGenericNames() {
  auto &state = projectState_.getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return;

  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto track = tracksNode.getChild(i);
    juce::String trackId = track[ProjectState::PROP_ID].toString();
    juce::String name = track[ProjectState::PROP_NAME].toString();

    if (isGenericName(name)) {
      issues_.emplace_back(RefactorIssue::Type::GenericTrackName, trackId,
                           "Track '" + name + "' has a generic name");
    }
  }
}

void ProjectRefactorerAgent::detectDeadClips() {
  auto &state = projectState_.getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return;

  // Determine song boundaries
  double songStart = options_.songStartBeats;
  double songEnd = options_.songEndBeats;

  // Auto-detect song end if not specified
  if (songEnd < 0.0) {
    songEnd = 0.0;
    for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
      auto track = tracksNode.getChild(i);
      auto clips = track.getChildWithName(ProjectState::ID_CLIPS);

      if (clips.isValid()) {
        for (int j = 0; j < clips.getNumChildren(); ++j) {
          auto clip = clips.getChild(j);
          double start = clip[ProjectState::PROP_START];
          double length = clip[ProjectState::PROP_LENGTH];
          songEnd = std::max(songEnd, start + length);
        }
      }
    }
  }

  // Now detect dead clips
  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto track = tracksNode.getChild(i);
    juce::String trackId = track[ProjectState::PROP_ID].toString();
    auto clips = track.getChildWithName(ProjectState::ID_CLIPS);

    if (!clips.isValid())
      continue;

    for (int j = 0; j < clips.getNumChildren(); ++j) {
      auto clip = clips.getChild(j);
      juce::String clipId = clip[ProjectState::PROP_ID].toString();
      double clipStart = clip[ProjectState::PROP_START];
      double clipLength = clip[ProjectState::PROP_LENGTH];
      double clipEnd = clipStart + clipLength;
      bool isMuted = clip.hasProperty("muted") && (bool)clip["muted"];

      // Check if muted
      if (isMuted) {
        issues_.emplace_back(RefactorIssue::Type::MutedClip, trackId, clipId,
                             "Clip is muted");
      }

      // Check if completely outside song bounds
      if (clipEnd < songStart || clipStart > songEnd) {
        issues_.emplace_back(RefactorIssue::Type::ClipOutOfBounds, trackId,
                             clipId, "Clip is outside song boundaries");
      }
    }
  }
}

void ProjectRefactorerAgent::detectScatteredSamples() {
  juce::File projectDir = projectState_.getProjectFile().getParentDirectory();

  if (!projectDir.exists())
    projectDir = juce::File::getCurrentWorkingDirectory();

  auto &state = projectState_.getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return;

  std::set<juce::String> seenFolders;

  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto track = tracksNode.getChild(i);
    juce::String trackId = track[ProjectState::PROP_ID].toString();
    auto clips = track.getChildWithName(ProjectState::ID_CLIPS);

    if (!clips.isValid())
      continue;

    for (int j = 0; j < clips.getNumChildren(); ++j) {
      auto clip = clips.getChild(j);
      juce::String clipId = clip[ProjectState::PROP_ID].toString();

      if (clip.hasProperty(ProjectState::PROP_AUDIO_FILE)) {
        juce::File audioFile(clip[ProjectState::PROP_AUDIO_FILE].toString());

        if (audioFile.exists()) {
          juce::File audioFolder = audioFile.getParentDirectory();

          // Check if the file is outside the project directory
          if (!audioFile.isAChildOf(projectDir)) {
            RefactorIssue issue(
                RefactorIssue::Type::ScatteredSamples, trackId, clipId,
                "Sample is external: " + audioFile.getFullPathName());
            issue.externalFile = audioFile;
            issues_.push_back(issue);

            seenFolders.insert(audioFolder.getFullPathName());
          }
        }
      }
    }
  }

  if (seenFolders.size() > 1) {
    DBG("ProjectRefactorerAgent: Found samples in " +
        juce::String(seenFolders.size()) + " external folders");
  }
}

void ProjectRefactorerAgent::detectMissingColors() {
  auto &state = projectState_.getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return;

  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto track = tracksNode.getChild(i);
    juce::String trackId = track[ProjectState::PROP_ID].toString();

    if (!track.hasProperty(ProjectState::PROP_COLOR)) {
      issues_.emplace_back(RefactorIssue::Type::NoTrackColor, trackId,
                           "Track has no color assigned");
    }
  }
}

void ProjectRefactorerAgent::detectGroupableEntities() {
  auto &state = projectState_.getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  if (!tracksNode.isValid())
    return;

  // Count tracks by category
  std::map<InstrumentCategory, std::vector<juce::String>> categoryTracks;

  for (int i = 0; i < tracksNode.getNumChildren(); ++i) {
    auto track = tracksNode.getChild(i);
    juce::String trackId = track[ProjectState::PROP_ID].toString();
    juce::String name = track[ProjectState::PROP_NAME].toString();

    InstrumentCategory cat = classifyFromName(name);
    if (cat != InstrumentCategory::Unknown) {
      categoryTracks[cat].push_back(trackId);
    }
  }

  // Check if drums could be grouped
  int drumTrackCount = 0;
  for (auto cat : {InstrumentCategory::Kick, InstrumentCategory::Snare,
                   InstrumentCategory::HiHat, InstrumentCategory::Cymbal,
                   InstrumentCategory::Toms, InstrumentCategory::Percussion}) {
    drumTrackCount += static_cast<int>(categoryTracks[cat].size());
  }

  if (drumTrackCount >= 3) {
    issues_.emplace_back(RefactorIssue::Type::UnorganizedDrums,
                         "", // No specific track
                         "Found " + juce::String(drumTrackCount) +
                             " drum tracks that could be grouped");
  }
}

//==============================================================================
// Plan Building
//==============================================================================

void ProjectRefactorerAgent::buildRefactorPlan() {
  currentPlan_ = RefactorPlan();
  currentPlan_.totalIssuesFound = static_cast<int>(issues_.size());

  auto &state = projectState_.getState();
  auto tracksNode = state.getChildWithName(ProjectState::ID_TRACKS);

  // Build track renames
  for (const auto &issue : issues_) {
    if (issue.type == RefactorIssue::Type::GenericTrackName) {
      auto track = projectState_.getTrack(issue.trackId);
      if (track.isValid()) {
        juce::String oldName = track[ProjectState::PROP_NAME].toString();
        InstrumentCategory cat = classifyFromName(oldName);

        // Try to get more info from the first clip's audio
        auto clips = track.getChildWithName(ProjectState::ID_CLIPS);
        if (clips.isValid() && clips.getNumChildren() > 0) {
          auto clip = clips.getChild(0);
          if (clip.hasProperty(ProjectState::PROP_AUDIO_FILE)) {
            juce::File audioFile(
                clip[ProjectState::PROP_AUDIO_FILE].toString());
            InstrumentCategory fileCat =
                classifyFromName(audioFile.getFileNameWithoutExtension());
            if (fileCat != InstrumentCategory::Unknown)
              cat = fileCat;
          }
        }

        // Generate new name
        int number = ++categoryCounters_[cat];
        juce::String newName = generateTrackName(cat, number);

        RefactorPlan::TrackRename rename;
        rename.trackId = issue.trackId;
        rename.oldName = oldName;
        rename.newName = newName;
        rename.category = cat;
        currentPlan_.trackRenames.push_back(rename);
      }
    }
  }

  // Build color changes
  for (const auto &issue : issues_) {
    if (issue.type == RefactorIssue::Type::NoTrackColor ||
        issue.type == RefactorIssue::Type::GenericTrackName) {
      auto track = projectState_.getTrack(issue.trackId);
      if (track.isValid()) {
        juce::String name = track[ProjectState::PROP_NAME].toString();
        InstrumentCategory cat = classifyFromName(name);

        RefactorPlan::TrackColorChange colorChange;
        colorChange.trackId = issue.trackId;
        colorChange.newColor = getCategoryColor(cat);
        currentPlan_.colorChanges.push_back(colorChange);
      }
    }
  }

  // Build clip deletions
  for (const auto &issue : issues_) {
    if (issue.type == RefactorIssue::Type::MutedClip ||
        issue.type == RefactorIssue::Type::ClipOutOfBounds) {
      RefactorPlan::ClipDeletion deletion;
      deletion.trackId = issue.trackId;
      deletion.clipId = issue.clipId;
      deletion.reason = issue.description;
      currentPlan_.clipsToDelete.push_back(deletion);
    }
  }

  // Build sample consolidations
  juce::File projectDir = projectState_.getProjectFile().getParentDirectory();
  juce::File samplesDir = getProjectSamplesFolder();

  for (const auto &issue : issues_) {
    if (issue.type == RefactorIssue::Type::ScatteredSamples) {
      RefactorPlan::SampleConsolidation consolidation;
      consolidation.sourceFile = issue.externalFile;
      consolidation.destFile =
          samplesDir.getChildFile(issue.externalFile.getFileName());
      consolidation.clipId = issue.clipId;
      currentPlan_.samplesToConsolidate.push_back(consolidation);
    }
  }

  // Build track groups
  std::map<InstrumentCategory, std::vector<juce::String>> categoryTracks;
  for (const auto &rename : currentPlan_.trackRenames) {
    if (rename.category != InstrumentCategory::Unknown) {
      categoryTracks[rename.category].push_back(rename.trackId);
    }
  }

  // Create drum group if we have enough drum tracks
  std::vector<juce::String> drumTracks;
  for (auto cat : {InstrumentCategory::Kick, InstrumentCategory::Snare,
                   InstrumentCategory::HiHat, InstrumentCategory::Cymbal,
                   InstrumentCategory::Toms, InstrumentCategory::Percussion}) {
    for (const auto &trackId : categoryTracks[cat])
      drumTracks.push_back(trackId);
  }

  if (drumTracks.size() >= 3) {
    TrackGroupDef drumGroup("Drums", InstrumentCategory::Kick);
    drumGroup.trackIds = drumTracks;
    currentPlan_.groupsToCreate.push_back(drumGroup);
  }

  // Update summary stats
  currentPlan_.tracksToRename =
      static_cast<int>(currentPlan_.trackRenames.size());
  currentPlan_.tracksToColor =
      static_cast<int>(currentPlan_.colorChanges.size());
  currentPlan_.groupsToCreate_ =
      static_cast<int>(currentPlan_.groupsToCreate.size());
  currentPlan_.clipsToRemove =
      static_cast<int>(currentPlan_.clipsToDelete.size());
  currentPlan_.samplesToMove =
      static_cast<int>(currentPlan_.samplesToConsolidate.size());
}

//==============================================================================
// Execution
//==============================================================================

void ProjectRefactorerAgent::execute(
    const RefactorPlan &plan,
    std::function<void(const RefactorProgress &)> onProgress,
    std::function<void(bool success, const juce::String &message)> onComplete) {
  juce::MessageManager::callAsync([this, plan, onProgress, onComplete]() {
    try {
      RefactorProgress progress;
      progress.totalSteps = 4;

      // Step 1: Rename tracks
      progress.currentStep = 1;
      progress.currentOperation = "Renaming tracks...";
      progress.progressPercent = 10.0f;
      if (onProgress)
        onProgress(progress);

      executeTrackRenames(plan);

      // Step 2: Apply colors
      progress.currentStep = 2;
      progress.currentOperation = "Applying track colors...";
      progress.progressPercent = 30.0f;
      if (onProgress)
        onProgress(progress);

      executeColorChanges(plan);

      // Step 3: Delete clips
      progress.currentStep = 3;
      progress.currentOperation = "Removing dead clips...";
      progress.progressPercent = 60.0f;
      if (onProgress)
        onProgress(progress);

      executeClipDeletions(plan);

      // Step 4: Consolidate samples
      progress.currentStep = 4;
      progress.currentOperation = "Consolidating samples...";
      progress.progressPercent = 80.0f;
      if (onProgress)
        onProgress(progress);

      executeSampleConsolidation(plan);

      // Done!
      progress.currentOperation = "Refactoring complete!";
      progress.progressPercent = 100.0f;
      if (onProgress)
        onProgress(progress);

      if (onComplete) {
        juce::String summary = "Refactoring complete:\n"
                               "• " +
                               juce::String(plan.trackRenames.size()) +
                               " tracks renamed\n"
                               "• " +
                               juce::String(plan.colorChanges.size()) +
                               " tracks colored\n"
                               "• " +
                               juce::String(plan.clipsToDelete.size()) +
                               " clips removed\n"
                               "• " +
                               juce::String(plan.samplesToConsolidate.size()) +
                               " samples consolidated";

        onComplete(true, summary);
      }
    } catch (const std::exception &e) {
      if (onComplete)
        onComplete(false, juce::String("Error: ") + e.what());
    }
  });
}

void ProjectRefactorerAgent::executeTrackRenames(const RefactorPlan &plan) {
  for (const auto &rename : plan.trackRenames) {
    projectState_.renameTrack(rename.trackId, rename.newName,
                              "Refactor: Rename track");
    DBG("ProjectRefactorerAgent: Renamed '" + rename.oldName + "' to '" +
        rename.newName + "'");
  }
}

void ProjectRefactorerAgent::executeColorChanges(const RefactorPlan &plan) {
  for (const auto &colorChange : plan.colorChanges) {
    auto track = projectState_.getTrack(colorChange.trackId);
    if (track.isValid()) {
      track.setProperty(ProjectState::PROP_COLOR,
                        colorChange.newColor.toString(),
                        &projectState_.getUndoManager());
      DBG("ProjectRefactorerAgent: Set color for track " + colorChange.trackId);
    }
  }
}

void ProjectRefactorerAgent::executeClipDeletions(const RefactorPlan &plan) {
  for (const auto &deletion : plan.clipsToDelete) {
    projectState_.deleteClip(deletion.trackId, deletion.clipId,
                             "Refactor: Remove dead clip");
    DBG("ProjectRefactorerAgent: Deleted clip " + deletion.clipId + " (" +
        deletion.reason + ")");
  }
}

void ProjectRefactorerAgent::executeSampleConsolidation(
    const RefactorPlan &plan) {
  // Create samples folder if needed
  juce::File samplesDir = getProjectSamplesFolder();
  if (!samplesDir.exists())
    samplesDir.createDirectory();

  for (const auto &consolidation : plan.samplesToConsolidate) {
    if (consolidation.sourceFile.existsAsFile()) {
      // Copy file
      if (consolidation.sourceFile.copyFileTo(consolidation.destFile)) {
        // Update clip reference
        auto [trackTree, clipTree] =
            projectState_.findClip(consolidation.clipId);
        if (clipTree.isValid()) {
          clipTree.setProperty(ProjectState::PROP_AUDIO_FILE,
                               consolidation.destFile.getFullPathName(),
                               &projectState_.getUndoManager());
        }

        DBG("ProjectRefactorerAgent: Consolidated " +
            consolidation.sourceFile.getFileName());
      } else {
        DBG("ProjectRefactorerAgent: Failed to copy " +
            consolidation.sourceFile.getFileName());
      }
    }
  }
}

void ProjectRefactorerAgent::cancel() { cancelled_.store(true); }

//==============================================================================
// Classification Methods
//==============================================================================

InstrumentCategory ProjectRefactorerAgent::classifyFromAnalysis(
    const AudioAnalysisResults &results) {
  // Classification based on spectral characteristics
  double centroid = results.spectralCentroidHz;
  double dominantFreq = results.dominantFrequencyHz;
  double subBass = results.subBassDb;
  double bass = results.bassDb;
  double mids = results.midsDb;
  double highMids = results.highMidsDb;
  double presence = results.presenceDb;
  double brilliance = results.brillianceDb;

  // Kick detection: Strong sub-bass, short duration, low centroid
  if (dominantFreq < 100.0 && subBass > bass + 6.0 &&
      results.durationSeconds < 0.5)
    return InstrumentCategory::Kick;

  // Snare detection: Strong mid/high-mid content, short duration
  if (results.durationSeconds < 0.5 && highMids > mids + 3.0 && presence > mids)
    return InstrumentCategory::Snare;

  // Hi-hat detection: High centroid, short, bright
  if (centroid > 5000.0 && results.durationSeconds < 0.3 &&
      brilliance > mids + 6.0)
    return InstrumentCategory::HiHat;

  // Cymbal detection: Very high centroid, longer tail
  if (centroid > 4000.0 && results.durationSeconds > 0.5 &&
      brilliance > bass + 12.0)
    return InstrumentCategory::Cymbal;

  // Bass detection: Low centroid, strong bass content
  if (centroid < 300.0 && bass > mids + 6.0)
    return InstrumentCategory::Bass;

  // Pad detection: Long duration, even frequency distribution
  if (results.durationSeconds > 2.0 && std::abs(bass - mids) < 6.0 &&
      std::abs(mids - highMids) < 6.0)
    return InstrumentCategory::Pad;

  // Vocals detection: Mid-range focused, stereo often narrow
  if (centroid > 300.0 && centroid < 3000.0 && mids > bass + 3.0 &&
      mids > highMids + 3.0)
    return InstrumentCategory::Vocals;

  // Keys/Synth: Medium centroid, musical frequency range
  if (centroid > 500.0 && centroid < 4000.0)
    return InstrumentCategory::Synth;

  return InstrumentCategory::Unknown;
}

InstrumentCategory
ProjectRefactorerAgent::classifyFromName(const juce::String &name) {
  juce::String lower = name.toLowerCase();

  // Drum patterns
  if (lower.contains("kick") || lower.contains("bd ") || lower == "bd" ||
      lower.contains("bassdrum"))
    return InstrumentCategory::Kick;

  if (lower.contains("snare") || lower.contains("sd ") || lower == "sd" ||
      lower.contains("snr"))
    return InstrumentCategory::Snare;

  if (lower.contains("hihat") || lower.contains("hi-hat") ||
      lower.contains("hh ") || lower == "hh" || lower.contains("hat"))
    return InstrumentCategory::HiHat;

  if (lower.contains("cymbal") || lower.contains("crash") ||
      lower.contains("ride"))
    return InstrumentCategory::Cymbal;

  if (lower.contains("tom"))
    return InstrumentCategory::Toms;

  if (lower.contains("perc") || lower.contains("shaker") ||
      lower.contains("tamb") || lower.contains("conga") ||
      lower.contains("bongo") || lower.contains("clap"))
    return InstrumentCategory::Percussion;

  // Bass patterns
  if (lower.contains("bass") || lower.contains("808") || lower.contains("sub"))
    return InstrumentCategory::Bass;

  // Melodic patterns
  if (lower.contains("guitar") || lower.contains("gtr"))
    return InstrumentCategory::Guitar;

  if (lower.contains("piano") || lower.contains("keys") ||
      lower.contains("rhodes") || lower.contains("organ") ||
      lower.contains("wurli"))
    return InstrumentCategory::Keys;

  if (lower.contains("synth") || lower.contains("lead") ||
      lower.contains("arp")) {
    if (lower.contains("lead"))
      return InstrumentCategory::Lead;
    return InstrumentCategory::Synth;
  }

  if (lower.contains("pad") || lower.contains("ambient") ||
      lower.contains("atmosphere"))
    return InstrumentCategory::Pad;

  // Vocals
  if (lower.contains("vocal") || lower.contains("vox") ||
      lower.contains("voice") || lower.contains("sing") ||
      lower.contains("choir"))
    return InstrumentCategory::Vocals;

  // Orchestral
  if (lower.contains("string") || lower.contains("violin") ||
      lower.contains("cello") || lower.contains("viola"))
    return InstrumentCategory::Strings;

  if (lower.contains("brass") || lower.contains("trumpet") ||
      lower.contains("trombone") || lower.contains("horn"))
    return InstrumentCategory::Brass;

  if (lower.contains("flute") || lower.contains("clarinet") ||
      lower.contains("oboe") || lower.contains("sax"))
    return InstrumentCategory::Woodwinds;

  // Effects
  if (lower.contains("fx") || lower.contains("sfx") ||
      lower.contains("effect") || lower.contains("riser") ||
      lower.contains("impact") || lower.contains("sweep"))
    return InstrumentCategory::FX;

  return InstrumentCategory::Unknown;
}

juce::String
ProjectRefactorerAgent::generateTrackName(InstrumentCategory category,
                                          int number) {
  juce::String baseName = getCategoryName(category);

  if (number > 1 || category == InstrumentCategory::Unknown)
    return baseName + " " + juce::String(number);

  return baseName;
}

bool ProjectRefactorerAgent::isGenericName(const juce::String &name) {
  juce::String lower = name.toLowerCase().trim();

  // Common generic patterns
  static const std::regex genericPatterns[] = {
      std::regex("^audio\\s*\\d*$"),
      std::regex("^track\\s*\\d*$"),
      std::regex("^midi\\s*\\d*$"),
      std::regex("^instrument\\s*\\d*$"),
      std::regex("^channel\\s*\\d*$"),
      std::regex("^recording\\s*\\d*$"),
      std::regex("^untitled\\s*\\d*$"),
      std::regex("^new\\s*(audio|midi|track)?\\s*\\d*$"),
  };

  std::string stdName = lower.toStdString();

  for (const auto &pattern : genericPatterns) {
    if (std::regex_match(stdName, pattern))
      return true;
  }

  return false;
}

juce::File ProjectRefactorerAgent::getProjectSamplesFolder() {
  juce::File projectDir = projectState_.getProjectFile().getParentDirectory();

  if (!projectDir.exists())
    projectDir = juce::File::getCurrentWorkingDirectory();

  return projectDir.getChildFile("Samples");
}

} // namespace ai
} // namespace zenith
