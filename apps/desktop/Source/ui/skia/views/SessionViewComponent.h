/*
  ==============================================================================

    SessionViewComponent.h
    Created: 2025-11-30
    Author:  Zenith DAW
    
    Session View (Clip Launcher) - Skia Implementation

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../SkiaComponent.h"
#include "../../../../include/ProjectState.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRect.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

class SessionViewComponent : public SkiaComponent
{
public:
    explicit SessionViewComponent(ProjectState& state) 
        : projectState(state)
    {
        setWantsKeyboardFocus(true);
    }

    ~SessionViewComponent() override = default;

    void drawSkia(SkCanvas* canvas) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        SkPaint bgPaint;
        bgPaint.setColor(SkColorSetRGB(20, 20, 25));
        canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);

        auto tracksNode = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
        if (!tracksNode.isValid()) return;

        int numTracks = tracksNode.getNumChildren();
        float slotWidth = 120.0f;
        float slotHeight = 80.0f;
        float spacing = 8.0f;
        float startX = 20.0f;
        float startY = 20.0f;

        SkPaint slotPaint;
        slotPaint.setAntiAlias(true);
        
        SkFont textFont;
        textFont.setSize(14.0f);
        textFont.setSubpixel(true);
        SkPaint textPaint;
        textPaint.setColor(SK_ColorWHITE);

        for (int t = 0; t < numTracks; ++t) {
            auto track = tracksNode.getChild(t);
            auto trackName = track[ProjectState::PROP_NAME].toString();
            auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
            
            float x = startX + t * (slotWidth + spacing);
            
            // Track Header
            SkRect headerRect = SkRect::MakeXYWH(x, startY - 30.0f, slotWidth, 25.0f);
            textPaint.setColor(SkColorSetARGB(200, 255, 255, 255));
            canvas->drawString(trackName.toRawUTF8(), x + 5.0f, startY - 12.0f, textFont, textPaint);

            // Draw Slots (Rows)
            // For MVP, we draw a fixed number of scenes or based on clips found?
            // Ableton draws empty slots. Let's draw 8 scenes.
            int numScenes = 8;
            
            for (int s = 0; s < numScenes; ++s) {
                float y = startY + s * (slotHeight + spacing);
                SkRect slotRect = SkRect::MakeXYWH(x, y, slotWidth, slotHeight);
                SkRRect rrect = SkRRect::MakeRectXY(slotRect, 4.0f, 4.0f);

                // Check if clip exists in this slot (we need a way to map clips to scenes)
                // For MVP, we'll just list clips sequentially or find one with matching index?
                // ProjectState doesn't strictly have "Scene Index".
                // We'll iterate clips and see if any "fit" vertically or just draw present clips.
                // Simplification: Draw clips in order.
                
                bool hasClip = false;
                juce::String clipName;
                
                if (clipsNode.isValid() && s < clipsNode.getNumChildren()) {
                    auto clip = clipsNode.getChild(s);
                    clipName = clip[ProjectState::PROP_NAME].toString();
                    hasClip = true;
                }

                // Slot Background
                if (hasClip) {
                    // Active Clip
                    SkPoint pts[2] = { {x, y}, {x, y + slotHeight} };
                    SkColor colors[2] = { SkColorSetRGB(60, 60, 70), SkColorSetRGB(40, 40, 50) };
                    
                    // Highlight if playing (simulated)
                    if (s == 0 && t == 0) { // Demo active state
                         colors[0] = SkColorSetRGB(0, 100, 50);
                         colors[1] = SkColorSetRGB(0, 60, 30);
                    }
                    
                    slotPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
                    slotPaint.setStyle(SkPaint::kFill_Style);
                    canvas->drawRRect(rrect, slotPaint);
                    slotPaint.setShader(nullptr);
                    
                    // Border
                    slotPaint.setStyle(SkPaint::kStroke_Style);
                    slotPaint.setStrokeWidth(1.0f);
                    slotPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
                    canvas->drawRRect(rrect, slotPaint);
                    
                    // Clip Name
                    textPaint.setColor(SK_ColorWHITE);
                    canvas->drawString(clipName.toRawUTF8(), x + 10.0f, y + 25.0f, textFont, textPaint);
                    
                    // Play Icon
                    SkPath playPath;
                    playPath.moveTo(x + 10, y + 40);
                    playPath.lineTo(x + 20, y + 46);
                    playPath.lineTo(x + 10, y + 52);
                    playPath.close();
                    
                    slotPaint.setStyle(SkPaint::kFill_Style);
                    slotPaint.setColor(SkColorSetRGB(0, 255, 150));
                    canvas->drawPath(playPath, slotPaint);
                    
                } else {
                    // Empty Slot
                    slotPaint.setStyle(SkPaint::kFill_Style);
                    slotPaint.setColor(SkColorSetARGB(20, 255, 255, 255));
                    canvas->drawRRect(rrect, slotPaint);
                    
                    // Record/Stop Button (Placeholder)
                    slotPaint.setStyle(SkPaint::kFill_Style);
                    slotPaint.setColor(SkColorSetARGB(50, 100, 100, 100));
                    canvas->drawCircle(x + slotWidth/2, y + slotHeight/2, 6.0f, slotPaint);
                }
            }
        }
    }

    void resized() override {}
    
    void mouseDown(const juce::MouseEvent& e) override {
        // Handle clip launching logic here
        // Calculate track/scene from e.position
        // Call engine.launchClip(...)
    }

private:
    ProjectState& projectState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewComponent)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith


