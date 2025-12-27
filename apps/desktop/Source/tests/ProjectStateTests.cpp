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
      
      // ValueTree reordering is usually handled by the manager internal logic.
      // Assuming TrackStateManager::moveTrack is implemented (it is in the header)
      // projectState.moveTrack(t1, 1);
      // expect(projectState.getTrackByIndex(1).getProperty(ProjectState::PROP_ID).toString() == t1);
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
      // Add point at time 2.0, value 0.5
      // Verify point exists
    }

    beginTest("Move automation point");
    {
      // Add point
      // Move it to new time/value
      // Verify updated
    }

    beginTest("Delete automation point");
    {
      // Add point
      // Delete it
      // Verify gone
      // Verify undo works
    }

    beginTest("Automation envelope interpolation");
    {
      // Add points at t=0 (v=0) and t=4 (v=1)
      // Sample at t=2
      // Expect interpolated value around 0.5
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

} // namespace tests
} // namespace zenith
