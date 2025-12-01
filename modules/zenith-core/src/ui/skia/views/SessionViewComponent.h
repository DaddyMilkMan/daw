/*
  ==============================================================================

    SessionViewComponent.h
    Created: 2025-11-30
    Author:  Zenith DAW
    
    Stub implementation for Session View (Clip Launcher).

  ==============================================================================
*/

#pragma once



#include <JuceHeader.h>

#include "../SkiaComponent.h"



#ifdef ZENITH_USE_SKIA

#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRect.h>
#include <include/core/SkColor.h>
#include <include/core/SkFont.h>
#include <include/core/SkPath.h>
#include <include/core/SkRRect.h>

#endif



namespace zenith {



#ifdef ZENITH_USE_SKIA



class SessionViewComponent : public SkiaComponent

{

public:

    SessionViewComponent()

    {

        // Initialize with a default size or properties if needed

    }



    ~SessionViewComponent() override {}



    void drawSkia(::SkCanvas* canvas) override

    {

        // Placeholder rendering

        // Grid Background
        SkPaint paint;
        paint.setColor(SkColorSetRGB(30, 30, 35));
        SkRect rect = SkRect::MakeWH((float)getWidth(), (float)getHeight());
        canvas->drawRect(rect, paint);
        
        // Draw Grid of Clip Slots
        float slotWidth = 100.0f;
        float slotHeight = 60.0f;
        float spacing = 10.0f;
        int cols = 8;
        int rows = 8;
        
        SkPaint slotPaint;
        slotPaint.setAntiAlias(true);
        
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float x = 20 + c * (slotWidth + spacing);
                float y = 20 + r * (slotHeight + spacing);
                
                SkRect slotRect = SkRect::MakeXYWH(x, y, slotWidth, slotHeight);
                
                // Slot Background
                slotPaint.setStyle(SkPaint::kFill_Style);
                slotPaint.setColor(SkColorSetARGB(255, 45, 45, 50));
                canvas->drawRoundRect(slotRect, 4, 4, slotPaint);
                
                // Slot Border
                slotPaint.setStyle(SkPaint::kStroke_Style);
                slotPaint.setStrokeWidth(1.0f);
                slotPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
                canvas->drawRoundRect(slotRect, 4, 4, slotPaint);
                
                // Play Button Triangle (Placeholder)
                if (c == 0) { // First column active for demo
                    SkPath playPath;
                    playPath.moveTo(x + 10, y + 20);
                    playPath.lineTo(x + 20, y + 30);
                    playPath.lineTo(x + 10, y + 40);
                    playPath.close();
                    
                    slotPaint.setStyle(SkPaint::kFill_Style);
                    slotPaint.setColor(SkColorSetRGB(0, 255, 100)); // Green
                    canvas->drawPath(playPath, slotPaint);
                }
            }
        }
        
        SkFont font;
        font.setSize(24);
        font.setEmbolden(true);
        font.setEdging(SkFont::Edging::kAntiAlias);
        SkPaint textPaint;
        textPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
        
        canvas->drawString("SESSION VIEW", (float)getWidth() - 200, (float)getHeight() - 50, font, textPaint);

    }

    

    void resized() override

    {

        // Layout logic here

    }



private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewComponent)

};



#endif // ZENITH_USE_SKIA



} // namespace zenith


