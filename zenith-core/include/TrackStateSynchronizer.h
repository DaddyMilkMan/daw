/**
 * @file TrackStateSynchronizer.h
 * @brief Synchronizes ProjectState track properties to Engine track objects
 *
 * Phase 11: Mixer → Engine Wiring
 *
 * This class listens to ValueTree changes in ProjectState and updates
 * the corresponding Engine track objects. This keeps the engine in sync
 * with the authoritative ProjectState while maintaining proper threading:
 *
 * - All ValueTree changes happen on MESSAGE THREAD (enforced by ProjectState)
 * - All Engine track updates happen on MESSAGE THREAD (this class enforces)
 * - Engine Track objects use atomics, so audio thread can safely read
 *
 * Thread Safety:
 * - This class must ONLY be used from the MESSAGE THREAD
 * - ValueTree::Listener callbacks run on the thread that modified the tree
 * - Since ProjectState APIs enforce message thread, we're safe here
 */

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


//==============================================================================
/**
 * @class TrackStateSynchronizer
 * @brief Binds ProjectState track properties to Engine track objects
 *
 * Usage:
 * ```cpp
 * auto synchronizer = std::make_unique<TrackStateSynchronizer>(projectState,
 * engine); synchronizer->initialize();  // Start listening
 * ```
 *
 * The synchronizer will:
 * 1. Listen to TRACKS node for track add/remove
 * 2. Listen to each TRACK node for property changes
 * (volume/pan/mute/solo/armed)
 * 3. Update Engine track objects when properties change
 * 4. Maintain index mapping between ProjectState tracks and Engine tracks
 */
class TrackStateSynchronizer : private juce::ValueTree::Listener {
public:
  //==========================================================================
  /**
   * @brief Constructor
   * @param projectState Reference to ProjectState (must outlive this object)
   * @param engine Reference to Engine (must outlive this object)
   */
  TrackStateSynchronizer(ProjectState &projectState, Engine &engine);

  /**
   * @brief Destructor
   */
  ~TrackStateSynchronizer() override;

  //==========================================================================
  /**
   * @brief Initialize synchronizer and start listening to ProjectState
   * @note Call this after both ProjectState and Engine are fully initialized
   */
  void initialize();

  /**
   * @brief Shutdown synchronizer and stop listening
   */
  void shutdown();

  /**
   * @brief Force full sync from ProjectState to Engine
   * @note Useful after loading a project or when tracks are added externally
   */
  void syncAll();

private:
  //==========================================================================
  // ValueTree::Listener interface
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override;

  //==========================================================================
  // Helper Methods
  //==========================================================================

  /**
   * @brief Sync a single track property to engine
   * @param track Track ValueTree node
   * @param property Property identifier
   */
  void syncTrackProperty(const juce::ValueTree &track,
                         const juce::Identifier &property);

  /**
   * @brief Get engine track index for a ProjectState track
   * @param track Track ValueTree node
   * @return Engine track index, or -1 if not found
   */
  int getEngineTrackIndex(const juce::ValueTree &track) const;

  /**
   * @brief Add track listeners to a track node
   */
  void addTrackListener(const juce::ValueTree &track);

  /**
   * @brief Remove track listeners from a track node
   */
  void removeTrackListener(const juce::ValueTree &track);

  //==========================================================================
  // Member Variables
  //==========================================================================

  ProjectState &projectState;
  Engine &engine;

  bool initialized{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackStateSynchronizer)
};
