/*
  ==============================================================================

    RightSidePanel.h
    Created: 2025-11-28
    Author:  David Chen + Isabella Moretti

    Layout container for Wingman console and scratch pads.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SkiaComponent.h"

namespace zenith {

#ifdef ZENITH_USE_SKIA

class RightSidePanel : public SkiaComponent {
public:
    RightSidePanel();
    ~RightSidePanel() override;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;

private:
    // WingmanComponent would be here
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightSidePanel)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
