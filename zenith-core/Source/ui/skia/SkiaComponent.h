/**
 * @file SkiaComponent.h
 * @brief Base interface for components that support native Skia rendering
 */

#pragma once

#ifdef ZENITH_USE_SKIA
    #include "include/core/SkCanvas.h"
    #include "include/core/SkRect.h"
#endif

namespace zenith {

/**
 * @class SkiaComponent
 * @brief Interface for components that can render natively with Skia
 *
 * Components implementing this interface can bypass JUCE Graphics
 * and render directly to an SkCanvas for better performance and
 * visual quality.
 *
 * The parent component (MainWindow) will call paintToSkia() instead
 * of the standard JUCE paint() method when rendering.
 */
class SkiaComponent
{
public:
    virtual ~SkiaComponent() = default;

    /**
     * @brief Check if this component supports native Skia rendering
     * @return true if paintToSkia() should be used instead of paint()
     */
    virtual bool supportsSkiaRendering() const { return true; }

    /**
     * @brief Render this component using native Skia APIs
     * @param canvas The Skia canvas to draw on
     * @param bounds The bounds of this component in canvas coordinates
     *
     * This method replaces the standard JUCE paint() for Skia-capable components.
     * All rendering should be done using Skia APIs (SkPaint, SkPath, etc.)
     * instead of JUCE Graphics.
     */
    virtual void paintToSkia(SkCanvas* canvas, SkRect bounds) = 0;
};

} // namespace zenith
