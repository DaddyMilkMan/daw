/*
  ==============================================================================

    MarkdownComponent.h
    Created: 2025-12-17
    Author:  Zenith DAW

    A high-performance Markdown renderer using Skia.
    Supports:
    - Headers (#, ##, ###)
    - Bold (**text**)
    - Code Blocks (```)
    - Inline Code (`)
    - Bullet points

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "ZenithDesignSystem.h"

// Forward declare Skia types to avoid heavy includes in header if possible
// But for member variables we need definitions or pointers.
// We'll use Pimpl or just include if we have the headers.
// Since we are in the source tree, we assume we can include Skia headers if needed,
// but let's keep it minimal and implementation-heavy.

namespace zenith {
namespace widgets {

class MarkdownComponent : public juce::Component {
public:
    MarkdownComponent();
    ~MarkdownComponent() override;

    void setMarkdown(const juce::String& markdownText);
    void appendMarkdown(const juce::String& markdownText);
    void clear();

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Skia integration
    void drawSkia(SkCanvas* canvas);

private:
    struct Token {
        enum class Type {
            Text,
            Header1,
            Header2,
            Header3,
            CodeBlock,
            InlineCode,
            Bullet,
            Paragraph
        };
        Type type;
        juce::String content;
        bool isBold = false;
    };

    struct RenderLine {
        juce::String text;
        SkFont font;
        SkColor color;
        float x;
        float y;
        bool isCodeBlockBackground = false;
        SkRect backgroundRect;
    };

    void parseMarkdown();
    void layoutContent(float width);

    juce::String rawMarkdown_;
    std::vector<Token> tokens_;
    std::vector<RenderLine> renderLines_;
    float totalHeight_ = 0.0f;

    // Cache Skia resources
    void updateFonts();
    SkFont fontBody_;
    SkFont fontH1_;
    SkFont fontH2_;
    SkFont fontCode_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownComponent)
};

} // namespace widgets
} // namespace zenith
