/*
  ==============================================================================

    RoutingGraphTest.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../engine/RoutingGraph.h"

namespace zenith {

class RoutingGraphTest : public juce::UnitTest {
public:
  RoutingGraphTest() : juce::UnitTest("Feedback Loop Stability", "RoutingGraph") {}

  void runTest() override {
    beginTest("Simple DAG");
    {
      RoutingGraph graph;
      RoutingGraph::Node a{"A", RoutingGraph::NodeType::Track, "Track A"};
      RoutingGraph::Node b{"B", RoutingGraph::NodeType::Bus, "Aux B"};
      
      graph.addNode(a);
      graph.addNode(b);
      graph.connect("A", "B");

      // Verify connections
      auto processingOrder = graph.getProcessingOrder();
      
      expectEquals((int)processingOrder.size(), 2);
      expect(processingOrder[0] == "A");
      expect(processingOrder[1] == "B");

      auto conns = graph.getConnectionsFrom("A");
      expectEquals((int)conns.size(), 1);
      expect(!conns[0].isFeedback);
    }

    beginTest("Simple Cycle (A <-> B)");
    {
      RoutingGraph graph;
      RoutingGraph::Node a{"A", RoutingGraph::NodeType::Track, "Track A"};
      RoutingGraph::Node b{"B", RoutingGraph::NodeType::Bus, "Aux B"};
      
      graph.addNode(a);
      graph.addNode(b);
      
      // A -> B -> A
      graph.connect("A", "B");
      graph.connect("B", "A");

      auto processingOrder = graph.getProcessingOrder();
      
      expectEquals((int)processingOrder.size(), 2);
      // Deterministic order should be preserved (A before B because A < B stringly?)
      // Actually because we broke the cycle.
      // If A->B is forward, B->A is feedback.
      // DFS explores Sorted IDs: A then B.
      // Visit A -> Gray.
      // Visit B -> Gray.
      // B sees neighbor A (Gray) -> B->A is BACK EDGE.
      // So B->A is flagged feedback.
      // Use efficient topological sort on A->B.
      // Order: A, B.
      
      expect(processingOrder[0] == "A");
      expect(processingOrder[1] == "B");

      // Verify B->A is marked feedback
      auto connsFromB = graph.getConnectionsFrom("B"); // B->A
      bool foundFeedback = false;
      for (const auto& c : connsFromB) {
          if (c.destId == "A" && c.isFeedback) foundFeedback = true;
      }
      expect(foundFeedback, "B->A should be marked as feedback");

      auto connsFromA = graph.getConnectionsFrom("A"); // A->B
      bool foundForward = false;
      for (const auto& c : connsFromA) {
          if (c.destId == "B" && !c.isFeedback) foundForward = true;
      }
      expect(foundForward, "A->B should be forward connection");
    }
    
    beginTest("Complex Cycle (A -> B -> C -> B)");
    {
      RoutingGraph graph;
      RoutingGraph::Node a{"A", RoutingGraph::NodeType::Track, "Track A"};
      RoutingGraph::Node b{"B", RoutingGraph::NodeType::Bus, "Aux B"};
      RoutingGraph::Node c{"C", RoutingGraph::NodeType::Bus, "Aux C"};
      
      graph.addNode(a);
      graph.addNode(b);
      graph.addNode(c);

      graph.connect("A", "B");
      graph.connect("B", "C");
      graph.connect("C", "B"); // Cycle here

      auto processingOrder = graph.getProcessingOrder();
      
      expectEquals((int)processingOrder.size(), 3);
      // Order should be A, B, C (assuming C->B is feedback)
      // DFS: Visit A. -> B -> C.
      // C sees B (Gray). C->B is Back Edge.
      
      expect(processingOrder[0] == "A");
      expect(processingOrder[1] == "B");
      expect(processingOrder[2] == "C");

      // Verify C->B is feedback
      auto connsFromC = graph.getConnectionsFrom("C");
      bool foundFeedback = false;
      for(const auto& conn : connsFromC) {
          if (conn.destId == "B" && conn.isFeedback) foundFeedback = true;
      }
      expect(foundFeedback, "C->B should be feedback");
    }
    
    beginTest("Self Loop (A -> A)");
    {
      RoutingGraph graph;
      RoutingGraph::Node a{"A", RoutingGraph::NodeType::Track, "Track A"};
      graph.addNode(a);
      graph.connect("A", "A");
      
      auto processingOrder = graph.getProcessingOrder();
      expectEquals((int)processingOrder.size(), 1);
      
      auto conns = graph.getConnectionsFrom("A"); // A->A
      expectEquals((int)conns.size(), 1);
      expect(conns[0].isFeedback, "Self-loop should be marked as feedback");
    }
  }
};

static RoutingGraphTest routingGraphTest;

} // namespace zenith
