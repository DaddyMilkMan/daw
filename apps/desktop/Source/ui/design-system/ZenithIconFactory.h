/*
  ==============================================================================
    ZenithIconFactory.h
    Created: 2026-02-18
    Author:  Swarm Agent 01

    Central repository for resolution-independent Skia vector icons.
    Zero external assets required - paths are baked into the binary.
  ==============================================================================
*/

#pragma once

#include "../ZenithSkia.h"
#include <juce_core/juce_core.h>

namespace zenith::design {

enum class IconType {
    Play,
    Stop,
    Record,
    Loop,
    Metronome,
    Folder,
    FileAudio,
    FileMidi,
    Plugin,
    Settings,
    Close,
    Minimize,
    Maximize,
    Search,
    ChevronDown,
    ChevronRight,
    Lock,
    Unlock,
    Mute,
    Solo,
    Trash
};

class ZenithIconFactory {
public:
    static SkPath getIcon(IconType type) {
        SkPath path;
        switch (type) {
            case IconType::Play:
                path.moveTo(0, 0);
                path.lineTo(12, 7);
                path.lineTo(0, 14);
                path.close();
                break;
                
            case IconType::Stop:
                path.addRect(SkRect::MakeXYWH(0, 0, 12, 12));
                break;
                
            case IconType::Record:
                path.addCircle(7, 7, 6);
                break;
                
            case IconType::Loop:
                // Draw a recycling/loop arrow shape
                path.moveTo(12, 4);
                path.lineTo(16, 8);
                path.lineTo(12, 12);
                path.moveTo(16, 8);
                path.lineTo(4, 8);
                path.arcTo(SkRect::MakeXYWH(0, 8, 8, 8), 270, -180, false);
                path.lineTo(8, 16);
                break;

            case IconType::Folder:
                path.moveTo(0, 2);
                path.lineTo(6, 2);
                path.lineTo(8, 4);
                path.lineTo(18, 4);
                path.lineTo(18, 14);
                path.lineTo(0, 14);
                path.close();
                break;

            case IconType::FileAudio:
                path.moveTo(2, 0);
                path.lineTo(10, 0);
                path.lineTo(14, 4);
                path.lineTo(14, 16);
                path.lineTo(2, 16);
                path.close();
                // Waveform squiggle inside
                path.moveTo(4, 8);
                path.lineTo(6, 4);
                path.lineTo(8, 12);
                path.lineTo(10, 6);
                path.lineTo(12, 8);
                break;

            case IconType::FileMidi:
                path.moveTo(2, 0);
                path.lineTo(10, 0);
                path.lineTo(14, 4);
                path.lineTo(14, 16);
                path.lineTo(2, 16);
                path.close();
                // Piano keys inside
                path.addRect(SkRect::MakeXYWH(4, 8, 2, 6));
                path.addRect(SkRect::MakeXYWH(7, 8, 2, 6));
                path.addRect(SkRect::MakeXYWH(10, 8, 2, 6));
                break;

            case IconType::Search:
                path.addCircle(6, 6, 5);
                path.moveTo(10, 10);
                path.lineTo(14, 14);
                break;

            case IconType::Settings:
                // Simple gear cog approximation
                path.addCircle(8, 8, 3);
                for(int i=0; i<8; ++i) {
                    float angle = i * 45.0f * 0.0174533f;
                    float ox = 8 + cos(angle) * 6;
                    float oy = 8 + sin(angle) * 6;
                    path.addRect(SkRect::MakeXYWH(ox-1, oy-1, 2, 2));
                }
                break;
                
            case IconType::ChevronRight:
                path.moveTo(4, 2);
                path.lineTo(10, 8);
                path.lineTo(4, 14);
                break;
                
            case IconType::ChevronDown:
                path.moveTo(2, 4);
                path.lineTo(8, 10);
                path.lineTo(14, 4);
                break;

            case IconType::Mute:
                path.addRect(SkRect::MakeXYWH(0, 0, 12, 12));
                path.moveTo(0, 0);
                path.lineTo(12, 12);
                path.moveTo(12, 0);
                path.lineTo(0, 12);
                break;

            case IconType::Solo:
                path.addCircle(6, 6, 5);
                path.addCircle(6, 6, 2);
                break;

            case IconType::Lock:
                path.addRect(SkRect::MakeXYWH(2, 6, 10, 8));
                path.addArc(SkRect::MakeXYWH(4, 0, 6, 12), 180, 180);
                break;

            default:
                // Fallback square
                path.addRect(SkRect::MakeXYWH(0, 0, 10, 10));
                break;
        }
        return path;
    }
    
    /**
     * @brief Draws an icon centered within the given bounds
     */
    static void drawIcon(SkCanvas* canvas, IconType type, const SkRect& bounds, SkColor color) {
        SkPath path = getIcon(type);
        SkRect pathBounds = path.getBounds();
        
        // Calculate scale to fit (with padding)
        float targetSize = std::min(bounds.width(), bounds.height()) * 0.7f;
        float scale = targetSize / std::max(pathBounds.width(), pathBounds.height());
        
        // Calculate offset to center
        float ox = bounds.centerX() - (pathBounds.centerX() * scale);
        float oy = bounds.centerY() - (pathBounds.centerY() * scale);
        
        canvas->save();
        canvas->translate(ox, oy);
        canvas->scale(scale, scale);
        
        SkPaint paint;
        paint.setColor(color);
        paint.setAntiAlias(true);
        
        // Style specific tweaks
        if (type == IconType::Search || type == IconType::ChevronRight || type == IconType::ChevronDown || type == IconType::FileAudio || type == IconType::FileMidi) {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(1.5f / scale); // Constant pixel width
        } else {
            paint.setStyle(SkPaint::kFill_Style);
        }
        
        canvas->drawPath(path, paint);
        canvas->restore();
    }
};

} // namespace zenith::design
