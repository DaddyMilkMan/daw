/**
 * @file SkiaButtonComponent.cpp
 * @brief Implementation of Skia-rendered button with spring physics
 */

#include "SkiaButtonComponent.h"

// Skia headers
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkPath.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkColor.h"
#include "include/effects/SkGradientShader.h"
#include "include/effects/SkImageFilters.h"

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

namespace {
    // Spring physics parameters (for smooth, natural motion)
    constexpr float SPRING_STIFFNESS = 350.0f;  // Higher = faster response
    constexpr float SPRING_DAMPING = 25.0f;     // Higher = less overshoot
    constexpr float ANIMATION_FPS = 60.0f;
    constexpr float ANIMATION_DT = 1.0f / ANIMATION_FPS;

    // Visual constants
    constexpr float PRESS_SCALE = 0.95f;        // Scale down when pressed
    constexpr float HOVER_GLOW_ALPHA = 0.3f;    // Glow opacity on hover
    constexpr float SHADOW_BLUR = 12.0f;        // Shadow blur radius
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

SkiaButtonComponent::SkiaButtonComponent(const juce::String& buttonText, Style style)
    : buttonText_(buttonText)
    , style_(style)
{
    // Create Skia renderer
    renderer_ = std::make_unique<SkiaRenderer>(*this);

    // Start animation timer
    startAnimationTimer();
}

SkiaButtonComponent::~SkiaButtonComponent()
{
    animationTimer_.stopTimer();
}

//==============================================================================
// Component Interface
//==============================================================================

void SkiaButtonComponent::paint(juce::Graphics& g)
{
    // Instead of using JUCE graphics, we render with Skia
    if (!renderer_->isInitialized())
    {
        // Initialize on first paint
        if (!renderer_->initialize())
        {
            // Fallback: Draw error message with JUCE
            g.fillAll(juce::Colours::darkred);
            g.setColour(juce::Colours::white);
            g.drawText("Skia init failed", getLocalBounds(),
                      juce::Justification::centred);
            return;
        }
    }

    // Render with Skia
    renderer_->render([this](SkCanvas* canvas) {
        drawButton(canvas);
    });
}

void SkiaButtonComponent::resized()
{
    if (renderer_ && renderer_->isInitialized())
    {
        auto bounds = getLocalBounds();
        renderer_->resize(bounds.getWidth(), bounds.getHeight());
    }
}

//==============================================================================
// Mouse Events
//==============================================================================

void SkiaButtonComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SkiaButtonComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    isPressed_ = false;
    repaint();
}

void SkiaButtonComponent::mouseDown(const juce::MouseEvent&)
{
    isPressed_ = true;
    repaint();
}

void SkiaButtonComponent::mouseUp(const juce::MouseEvent& event)
{
    bool wasPressed = isPressed_;
    isPressed_ = false;

    if (wasPressed && event.mouseWasClicked() && onClick)
    {
        onClick();
    }

    repaint();
}

//==============================================================================
// Configuration
//==============================================================================

void SkiaButtonComponent::setButtonText(const juce::String& text)
{
    if (buttonText_ != text)
    {
        buttonText_ = text;
        repaint();
    }
}

void SkiaButtonComponent::setStyle(Style style)
{
    if (style_ != style)
    {
        style_ = style;
        repaint();
    }
}

void SkiaButtonComponent::setBlurEnabled(bool enable)
{
    if (blurEnabled_ != enable)
    {
        blurEnabled_ = enable;
        repaint();
    }
}

void SkiaButtonComponent::setCornerRadius(float radius)
{
    if (cornerRadius_ != radius)
    {
        cornerRadius_ = radius;
        repaint();
    }
}

//==============================================================================
// Animation
//==============================================================================

void SkiaButtonComponent::startAnimationTimer()
{
    // Use JUCE timer for simplicity (will upgrade to dedicated render thread later)
    animationTimer_.setCallback([this]() {
        updateAnimations();
    });

    // Run at 60Hz minimum (will support 120Hz later)
    animationTimer_.startTimer((int)(1000.0f / ANIMATION_FPS));
}

void SkiaButtonComponent::updateAnimations()
{
    bool needsRepaint = false;

    // Spring physics for hover animation
    float hoverTarget = isHovered_ ? 1.0f : 0.0f;
    if (std::abs(hoverProgress_ - hoverTarget) > 0.001f)
    {
        // Spring force: F = -k * (x - target) - d * velocity
        float force = -SPRING_STIFFNESS * (hoverProgress_ - hoverTarget)
                     - SPRING_DAMPING * hoverVelocity_;

        hoverVelocity_ += force * ANIMATION_DT;
        hoverProgress_ += hoverVelocity_ * ANIMATION_DT;

        // Clamp to [0, 1]
        hoverProgress_ = std::max(0.0f, std::min(1.0f, hoverProgress_));

        needsRepaint = true;
    }

    // Spring physics for press animation
    float pressTarget = isPressed_ ? 1.0f : 0.0f;
    if (std::abs(pressProgress_ - pressTarget) > 0.001f)
    {
        float force = -SPRING_STIFFNESS * 2.0f * (pressProgress_ - pressTarget)
                     - SPRING_DAMPING * 1.5f * pressVelocity_;

        pressVelocity_ += force * ANIMATION_DT;
        pressProgress_ += pressVelocity_ * ANIMATION_DT;

        pressProgress_ = std::max(0.0f, std::min(1.0f, pressProgress_));

        needsRepaint = true;
    }

    if (needsRepaint)
    {
        repaint();
    }
}

//==============================================================================
// Skia Drawing
//==============================================================================

void SkiaButtonComponent::drawButton(SkCanvas* canvas)
{
    auto bounds = getLocalBounds();
    float width = (float)bounds.getWidth();
    float height = (float)bounds.getHeight();

    // Calculate press scale (button shrinks when pressed)
    float scale = 1.0f - (pressProgress_ * (1.0f - PRESS_SCALE));
    float scaledWidth = width * scale;
    float scaledHeight = height * scale;
    float offsetX = (width - scaledWidth) / 2.0f;
    float offsetY = (height - scaledHeight) / 2.0f;

    // Button rectangle
    SkRect buttonRect = SkRect::MakeXYWH(offsetX, offsetY, scaledWidth, scaledHeight);
    SkRRect roundRect = SkRRect::MakeRectXY(buttonRect, cornerRadius_, cornerRadius_);

    //==========================================================================
    // Draw shadow (when not pressed)
    //==========================================================================

    if (pressProgress_ < 0.5f)
    {
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        shadowPaint.setColor(SkColorSetARGB(
            (int)(60 * (1.0f - pressProgress_ * 2.0f)), 0, 0, 0));

        // Blur for soft shadow
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(
            kNormal_SkBlurStyle, SHADOW_BLUR / 2.0f));

        // Offset shadow down slightly
        SkRRect shadowRect = roundRect;
        shadowRect.offset(0, 3.0f * (1.0f - pressProgress_));

        canvas->drawRRect(shadowRect, shadowPaint);
    }

    //==========================================================================
    // Draw button body with gradient
    //==========================================================================

    SkPaint bodyPaint;
    bodyPaint.setAntiAlias(true);

    // Get base color from style
    auto baseColour = getStyleColour();

    // Convert JUCE color to Skia
    SkColor topColor = SkColorSetARGB(
        baseColour.getAlpha(),
        (int)(baseColour.getRed() * 1.2f),    // Lighter at top
        (int)(baseColour.getGreen() * 1.2f),
        (int)(baseColour.getBlue() * 1.2f)
    );

    SkColor bottomColor = SkColorSetARGB(
        baseColour.getAlpha(),
        (int)(baseColour.getRed() * 0.8f),    // Darker at bottom
        (int)(baseColour.getGreen() * 0.8f),
        (int)(baseColour.getBlue() * 0.8f)
    );

    // Vertical gradient (Apple style)
    SkColor colors[] = { topColor, bottomColor };
    SkPoint points[] = {
        SkPoint::Make(buttonRect.centerX(), buttonRect.top()),
        SkPoint::Make(buttonRect.centerX(), buttonRect.bottom())
    };

    sk_sp<SkShader> gradient = SkGradientShader::MakeLinear(
        points, colors, nullptr, 2, SkTileMode::kClamp);

    bodyPaint.setShader(gradient);

    canvas->drawRRect(roundRect, bodyPaint);

    //==========================================================================
    // Draw inner highlight (top 30% of button)
    //==========================================================================

    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setColor(SkColorSetARGB(30, 255, 255, 255));  // 12% white

    SkRect highlightRect = buttonRect;
    highlightRect.fBottom = highlightRect.fTop + (highlightRect.height() * 0.3f);

    SkRRect highlightRRect = SkRRect::MakeRectXY(
        highlightRect, cornerRadius_, cornerRadius_ / 2.0f);

    canvas->drawRRect(highlightRRect, highlightPaint);

    //==========================================================================
    // Draw hover glow
    //==========================================================================

    if (hoverProgress_ > 0.01f)
    {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(3.0f);

        // Brighter version of base color
        auto glowColour = baseColour.brighter(0.4f);
        glowPaint.setColor(SkColorSetARGB(
            (int)(HOVER_GLOW_ALPHA * 255 * hoverProgress_),
            glowColour.getRed(),
            glowColour.getGreen(),
            glowColour.getBlue()
        ));

        // Expand rect for glow
        SkRRect glowRect = roundRect;
        glowRect.outset(2.0f, 2.0f);

        canvas->drawRRect(glowRect, glowPaint);
    }

    //==========================================================================
    // Draw text
    //==========================================================================

    if (buttonText_.isNotEmpty())
    {
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SK_ColorWHITE);

        // TODO(zenith-core#1): Use proper font once we integrate FreeType
        // For now, use Skia's default font
        SkFont font(nullptr, 14.0f);

        // Measure text
        SkRect textBounds;
        font.measureText(buttonText_.toRawUTF8(), buttonText_.length(),
                        SkTextEncoding::kUTF8, &textBounds);

        // Center text
        float textX = buttonRect.centerX() - textBounds.width() / 2.0f;
        float textY = buttonRect.centerY() + textBounds.height() / 2.0f;

        canvas->drawString(buttonText_.toRawUTF8(), textX, textY,
                          font, textPaint);
    }

    //==========================================================================
    // Draw press overlay (darkens button when pressed)
    //==========================================================================

    if (pressProgress_ > 0.01f)
    {
        SkPaint pressOverlay;
        pressOverlay.setAntiAlias(true);
        pressOverlay.setColor(SkColorSetARGB(
            (int)(50 * pressProgress_), 0, 0, 0));  // 20% black

        canvas->drawRRect(roundRect, pressOverlay);
    }
}

juce::Colour SkiaButtonComponent::getStyleColour() const
{
    switch (style_)
    {
        case Style::Primary:   return juce::Colour(0xff0A84FF);  // Apple blue
        case Style::Secondary: return juce::Colour(0xff3A3A3C);  // Gray
        case Style::Success:   return juce::Colour(0xff34C759);  // Apple green
        case Style::Danger:    return juce::Colour(0xffFF3B30);  // Apple red
        case Style::Warning:   return juce::Colour(0xffFF9500);  // Apple orange
        default:               return juce::Colour(0xff0A84FF);
    \n    default: break;\n\n    default: break;\n}
}

} // namespace zenith


