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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    MarkdownComponent.h
    Created: 2025-12-17
    Author:  Zenith DAW

    A simplified Markdown renderer using generic AttributedString.
    Supports: **Bold**, *Italic*, `Code`.


  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {
namespace widgets {

class MarkdownComponent : public juce::Component {
public:
  MarkdownComponent();
  ~MarkdownComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void setMarkdown(const juce::String& markdownText);
  void appendMarkdown(const juce::String& markdownText);
  /** Appends a chat message with standard formatting (Timestamp + Speaker + Message). */
  void appendMessage(const juce::String& speaker, const juce::String& message);
  void clear();

private:
  class ContentComp : public juce::Component {
  public:
    void paint(juce::Graphics &g) override;
    void append(const juce::AttributedString &text);
    void clear();
    
  private:
    juce::AttributedString content_;
    float height_ = 0.0f;
  };

  std::unique_ptr<juce::Viewport> viewport_;
  std::unique_ptr<ContentComp> contentComp_;
  
  juce::AttributedString parseMarkdown(const juce::String &text, const juce::Colour& colour);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownComponent)
};

} // namespace widgets
} // namespace zenith
