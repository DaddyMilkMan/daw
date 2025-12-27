/**
 * @file ProjectStateTests.cpp
 * @brief Unit tests for ProjectState and ValueTree operations
 * @author Testing Team - Operation Polish Phase 2
 *
 * Priority: ProjectState tests (second priority as requested)
 * Tests data integrity, undo/redo, and state mutations
 */

#include "ProjectState.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>


namespace zenith {
namespace tests {

/**
 * @class ValueTreeIntegrityTests
 * @brief Tests for ValueTree data integrity
 */
class ValueTreeIntegrityTests : public juce::UnitTest {
public:
  ValueTreeIntegrityTests()
      : juce::UnitTest("ValueTree Integrity", "ProjectState") {}

  void runTest() override {
    beginTest("ValueTree creation");
    {
      juce::ValueTree tree("TestTree");
      expect(tree.isValid());
      expect(tree.getType().toString() == "TestTree");
    }

    beginTest("Property get/set");
    {
      juce::ValueTree tree("Test");
      tree.setProperty("name", "TestName", nullptr);
      tree.setProperty("value", 42, nullptr);
      tree.setProperty("enabled", true, nullptr);

      expect(tree.getProperty("name").toString() == "TestName");
      expect((int)tree.getProperty("value") == 42);
      expect((bool)tree.getProperty("enabled") == true);
    }

    beginTest("Child add/remove");
    {
      juce::ValueTree parent("Parent");
      juce::ValueTree child1("Child1");
      juce::ValueTree child2("Child2");

      parent.appendChild(child1, nullptr);
      parent.appendChild(child2, nullptr);

      expect(parent.getNumChildren() == 2);
      expect(parent.getChild(0).getType().toString() == "Child1");
      expect(parent.getChild(1).getType().toString() == "Child2");

      parent.removeChild(0, nullptr);
      expect(parent.getNumChildren() == 1);
      expect(parent.getChild(0).getType().toString() == "Child2");
    }
  }
};

/**
 * @class UndoRedoTests
 * @brief Tests for undo/redo functionality
 */
class UndoRedoTests : public juce::UnitTest {
public:
  UndoRedoTests() : juce::UnitTest("Undo/Redo", "ProjectState") {}

  void runTest() override {
    beginTest("Simple undo/redo");
    {
      juce::UndoManager undoManager;
      juce::ValueTree tree("Test");

      undoManager.beginNewTransaction();
      tree.setProperty("value", 10, &undoManager);
      expect((int)tree.getProperty("value") == 10);

      undoManager.beginNewTransaction();
      tree.setProperty("value", 20, &undoManager);
      expect((int)tree.getProperty("value") == 20);

      undoManager.undo();
      expect((int)tree.getProperty("value") == 10);

      undoManager.redo();
      expect((int)tree.getProperty("value") == 20);
    }

    beginTest("Multiple undos");
    {
      juce::UndoManager undoManager;
      juce::ValueTree tree("Test");

      for (int i = 0; i < 5; ++i) {
        undoManager.beginNewTransaction();
        tree.setProperty("counter", i, &undoManager);
      }

      expect((int)tree.getProperty("counter") == 4);

      undoManager.undo(); // 3
      undoManager.undo(); // 2
      undoManager.undo(); // 1

      expect((int)tree.getProperty("counter") == 1);
    }

    beginTest("Undo with child operations");
    {
      juce::UndoManager undoManager;
      juce::ValueTree parent("Parent");

      juce::ValueTree child("Child");
      parent.appendChild(child, &undoManager);
      expect(parent.getNumChildren() == 1);

      undoManager.undo();
      expect(parent.getNumChildren() == 0);

      undoManager.redo();
      expect(parent.getNumChildren() == 1);
    }
  }
};

/**
 * @class TrackManipulationTests
 * @brief Tests for track add/remove/reorder
 */
class TrackManipulationTests : public juce::UnitTest {
public:
  TrackManipulationTests()
      : juce::UnitTest("Track Manipulation", "ProjectState") {}

  void runTest() override {
    beginTest("Add track");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio 1", "audio");
      expect(projectState.getNumTracks() == 1);
      expect(projectState.getTrackName(trackId) == "Audio 1");
      expect(projectState.getTrackType(trackId) == "audio");
    }

    beginTest("Remove track");
    {
      ProjectState projectState;
      auto t1 = projectState.addTrack("T1", "audio");
      auto t2 = projectState.addTrack("T2", "midi");
      expect(projectState.getNumTracks() == 2);
      
      projectState.removeTrack(t1);
      expect(projectState.getNumTracks() == 1);
      expect(projectState.getTrackByIndex(0).getProperty(ProjectState::PROP_NAME).toString() == "T2");
    }

    beginTest("Reorder tracks");
    {
      ProjectState projectState;
      auto t1 = projectState.addTrack("T1", "audio");
      auto t2 = projectState.addTrack("T2", "midi");
      
      // Move T1 (index 0) to position 1
      projectState.moveTrack(t1, 1);
      
      expectEquals(projectState.getTrackByIndex(1).getProperty(ProjectState::PROP_ID).toString(), t1);
      expectEquals(projectState.getTrackByIndex(0).getProperty(ProjectState::PROP_ID).toString(), t2);
    }
  }
};

/**
 * @class ClipManipulationTests
 * @brief Tests for clip creation and manipulation
 */
class ClipManipulationTests : public juce::UnitTest {
public:
  ClipManipulationTests()
      : juce::UnitTest("Clip Manipulation", "ProjectState") {}

  void runTest() override {
    beginTest("Add clip to track");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio", "audio");
      auto clipId = projectState.addClip(trackId, 0.0, 4.0, "Clip Add");
      
      expect(clipId.isNotEmpty());
      expect(projectState.getClip(trackId, clipId).isValid());
    }

    beginTest("Move clip position");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio", "audio");
      auto clipId = projectState.addClip(trackId, 0.0, 4.0, "Clip Move");
      
      projectState.setClipRange(clipId, 4.0, 4.0, "Move");
      auto clip = projectState.getClip(trackId, clipId);
      expect((double)clip.getProperty(ProjectState::PROP_START_BEATS) == 4.0);
    }

    beginTest("Delete clip");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio", "audio");
      auto clipId = projectState.addClip(trackId, 0.0, 4.0, "Clip Delete");
      
      projectState.deleteClip(trackId, clipId, "Delete");
      expect(!projectState.getClip(trackId, clipId).isValid());
      
      projectState.undo();
      expect(projectState.getClip(trackId, clipId).isValid());
    }

    beginTest("Split clip");
    {
      ProjectState projectState;
      // Set SampleRate to 48000 to match test assumption
      projectState.getState().setProperty(ProjectState::PROP_SAMPLE_RATE, 48000.0, nullptr);
      
      auto trackId = projectState.addTrack("Audio", "audio");
      auto clipId = projectState.addClip(trackId, 0.0, 4.0, "Clip Split");
      
      // Assuming 48000 Hz and 2 beats per second (120 BPM)
      // 1 beat = 24000 samples. Split at 1 beat = 24000 samples.
      projectState.splitClip(trackId, clipId, 24000, "Split");
      
      auto track = projectState.getTrack(trackId);
      auto clips = track.getChildWithName(ProjectState::ID_CLIPS);
      expectEquals(clips.getNumChildren(), 2, "Splitting should result in two clips");
      
      auto c1 = clips.getChild(0);
      auto c2 = clips.getChild(1);
      
      expectEquals((double)c1.getProperty(ProjectState::PROP_LENGTH_BEATS), 1.0, "First clip should be 1 beat long");
      expectEquals((double)c2.getProperty(ProjectState::PROP_START_BEATS), 1.0, "Second clip should start at 1 beat");
      expectEquals((double)c2.getProperty(ProjectState::PROP_LENGTH_BEATS), 3.0, "Second clip should be 3 beats long");
    }
  }
};

/**
 * @class AutomationDataTests
 * @brief Tests for automation envelope data
 */
class AutomationDataTests : public juce::UnitTest {
public:
  AutomationDataTests() : juce::UnitTest("Automation Data", "ProjectState") {}

  void runTest() override {
    beginTest("Add automation point");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio", "audio");
      
      // Add point: Vol parameter, time 2.0, value 0.5
      auto pointId = projectState.addAutomationPoint(trackId, "volume", 2.0, 0.5, "Add Point");
      
      expect(pointId.isNotEmpty());
      
      auto envelope = projectState.getAutomationEnvelope(trackId, "volume");
      expect(envelope.isValid());
      auto points = envelope.getChildWithName(ProjectState::ID_POINTS);
      expect(points.isValid());
      expect(points.getNumChildren() == 1);
      
      auto point = points.getChild(0);
      expectEquals((double)point.getProperty(ProjectState::PROP_TIME_BEATS), 2.0);
      expectEquals((double)point.getProperty(ProjectState::PROP_VALUE), 0.5);
    }

    beginTest("Move automation point");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio", "audio");
      auto pointId = projectState.addAutomationPoint(trackId, "volume", 2.0, 0.5, "Add Point");
      
      bool moved = projectState.moveAutomationPoint(trackId, "volume", pointId, 3.0, 0.8, "Move Point");
      expect(moved);
      
      auto envelope = projectState.getAutomationEnvelope(trackId, "volume");
      auto points = envelope.getChildWithName(ProjectState::ID_POINTS);
      auto point = points.getChild(0);
      
      expectEquals((double)point.getProperty(ProjectState::PROP_TIME_BEATS), 3.0); // Moved
      expectEquals((double)point.getProperty(ProjectState::PROP_VALUE), 0.8);      // Changed value
    }

    beginTest("Delete automation point");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio", "audio");
      auto pointId = projectState.addAutomationPoint(trackId, "volume", 2.0, 0.5, "Add Point");
      
      bool deleted = projectState.deleteAutomationPoint(trackId, "volume", pointId, "Delete Point");
      expect(deleted);
      
      auto envelope = projectState.getAutomationEnvelope(trackId, "volume");
      auto points = envelope.getChildWithName(ProjectState::ID_POINTS);
      expect(points.getNumChildren() == 0);
      
      projectState.undo();
      envelope = projectState.getAutomationEnvelope(trackId, "volume");
      points = envelope.getChildWithName(ProjectState::ID_POINTS);
      expect(points.getNumChildren() == 1);
    }

    beginTest("Automation envelope interpolation");
    {
      // Note: Interpolation logic usually lives in AutomationLane or similar, 
      // not ProjectState directly which is just data storage.
      // However, we can verifying sorting/ordering if applicable.
      // For now, let's verify multiple points are stored correctly.
      
      ProjectState projectState;
      auto trackId = projectState.addTrack("Audio", "audio");
      
      projectState.addAutomationPoint(trackId, "volume", 0.0, 0.0, "P1");
      projectState.addAutomationPoint(trackId, "volume", 4.0, 1.0, "P2");
      
      auto envelope = projectState.getAutomationEnvelope(trackId, "volume");
      auto points = envelope.getChildWithName(ProjectState::ID_POINTS);
      // Children might not be sorted by ProjectState itself unless the manager enforces it.
      // Let's just check count.
      expect(points.getNumChildren() == 2);
    }
  }
};

/**
 * @class ProjectSerializationTests
 * @brief Tests for project save/load
 */
class ProjectSerializationTests : public juce::UnitTest {
public:
  ProjectSerializationTests()
      : juce::UnitTest("Project Serialization", "ProjectState") {}

  void runTest() override {
    beginTest("Save project to XML");
    {
      ProjectState projectState;
      projectState.setProjectName("TestProject");
      projectState.setTempo(140.0);

      auto xml = projectState.getState().createXml();
      expect(xml != nullptr);
      expect(xml->getTagName() == "PROJECT");
      expect(xml->getStringAttribute("name") == "TestProject");
    }

    beginTest("Binary write/read");
    {
      ProjectState projectState;
      projectState.addTrack("T1", "audio");
      
      juce::MemoryBlock mb;
      juce::MemoryOutputStream mo(mb, false);
      projectState.getState().writeToStream(mo);
      
      juce::MemoryInputStream mi(mb, false);
      auto loadedTree = juce::ValueTree::readFromStream(mi);
      
      expect(loadedTree.isValid());
      expect(loadedTree.getChildWithName(ProjectState::ID_TRACKS).getNumChildren() == 1);
    }
  }
};

// Register all tests
static ValueTreeIntegrityTests valueTreeIntegrityTests;
static UndoRedoTests undoRedoTests;
static TrackManipulationTests trackManipulationTests;
static ClipManipulationTests clipManipulationTests;
static AutomationDataTests automationDataTests;
static ProjectSerializationTests projectSerializationTests;

/**
 * @class PianoRollFeaturesTests
 * @brief Tests for new Piano Roll features: Tension, Humanize, Legato
 */
class PianoRollFeaturesTests : public juce::UnitTest {
public:
  PianoRollFeaturesTests()
      : juce::UnitTest("Piano Roll Features", "ProjectState") {}

  void runTest() override {
    beginTest("Tension Storage");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("MIDI", "midi");
      auto clipId = projectState.createEmptyClip(trackId, 0.0, 4.0, true, "Clip", "Add MIDI Clip");
      
      // Add note with tension using MidiNoteSpec
      ProjectState::MidiNoteSpec note;
      note.pitch = 60;
      note.velocity = 100;
      note.startBeats = 0.0;
      note.lengthBeats = 1.0;
      note.tension = 0.5f;
      
      auto noteId = projectState.addMidiNote(clipId, note, "Add Tension Note");
      
      auto notes = projectState.getMidiNotesForClip(clipId);
      expect(notes.size() == 1);
      expectEquals(notes[0].tension, 0.5f);
    }

    beginTest("Humanize Logic");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("MIDI", "midi");
      auto clipId = projectState.createEmptyClip(trackId, 0.0, 4.0, true, "Clip", "Add MIDI Clip");
      
      // Add exact notes
      projectState.addNote(clipId, 0.0, 1.0, 60, 100, "Note 1");
      projectState.addNote(clipId, 1.0, 1.0, 62, 100, "Note 2");
      
      // Apply humanize (amount 0.1, verify some change happened)
      projectState.humanizeClip(clipId, 0.1f, 0.1f);
      
      auto notes = projectState.getMidiNotesForClip(clipId);
      expect(notes.size() == 2);
      
      // Verify values shifted slightly but not too much
      // Note: Randomness is hard to test deterministically without mocking RNG,
      // but we can check bounds.
      bool velocityChanged = (notes[0].velocity != 100) || (notes[1].velocity != 100);
      bool timingChanged = (notes[0].startBeats != 0.0) || (notes[1].startBeats != 1.0);
      
      // Small chance they picked 0 shift, but unlikely for both
      if (!velocityChanged && !timingChanged) {
         // Try again with stronger setting if it failed to shift (unlikely)
         projectState.humanizeClip(clipId, 0.5f, 0.5f);
         notes = projectState.getMidiNotesForClip(clipId);
         velocityChanged = (notes[0].velocity != 100);
         timingChanged = (notes[0].startBeats != 0.0);
      }
      
      expect(velocityChanged || timingChanged, "Humanize should alter velocity or timing");
    }

    beginTest("Legato Logic");
    {
      ProjectState projectState;
      auto trackId = projectState.addTrack("MIDI", "midi");
      auto clipId = projectState.createEmptyClip(trackId, 0.0, 4.0, true, "Clip", "Add MIDI Clip");
      
      // Add gap notes
      // Note 1: 0.0 - 0.5 (Gap 0.5 to next note)
      // Note 2: 1.0 - 1.5 
      projectState.addNote(clipId, 0.0, 0.5, 60, 100, "Note 1");
      projectState.addNote(clipId, 1.0, 0.5, 62, 100, "Note 2");
      
      projectState.legatoClip(clipId);
      
      auto notes = projectState.getMidiNotesForClip(clipId);
      expect(notes.size() == 2);
      
      // Note 1 length should now be 1.0 (extending to Note 2 start)
      expectWithinAbsoluteError(notes[0].lengthBeats, 1.0, 0.001);
    }
  }
};

static PianoRollFeaturesTests pianoRollFeaturesTests;


} // namespace tests
} // namespace zenith
