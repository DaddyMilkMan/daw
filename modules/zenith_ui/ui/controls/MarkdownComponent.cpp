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

#include "MarkdownComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithTheme.h"
#include "../../engine/ZenithLogger.h"

namespace zenith {
namespace widgets {

//==============================================================================
// ContentComp
//==============================================================================

void MarkdownComponent::ContentComp::paint(juce::Graphics &g) {
  content_.draw(g, getLocalBounds().toFloat().reduced(10.0f));
}

void MarkdownComponent::ContentComp::append(const juce::AttributedString &text) {
  ZENITH_LOG_INFO("ContentComp: appending text...");
  content_.append(text);
  
  // Recalculate height
  ZENITH_LOG_INFO("ContentComp: creating layout...");
  juce::TextLayout layout;
  layout.createLayout(content_, 600.0f); // approx width
  ZENITH_LOG_INFO("ContentComp: layout created");
  height_ = layout.getHeight() + 20.0f;
  setSize(getWidth(), (int)height_);
}

void MarkdownComponent::ContentComp::clear() {
  content_.clear();
  height_ = 0.0f;
  setSize(getWidth(), 0);
}

//==============================================================================
// MarkdownComponent
//==============================================================================

MarkdownComponent::MarkdownComponent() {
  viewport_ = std::make_unique<juce::Viewport>();
  contentComp_ = std::make_unique<ContentComp>();
  
  viewport_->setViewedComponent(contentComp_.get(), false);
  viewport_->setScrollBarsShown(true, false);
  addAndMakeVisible(viewport_.get());
}

MarkdownComponent::~MarkdownComponent() = default;

void MarkdownComponent::paint(juce::Graphics &g) {
    g.fillAll(design::toJuceColour(design::unified::bg_02()));
    g.setColour(design::toJuceColour(design::unified::border_default()));
    g.drawRect(getLocalBounds(), 1);
}

void MarkdownComponent::resized() {
  viewport_->setBounds(getLocalBounds().reduced(1));
  contentComp_->setSize(viewport_->getWidth(), juce::jmax(viewport_->getHeight(), contentComp_->getHeight()));
}

void MarkdownComponent::clear() {
    contentComp_->clear();
}

void MarkdownComponent::appendMessage(const juce::String &speaker, const juce::String &message) {
  // 1. Timestamp & Speaker
  juce::String timestamp = juce::Time::getCurrentTime().toString(false, true, false, true);
  juce::AttributedString header;
  header.setJustification(juce::Justification::topLeft);
  header.append("\n[" + timestamp + "] ", design::typography::getJuceFont(10.0f), design::toJuceColour(design::colors::TEXT_SECONDARY));
  header.append(speaker + ":\n", design::typography::getJuceFont(14.0f, design::FontWeight::Bold), 
                speaker == "You" ? design::toJuceColour(design::colors::TEXT_PRIMARY) : design::toJuceColour(design::colors::ACCENT_PRIMARY));
  
  contentComp_->append(header);

  // 2. Body with Markdown
  juce::Colour msgColor = design::toJuceColour(design::colors::TEXT_PRIMARY);
  auto body = parseMarkdown(message, msgColor);
  contentComp_->append(body);
  
  // Scroll to bottom
  viewport_->setViewPosition(0, contentComp_->getHeight());
}

// State Machine Parser for "A+" Quality
juce::AttributedString MarkdownComponent::parseMarkdown(const juce::String &text, const juce::Colour& colour) {
  juce::AttributedString as;
  as.setJustification(juce::Justification::topLeft);
  
  const juce::Font regular = design::typography::getJuceFont(14.0f);
  const juce::Font bold = regular.withStyle(juce::Font::bold);
  const juce::Font italic = regular.withStyle(juce::Font::italic);
  const juce::Font monospace = design::typography::getJuceMonoFont(13.0f);
  const juce::Colour codeBg = juce::Colour(0xff2d2d2d); // Dark box for code

  juce::String currentSegment;
  juce::Font currentFont = regular;
  juce::Colour currentColor = colour;
  
  // States
  bool isBold = false;
  bool isItalic = false;
  bool isCode = false;
  bool isCodeBlock = false;

  auto flush = [&](bool forceMonospace = false) {
     if (currentSegment.isNotEmpty()) {
         juce::Font f = forceMonospace ? monospace : currentFont;
         
         // Apply Bold/Italic logic if not in code (Code overrides styles)
         if (!forceMonospace && !isCode && !isCodeBlock) {
             if (isBold && isItalic) f = regular.boldened().italicised();
             else if (isBold) f = bold;
             else if (isItalic) f = italic;
             else f = regular;
         }
         
         // Code block background is hard in AttributedString, we simulated it with color/font usually
          // For A+, we'll just use the monospace font and a slightly lighter color
          juce::Colour c = (isCode || isCodeBlock) ? design::toJuceColour(design::colors::TEXT_SECONDARY) : currentColor;
         
         as.append(currentSegment, f, c);
         currentSegment.clear();
     }
  };

  int i = 0;
  while (i < text.length()) {
      juce::juce_wchar c = text[i];
      
      // 1. Code Block (```)
      if (text.substring(i).startsWith("```")) {
          flush();
          isCodeBlock = !isCodeBlock;
          i += 3;
          
          // If entering code block, add a newline for separation if needed
          if (isCodeBlock) as.append("\n", monospace, colour);
          continue;
      }
      
      // 2. Inline Code (`) - Only if not in block
      if (c == '`' && !isCodeBlock) {
          flush();
          isCode = !isCode;
          i++;
          continue;
      }
      
      // 3. Bold (**) - Only if not in code
      if (!isCode && !isCodeBlock && text.substring(i).startsWith("**")) {
          flush();
          isBold = !isBold;
          i += 2;
          continue;
      }
      
      // 4. Italic (*) - Only if not in code
      if (!isCode && !isCodeBlock && c == '*') {
          flush();
          isItalic = !isItalic;
          i++;
          continue;
      }
      
      // Regular Char
      currentSegment += c;
      i++;
  }
  
  flush(); // Final flush
  return as;
}

} // namespace widgets
} // namespace zenith
