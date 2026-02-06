/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <effects/SkGradientShader.h>

namespace zenith {

class WingmanChatBubble : public SkiaComponent {
public:
  WingmanChatBubble(const juce::String &text, bool isUser)
      : text_(text), isUser_(isUser) {}

  void drawSkia(SkCanvas *canvas) override {
    if (canvas == nullptr) return;

    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Highly rounded corners (bubble look)
    float radius = design::dimensions::RADIUS_LG;  // 12px
    SkRRect rrect = SkRRect::MakeRectXY(rect, radius, radius);

    //==========================================================================
    // 1. Subtle Shadow/Glow
    //==========================================================================
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(isUser_
        ? design::withAlpha(design::colors::ACCENT_PRIMARY, 0.15f)
        : design::withAlpha(design::colors::BG_00, 0.3f));
    shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
    canvas->drawRRect(rrect, shadowPaint);

    //==========================================================================
    // 2. Background (Glassmorphic Gradient)
    //==========================================================================
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    SkColor colors[2];
    if (isUser_) {
      // User bubbles: cyan/accent tint
      colors[0] = design::withAlpha(design::colors::ACCENT_PRIMARY, 0.20f);
      colors[1] = design::withAlpha(design::colors::ACCENT_PRIMARY, 0.08f);
    } else {
      // Assistant bubbles: neutral glassmorphism
      colors[0] = design::withAlpha(design::colors::BG_03, 0.65f);
      colors[1] = design::withAlpha(design::colors::BG_02, 0.45f);
    }

    SkPoint pts[2] = {{0, 0}, {0, rect.height()}};
    bgPaint.setShader(SkGradientShader::MakeLinear(
        pts, colors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRRect(rrect, bgPaint);

    //==========================================================================
    // 3. Hairline Border (0.5px)
    //==========================================================================
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(0.5f);  // HAIRLINE

    SkColor borderColor = isUser_
        ? design::withAlpha(design::colors::ACCENT_PRIMARY, 0.35f)
        : design::withAlpha(design::colors::BORDER_SUBTLE, 0.5f);
    borderPaint.setColor(borderColor);
    canvas->drawRRect(rrect, borderPaint);

    //==========================================================================
    // 4. Text Content (with Word Wrapping)
    //==========================================================================
    SkFont font = design::typography::getSkFont(design::typography::FONT_MD);
    
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_PRIMARY);

    float padding = design::spacing::MD;
    float maxWidth = bounds.getWidth() - padding * 2.5f;
    float lineHeight = font.getSpacing();
    if (lineHeight < 1.0f) lineHeight = font.getSize() * 1.4f;

    // Use a simple word wrapping helper
    auto wrapText = [&](const juce::String& text, float maxW) -> juce::StringArray {
        if (maxW <= 0) return { text };
        
        juce::StringArray result;
        juce::StringArray words;
        words.addTokens(text, " ", "");
        
        juce::String currentLine;
        for (int i = 0; i < words.size(); ++i) {
            juce::String testLine = currentLine.isEmpty() ? words[i] : currentLine + " " + words[i];
            
            auto utf8 = testLine.toRawUTF8();
            size_t bytes = strlen(utf8); // Safe for null-terminated UTF-8
            float w = font.measureText(utf8, bytes, SkTextEncoding::kUTF8);
            
            if (w > maxW && !currentLine.isEmpty()) {
                result.add(currentLine);
                currentLine = words[i];
            } else {
                currentLine = testLine;
            }
        }
        if (!currentLine.isEmpty()) result.add(currentLine);
        return result;
    };

    juce::StringArray lineWraps;
    juce::StringArray paragraphs;
    paragraphs.addLines(text_);
    
    for (const auto& p : paragraphs) {
        if (p.isEmpty()) continue;
        lineWraps.addArray(wrapText(p, maxWidth));
    }

    float y = padding + font.getSize() * 0.9f;
    for (const auto &line : lineWraps) {
      canvas->drawString(line.toRawUTF8(), padding * 1.25f, y, font, textPaint);
      y += lineHeight;
    }
  }

  void paint(juce::Graphics &g) override {
      // Skia rendering used - see drawSkia()
      juce::ignoreUnused(g);
  }

  int getRequiredHeight(int width) {
    if (width <= 0) return 42;
    
    SkFont font = design::typography::getSkFont(design::typography::FONT_MD);
    float padding = design::spacing::MD;
    float maxWidth = (float)width - padding * 2.5f;
    if (maxWidth <= 0) maxWidth = (float)width;
    
    auto wrapCount = [&](const juce::String& text, float maxW) -> int {
        if (maxW <= 0) return 1;
        
        int count = 0;
        juce::StringArray words;
        words.addTokens(text, " ", "");
        
        juce::String currentLine;
        for (int i = 0; i < words.size(); ++i) {
            juce::String testLine = currentLine.isEmpty() ? words[i] : currentLine + " " + words[i];
            
            auto utf8 = testLine.toRawUTF8();
            size_t bytes = strlen(utf8);
            float w = font.measureText(utf8, bytes, SkTextEncoding::kUTF8);
            
            if (w > maxW && !currentLine.isEmpty()) {
                count++;
                currentLine = words[i];
            } else {
                currentLine = testLine;
            }
        }
        if (!currentLine.isEmpty()) count++;
        return std::max(1, count);
    };

    int totalLines = 0;
    juce::StringArray paragraphs;
    paragraphs.addLines(text_);
    for (const auto& p : paragraphs) {
        totalLines += wrapCount(p, maxWidth);
    }
    if (totalLines == 0) totalLines = 1;

    float lineHeight = font.getSpacing();
    if (lineHeight < 1.0f) lineHeight = font.getSize() * 1.4f;

    int height = static_cast<int>(padding * 2 + totalLines * lineHeight);
    return std::max(height, 42);
  }

private:
  juce::String text_;
  bool isUser_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanChatBubble)
};

} // namespace zenith
