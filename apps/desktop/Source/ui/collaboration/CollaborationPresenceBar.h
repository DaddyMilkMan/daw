#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../network/CollaborationManager.h"
#include "../../ui/framework/SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

class CollaborationPresenceBar : public SkiaComponent,
                                 private juce::ChangeListener {
public:
    CollaborationPresenceBar() {
        CollaborationManager::getInstance().addChangeListener(this);
    }

    ~CollaborationPresenceBar() override {
        CollaborationManager::getInstance().removeChangeListener(this);
    }

    void drawSkia(SkCanvas* canvas) override {
        auto users = CollaborationManager::getInstance().getRemoteUsers();
        auto localName = CollaborationManager::getInstance().getLocalUserName();
        
        float x = (float)getWidth() - 10;
        float avatarSize = 32.0f;
        float spacing = 8.0f;

        // Draw Remote Users
        for (const auto& user : users) {
            if (!user.isOnline) continue;
            
            x -= avatarSize;
            drawAvatar(canvas, SkRect::MakeXYWH(x, ((float)getHeight() - avatarSize) * 0.5f, avatarSize, avatarSize), 
                       user.name, zenith::design::toSkColor(user.color));
            x -= spacing;
        }

        // Draw Local User
        x -= avatarSize;
        drawAvatar(canvas, SkRect::MakeXYWH(x, ((float)getHeight() - avatarSize) * 0.5f, avatarSize, avatarSize), 
                   localName, SkColorSetRGB(100, 100, 100)); // Grey
    }

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        repaint();
    }

    void drawAvatar(SkCanvas* canvas, SkRect bounds, const juce::String& name, SkColor color) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(color);
        canvas->drawOval(bounds, paint);
        
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(1.0f);
        paint.setColor(SK_ColorWHITE);
        canvas->drawOval(bounds, paint);

        if (name.isNotEmpty()) {
            SkFont font = design::getSkFont(bounds.height() * 0.5f, design::FontWeight::Bold);
            juce::String initials = name.substring(0, 1).toUpperCase();
            
            paint.setStyle(SkPaint::kFill_Style);
            paint.setColor(SK_ColorWHITE);
            
            SkRect textBounds;
            font.measureText(initials.toRawUTF8(), initials.length(), SkTextEncoding::kUTF8, &textBounds);
            
            float textX = bounds.centerX() - textBounds.width() / 2 - textBounds.left();
            float textY = bounds.centerY() + textBounds.height() / 2 - textBounds.bottom(); // Centering cap height roughly

            canvas->drawString(initials.toRawUTF8(), textX, textY, font, paint);
        }
    }
};

} // namespace zenith
