/*
  ==============================================================================

    AIChatMessage.cpp
    Created: 2025-12-29
    Author:  Zenith DAW Team

    Complete implementation of AI Chat Message Bubble Component

  ==============================================================================
*/

#include "AIChatMessage.h"
#include "../framework/BackdropBlur.h"
#include "../design-system/ZenithDesignSystem.h"
#include <core/SkBlurTypes.h>
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
// Construction
//==============================================================================

AIChatMessage::AIChatMessage(const juce::String& message, 
                             ChatMessageType type,
                             const juce::Time& timestamp)
    : rawMessage_(message)
    , messageType_(type)
    , timestamp_(timestamp)
{
    // Parse the message into segments (text and code blocks)
    parseMessage();
    
    // Start timer for animations (thinking shimmer, long press detection)
    startTimerHz(60);
    
    // Enable mouse tracking for code block hover
    setWantsKeyboardFocus(false);
}

AIChatMessage::~AIChatMessage()
{
    stopTimer();
}

//==============================================================================
// State Management
//==============================================================================

void AIChatMessage::setThinking(bool thinking)
{
    if (isThinking_ != thinking) {
        isThinking_ = thinking;
        shimmerPhase_ = 0.0f;
        markDirty();
    }
}

void AIChatMessage::setMessage(const juce::String& message)
{
    if (rawMessage_ != message) {
        rawMessage_ = message;
        parseMessage();
        layoutDirty_ = true;
        markDirty();
    }
}

float AIChatMessage::calculatePreferredHeight(float availableWidth) const
{
    if (!layoutDirty_ && std::abs(cachedWidth_ - availableWidth) < 1.0f) {
        return cachedHeight_;
    }
    
    // Calculate max bubble width (80% of parent)
    float maxBubbleWidth = availableWidth * MAX_WIDTH_RATIO;
    float contentWidth = maxBubbleWidth - (2.0f * PADDING);
    
    // Measure content height
    float totalHeight = PADDING; // Top padding
    
    SkFont textFont = design::typography::getSkFont(14.0f, design::typography::FontWeight::Regular);
    SkFont monoFont = design::typography::getMonoFont(13.0f, design::typography::FontWeight::Regular);
    
    for (const auto& segment : segments_) {
        if (segment.type == MessageSegment::Type::CodeBlock) {
            // Code block has extra padding and dark background
            float codeHeight = measureTextHeight(segment.content, contentWidth - CODE_BLOCK_PADDING * 2, monoFont);
            totalHeight += codeHeight + CODE_BLOCK_PADDING * 2 + design::spacing::SM;
        } else {
            totalHeight += measureTextHeight(segment.content, contentWidth, textFont);
        }
    }
    
    totalHeight += PADDING; // Bottom padding
    
    // Add timestamp height
    totalHeight += TIMESTAMP_MARGIN + design::typography::FONT_XS + 2.0f;
    
    cachedWidth_ = availableWidth;
    cachedHeight_ = totalHeight;
    layoutDirty_ = false;
    
    return totalHeight;
}

//==============================================================================
// Message Parsing
//==============================================================================

void AIChatMessage::parseMessage()
{
    segments_ = parseMarkdown(rawMessage_);
    codeBlocks_.clear();
    layoutDirty_ = true;
}

std::vector<MessageSegment> AIChatMessage::parseMarkdown(const juce::String& text)
{
    std::vector<MessageSegment> result;
    
    // Simple parser for code blocks and inline code
    int pos = 0;
    int textLength = text.length();
    juce::String currentText;
    
    while (pos < textLength) {
        // Check for code block (```)
        if (pos + 2 < textLength && text.substring(pos, pos + 3) == "```") {
            // Save accumulated text
            if (currentText.isNotEmpty()) {
                MessageSegment textSeg;
                textSeg.type = MessageSegment::Type::Text;
                textSeg.content = currentText;
                result.push_back(textSeg);
                currentText.clear();
            }
            
            // Find the language specifier (if any) and end of code block
            int startPos = pos + 3;
            int langEnd = text.indexOfChar(startPos, '\n');
            if (langEnd < 0) langEnd = textLength;
            
            juce::String language = text.substring(startPos, langEnd).trim();
            
            int codeStart = langEnd + 1;
            int codeEnd = text.indexOf(codeStart, "```");
            if (codeEnd < 0) codeEnd = textLength;
            
            MessageSegment codeSeg;
            codeSeg.type = MessageSegment::Type::CodeBlock;
            codeSeg.language = language;
            codeSeg.content = text.substring(codeStart, codeEnd).trim();
            result.push_back(codeSeg);
            
            pos = (codeEnd < textLength) ? codeEnd + 3 : textLength;
        }
        // Check for inline code (`)
        else if (text[pos] == '`' && (pos + 1 >= textLength || text[pos + 1] != '`')) {
            // Save accumulated text
            if (currentText.isNotEmpty()) {
                MessageSegment textSeg;
                textSeg.type = MessageSegment::Type::Text;
                textSeg.content = currentText;
                result.push_back(textSeg);
                currentText.clear();
            }
            
            int codeEnd = text.indexOfChar(pos + 1, '`');
            if (codeEnd < 0) codeEnd = textLength;
            
            MessageSegment codeSeg;
            codeSeg.type = MessageSegment::Type::InlineCode;
            codeSeg.content = text.substring(pos + 1, codeEnd);
            result.push_back(codeSeg);
            
            pos = (codeEnd < textLength) ? codeEnd + 1 : textLength;
        }
        else {
            currentText += text[pos];
            pos++;
        }
    }
    
    // Don't forget the last text segment
    if (currentText.isNotEmpty()) {
        MessageSegment textSeg;
        textSeg.type = MessageSegment::Type::Text;
        textSeg.content = currentText;
        result.push_back(textSeg);
    }
    
    // If no segments created, create at least one with original text
    if (result.empty()) {
        MessageSegment textSeg;
        textSeg.type = MessageSegment::Type::Text;
        textSeg.content = text;
        result.push_back(textSeg);
    }
    
    return result;
}

//==============================================================================
// Main Rendering
//==============================================================================

void AIChatMessage::drawSkia(SkCanvas* canvas)
{
    auto localBounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(localBounds.getWidth(), localBounds.getHeight());
    
    // Calculate bubble bounds based on message type
    SkRect bubbleBounds = calculateBubbleBounds(skBounds.width());
    
    // Draw based on message type
    switch (messageType_) {
        case ChatMessageType::User:
            drawUserMessage(canvas, bubbleBounds);
            break;
        case ChatMessageType::AI:
            drawAIMessage(canvas, bubbleBounds);
            break;
        case ChatMessageType::System:
            drawSystemMessage(canvas, bubbleBounds);
            break;
        case ChatMessageType::Error:
            drawErrorMessage(canvas, bubbleBounds);
            break;
    }
    
    // Draw timestamp
    bool alignRight = (messageType_ == ChatMessageType::User);
    drawTimestamp(canvas, bubbleBounds, alignRight);
    
    // Draw thinking shimmer overlay if active
    if (isThinking_) {
        drawThinkingShimmer(canvas, bubbleBounds);
    }
}

SkRect AIChatMessage::calculateBubbleBounds(float parentWidth) const
{
    float maxWidth = parentWidth * MAX_WIDTH_RATIO;
    float height = cachedHeight_ - (TIMESTAMP_MARGIN + design::typography::FONT_XS + 2.0f);
    
    // Align based on message type
    float x = 0.0f;
    if (messageType_ == ChatMessageType::User) {
        // Right align
        x = parentWidth - maxWidth;
    } else if (messageType_ == ChatMessageType::System) {
        // Center align
        x = (parentWidth - maxWidth) / 2.0f;
    }
    // AI and Error are left aligned (x = 0)
    
    return SkRect::MakeXYWH(x, 0, maxWidth, height);
}

//==============================================================================
// User Message Drawing
//==============================================================================

void AIChatMessage::drawUserMessage(SkCanvas* canvas, const SkRect& bounds)
{
    using namespace design;
    
    // Solid BG_02 background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(colors::BG_02);
    
    SkRRect rrect = SkRRect::MakeRectXY(bounds, CORNER_RADIUS, CORNER_RADIUS);
    canvas->drawRRect(rrect, bgPaint);
    
    // Draw message content (right aligned)
    SkRect contentBounds = bounds;
    contentBounds.inset(PADDING, PADDING);
    drawMessageContent(canvas, contentBounds, true);
}

//==============================================================================
// AI Message Drawing
//==============================================================================

void AIChatMessage::drawAIMessage(SkCanvas* canvas, const SkRect& bounds)
{
    using namespace design;
    
    // Glassmorphic background (0.6 opacity, blur)
    BackdropBlur::drawBlurredPanel(
        canvas, bounds, CORNER_RADIUS,
        16.0f,                          // Blur radius
        colors::BG_02,                  // Tint color
        0.6f,                           // Tint opacity
        true                            // Draw highlight
    );
    
    // 1px CYAN border at 30% opacity
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(withAlpha(colors::CYAN, 0.30f));
    
    SkRRect borderRRect = SkRRect::MakeRectXY(bounds, CORNER_RADIUS, CORNER_RADIUS);
    borderRRect.inset(0.5f, 0.5f);
    canvas->drawRRect(borderRRect, borderPaint);
    
    // Draw message content (left aligned)
    SkRect contentBounds = bounds;
    contentBounds.inset(PADDING, PADDING);
    drawMessageContent(canvas, contentBounds, false);
}

//==============================================================================
// System Message Drawing
//==============================================================================

void AIChatMessage::drawSystemMessage(SkCanvas* canvas, const SkRect& bounds)
{
    using namespace design;
    
    // Subtle semi-transparent background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(withAlpha(colors::BG_03, 0.5f));
    
    SkRRect rrect = SkRRect::MakeRectXY(bounds, CORNER_RADIUS, CORNER_RADIUS);
    canvas->drawRRect(rrect, bgPaint);
    
    // Subtle border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(colors::BORDER_SUBTLE);
    
    SkRRect borderRRect = rrect;
    borderRRect.inset(0.5f, 0.5f);
    canvas->drawRRect(borderRRect, borderPaint);
    
    // Draw message content (centered)
    SkRect contentBounds = bounds;
    contentBounds.inset(PADDING, PADDING);
    drawMessageContent(canvas, contentBounds, false);
}

//==============================================================================
// Error Message Drawing
//==============================================================================

void AIChatMessage::drawErrorMessage(SkCanvas* canvas, const SkRect& bounds)
{
    using namespace design;
    
    // Semi-transparent red-tinted background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(withAlpha(colors::RED, 0.15f));
    
    SkRRect rrect = SkRRect::MakeRectXY(bounds, CORNER_RADIUS, CORNER_RADIUS);
    canvas->drawRRect(rrect, bgPaint);
    
    // Red border at 40% opacity
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(withAlpha(colors::RED, 0.40f));
    
    SkRRect borderRRect = rrect;
    borderRRect.inset(0.5f, 0.5f);
    canvas->drawRRect(borderRRect, borderPaint);
    
    // Draw message content
    SkRect contentBounds = bounds;
    contentBounds.inset(PADDING, PADDING);
    drawMessageContent(canvas, contentBounds, false);
}

//==============================================================================
// Content Rendering
//==============================================================================

void AIChatMessage::drawMessageContent(SkCanvas* canvas, const SkRect& contentBounds, bool alignRight)
{
    using namespace design;
    
    SkFont textFont = typography::getSkFont(14.0f, typography::FontWeight::Regular);
    SkFont monoFont = typography::getMonoFont(13.0f, typography::FontWeight::Regular);
    
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(colors::TEXT_PRIMARY);
    
    float yOffset = contentBounds.top();
    float contentWidth = contentBounds.width();
    int codeBlockIndex = 0;
    
    // Clear old code block bounds
    codeBlocks_.clear();
    
    for (const auto& segment : segments_) {
        if (segment.type == MessageSegment::Type::CodeBlock) {
            // Draw code block with dark background
            float codeHeight = measureTextHeight(segment.content, 
                contentWidth - CODE_BLOCK_PADDING * 2, monoFont);
            
            SkRect codeRect = SkRect::MakeXYWH(
                contentBounds.left(),
                yOffset,
                contentWidth,
                codeHeight + CODE_BLOCK_PADDING * 2
            );
            
            drawCodeBlock(canvas, codeRect, segment.content, segment.language, codeBlockIndex);
            
            // Store code block info for hover detection
            CodeBlockInfo info;
            info.bounds = juce::Rectangle<float>(
                codeRect.left(), codeRect.top(),
                codeRect.width(), codeRect.height()
            );
            info.code = segment.content;
            info.isHovered = (hoveredCodeBlockIndex_ == codeBlockIndex);
            codeBlocks_.push_back(info);
            
            yOffset += codeRect.height() + spacing::SM;
            codeBlockIndex++;
        }
        else if (segment.type == MessageSegment::Type::InlineCode) {
            // Inline code with subtle background
            SkFont inlineFont = typography::getMonoFont(13.0f, typography::FontWeight::Regular);
            
            // Measure text width
            SkRect textBounds;
            inlineFont.measureText(segment.content.toRawUTF8(), 
                segment.content.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, &textBounds);
            
            float inlineWidth = textBounds.width() + 8.0f; // Padding
            float inlineHeight = textBounds.height() + 4.0f;
            
            float x = alignRight ? (contentBounds.right() - inlineWidth) : contentBounds.left();
            
            // Background
            SkRect inlineRect = SkRect::MakeXYWH(x, yOffset, inlineWidth, inlineHeight);
            SkPaint bgPaint;
            bgPaint.setAntiAlias(true);
            bgPaint.setColor(withAlpha(colors::BG_00, 0.6f));
            canvas->drawRRect(SkRRect::MakeRectXY(inlineRect, 4.0f, 4.0f), bgPaint);
            
            // Text
            SkPaint codePaint;
            codePaint.setAntiAlias(true);
            codePaint.setColor(colors::CYAN);
            canvas->drawString(segment.content.toRawUTF8(), 
                x + 4.0f, yOffset + inlineHeight - 4.0f, inlineFont, codePaint);
            
            yOffset += inlineHeight + 4.0f;
        }
        else {
            // Regular text - simple line-by-line rendering
            juce::StringArray lines;
            lines.addLines(segment.content);
            
            for (const auto& line : lines) {
                if (line.isEmpty()) {
                    yOffset += textFont.getSize() * 0.5f; // Half line for empty lines
                    continue;
                }
                
                // Word wrap the line
                juce::StringArray words;
                words.addTokens(line, " ", "");
                
                juce::String currentLine;
                float lineHeight = textFont.getSize() * 1.4f;
                
                for (const auto& word : words) {
                    juce::String testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
                    
                    SkRect measureBounds;
                    textFont.measureText(testLine.toRawUTF8(), 
                        testLine.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, &measureBounds);
                    
                    if (measureBounds.width() > contentWidth && currentLine.isNotEmpty()) {
                        // Draw current line and start new one
                        float x = alignRight ? 
                            (contentBounds.right() - measureBounds.width()) : 
                            contentBounds.left();
                        
                        canvas->drawString(currentLine.toRawUTF8(),
                            contentBounds.left(), yOffset + textFont.getSize(), 
                            textFont, textPaint);
                        
                        yOffset += lineHeight;
                        currentLine = word;
                    } else {
                        currentLine = testLine;
                    }
                }
                
                // Draw remaining text
                if (currentLine.isNotEmpty()) {
                    canvas->drawString(currentLine.toRawUTF8(),
                        contentBounds.left(), yOffset + textFont.getSize(), 
                        textFont, textPaint);
                    yOffset += lineHeight;
                }
            }
        }
    }
}

void AIChatMessage::drawCodeBlock(SkCanvas* canvas, const SkRect& bounds, 
                                   const juce::String& code, 
                                   const juce::String& language, int index)
{
    using namespace design;
    
    // Dark background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(colors::BG_00);
    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 6.0f, 6.0f), bgPaint);
    
    // Subtle border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(colors::BORDER_SUBTLE);
    
    SkRRect borderRRect = SkRRect::MakeRectXY(bounds, 6.0f, 6.0f);
    borderRRect.inset(0.5f, 0.5f);
    canvas->drawRRect(borderRRect, borderPaint);
    
    // Language label (if present)
    float textStartY = bounds.top() + CODE_BLOCK_PADDING;
    if (language.isNotEmpty()) {
        SkFont labelFont = typography::getSkFont(10.0f, typography::FontWeight::Medium);
        SkPaint labelPaint;
        labelPaint.setAntiAlias(true);
        labelPaint.setColor(colors::TEXT_TERTIARY);
        
        canvas->drawString(language.toRawUTF8(), 
            bounds.left() + CODE_BLOCK_PADDING, 
            textStartY + labelFont.getSize(),
            labelFont, labelPaint);
        
        textStartY += labelFont.getSize() + 4.0f;
    }
    
    // Code text
    SkFont monoFont = typography::getMonoFont(13.0f, typography::FontWeight::Regular);
    SkPaint codePaint;
    codePaint.setAntiAlias(true);
    codePaint.setColor(colors::TEXT_PRIMARY);
    
    float lineHeight = monoFont.getSize() * 1.3f;
    juce::StringArray lines;
    lines.addLines(code);
    
    for (const auto& line : lines) {
        canvas->drawString(line.toRawUTF8(),
            bounds.left() + CODE_BLOCK_PADDING,
            textStartY + monoFont.getSize(),
            monoFont, codePaint);
        textStartY += lineHeight;
    }
    
    // Copy button (appears on hover)
    if (index >= 0 && index < static_cast<int>(codeBlocks_.size()) && 
        codeBlocks_[static_cast<size_t>(index)].isHovered) {
        SkRect buttonBounds = SkRect::MakeXYWH(
            bounds.right() - 60.0f,
            bounds.top() + 4.0f,
            56.0f,
            24.0f
        );
        drawCopyButton(canvas, buttonBounds, copyButtonHovered_);
    }
}

void AIChatMessage::drawTimestamp(SkCanvas* canvas, const SkRect& messageBounds, bool alignRight)
{
    using namespace design;
    
    juce::String timeStr = timestamp_.toString(true, true, false, true);
    
    SkFont timestampFont = typography::getSkFont(typography::FONT_XS, typography::FontWeight::Regular);
    SkPaint timestampPaint;
    timestampPaint.setAntiAlias(true);
    timestampPaint.setColor(colors::TEXT_TERTIARY);
    
    float y = messageBounds.bottom() + TIMESTAMP_MARGIN + typography::FONT_XS;
    
    if (alignRight) {
        // Measure to right-align
        SkRect textBounds;
        timestampFont.measureText(timeStr.toRawUTF8(), 
            timeStr.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, &textBounds);
        
        canvas->drawString(timeStr.toRawUTF8(),
            messageBounds.right() - textBounds.width(),
            y, timestampFont, timestampPaint);
    } else {
        canvas->drawString(timeStr.toRawUTF8(),
            messageBounds.left(), y, timestampFont, timestampPaint);
    }
}

void AIChatMessage::drawThinkingShimmer(SkCanvas* canvas, const SkRect& bounds)
{
    using namespace design;
    
    // Animated gradient shimmer overlay
    SkPaint shimmerPaint;
    shimmerPaint.setAntiAlias(true);
    shimmerPaint.setBlendMode(SkBlendMode::kSrcOver);
    
    // Create moving gradient
    float shimmerWidth = bounds.width() * 0.4f;
    float shimmerX = bounds.left() + (bounds.width() + shimmerWidth) * shimmerPhase_ - shimmerWidth;
    
    SkPoint gradPoints[2] = {
        {shimmerX, bounds.centerY()},
        {shimmerX + shimmerWidth, bounds.centerY()}
    };
    
    SkColor gradColors[3] = {
        0x00FFFFFF,                           // Transparent
        withAlpha(colors::CYAN, 0.3f),        // Cyan highlight
        0x00FFFFFF                            // Transparent
    };
    SkScalar positions[3] = {0.0f, 0.5f, 1.0f};
    
    shimmerPaint.setShader(SkGradientShader::MakeLinear(
        gradPoints, gradColors, positions, 3, SkTileMode::kClamp));
    
    SkRRect shimmerRRect = SkRRect::MakeRectXY(bounds, CORNER_RADIUS, CORNER_RADIUS);
    canvas->drawRRect(shimmerRRect, shimmerPaint);
}

void AIChatMessage::drawCopyButton(SkCanvas* canvas, const SkRect& bounds, bool isHovered)
{
    using namespace design;
    
    // Button background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(isHovered ? withAlpha(colors::CYAN, 0.3f) : withAlpha(colors::BG_03, 0.9f));
    canvas->drawRRect(SkRRect::MakeRectXY(bounds, 4.0f, 4.0f), bgPaint);
    
    // Button border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(isHovered ? colors::CYAN : colors::BORDER_DEFAULT);
    
    SkRRect borderRRect = SkRRect::MakeRectXY(bounds, 4.0f, 4.0f);
    borderRRect.inset(0.5f, 0.5f);
    canvas->drawRRect(borderRRect, borderPaint);
    
    // "Copy" text
    SkFont buttonFont = typography::getSkFont(11.0f, typography::FontWeight::Medium);
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(isHovered ? colors::CYAN : colors::TEXT_SECONDARY);
    
    canvas->drawString("Copy", 
        bounds.centerX() - 14.0f,
        bounds.centerY() + 4.0f,
        buttonFont, textPaint);
}

//==============================================================================
// Helper Methods
//==============================================================================

float AIChatMessage::measureTextHeight(const juce::String& text, float maxWidth, const SkFont& font) const
{
    if (text.isEmpty()) return font.getSize();
    
    float lineHeight = font.getSize() * 1.4f;
    float totalHeight = 0.0f;
    
    juce::StringArray lines;
    lines.addLines(text);
    
    for (const auto& line : lines) {
        if (line.isEmpty()) {
            totalHeight += lineHeight * 0.5f;
            continue;
        }
        
        // Word wrap estimation
        juce::StringArray words;
        words.addTokens(line, " ", "");
        
        juce::String currentLine;
        int lineCount = 0;
        
        for (const auto& word : words) {
            juce::String testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
            
            SkRect measureBounds;
            font.measureText(testLine.toRawUTF8(), 
                testLine.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, &measureBounds);
            
            if (measureBounds.width() > maxWidth && currentLine.isNotEmpty()) {
                lineCount++;
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }
        
        if (currentLine.isNotEmpty()) {
            lineCount++;
        }
        
        totalHeight += lineCount * lineHeight;
    }
    
    return totalHeight;
}

void AIChatMessage::copyToClipboard(const juce::String& text)
{
    juce::SystemClipboard::copyTextToClipboard(text);
    
    if (onCopy) {
        onCopy(text);
    }
}

void AIChatMessage::handleLongPress()
{
    if (!longPressTriggered_) {
        longPressTriggered_ = true;
        copyToClipboard(rawMessage_);
    }
}

//==============================================================================
// Mouse Events
//==============================================================================

void AIChatMessage::mouseDown(const juce::MouseEvent& event)
{
    SkiaComponent::mouseDown(event);
    
    isPressed_ = true;
    pressStartTime_ = juce::Time::getMillisecondCounter();
    longPressTriggered_ = false;
    
    // Check if clicking on a code block copy button
    if (hoveredCodeBlockIndex_ >= 0 && 
        hoveredCodeBlockIndex_ < static_cast<int>(codeBlocks_.size())) {
        
        auto& block = codeBlocks_[static_cast<size_t>(hoveredCodeBlockIndex_)];
        SkRect buttonBounds = SkRect::MakeXYWH(
            block.bounds.getRight() - 60.0f,
            block.bounds.getY() + 4.0f,
            56.0f,
            24.0f
        );
        
        auto pos = event.position;
        if (pos.x >= buttonBounds.left() && pos.x <= buttonBounds.right() &&
            pos.y >= buttonBounds.top() && pos.y <= buttonBounds.bottom()) {
            
            copyToClipboard(block.code);
            if (onCodeCopy) {
                onCodeCopy(block.code);
            }
            isPressed_ = false;
        }
    }
}

void AIChatMessage::mouseUp(const juce::MouseEvent& event)
{
    SkiaComponent::mouseUp(event);
    isPressed_ = false;
    longPressTriggered_ = false;
}

void AIChatMessage::mouseDrag(const juce::MouseEvent& event)
{
    SkiaComponent::mouseDrag(event);
    
    // Cancel long press if dragged too far
    if (isPressed_ && event.getDistanceFromDragStart() > 10.0f) {
        isPressed_ = false;
    }
}

void AIChatMessage::mouseMove(const juce::MouseEvent& event)
{
    SkiaComponent::mouseMove(event);
    
    // Check code block hover
    auto pos = event.position;
    int newHoveredIndex = -1;
    
    for (size_t i = 0; i < codeBlocks_.size(); ++i) {
        auto& block = codeBlocks_[i];
        if (pos.x >= block.bounds.getX() && 
            pos.x <= block.bounds.getRight() &&
            pos.y >= block.bounds.getY() && 
            pos.y <= block.bounds.getBottom()) {
            
            newHoveredIndex = static_cast<int>(i);
            break;
        }
    }
    
    if (newHoveredIndex != hoveredCodeBlockIndex_) {
        hoveredCodeBlockIndex_ = newHoveredIndex;
        markDirty();
    }
    
    // Check copy button hover
    if (hoveredCodeBlockIndex_ >= 0 && 
        hoveredCodeBlockIndex_ < static_cast<int>(codeBlocks_.size())) {
        
        auto& block = codeBlocks_[static_cast<size_t>(hoveredCodeBlockIndex_)];
        SkRect buttonBounds = SkRect::MakeXYWH(
            block.bounds.getRight() - 60.0f,
            block.bounds.getY() + 4.0f,
            56.0f,
            24.0f
        );
        
        bool newButtonHovered = (pos.x >= buttonBounds.left() && pos.x <= buttonBounds.right() &&
                                  pos.y >= buttonBounds.top() && pos.y <= buttonBounds.bottom());
        
        if (newButtonHovered != copyButtonHovered_) {
            copyButtonHovered_ = newButtonHovered;
            markDirty();
        }
    }
}

void AIChatMessage::mouseExit(const juce::MouseEvent& event)
{
    SkiaComponent::mouseExit(event);
    
    if (hoveredCodeBlockIndex_ >= 0) {
        hoveredCodeBlockIndex_ = -1;
        copyButtonHovered_ = false;
        markDirty();
    }
}

//==============================================================================
// Timer
//==============================================================================

void AIChatMessage::timerCallback()
{
    // Handle thinking shimmer animation
    if (isThinking_) {
        shimmerPhase_ += 0.02f;
        if (shimmerPhase_ > 1.0f) {
            shimmerPhase_ = 0.0f;
        }
        markDirty();
    }
    
    // Handle long press detection
    if (isPressed_ && !longPressTriggered_) {
        juce::int64 elapsed = juce::Time::getMillisecondCounter() - pressStartTime_;
        if (elapsed >= LONG_PRESS_MS) {
            handleLongPress();
        }
    }
    
    // Call base timer callback
    SkiaComponent::timerCallback();
}

} // namespace zenith
