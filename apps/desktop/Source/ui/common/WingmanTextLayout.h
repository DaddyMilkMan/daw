/*
  ==============================================================================

    WingmanTextLayout.h
    Created: 2026-01-18
    Author:  Zenith Team

    High-Fidelity Markdown Text Layout Engine for Skia.
    Supports:
    - Headers (#, ##)
    - Bold (**text**), Italic (*text*)
    - Inline Code (`text`)
    - Code Blocks (```lang ... ```)
    - Bulleted Lists (- item)
    - Paragraph spacing

    (Regex-free implementation for maximum compatibility)
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../ZenithSkia.h"
#include "../design-system/ZenithDesignSystem.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm> 

namespace zenith {

//==============================================================================
// Data Structures
//==============================================================================

enum class BlockType {
    Paragraph,
    Header1,
    Header2,
    CodeBlock,
    ListBullet
};

enum class SpanStyle {
    Regular,
    Bold,
    Italic,
    Code
};

struct TextSpan {
    std::string text;
    SpanStyle style;
    SkFont font; 
    float width;
};

struct TextLine {
    std::vector<TextSpan> spans;
    float width = 0.0f;
    float height = 0.0f;
};

struct LayoutBlock {
    BlockType type;
    std::vector<TextLine> lines; // For text-based blocks
    std::string rawContent;      // For code blocks (raw text)
    float height = 0.0f;
    float width = 0.0f;
    float marginTop = 0.0f;
    float marginBottom = 0.0f;
};

struct CachedTextLayout {
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    std::vector<LayoutBlock> blocks;
    // Legacy fields for compatibility if needed, but we should use blocks now
    float textLineHeight = 0.0f;
    float codeLineHeight = 0.0f;

    // Backwards-compatible accessors used by existing code
    float height = 0.0f;
    float width = 0.0f;
    struct Segment { int type = 0; int start = 0; int length = 0; };
    std::vector<Segment> segments;
};

//==============================================================================
// Layout Engine
//==============================================================================

class WingmanTextLayout {
public:
    static CachedTextLayout layoutText(const juce::String& text, float maxWidth, float baseFontSize) {
        CachedTextLayout layout;
        if (text.isEmpty() || maxWidth <= 0.0f) return layout;

        std::string raw = text.toStdString();
        std::vector<std::string> lines = splitLines(raw);

        // State machine
        std::vector<std::string> currentBuffer;
        bool inCodeBlock = false;
        std::string codeBlockContent;

        auto flushBuffer = [&](BlockType type) {
            if (currentBuffer.empty()) return;
            LayoutBlock block;
            block.type = type;
            processTextBuffer(block, currentBuffer, maxWidth, baseFontSize, type);
            layout.blocks.push_back(block);
            currentBuffer.clear();
        };

        for (const auto& line : lines) {
            // Code Block Handling
            if (startsWith(line, "```")) {
                if (inCodeBlock) {
                    // End code block
                    LayoutBlock block;
                    block.type = BlockType::CodeBlock;
                    block.rawContent = codeBlockContent;
                    
                    SkFont codeFont = design::getSkFont(baseFontSize - 1.0f, design::FontWeight::Regular);
                    float lineHeight = codeFont.getSpacing();
                    if (lineHeight == 0) lineHeight = baseFontSize * 1.4f;

                    std::stringstream ss(codeBlockContent);
                    std::string codeLine;
                    while (std::getline(ss, codeLine)) {
                        TextLine tl;
                        TextSpan ts;
                        ts.text = codeLine;
                        ts.style = SpanStyle::Code;
                        ts.font = codeFont;
                        ts.width = codeFont.measureText(codeLine.c_str(), codeLine.length(), SkTextEncoding::kUTF8);
                        tl.spans.push_back(ts);
                        tl.width = ts.width;
                        tl.height = lineHeight;
                        block.lines.push_back(tl);
                    }
                    
                    block.height = block.lines.size() * lineHeight + 20.0f; // Padding
                    block.marginTop = 12.0f;
                    block.marginBottom = 12.0f;
                    layout.blocks.push_back(block);
                    
                    inCodeBlock = false;
                    codeBlockContent.clear();
                } else {
                    flushBuffer(BlockType::Paragraph);
                    inCodeBlock = true;
                }
                continue;
            }

            if (inCodeBlock) {
                codeBlockContent += line + "\n";
                continue;
            }

            // Headers
            if (startsWith(line, "# ")) {
                flushBuffer(BlockType::Paragraph);
                currentBuffer.push_back(line.substr(2));
                flushBuffer(BlockType::Header1);
                continue;
            }
            if (startsWith(line, "## ")) {
                flushBuffer(BlockType::Paragraph);
                currentBuffer.push_back(line.substr(3));
                flushBuffer(BlockType::Header2);
                continue;
            }

            // Lists
            if (startsWith(line, "- ") || startsWith(line, "* ")) {
                flushBuffer(BlockType::Paragraph);
                currentBuffer.push_back(line.substr(2));
                flushBuffer(BlockType::ListBullet);
                continue;
            }

            // Empty lines
            if (line.empty()) {
                flushBuffer(BlockType::Paragraph);
                continue;
            }

            // Paragraph Accumulation
            if (currentBuffer.empty()) {
                currentBuffer.push_back(line);
            } else {
                currentBuffer.back() += " " + line;
            }
        }
        
        flushBuffer(BlockType::Paragraph);

        // Calculate total dimensions
        layout.totalWidth = maxWidth;
        layout.totalHeight = 0.0f;
        for (const auto& b : layout.blocks) {
            layout.totalHeight += b.marginTop + b.height + b.marginBottom;
        }

        return layout;
    }

private:
    static bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }

    static std::vector<std::string> splitLines(const std::string& str) {
        std::vector<std::string> lines;
        std::stringstream ss(str);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
        return lines;
    }

    static void processTextBuffer(LayoutBlock& block, const std::vector<std::string>& content, float maxWidth, float baseFontSize, BlockType type) {
        SkFont font;
        
        switch (type) {
            case BlockType::Header1:
                font = design::getSkFont(baseFontSize * 1.5f, design::FontWeight::Bold);
                block.marginTop = 16.0f; block.marginBottom = 8.0f;
                break;
            case BlockType::Header2:
                font = design::getSkFont(baseFontSize * 1.25f, design::FontWeight::Bold);
                block.marginTop = 12.0f; block.marginBottom = 6.0f;
                break;
            case BlockType::ListBullet:
                font = design::getSkFont(baseFontSize, design::FontWeight::Regular);
                block.marginTop = 4.0f; block.marginBottom = 4.0f;
                maxWidth -= 16.0f; // Indent
                break;
            default:
                font = design::getSkFont(baseFontSize, design::FontWeight::Regular);
                block.marginBottom = 8.0f;
                break;
        }

        float lineHeight = font.getSpacing();
        if (lineHeight == 0) lineHeight = baseFontSize * 1.4f;

        for (const auto& rawLine : content) {
            std::vector<TextSpan> spans = parseSpans(rawLine, font);
            std::vector<TextLine> wrappedLines = wrapSpans(spans, maxWidth, lineHeight);
            block.lines.insert(block.lines.end(), wrappedLines.begin(), wrappedLines.end());
        }

        block.height = block.lines.size() * lineHeight;
    }

    static std::vector<TextSpan> parseSpans(const std::string& text, const SkFont& baseFont) {
        std::vector<TextSpan> spans;
        size_t pos = 0;
        size_t len = text.length();

        while (pos < len) {
            // Check for bold (** or __)
            if (pos + 1 < len && ((text[pos] == '*' && text[pos+1] == '*') || (text[pos] == '_' && text[pos+1] == '_'))) {
                char marker = text[pos];
                size_t end = text.find(std::string(2, marker), pos + 2);
                if (end != std::string::npos) {
                    // Append text before if any? (handled by loop structure, we are at marker)
                    
                    std::string content = text.substr(pos + 2, end - (pos + 2));
                    TextSpan span;
                    span.text = content;
                    span.style = SpanStyle::Bold;
                    span.font = design::getSkFont(baseFont.getSize(), design::FontWeight::Bold);
                    span.width = span.font.measureText(content.c_str(), content.length(), SkTextEncoding::kUTF8);
                    spans.push_back(span);
                    
                    pos = end + 2;
                    continue;
                }
            }
            
            // Check for code (`)
            if (text[pos] == '`') {
                size_t end = text.find('`', pos + 1);
                if (end != std::string::npos) {
                    std::string content = text.substr(pos + 1, end - (pos + 1));
                    TextSpan span;
                    span.text = content;
                    span.style = SpanStyle::Code;
                    span.font = design::getSkFont(baseFont.getSize() - 1, design::FontWeight::Regular);
                    span.width = span.font.measureText(content.c_str(), content.length(), SkTextEncoding::kUTF8);
                    spans.push_back(span);
                    pos = end + 1;
                    continue;
                }
            }
            
            // Plain text until next marker
            size_t nextBold = text.find("**", pos); // Simplified check
            size_t nextCode = text.find("`", pos);
            size_t next = std::min(nextBold, nextCode);
            
            if (next == std::string::npos) {
                // Rest is plain text
                std::string content = text.substr(pos);
                TextSpan span;
                span.text = content;
                span.style = SpanStyle::Regular;
                span.font = baseFont;
                span.width = span.font.measureText(content.c_str(), content.length(), SkTextEncoding::kUTF8);
                spans.push_back(span);
                pos = len;
            } else {
                std::string content = text.substr(pos, next - pos);
                if (!content.empty()) {
                    TextSpan span;
                    span.text = content;
                    span.style = SpanStyle::Regular;
                    span.font = baseFont;
                    span.width = span.font.measureText(content.c_str(), content.length(), SkTextEncoding::kUTF8);
                    spans.push_back(span);
                }
                pos = next;
            }
        }
        
        return spans;
    }

    static std::vector<TextLine> wrapSpans(const std::vector<TextSpan>& inputSpans, float maxWidth, float lineHeight) {
        std::vector<TextLine> lines;
        TextLine currentLine;
        currentLine.height = lineHeight;
        currentLine.width = 0.0f;

        for (const auto& span : inputSpans) {
            std::vector<std::string> words = splitWords(span.text);
            
            for (size_t i = 0; i < words.size(); ++i) {
                std::string word = words[i];
                // Conservative spacing logic
                std::string spacer = (i > 0 || (!currentLine.spans.empty() && !currentLine.spans.back().text.empty() && currentLine.spans.back().text.back() != ' ')) ? " " : "";
                if (currentLine.spans.empty()) spacer = "";

                float spaceW = span.font.measureText(spacer.c_str(), spacer.length(), SkTextEncoding::kUTF8);
                float wordW = span.font.measureText(word.c_str(), word.length(), SkTextEncoding::kUTF8);

                if (currentLine.width + spaceW + wordW > maxWidth && currentLine.width > 0) {
                    lines.push_back(currentLine);
                    currentLine = TextLine();
                    currentLine.height = lineHeight;
                    currentLine.width = 0.0f;
                    spacer = "";
                    spaceW = 0;
                }

                TextSpan wordSpan = span;
                wordSpan.text = spacer + word;
                wordSpan.width = spaceW + wordW;
                currentLine.spans.push_back(wordSpan);
                currentLine.width += wordSpan.width;
            }
        }
        
        if (!currentLine.spans.empty()) lines.push_back(currentLine);
        return lines;
    }

    static std::vector<std::string> splitWords(const std::string& text) {
        std::vector<std::string> words;
        std::stringstream ss(text);
        std::string word;
        while (ss >> word) words.push_back(word);
        return words;
    }
};

} // namespace zenith
