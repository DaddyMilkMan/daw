/*
  ==============================================================================

    MarkdownComponent.cpp
    Created: 2025-12-17
    Author:  Zenith DAW

  ==============================================================================
*/

#include "MarkdownComponent.h"
#include "../ZenithTheme.h"

namespace zenith {
namespace widgets {

//==============================================================================
// ContentComp
//==============================================================================

void MarkdownComponent::ContentComp::paint(juce::Graphics &g) {
  content_.draw(g, getLocalBounds().toFloat().reduced(10.0f));
}

void MarkdownComponent::ContentComp::append(const juce::AttributedString &text) {
  content_.append(text);
  
  // Recalculate height
  juce::TextLayout layout;
  layout.createLayout(content_, 600.0f); // approx width
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
    g.fillAll(ZenithTheme::Colors::bg_02);
    g.setColour(ZenithTheme::Colors::border_default);
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
  header.append("\n[" + timestamp + "] ", ZenithTheme::Typography::getSmallFont(), ZenithTheme::Colors::text_secondary);
  header.append(speaker + ":\n", ZenithTheme::Typography::getBodyFont().boldened(), 
                speaker == "You" ? ZenithTheme::Colors::accent_secondary : ZenithTheme::Colors::accent_primary);
  
  contentComp_->append(header);

  // 2. Body with Markdown
  juce::Colour msgColor = ZenithTheme::Colors::text_primary;
  auto body = parseMarkdown(message, msgColor);
  contentComp_->append(body);
  
  // Scroll to bottom
  viewport_->setViewPosition(0, contentComp_->getHeight());
}

juce::AttributedString MarkdownComponent::parseMarkdown(const juce::String &text, const juce::Colour& colour) {
  juce::AttributedString as;
  as.setJustification(juce::Justification::topLeft);
  
  // Simple parser state machine
  // We handle **bold**, *italic*
  
  juce::Font regular = ZenithTheme::Typography::getBodyFont();
  juce::Font bold = regular.boldened();

  // Very basic: just check blocks (improving from last attempt)
  
  juce::String currentText = text;
  
  int boldStart = currentText.indexOf("**");
  if (boldStart >= 0) {
      int boldEnd = currentText.indexOf(boldStart + 2, "**");
      if (boldEnd > boldStart) {
           juce::String pre = currentText.substring(0, boldStart);
           juce::String mid = currentText.substring(boldStart + 2, boldEnd);
           juce::String post = currentText.substring(boldEnd + 2);
           
           as.append(pre, regular, colour);
           as.append(mid, bold, colour);
           as.append(post, regular, colour);
           return as;
      }
  }

  as.append(text, regular, colour);
  return as;
}

} // namespace widgets
} // namespace zenith
