/*
  ==============================================================================

    MarkdownComponent.h
    Created: 2025-12-17
    Author:  Zenith DAW

    A simplified Markdown renderer using generic AttributedString.
    Supports: **Bold**, *Italic*, `Code`.

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"

namespace zenith {
namespace widgets {

class MarkdownComponent : public SkiaComponent {
public:
  MarkdownComponent();
  ~MarkdownComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void paint(juce::Graphics& g) override { SkiaComponent::paint(g); }
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
