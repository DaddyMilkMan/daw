/**
 * @file ZenithLookAndFeel.cpp
 * @brief Implementation of modern vibrant professional design system
 *
 * Implements design principles from industry research:
 * - Logic Pro: color-coded organization, clean typography
 * - Ableton Live: cyan accents, dark backgrounds, minimal chrome
 * - Material Design: elevation system, 8px grid, proper contrast
 * - WCAG AAA: 7:1 text contrast, 4.5:1 UI component contrast
 */

#include "ZenithLookAndFeel.h"
#include <cmath>

namespace zenith {

//==============================================================================
// Constructor
//==============================================================================

ZenithLookAndFeel::ZenithLookAndFeel()
{
    // === Window & Background Colors ===
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Elevation::dp0));
    setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(Elevation::dp0));
    
    // === Text Colors (87% opacity for primary) ===
    setColour(juce::Label::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    
    // === TextEditor Colors ===
    setColour(juce::TextEditor::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(Elevation::dp1));
    setColour(juce::TextEditor::highlightColourId, juce::Colour(Colors::accentPrimary).withAlpha(0.3f));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(Colors::borderSubtle));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    
    // === Button Colors ===
    setColour(juce::TextButton::buttonColourId, juce::Colour(Elevation::dp4));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::TextButton::textColourOffId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextButton::textColourOnId, juce::Colour(Colors::textOnAccent));
    
    // === ComboBox Colors ===
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(Elevation::dp2));
    setColour(juce::ComboBox::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(Colors::borderSubtle));
    setColour(juce::ComboBox::buttonColourId, juce::Colour(Elevation::dp4));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(Colors::textSecondary));
    setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(Colors::accentPrimary));
    
    // === Slider Colors ===
    setColour(juce::Slider::thumbColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::Slider::trackColourId, juce::Colour(Colors::accentPrimary).withAlpha(0.6f));
    setColour(juce::Slider::backgroundColourId, juce::Colour(Elevation::dp1));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(Elevation::dp1));
    setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(Colors::accentPrimary).withAlpha(0.3f));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(Colors::borderSubtle));
    
    // === Scrollbar Colors ===
    setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ScrollBar::thumbColourId, juce::Colour(Elevation::dp8));
    setColour(juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);
    
    // === ListBox Colors ===
    setColour(juce::ListBox::backgroundColourId, juce::Colour(Elevation::dp1));
    setColour(juce::ListBox::outlineColourId, juce::Colour(Colors::borderSubtle));
    setColour(juce::ListBox::textColourId, juce::Colour(Colors::textPrimary));
    
    // === TreeView Colors ===
    setColour(juce::TreeView::backgroundColourId, juce::Colour(Elevation::dp1));
    setColour(juce::TreeView::linesColourId, juce::Colour(Colors::borderSubtle));
    setColour(juce::TreeView::dragAndDropIndicatorColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::TreeView::selectedItemBackgroundColourId, juce::Colour(Elevation::dp8));
    
    // === ToggleButton Colors ===
    setColour(juce::ToggleButton::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(Colors::textDisabled));
    
    // === PopupMenu Colors ===
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(Elevation::dp8));
    setColour(juce::PopupMenu::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::PopupMenu::headerTextColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(Colors::textOnAccent));
    
    // === Tooltip Colors ===
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(Elevation::dp24));
    setColour(juce::TooltipWindow::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::TooltipWindow::outlineColourId, juce::Colour(Colors::accentPrimary));
    
    // === AlertWindow Colors ===
    setColour(juce::AlertWindow::backgroundColourId, juce::Colour(Elevation::dp8));
    setColour(juce::AlertWindow::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::AlertWindow::outlineColourId, juce::Colour(Colors::borderMedium));
    
    // === ProgressBar Colors ===
    setColour(juce::ProgressBar::backgroundColourId, juce::Colour(Elevation::dp1));
    setColour(juce::ProgressBar::foregroundColourId, juce::Colour(Colors::accentPrimary));
    
    // === Toolbar Colors ===
    setColour(juce::Toolbar::backgroundColourId, juce::Colour(Elevation::dp4));
    setColour(juce::Toolbar::separatorColourId, juce::Colour(Colors::borderSubtle));
    setColour(juce::Toolbar::buttonMouseOverBackgroundColourId, juce::Colour(Elevation::dp8));
    setColour(juce::Toolbar::buttonMouseDownBackgroundColourId, juce::Colour(Elevation::dp4));
    
    // === TabBar Colors ===
    setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(Elevation::dp0));
    setColour(juce::TabbedComponent::outlineColourId, juce::Colour(Colors::borderSubtle));
    setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colour(Colors::accentPrimary));
    setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colour(Colors::textSecondary));
    setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colour(Colors::textPrimary));
    
    // === PropertyComponent Colors ===
    setColour(juce::PropertyComponent::backgroundColourId, juce::Colour(Elevation::dp1));
    setColour(juce::PropertyComponent::labelTextColourId, juce::Colour(Colors::textSecondary));
}

//==============================================================================
// Button Drawing
//==============================================================================

void ZenithLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                            juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    
    auto baseColour = getButtonColour(button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    // Draw subtle glow on hover (dark mode optimization)
    if (shouldDrawButtonAsHighlighted && !shouldDrawButtonAsDown && button.isEnabled())
    {
        drawGlow(g, bounds, Radius::m, baseColour, Shadows::glowSubtle);
    }
    
    // Fill with base color
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, Radius::m);
    
    // Subtle border (1px)
    if (!shouldDrawButtonAsDown && button.isEnabled())
    {
        g.setColour(juce::Colour(Colors::borderSubtle));
        g.drawRoundedRectangle(bounds, Radius::m, 1.0f);
    }
    
    // Active state indicator for toggle buttons
    if (button.getToggleState() && !shouldDrawButtonAsDown)
    {
        auto indicatorBounds = bounds.removeFromBottom(2.0f);
        g.setColour(juce::Colour(Colors::accentPrimary).brighter(0.2f));
        g.fillRoundedRectangle(indicatorBounds, 1.0f);
    }
}

void ZenithLookAndFeel::drawButtonText(juce::Graphics& g,
                                      juce::TextButton& button,
                                      bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    g.setFont(Typography::getBody());
    
    // Determine text color based on state
    juce::Colour textColour;
    if (!button.isEnabled())
    {
        textColour = juce::Colour(Colors::textDisabled);
    }
    else if (button.getToggleState())
    {
        // High contrast text on accent background
        textColour = juce::Colour(Colors::textOnAccent);
    }
    else
    {
        textColour = juce::Colour(Colors::textPrimary);
    }
    
    g.setColour(textColour);
    
    auto textBounds = button.getLocalBounds();
    
    // Tactile feedback: shift text down 1px when pressed
    if (shouldDrawButtonAsDown)
    {
        textBounds.translate(0, 1);
    }
    
    g.drawText(button.getButtonText(),
              textBounds,
              juce::Justification::centred,
              true);
}

void ZenithLookAndFeel::drawToggleButton(juce::Graphics& g,
                                        juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto fontSize = juce::jmin(15.0f, (float)button.getHeight() * 0.65f);
    auto tickWidth = fontSize * 1.5f;
    
    // Draw checkbox/toggle area
    auto tickBounds = bounds.removeFromLeft(tickWidth).reduced(Spacing::xs);
    
    auto baseColour = button.getToggleState() 
                    ? juce::Colour(Colors::accentPrimary)
                    : juce::Colour(Elevation::dp2);
    
    if (!button.isEnabled())
    {
        baseColour = juce::Colour(Elevation::dp1);
    }
    else if (shouldDrawButtonAsDown)
    {
        baseColour = baseColour.darker(0.2f);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColour = baseColour.brighter(0.15f);
        // Subtle glow on hover
        drawGlow(g, tickBounds, Radius::s, baseColour, Shadows::glowSubtle * 0.5f);
    }
    
    // Fill toggle box
    g.setColour(baseColour);
    g.fillRoundedRectangle(tickBounds, Radius::s);
    
    // Border
    g.setColour(juce::Colour(Colors::borderSubtle));
    g.drawRoundedRectangle(tickBounds, Radius::s, 1.0f);
    
    // Draw checkmark if toggled
    if (button.getToggleState())
    {
        g.setColour(juce::Colour(Colors::textOnAccent));
        auto checkSize = tickBounds.getHeight() * 0.5f;
        auto checkBounds = tickBounds.reduced(tickBounds.getHeight() * 0.25f);
        
        juce::Path checkMark;
        checkMark.startNewSubPath(checkBounds.getX(), checkBounds.getCentreY());
        checkMark.lineTo(checkBounds.getCentreX() - checkSize * 0.2f, checkBounds.getBottom() - checkSize * 0.3f);
        checkMark.lineTo(checkBounds.getRight(), checkBounds.getY());
        
        g.strokePath(checkMark, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    
    // Draw label text
    if (button.getButtonText().isNotEmpty())
    {
        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(Typography::getBody());
        
        auto textBounds = bounds.reduced(Spacing::xs, 0);
        g.drawFittedText(button.getButtonText(),
                        textBounds.toNearestInt(),
                        juce::Justification::centredLeft,
                        1);
    }
}

//==============================================================================
// Slider Drawing
//==============================================================================

void ZenithLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                        int x, int y, int width, int height,
                                        float sliderPos,
                                        float rotaryStartAngle,
                                        float rotaryEndAngle,
                                        juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(Spacing::s);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(8.0f, radius * 0.4f);
    auto arcRadius = radius - lineW * 0.5f;
    
    auto centre = bounds.getCentre();
    
    // Background arc (track) - darker
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y,
                               arcRadius, arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               rotaryEndAngle,
                               true);
    
    g.setColour(juce::Colour(Elevation::dp1));
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Value arc (filled track) - vibrant accent
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y,
                              arcRadius, arcRadius,
                              0.0f,
                              rotaryStartAngle,
                              toAngle,
                              true);
        
        auto trackColour = juce::Colour(Colors::accentPrimary);
        
        // Glow effect on hover
        if (slider.isMouseOverOrDragging())
        {
            g.setColour(trackColour.withAlpha(0.4f));
            g.strokePath(valueArc, juce::PathStrokeType(lineW + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        
        g.setColour(trackColour);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    
    // Thumb indicator (pointer line from center)
    juce::Point<float> thumbPoint(centre.x + arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
                                 centre.y + arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));
    
    auto thumbColour = slider.isEnabled() ? juce::Colour(Colors::textPrimary) : juce::Colour(Colors::textDisabled);
    g.setColour(thumbColour);
    g.drawLine(centre.x, centre.y, thumbPoint.x, thumbPoint.y, 2.5f);
    
    // Center dot
    g.fillEllipse(centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

void ZenithLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                        int x, int y, int width, int height,
                                        float sliderPos,
                                        float minSliderPos,
                                        float maxSliderPos,
                                        juce::Slider::SliderStyle style,
                                        juce::Slider& slider)
{
    auto isVertical = (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical);
    auto trackWidth = isVertical ? width : height;
    trackWidth = juce::jmin(trackWidth, 32);
    
    auto trackBounds = isVertical
        ? juce::Rectangle<float>((float)x + (width - trackWidth) * 0.5f, (float)y, (float)trackWidth, (float)height)
        : juce::Rectangle<float>((float)x, (float)y + (height - trackWidth) * 0.5f, (float)width, (float)trackWidth);
    
    // Background track
    g.setColour(juce::Colour(Elevation::dp1));
    g.fillRoundedRectangle(trackBounds, Radius::s);
    
    // Filled portion (value)
    auto filledBounds = trackBounds;
    if (isVertical)
    {
        auto valueHeight = trackBounds.getHeight() * (1.0f - sliderPos / height);
        filledBounds.removeFromTop(valueHeight);
    }
    else
    {
        auto valueWidth = sliderPos - x;
        filledBounds.setWidth(valueWidth);
    }
    
    auto fillColour = slider.isEnabled() 
                    ? juce::Colour(Colors::accentPrimary) 
                    : juce::Colour(Colors::textDisabled);
    
    // Glow effect on hover
    if (slider.isMouseOverOrDragging() && slider.isEnabled())
    {
        drawGlow(g, filledBounds, Radius::s, fillColour, Shadows::glowSubtle);
    }
    
    g.setColour(fillColour);
    g.fillRoundedRectangle(filledBounds, Radius::s);
    
    // Subtle border
    g.setColour(juce::Colour(Colors::borderSubtle));
    g.drawRoundedRectangle(trackBounds, Radius::s, 1.0f);
    
    // Thumb (optional for non-bar styles)
    if (style != juce::Slider::LinearBarVertical && style != juce::Slider::LinearBar)
    {
        auto thumbSize = trackWidth * 1.2f;
        auto thumbBounds = isVertical
            ? juce::Rectangle<float>(trackBounds.getCentreX() - thumbSize * 0.5f, sliderPos - thumbSize * 0.5f, thumbSize, thumbSize)
            : juce::Rectangle<float>(sliderPos - thumbSize * 0.5f, trackBounds.getCentreY() - thumbSize * 0.5f, thumbSize, thumbSize);
        
        // Thumb glow
        if (slider.isMouseOverOrDragging())
        {
            g.setColour(fillColour.withAlpha(0.3f));
            g.fillEllipse(thumbBounds.expanded(4.0f));
        }
        
        // Thumb body
        g.setColour(fillColour);
        g.fillEllipse(thumbBounds);
        
        // Thumb outline
        g.setColour(juce::Colour(Colors::borderMedium));
        g.drawEllipse(thumbBounds, 1.5f);
    }
}

//==============================================================================
// ComboBox Drawing
//==============================================================================

void ZenithLookAndFeel::drawComboBox(juce::Graphics& g,
                                    int width, int height,
                                    bool isButtonDown,
                                    int buttonX, int buttonY,
                                    int buttonW, int buttonH,
                                    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
    
    // Background
    auto bgColour = box.findColour(juce::ComboBox::backgroundColourId);
    
    if (!box.isEnabled())
    {
        bgColour = juce::Colour(Elevation::dp1);
    }
    else if (isButtonDown)
    {
        bgColour = juce::Colour(Elevation::dp8);
    }
    else if (box.hasKeyboardFocus(true))
    {
        bgColour = juce::Colour(Elevation::dp4);
    }
    
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, Radius::m);
    
    // Border
    auto borderColour = box.hasKeyboardFocus(true) 
                      ? juce::Colour(Colors::accentPrimary)
                      : juce::Colour(Colors::borderSubtle);
    
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, Radius::m, box.hasKeyboardFocus(true) ? 2.0f : 1.0f);
    
    // Dropdown arrow area background
    auto arrowZone = juce::Rectangle<float>((float)buttonX, (float)buttonY, (float)buttonW, (float)buttonH);
    g.setColour(juce::Colour(Elevation::dp4));
    g.fillRoundedRectangle(arrowZone.reduced(2.0f), Radius::s);
    
    // Dropdown arrow
    juce::Path arrow;
    auto arrowBounds = arrowZone.reduced(buttonW * 0.3f);
    arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                     arrowBounds.getRight(), arrowBounds.getY(),
                     arrowBounds.getCentreX(), arrowBounds.getBottom());
    
    auto arrowColour = box.isEnabled() 
                     ? juce::Colour(Colors::textSecondary)
                     : juce::Colour(Colors::textDisabled);
    
    g.setColour(arrowColour);
    g.fillPath(arrow);
}

//==============================================================================
// Label Drawing
//==============================================================================

void ZenithLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        auto textColour = label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha);
        
        g.setColour(textColour);
        
        auto font = label.getFont();
        g.setFont(font);
        
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        
        g.drawFittedText(label.getText(),
                        textArea,
                        label.getJustificationType(),
                        juce::jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
                        label.getMinimumHorizontalScale());
        
        // Outline (if needed)
        g.setColour(label.findColour(juce::Label::outlineColourId));
        g.drawRect(label.getLocalBounds());
    }
}

//==============================================================================
// Scrollbar Drawing
//==============================================================================

void ZenithLookAndFeel::drawScrollbar(juce::Graphics& g,
                                     juce::ScrollBar& scrollbar,
                                     int x, int y, int width, int height,
                                     bool isScrollbarVertical,
                                     int thumbStartPosition,
                                     int thumbSize,
                                     bool isMouseOver,
                                     bool isMouseDown)
{
    // Track background (minimal/transparent)
    g.setColour(juce::Colour(Elevation::dp0).withAlpha(0.3f));
    g.fillRect(x, y, width, height);
    
    // Thumb
    auto thumbBounds = isScrollbarVertical
        ? juce::Rectangle<int>(x + 2, thumbStartPosition, width - 4, thumbSize)
        : juce::Rectangle<int>(thumbStartPosition, y + 2, thumbSize, height - 4);
    
    auto thumbColour = scrollbar.findColour(juce::ScrollBar::thumbColourId);
    
    if (isMouseDown)
    {
        thumbColour = thumbColour.brighter(0.3f);
    }
    else if (isMouseOver)
    {
        thumbColour = thumbColour.brighter(0.15f);
    }
    
    g.setColour(thumbColour);
    g.fillRoundedRectangle(thumbBounds.toFloat(), Radius::s);
}

//==============================================================================
// Tab Button Drawing
//==============================================================================

void ZenithLookAndFeel::drawTabButton(juce::TabBarButton& button,
                                     juce::Graphics& g,
                                     bool isMouseOver,
                                     bool isMouseDown)
{
    auto activeArea = button.getActiveArea();
    auto isFrontTab = button.isFrontTab();
    
    // Background elevation
    auto bgColour = isFrontTab 
                  ? juce::Colour(Elevation::dp4)
                  : juce::Colour(Elevation::dp1);
    
    if (!isFrontTab && isMouseOver)
    {
        bgColour = juce::Colour(Elevation::dp2);
    }
    
    g.setColour(bgColour);
    g.fillRect(activeArea);
    
    // Active indicator (bottom bar)
    if (isFrontTab)
    {
        auto indicatorBounds = activeArea.removeFromBottom(3);
        g.setColour(juce::Colour(Colors::accentPrimary));
        g.fillRect(indicatorBounds);
    }
    
    // Text
    auto textColour = isFrontTab 
                    ? juce::Colour(Colors::textPrimary)
                    : juce::Colour(Colors::textSecondary);
    
    g.setColour(textColour);
    g.setFont(Typography::getBody());
    
    auto textArea = activeArea.reduced(Spacing::m, 0);
    g.drawText(button.getButtonText(),
              textArea,
              juce::Justification::centred,
              true);
}

//==============================================================================
// Popup Menu Drawing
//==============================================================================

void ZenithLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    
    // Background with elevation
    g.setColour(juce::Colour(Elevation::dp8));
    g.fillRoundedRectangle(bounds, Radius::l);
    
    // Subtle border
    g.setColour(juce::Colour(Colors::borderMedium));
    g.drawRoundedRectangle(bounds.reduced(0.5f), Radius::l, 1.0f);
    
    // Subtle inner highlight (top edge)
    g.setColour(juce::Colour(Elevation::dp12).withAlpha(0.5f));
    g.fillRect(bounds.removeFromTop(1.0f));
}

void ZenithLookAndFeel::drawPopupMenuItem(juce::Graphics& g,
                                         const juce::Rectangle<int>& area,
                                         bool isSeparator,
                                         bool isActive,
                                         bool isHighlighted,
                                         bool isTicked,
                                         bool hasSubMenu,
                                         const juce::String& text,
                                         const juce::String& shortcutKeyText,
                                         const juce::Drawable* icon,
                                         const juce::Colour* textColour)
{
    if (isSeparator)
    {
        auto r = area.reduced(Spacing::m, 0);
        r.removeFromTop(r.getHeight() / 2 - 1);
        
        g.setColour(juce::Colour(Colors::borderSubtle));
        g.fillRect(r.removeFromTop(1));
        return;
    }
    
    auto bounds = area.reduced(1, 0);
    
    // Highlighted background
    if (isHighlighted && isActive)
    {
        g.setColour(juce::Colour(Colors::accentPrimary));
        g.fillRoundedRectangle(bounds.toFloat(), Radius::s);
    }
    
    // Text color
    auto colour = textColour != nullptr
                ? *textColour
                : findColour(juce::PopupMenu::textColourId);
    
    if (isHighlighted && isActive)
    {
        colour = juce::Colour(Colors::textOnAccent);
    }
    else if (!isActive)
    {
        colour = juce::Colour(Colors::textDisabled);
    }
    
    g.setColour(colour);
    g.setFont(Typography::getBody());
    
    auto textBounds = bounds.reduced(Spacing::m, 0);
    
    // Tick mark
    if (isTicked)
    {
        auto tickBounds = textBounds.removeFromLeft(textBounds.getHeight()).reduced(4).toFloat();
        
        juce::Path tick;
        tick.startNewSubPath(tickBounds.getX(), tickBounds.getCentreY());
        tick.lineTo(tickBounds.getCentreX(), tickBounds.getBottom());
        tick.lineTo(tickBounds.getRight(), tickBounds.getY());
        
        g.strokePath(tick, juce::PathStrokeType(2.0f));
    }
    else if (icon != nullptr)
    {
        auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight()).reduced(4);
        icon->drawWithin(g, iconBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
    
    // Sub-menu arrow
    if (hasSubMenu)
    {
        auto arrowZone = textBounds.removeFromRight(textBounds.getHeight()).reduced(8).toFloat();
        
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getX(), arrowZone.getY(),
                         arrowZone.getX(), arrowZone.getBottom(),
                         arrowZone.getRight(), arrowZone.getCentreY());
        
        g.fillPath(arrow);
    }
    
    // Main text
    g.drawFittedText(text, textBounds.reduced(Spacing::xs, 0), 
                    juce::Justification::centredLeft, 1);
    
    // Shortcut text
    if (shortcutKeyText.isNotEmpty())
    {
        g.setFont(Typography::getSmall());
        g.setColour(colour.withAlpha(0.6f));
        g.drawText(shortcutKeyText, textBounds, juce::Justification::centredRight, true);
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

void ZenithLookAndFeel::drawRoundedRect(juce::Graphics& g,
                                       const juce::Rectangle<float>& bounds,
                                       float cornerSize,
                                       const juce::Colour& fillColour,
                                       const juce::Colour& strokeColour,
                                       float strokeWidth)
{
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    if (strokeWidth > 0.0f && strokeColour != juce::Colour())
    {
        g.setColour(strokeColour);
        g.drawRoundedRectangle(bounds, cornerSize, strokeWidth);
    }
}

void ZenithLookAndFeel::drawGlow(juce::Graphics& g,
                                const juce::Rectangle<float>& bounds,
                                float cornerSize,
                                const juce::Colour& glowColour,
                                float glowSize)
{
    // Draw glow as expanding rectangles with decreasing opacity
    // Better than shadow for dark mode
    auto numSteps = 8;
    auto stepSize = glowSize / numSteps;
    
    for (int i = 0; i < numSteps; ++i)
    {
        auto expansion = stepSize * (i + 1);
        auto alpha = glowColour.getFloatAlpha() * (1.0f - (float)i / numSteps);
        
        g.setColour(glowColour.withAlpha(alpha));
        g.drawRoundedRectangle(bounds.expanded(expansion), cornerSize + expansion * 0.5f, 1.0f);
    }
}

juce::Colour ZenithLookAndFeel::getButtonColour(juce::Button& button,
                                               bool isHighlighted,
                                               bool isDown)
{
    juce::Colour baseColour;
    
    if (!button.isEnabled())
    {
        return juce::Colour(Elevation::dp1).withAlpha(0.5f);
    }
    
    if (button.getToggleState())
    {
        baseColour = juce::Colour(Colors::accentPrimary);
        
        if (isDown)
            return juce::Colour(Colors::accentPrimaryPressed);
        if (isHighlighted)
            return juce::Colour(Colors::accentPrimaryHover);
        
        return baseColour;
    }
    else
    {
        baseColour = juce::Colour(Elevation::dp4);
        
        if (isDown)
            return juce::Colour(Elevation::dp2);
        if (isHighlighted)
            return juce::Colour(Elevation::dp8);
        
        return baseColour;
    }
}

juce::Colour ZenithLookAndFeel::getTrackColor(int index)
{
    // Cycle through track colors (frequency-based mapping)
    static const juce::uint32 trackColors[] = {
        Colors::trackRedDark,    // 0 - Bass/Kick
        Colors::trackOrange,     // 1 - Bass
        Colors::trackAmber,      // 2 - Low-mid
        Colors::trackYellow,     // 3 - Mid
        Colors::trackLime,       // 4 - Mid-high
        Colors::trackGreen,      // 5 - High
        Colors::trackCyan,       // 6 - Very high
        Colors::trackBlue,       // 7 - Bright
        Colors::trackIndigo,     // 8 - Synth
        Colors::trackPurple,     // 9 - Synth
        Colors::trackMagenta,    // 10 - Synth
        Colors::trackPink,       // 11 - Special
    };
    
    constexpr int numColors = sizeof(trackColors) / sizeof(trackColors[0]);
    return juce::Colour(trackColors[index % numColors]);
}

float ZenithLookAndFeel::calculateContrastRatio(const juce::Colour& fg, const juce::Colour& bg)
{
    // Calculate WCAG contrast ratio
    auto getLuminance = [](const juce::Colour& c) -> float {
        auto r = c.getFloatRed();
        auto g = c.getFloatGreen();
        auto b = c.getFloatBlue();
        
        // Convert to linear RGB
        auto toLinear = [](float val) -> float {
            return val <= 0.03928f ? val / 12.92f : std::pow((val + 0.055f) / 1.055f, 2.4f);
        };
        
        r = toLinear(r);
        g = toLinear(g);
        b = toLinear(b);
        
        // Calculate relative luminance
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    };
    
    auto l1 = getLuminance(fg);
    auto l2 = getLuminance(bg);
    
    // Ensure l1 is lighter
    if (l2 > l1)
        std::swap(l1, l2);
    
    return (l1 + 0.05f) / (l2 + 0.05f);
}

juce::Colour ZenithLookAndFeel::ensureReadableText(const juce::Colour& background, bool requireAAA)
{
    auto targetRatio = requireAAA ? 7.0f : 4.5f;
    
    // Try white first (87% opacity as per Material Design)
    auto whiteText = juce::Colour(Colors::textPrimary);
    if (calculateContrastRatio(whiteText, background) >= targetRatio)
        return whiteText;
    
    // Try black
    auto blackText = juce::Colour(Colors::textOnAccent);
    if (calculateContrastRatio(blackText, background) >= targetRatio)
        return blackText;
    
    // If neither works, adjust brightness
    return background.getBrightness() > 0.5f ? blackText : whiteText;
}

} // namespace zenith
