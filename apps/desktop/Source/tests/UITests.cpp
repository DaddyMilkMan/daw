/*
  ==============================================================================

    UITests.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

    Unit tests for UI layout and rendering infrastructure.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../ui/skia/ZenithLayout.h"
#include "../ui/skia/RenderTree.h"

class UITests : public juce::UnitTest {
public:
    UITests() : juce::UnitTest("UITests") {}

    void runTest() override {
        beginTest("ZenithLayout Grid");
        {
            std::vector<juce::Component> comps(4);
            std::vector<juce::Component*> ptrs;
            for(auto& c : comps) ptrs.push_back(&c);
            
            juce::Rectangle<int> bounds(0, 0, 200, 200);
            // 2x2 grid, 200x200 total. 
            // Gap 0.
            // Each item should be 100x100.
            zenith::ZenithLayout::grid(bounds, ptrs, 2, 0.0f, 0.0f);
            
            expectEquals(comps[0].getWidth(), 100);
            expectEquals(comps[0].getHeight(), 100);
            expectEquals(comps[0].getX(), 0);
            expectEquals(comps[0].getY(), 0);
            
            expectEquals(comps[3].getX(), 100);
            expectEquals(comps[3].getY(), 100);
        }
        
        beginTest("FrameBufferSwap Lock-Free");
        {
            zenith::render::FrameBufferSwap swap;
            
            // Initial state
            expect(!swap.hasNewFrame());
            
            // Write frame
            auto* write = swap.getWriteBuffer();
            write->frameNumber = 1;
            swap.swapWriteToReady();
            
            // Should have frame
            expect(swap.hasNewFrame());
            
            // Read frame
            auto* read = swap.getLatestFrame();
            expectEquals((int)read->frameNumber, 1);
            
            // Should not have new frame immediately
            expect(!swap.hasNewFrame());
            
            // Write another frame
            auto* write2 = swap.getWriteBuffer();
            write2->frameNumber = 2;
            swap.swapWriteToReady();
            
            expect(swap.hasNewFrame());
            auto* read2 = swap.getLatestFrame();
            expectEquals((int)read2->frameNumber, 2);
        }
    }
};

static UITests uiTests;
