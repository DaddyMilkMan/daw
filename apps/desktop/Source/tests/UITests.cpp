/*
  ==============================================================================

    UITests.cpp
    Created: 2025-12-03
    Author:  Zenith DAW

    Unit tests for UI layout and rendering infrastructure.

  ==============================================================================
*/

#include "../ui/design-system/ZenithLayout.h"
#include "../ui/dashboards/MetricsChart.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>


// RenderTree.h depends on Skia types (SkRect, SkColor), only include when Skia
// enabled
#include "../ui/framework/RenderTree.h"


class UITests : public juce::UnitTest {
public:
  UITests() : juce::UnitTest("UITests") {}

  void runTest() override {
    beginTest("ZenithLayout Grid");
    {
      std::vector<juce::Component> comps(4);
      std::vector<juce::Component *> ptrs;
      for (auto &c : comps)
        ptrs.push_back(&c);

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

#ifdef ZENITH_USE_SKIA
    beginTest("FrameBufferSwap Lock-Free");
    {
      zenith::render::FrameBufferSwap swap;

      // Initial state
      expect(!swap.hasNewFrame());

      // Write frame
      auto *write = swap.getWriteBuffer();
      write->frameNumber = 1;
      swap.swapWriteToReady();

      // Should have frame
      expect(swap.hasNewFrame());

      // Read frame
      auto *read = swap.getLatestFrame();
      expectEquals((int)read->frameNumber, 1);

      // Should not have new frame immediately
      expect(!swap.hasNewFrame());

      // Write another frame
      auto *write2 = swap.getWriteBuffer();
      write2->frameNumber = 2;
      swap.swapWriteToReady();

      expect(swap.hasNewFrame());
      auto *read2 = swap.getLatestFrame();
      expectEquals((int)read2->frameNumber, 2);
    }
#endif // ZENITH_USE_SKIA

    beginTest("MetricsChart Multi-Series");
    {
      zenith::ui::MetricsChart chart(zenith::ui::MetricsChart::ChartType::Line);
      expectEquals(chart.getSeriesCount(), 0);
      
      chart.addSeries("SeriesA", juce::Colours::red);
      expectEquals(chart.getSeriesCount(), 1);
      
      chart.addDataPointToSeries("SeriesA", 10.0f, 20.0f);
      expectEquals(chart.getSeriesPointCount("SeriesA"), 1);
      
      chart.clearSeries("SeriesA");
      expectEquals(chart.getSeriesPointCount("SeriesA"), 0);
      expectEquals(chart.getSeriesCount(), 1);
      
      chart.clearAllSeries();
      expectEquals(chart.getSeriesCount(), 0);
    }
  }
};

static UITests uiTests;
