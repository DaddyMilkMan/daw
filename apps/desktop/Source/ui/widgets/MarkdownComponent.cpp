/*
  ==============================================================================

    MarkdownComponent.cpp
    Created: 2025-12-17
    Author:  Zenith DAW

  ==============================================================================
*/

#include "MarkdownComponent.h"
#include <sstream>

namespace zenith {
namespace widgets {

MarkdownComponent::MarkdownComponent() {
  setOpaque(true);
}

MarkdownComponent::~MarkdownComponent() {}

void MarkdownComponent::setMarkdown(const juce::String &markdownText) {
  rawMarkdown_ = markdownText;
  parseMarkdown();
  layoutContent(getWidth());
  repaint();
}

void MarkdownComponent::appendMarkdown(const juce::String &markdownText) {
  rawMarkdown_ += markdownText;
  parseMarkdown();
  layoutContent(getWidth());
  repaint();
}

void MarkdownComponent::clear() {
  rawMarkdown_.clear();
  tokens_.clear();
  renderLines_.clear();
  totalHeight_ = 0.0f;
  repaint();
}

void MarkdownComponent::updateFonts() {
  using namespace zenith::design;
  fontBody_ = typography::getSkFont(typography::FONT_MD, FontWeight::Regular);
  fontH1_ = typography::getSkFont(typography::FONT_XL, FontWeight::Bold);
  fontH2_ = typography::getSkFont(typography::FONT_LG, FontWeight::Bold);
  
  // Use JetBrains Mono for code
  fontCode_ = FontManager::getInstance().getMonoFont(typography::FONT_SM, FontWeight::Regular);
}

void MarkdownComponent::paint(juce::Graphics &g) {
  // JUCE paint is empty because we use drawSkia
  // usage of this component implies it's added to a Skia-aware parent or renderer
}

void MarkdownComponent::resized() {
  layoutContent(getWidth());
}

void MarkdownComponent::parseMarkdown() {
  tokens_.clear();
  
  // Simple line-based parser
  std::stringstream ss(rawMarkdown_.toStdString());
  std::string line;
  bool inCodeBlock = false;

  while (std::getline(ss, line)) {
    juce::String jLine(line.c_str()); // Convert back to JUCE string for easy handling

    // Handle Code Blocks
    if (jLine.trim().startsWith("```")) {
      inCodeBlock = !inCodeBlock;
      continue;
    }

    if (inCodeBlock) {
      Token token;
      token.type = Token::Type::CodeBlock;
      token.content = jLine;
      tokens_.push_back(token);
      continue;
    }

    // Handle Headers
    if (jLine.startsWith("# ")) {
      Token token;
      token.type = Token::Type::Header1;
      token.content = jLine.substring(2);
      tokens_.push_back(token);
    } else if (jLine.startsWith("## ")) {
      Token token;
      token.type = Token::Type::Header2;
      token.content = jLine.substring(3);
      tokens_.push_back(token);
    } else {
      // Normal text - Todo: formatting *bold* within line
      // For now, treat whole line as text, but we can iterate to find **
      
      Token token;
      token.type = Token::Type::Paragraph;
      
      // Simple bold detection at start for now (e.g. **User:**)
      if (jLine.trim().startsWith("**") && jLine.contains("**", 2)) {
         // This is a naive split
         token.content = jLine; 
         // Real parser would split into multiple inline tokens
      } else {
         token.content = jLine;
      }
      
      tokens_.push_back(token);
    }
  }
}

void MarkdownComponent::layoutContent(float width) {
  if (width <= 0) return;
  updateFonts();

  renderLines_.clear();
  float currentY = 10.0f; // Padding top
  float xPadding = 10.0f;
  float contentWidth = width - (xPadding * 2);

  for (const auto& token : tokens_) {
    SkFont* font = &fontBody_;
    SkColor color = design::colors::TEXT_PRIMARY;
    float lineHeight = 20.0f; // Default
    float marginTop = 4.0f;
    bool isCode = false;

    switch (token.type) {
      case Token::Type::Header1:
        font = &fontH1_;
        lineHeight = 40.0f;
        marginTop = 16.0f;
        color = design::colors::NEON_CYAN;
        break;
      case Token::Type::Header2:
        font = &fontH2_;
        lineHeight = 32.0f;
        marginTop = 12.0f;
        color = design::colors::TEXT_PRIMARY; // Bright white
        break;
      case Token::Type::CodeBlock:
        font = &fontCode_;
        lineHeight = 18.0f;
        color = design::colors::NEON_GREEN; // Matrix style code
        isCode = true;
        break;
      default:
        font = &fontBody_;
        lineHeight = 22.0f;
        color = design::colors::TEXT_SECONDARY;
        break;
    }

    currentY += marginTop;

    // Word Wrap
    // Naive split by space
    juce::StringArray words;
    words.addTokens(token.content, " ", "");
    
    juce::String currentLineStr;
    float currentLineWidth = 0.0f;
    
    for (auto& word : words) {
       std::string wordStd = (word + " ").toStdString(); // Add space back
       float wordWidth = font->measureText(wordStd.c_str(), wordStd.length(), SkTextEncoding::kUTF8);
       
       if (currentLineWidth + wordWidth > contentWidth) {
           // Push line
           RenderLine line;
           line.text = currentLineStr;
           line.font = *font;
           line.color = color;
           line.x = xPadding;
           line.y = currentY + lineHeight; // Baseline
           line.isCodeBlockBackground = isCode;
           
           if (isCode) {
               line.backgroundRect = SkRect::MakeXYWH(xPadding - 4, currentY, contentWidth + 8, lineHeight);
           }
           
           renderLines_.push_back(line);
           
           currentY += lineHeight;
           currentLineStr = word + " "; // Start new line
           currentLineWidth = wordWidth;
       } else {
           currentLineStr += (word + " ");
           currentLineWidth += wordWidth;
       }
    }
    
    // Push remaining
    if (currentLineStr.isNotEmpty()) {
       RenderLine line;
       line.text = currentLineStr;
       line.font = *font;
       line.color = color;
       line.x = xPadding;
       line.y = currentY + lineHeight; // Baseline
       line.isCodeBlockBackground = isCode;
           
       if (isCode) {
           line.backgroundRect = SkRect::MakeXYWH(xPadding - 4, currentY, contentWidth + 8, lineHeight);
       }
       
       renderLines_.push_back(line);
       currentY += lineHeight;
    }
  }
  
  totalHeight_ = currentY + 20.0f; // Padding bottom
  
  // Resize component to fit content? 
  // WingmanPanel uses a viewport probably, so we should set bounds there.
  // But we can't resize ourselves relative to parent here easily without looping.
}

void MarkdownComponent::drawSkia(SkCanvas *canvas) {
    // Background is transparent (handled by parent or setOpaque)
    
    for (const auto& line : renderLines_) {
        // Draw background if needed (Code blocks)
        if (line.isCodeBlockBackground) {
             SkPaint bgPaint;
             bgPaint.setColor(SkColorSetARGB(40, 0, 0, 0)); // Darker background
             canvas->drawRect(line.backgroundRect, bgPaint);
        }
        
        SkPaint paint;
        paint.setColor(line.color);
        paint.setAntiAlias(true);
        
        canvas->drawString(line.text.toStdString().c_str(), line.x, line.y, line.font, paint);
    }
}

} // namespace widgets
} // namespace zenith
