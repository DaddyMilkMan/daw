/**
 * @file GraphStressTest.cpp
 * @brief Stress tests for the Topological Audio Engine
 * @author Agent Antigravity
 */

#include "engine/RoutingGraph.h"
#include "engine/ZenithLogger.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

class GraphStressTest : public juce::UnitTest {
public:
  GraphStressTest()
      : juce::UnitTest("Graph Stress Test", "TopologicalEngine") {}

  void runTest() override {
    testInfiniteLoop();
    testMegaSplit();
    testGhostIsland();
  }

  void testInfiniteLoop() {
    beginTest("The Infinite Loop (Cycle Detection)");

    // Suppress expected warning
    zenith::ZenithLogger::getInstance().setLogToConsole(false);

    zenith::RoutingGraph graph;

    // Create Nodes
    zenith::RoutingGraph::Node nodeA;
    nodeA.id = "Track-A";
    nodeA.type = zenith::RoutingGraph::NodeType::Track;
    nodeA.name = "Track A";

    zenith::RoutingGraph::Node nodeB;
    nodeB.id = "Track-B";
    nodeB.type = zenith::RoutingGraph::NodeType::Track;
    nodeB.name = "Track B";

    graph.addNode(nodeA);
    graph.addNode(nodeB);

    // Create Cycle: A -> B -> A
    graph.connect("Track-A", "Track-B");
    graph.connect("Track-B", "Track-A");

    // Restore logging
    zenith::ZenithLogger::getInstance().setLogToConsole(true);

    // Assertions
    auto order = graph.getProcessingOrder();

    expect(order.size() == 2,
           "Processing order should contain 2 nodes despite cycle");

    bool containsA = false;
    bool containsB = false;
    for (const auto &id : order) {
      if (id == "Track-A")
        containsA = true;
      if (id == "Track-B")
        containsB = true;
    }

    expect(containsA, "Order must contain Track-A");
    expect(containsB, "Order must contain Track-B");

    logMessage("[PASS] Infinite Loop handled gracefully.");
  }

  void testMegaSplit() {
    beginTest("The Mega-Split (Performance)");

    zenith::RoutingGraph graph;

    // Source
    zenith::RoutingGraph::Node src;
    src.id = "Source";
    src.type = zenith::RoutingGraph::NodeType::Track;
    graph.addNode(src);

    int numDests = 100;
    for (int i = 0; i < numDests; ++i) {
      zenith::RoutingGraph::Node dst;
      dst.id = "Dest-" + juce::String(i);
      dst.type = zenith::RoutingGraph::NodeType::Track;
      graph.addNode(dst);

      // Route
      graph.connect("Source", dst.id);
    }

    // Measure time to update snapshot (which triggers sort)
    // We add one trigger node to force a graph update
    double startTime = juce::Time::getMillisecondCounterHiRes();

    zenith::RoutingGraph::Node trigger;
    trigger.id = "Trigger";
    trigger.type = zenith::RoutingGraph::NodeType::Track;
    graph.addNode(trigger);
    graph.connect("Source", "Trigger");

    double endTime = juce::Time::getMillisecondCounterHiRes();
    double duration = endTime - startTime;

    expect(duration < 10.0,
           "Graph update took too long: " + juce::String(duration) + "ms");

    auto order = graph.getProcessingOrder();
    expect(order.size() == static_cast<size_t>(numDests + 2),
           "Incorrect node count");

    // Check Source is processed BEFORE destinations
    // Find index of Source
    int srcIdx = -1;
    for (size_t i = 0; i < order.size(); ++i) {
      if (order[i] == "Source") {
        srcIdx = (int)i;
        break;
      }
    }

    // All destinations must be AFTER source
    for (int i = 0; i < numDests; ++i) {
      juce::String id = "Dest-" + juce::String(i);
      int dstIdx = -1;
      for (size_t k = 0; k < order.size(); ++k) {
        if (order[k] == id) {
          dstIdx = (int)k;
          break;
        }
      }
      expect(dstIdx > srcIdx, "Destination " + id + " processed before Source");
    }

    logMessage("[PASS] Mega-Split benchmark: " + juce::String(duration) + "ms");
  }

  void testGhostIsland() {
    beginTest("The Ghost Island (Disconnected Components)");

    zenith::RoutingGraph graph;

    zenith::RoutingGraph::Node nodeA;
    nodeA.id = "Track-A";
    nodeA.type = zenith::RoutingGraph::NodeType::Track;
    graph.addNode(nodeA);

    zenith::RoutingGraph::Node master;
    master.id = "Master";
    master.type = zenith::RoutingGraph::NodeType::Master;
    graph.addNode(master);

    zenith::RoutingGraph::Node island;
    island.id = "Island-Track";
    island.type = zenith::RoutingGraph::NodeType::Track;
    graph.addNode(island);

    graph.connect("Track-A", "Master");

    auto order = graph.getProcessingOrder();

    bool foundIsland = false;
    for (const auto &id : order) {
      if (id == "Island-Track")
        foundIsland = true;
    }

    expect(foundIsland,
           "Disconnected 'Island-Track' missing from processing order");
    expect(order.size() == 3, "Order size mismatch");

    logMessage("[PASS] Ghost Island recovered.");
  }
};

static GraphStressTest graphStressTest;

} // namespace tests
} // namespace zenith
