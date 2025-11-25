/**
 * @file SkiaComboBox.h
 * @brief Beautiful GPU-accelerated combo box/dropdown with native Skia rendering
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

#ifdef ZENITH_USE_SKIA
    #include "SkiaComponent.h"
    #include "SkiaTextRenderer.h"
    #include "SkiaTheme.h"
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkRRect.h>
    #include <include/core/SkPath.h>
    #include <functional>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

/**
 * @class SkiaComboBox
 * @brief GPU-accelerated dropdown/combo box with smooth animations
 */
class SkiaComboBox : public juce::Component, public SkiaComponent, private juce::Timer
{
public:
    SkiaComboBox()
        : selectedIndex_(-1)
        , isOpen_(false)
        , isHovered_(false)
        , openProgress_(0.0f)
        , openVelocity_(0.0f)
        , hoverProgress_(0.0f)
        , hoverVelocity_(0.0f)
        , hoveredItem_(-1)
    {
        setOpaque(false);
        startTimer(16); // 60 FPS
    }

    ~SkiaComboBox() override { stopTimer(); }

    //==========================================================================
    // Configuration
    //==========================================================================

    void addItem(const juce::String& text, int itemId)
    {
        Item item;
        item.text = text;
        item.id = itemId;
        items_.push_back(item);
        repaint();
    }

    void clear()
    {
        items_.clear();
        selectedIndex_ = -1;
        repaint();
    }

    void setSelectedId(int itemId, bool sendNotification = true)
    {
        for (size_t i = 0; i < items_.size(); ++i)
        {
            if (items_[i].id == itemId)
            {
                setSelectedIndex(static_cast<int>(i), sendNotification);
                return;
            }
        }
    }

    void setSelectedIndex(int index, bool sendNotification = true)
    {
        if (selectedIndex_ != index && index >= -1 && index < static_cast<int>(items_.size()))
        {
            selectedIndex_ = index;

            if (sendNotification && onChange)
                onChange();

            repaint();
        }
    }

    int getSelectedId() const
    {
        return (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()))
            ? items_[selectedIndex_].id
            : 0;
    }

    int getSelectedIndex() const { return selectedIndex_; }

    juce::String getText() const
    {
        return (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()))
            ? items_[selectedIndex_].text
            : juce::String();
    }

    //==========================================================================
    // Callback
    //==========================================================================

    std::function<void()> onChange;

    //==========================================================================
    // Component overrides
    //==========================================================================

    void paint(juce::Graphics& g) override { juce::ignoreUnused(g); }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (isOpen_)
        {
            // Check if clicked on an item
            int item = getItemAtPosition(e.position.y);
            if (item >= 0 && item < static_cast<int>(items_.size()))
            {
                setSelectedIndex(item, true);
            }
            isOpen_ = false;
        }
        else
        {
            isOpen_ = true;
        }
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        isHovered_ = true;

        if (isOpen_)
        {
            int item = getItemAtPosition(e.position.y);
            if (hoveredItem_ != item)
            {
                hoveredItem_ = item;
                repaint();
            }
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        isHovered_ = false;
        hoveredItem_ = -1;
        repaint();
    }

    //==========================================================================
    // SkiaComponent implementation
    //==========================================================================

    bool supportsSkiaRendering() const override { return true; }

    void paintToSkia(SkCanvas* canvas, SkRect bounds) override
    {
        const auto& theme = SkiaTheme::getInstance();
        const auto& colors = theme.getColors();

        const float cornerRadius = 4.0f;
        const float itemHeight = 28.0f;

        // Main combo box background
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        bgPaint.setColor(isOpen_ ? colors.surfaceHover : colors.surfaceDefault);

        SkRRect mainRRect = SkRRect::MakeRectXY(bounds, cornerRadius, cornerRadius);
        canvas->drawRRect(mainRRect, bgPaint);

        // Border
        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(isHovered_ ? 2.0f : 1.0f);
        borderPaint.setColor(isHovered_ ? colors.primary : colors.border);
        canvas->drawRRect(mainRRect, borderPaint);

        // Selected text
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size()))
        {
            SkiaTextRenderer textRenderer;
            TextRenderOptions opts;
            opts.color = colors.textPrimary;
            opts.antiAlias = true;
            opts.effects = TextEffect::None;

            float textX = bounds.x() + 8.0f;
            float textY = bounds.centerY() + 4.0f;

            textRenderer.drawText(canvas, items_[selectedIndex_].text, textX, textY, TextStyle::Regular, opts);
        }

        // Dropdown arrow
        {
            float arrowSize = 8.0f;
            float arrowX = bounds.right() - arrowSize - 8.0f;
            float arrowY = bounds.centerY();

            SkPaint arrowPaint;
            arrowPaint.setAntiAlias(true);
            arrowPaint.setColor(colors.textSecondary);
            arrowPaint.setStyle(SkPaint::kFill_Style);

            SkPath arrowPath;
            arrowPath.moveTo(arrowX, arrowY - arrowSize * 0.3f);
            arrowPath.lineTo(arrowX + arrowSize, arrowY - arrowSize * 0.3f);
            arrowPath.lineTo(arrowX + arrowSize * 0.5f, arrowY + arrowSize * 0.3f);
            arrowPath.close();

            canvas->drawPath(arrowPath, arrowPaint);
        }

        // Dropdown menu (if open)
        if (isOpen_ && openProgress_ > 0.01f)
        {
            float dropdownHeight = items_.size() * itemHeight;
            float alpha = openProgress_;

            SkRect dropdownBounds = SkRect::MakeXYWH(
                bounds.x(),
                bounds.bottom() + 2,
                bounds.width(),
                dropdownHeight * alpha
            );

            // Shadow
            SkPaint shadowPaint;
            shadowPaint.setAntiAlias(true);
            shadowPaint.setColor(SkColorSetARGB(static_cast<uint8_t>(50 * alpha), 0, 0, 0));
            SkRRect shadowRRect = SkRRect::MakeRectXY(dropdownBounds.makeOffset(0, 2), cornerRadius, cornerRadius);
            canvas->drawRRect(shadowRRect, shadowPaint);

            // Background
            SkPaint dropdownBgPaint;
            dropdownBgPaint.setAntiAlias(true);
            dropdownBgPaint.setColor(colors.surfaceDefault);
            SkRRect dropdownRRect = SkRRect::MakeRectXY(dropdownBounds, cornerRadius, cornerRadius);
            canvas->drawRRect(dropdownRRect, dropdownBgPaint);

            // Border
            SkPaint dropdownBorderPaint;
            dropdownBorderPaint.setAntiAlias(true);
            dropdownBorderPaint.setStyle(SkPaint::kStroke_Style);
            dropdownBorderPaint.setStrokeWidth(1.0f);
            dropdownBorderPaint.setColor(colors.border);
            canvas->drawRRect(dropdownRRect, dropdownBorderPaint);

            // Items
            canvas->save();
            canvas->clipRect(dropdownBounds);

            for (size_t i = 0; i < items_.size(); ++i)
            {
                float itemY = dropdownBounds.y() + i * itemHeight;
                SkRect itemBounds = SkRect::MakeXYWH(dropdownBounds.x(), itemY, dropdownBounds.width(), itemHeight);

                bool isItemHovered = (static_cast<int>(i) == hoveredItem_);
                bool isItemSelected = (static_cast<int>(i) == selectedIndex_);

                // Item background
                if (isItemHovered || isItemSelected)
                {
                    SkPaint itemBgPaint;
                    itemBgPaint.setAntiAlias(true);
                    itemBgPaint.setColor(isItemHovered ? colors.surfaceHover : colors.backgroundSecondary);
                    canvas->drawRect(itemBounds, itemBgPaint);
                }

                // Item text
                SkiaTextRenderer textRenderer;
                TextRenderOptions opts;
                opts.color = isItemSelected ? colors.primary : colors.textPrimary;
                opts.antiAlias = true;
                opts.effects = TextEffect::None;

                float textX = itemBounds.x() + 8.0f;
                float textY = itemBounds.centerY() + 4.0f;

                textRenderer.drawText(canvas, items_[i].text, textX, textY, TextStyle::Regular, opts);
            }

            canvas->restore();
        }
    }

private:
    void timerCallback() override
    {
        const float dt = 0.016f;
        const float stiffness = 400.0f;
        const float damping = 25.0f;

        bool needsRepaint = false;

        // Open animation
        float openTarget = isOpen_ ? 1.0f : 0.0f;
        if (std::abs(openProgress_ - openTarget) > 0.001f)
        {
            float force = -stiffness * (openProgress_ - openTarget) - damping * openVelocity_;
            openVelocity_ += force * dt;
            openProgress_ += openVelocity_ * dt;
            openProgress_ = juce::jlimit(0.0f, 1.0f, openProgress_);
            needsRepaint = true;
        }

        // Hover animation
        float hoverTarget = isHovered_ ? 1.0f : 0.0f;
        if (std::abs(hoverProgress_ - hoverTarget) > 0.001f)
        {
            float force = -stiffness * (hoverProgress_ - hoverTarget) - damping * hoverVelocity_;
            hoverVelocity_ += force * dt;
            hoverProgress_ += hoverVelocity_ * dt;
            hoverProgress_ = juce::jlimit(0.0f, 1.0f, hoverProgress_);
            needsRepaint = true;
        }

        if (needsRepaint)
            repaint();
    }

    int getItemAtPosition(float y) const
    {
        if (!isOpen_)
            return -1;

        const float itemHeight = 28.0f;
        float dropdownY = getHeight() + 2;

        if (y < dropdownY)
            return -1;

        int item = static_cast<int>((y - dropdownY) / itemHeight);
        return (item >= 0 && item < static_cast<int>(items_.size())) ? item : -1;
    }

    struct Item
    {
        juce::String text;
        int id = 0;
    };

    std::vector<Item> items_;
    int selectedIndex_;
    bool isOpen_;
    bool isHovered_;
    float openProgress_;
    float openVelocity_;
    float hoverProgress_;
    float hoverVelocity_;
    int hoveredItem_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaComboBox)
};

#endif // ZENITH_USE_SKIA

} // namespace zenith
