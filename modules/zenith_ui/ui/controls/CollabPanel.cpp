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
    CollabPanel.cpp
    Created: 25 Dec 2025
    Author:  Zenith DAW
  ==============================================================================
*/


#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>

namespace zenith {

CollabPanel::CollabPanel() {
    setWantsKeyboardFocus(true);
    CollaborationManager::getInstance().addChangeListener(this);
    setSize(350, 600);
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(30); // Animation updates
}

CollabPanel::~CollabPanel() {
    CollaborationManager::getInstance().removeChangeListener(this);
}

void CollabPanel::changeListenerCallback(juce::ChangeBroadcaster*) {
    markDirty();
}

void CollabPanel::timerCallback() {
    auto state = CollaborationManager::getInstance().getState();
    if (state != CollaborationManager::ConnectionState::Disconnected &&
        state != CollaborationManager::ConnectionState::Error) {
        markDirty(); // Continuous repaint for animations
    }
}

void CollabPanel::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    SkPaint paint;
    paint.setAntiAlias(true);

    // --- Background with glassmorphism ---
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(230, 25, 25, 35));
    canvas->drawRoundRect(SkRect::MakeWH(w, h), design::dimensions::RADIUS_LG, design::dimensions::RADIUS_LG, bgPaint);

    // Subtle border glow
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(design::unified::border_subtle());
    canvas->drawRoundRect(SkRect::MakeWH(w, h), design::dimensions::RADIUS_LG, design::dimensions::RADIUS_LG, borderPaint);

    // --- Title ---
    SkFont titleFont;
    titleFont.setSize(22.0f);
    paint.setColor(design::unified::text_primary());
    
    // Dynamic padding
    float padding = 20.0f;
    float currentY = 35.0f;
    
    canvas->drawString("Real-Time Collaboration", padding, currentY, titleFont, paint);

    // Subtitle
    SkFont subFont;
    subFont.setSize(11.0f);
    paint.setColor(design::unified::text_secondary());
    canvas->drawString("P2P • UDP Hole Punching • Global", padding, currentY + 17.0f, subFont, paint);

    auto& mgr = CollaborationManager::getInstance();
    auto state = mgr.getState();

    float yOffset = 70.0f;

    // --- Name Input Section ---
    drawSection(canvas, "Your Name", yOffset, w);
    yOffset += 25.0f;
    drawTextField(canvas, nameInputBounds_, userName_, "Enter your name...",
                  activeField_ == Field::Name);
    yOffset += 50.0f;

    // --- Security Warning ---
    SkFont warningFont;
    warningFont.setSize(11.0f);
    paint.setColor(design::unified::warning());
    const char* warningIcon = "⚠️";
    const char* warningText = " Warning: Connection is not encrypted.";
    const char* warningText2 = "Do not use on untrusted networks.";
    canvas->drawString(warningIcon, padding, yOffset, warningFont, paint);
    canvas->drawString(warningText, padding + 20, yOffset, warningFont, paint);
    yOffset += 15.0f;
    canvas->drawString(warningText2, padding + 20, yOffset, warningFont, paint);
    yOffset += 25.0f;

    // --- Host Section ---
    drawSection(canvas, "Host Session", yOffset, w);
    yOffset += 25.0f;
    drawButton(canvas, hostButtonBounds_, "Start Hosting",
               state == CollaborationManager::ConnectionState::Hosting ||
                   state == CollaborationManager::ConnectionState::Punching,
               design::unified::accent_primary());

    if (state == CollaborationManager::ConnectionState::Hosting ||
        state == CollaborationManager::ConnectionState::Connected) {
        // Show code
        yOffset += 45.0f;
        SkFont codeFont;
        codeFont.setSize(36.0f);
        paint.setColor(design::unified::success());
        juce::String code = mgr.getCurrentCode();
        float codeWidth = codeFont.measureText(code.toRawUTF8(), code.length(),
                                               SkTextEncoding::kUTF8);
        canvas->drawString(code.toRawUTF8(), (w - codeWidth) / 2.0f, yOffset,
                           codeFont, paint);

        // Copy hint
        yOffset += 20.0f;
        paint.setColor(design::unified::text_tertiary());
        subFont.setSize(10.0f);
        canvas->drawString("Click code to copy", (w - 100.0f) / 2.0f, yOffset,
                           subFont, paint);
    }
    yOffset += 50.0f;

    // --- Join Section ---
    drawSection(canvas, "Join Session", yOffset, w);
    yOffset += 25.0f;
    drawTextField(canvas, codeInputBounds_, sessionCode_, "Enter 4-digit code",
                  activeField_ == Field::Code);
    yOffset += 45.0f;
    drawButton(canvas, joinButtonBounds_, "Join",
               state == CollaborationManager::ConnectionState::Connecting,
               design::unified::accent_secondary());
    yOffset += 50.0f;

    // --- Status ---
    drawStatusIndicator(canvas, state, yOffset, w);

    // --- Connected Users ---
    if (state == CollaborationManager::ConnectionState::Connected) {
        yOffset += 40.0f;
        drawSection(canvas, "Connected Users", yOffset, w);
        yOffset += 25.0f;

        const auto& users = mgr.getRemoteUsers();
        for (const auto& user : users) {
            if (user.isOnline) {
                // User pill
                SkPaint userPaint;
                userPaint.setColor(SkColorSetARGB(50, 0, 255, 200));
                canvas->drawRoundRect(
                    SkRect::MakeXYWH(20.0f, yOffset, w - 40.0f, 24.0f), design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM,
                    userPaint);

                // Online dot
                paint.setColor(design::unified::success());
                canvas->drawCircle(32.0f, yOffset + 12.0f, 4.0f, paint);

                // Name
                paint.setColor(design::unified::text_primary());
                subFont.setSize(12.0f);
                canvas->drawString(user.name.toRawUTF8(), 45.0f, yOffset + 16.0f,
                                   subFont, paint);
                yOffset += 30.0f;
            }
        }

        // Disconnect button
        yOffset += 10.0f;
        drawButton(canvas, disconnectButtonBounds_, "Disconnect", false,
                   design::unified::error());
    }
}

void CollabPanel::mouseDown(const juce::MouseEvent& e) {
    auto pos = e.position;

    // Check text fields
    if (nameInputBounds_.contains(pos.x, pos.y)) {
        activeField_ = Field::Name;
        grabKeyboardFocus();
        markDirty();
        return;
    }
    if (codeInputBounds_.contains(pos.x, pos.y)) {
        activeField_ = Field::Code;
        grabKeyboardFocus();
        markDirty();
        return;
    }

    // Check buttons
    if (hostButtonBounds_.contains(pos.x, pos.y)) {
        CollaborationManager::getInstance().setLocalUserName(userName_);
        CollaborationManager::getInstance().startHosting();
        return;
    }
    if (joinButtonBounds_.contains(pos.x, pos.y)) {
        CollaborationManager::getInstance().setLocalUserName(userName_);
        CollaborationManager::getInstance().joinSession(sessionCode_);
        return;
    }
    if (disconnectButtonBounds_.contains(pos.x, pos.y)) {
        CollaborationManager::getInstance().disconnect();
        return;
    }

    // Click on code to copy
    auto& mgr = CollaborationManager::getInstance();
    if (mgr.getState() == CollaborationManager::ConnectionState::Hosting ||
        mgr.getState() == CollaborationManager::ConnectionState::Connected) {
        juce::SystemClipboard::copyTextToClipboard(mgr.getCurrentCode());
    }

    activeField_ = Field::None;
    markDirty();
}

bool CollabPanel::keyPressed(const juce::KeyPress& key) {
    if (activeField_ == Field::None)
        return false;

    if (key == juce::KeyPress::backspaceKey) {
        if (activeField_ == Field::Name && userName_.isNotEmpty()) {
            userName_ = userName_.dropLastCharacters(1);
        } else if (activeField_ == Field::Code && sessionCode_.isNotEmpty()) {
            sessionCode_ = sessionCode_.dropLastCharacters(1);
        }
        markDirty();
        return true;
    }

    if (key == juce::KeyPress::returnKey) {
        if (activeField_ == Field::Code && sessionCode_.length() == 4) {
            CollaborationManager::getInstance().setLocalUserName(userName_);
            CollaborationManager::getInstance().joinSession(sessionCode_);
        }
        activeField_ = Field::None;
        markDirty();
        return true;
    }

    juce::juce_wchar c = key.getTextCharacter();
    if (c != 0) {
        if (activeField_ == Field::Name && userName_.length() < 20) {
            userName_ += juce::String::charToString(c);
        } else if (activeField_ == Field::Code && sessionCode_.length() < 4 &&
                   c >= '0' && c <= '9') {
            sessionCode_ += juce::String::charToString(c);
        }
        markDirty();
        return true;
    }

    return false;
}

void CollabPanel::resized() {
    float w = (float)getWidth();

    // Calculate bounds for interactive elements
    nameInputBounds_ = SkRect::MakeXYWH(20.0f, 95.0f, w - 40.0f, 32.0f);
    hostButtonBounds_ = SkRect::MakeXYWH(20.0f, 175.0f, w - 40.0f, 36.0f);
    codeInputBounds_ = SkRect::MakeXYWH(20.0f, 295.0f, w - 40.0f, 32.0f);
    joinButtonBounds_ = SkRect::MakeXYWH(20.0f, 340.0f, w - 40.0f, 36.0f);
    disconnectButtonBounds_ = SkRect::MakeXYWH(20.0f, 480.0f, w - 40.0f, 32.0f);
}

void CollabPanel::drawSection(SkCanvas* canvas, const char* title, float y, float w) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(design::unified::text_tertiary());

    SkFont font;
    font.setSize(12.0f);
    canvas->drawString(title, 20.0f, y, font, paint);

    // Underline
    paint.setColor(design::unified::border_subtle());
    canvas->drawLine(20.0f, y + 5.0f, w - 20.0f, y + 5.0f, paint);
}

void CollabPanel::drawTextField(SkCanvas* canvas, const SkRect& bounds,
                               const juce::String& text, const char* placeholder,
                               bool focused) {
    SkPaint paint;
    paint.setAntiAlias(true);

    // Background
    paint.setColor(focused ? design::unified::withAlpha(design::unified::accent_primary(), 0.3f)
                           : design::unified::bg_02());
    canvas->drawRoundRect(bounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, paint);

    // Border
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(focused ? 2.0f : 1.0f);
    paint.setColor(focused ? design::unified::accent_primary()
                           : design::unified::border_default());
    canvas->drawRoundRect(bounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, paint);

    // Text
    SkFont font;
    font.setSize(14.0f);
    paint.setStyle(SkPaint::kFill_Style);

    if (text.isEmpty()) {
        paint.setColor(design::unified::text_tertiary());
        canvas->drawString(placeholder, bounds.fLeft + 10.0f, bounds.fTop + 21.0f,
                           font, paint);
    } else {
        paint.setColor(design::unified::text_primary());
        canvas->drawString(text.toRawUTF8(), bounds.fLeft + 10.0f,
                           bounds.fTop + 21.0f, font, paint);
    }

    // Cursor
    if (focused) {
        float cursorX = bounds.fLeft + 10.0f +
                        font.measureText(text.toRawUTF8(), text.length(),
                                         SkTextEncoding::kUTF8);
        paint.setColor(design::unified::accent_primary());
        canvas->drawLine(cursorX, bounds.fTop + 8.0f, cursorX,
                         bounds.fTop + 24.0f, paint);
    }
}

void CollabPanel::drawButton(SkCanvas* canvas, const SkRect& bounds, const char* label,
                            bool active, SkColor accentColor) {
    SkPaint paint;
    paint.setAntiAlias(true);

    // Gradient background
    SkColor colors[2] = {active ? accentColor
                                : design::unified::withAlpha(accentColor, 0.2f),
                         active ? design::unified::withAlpha(accentColor, 0.7f)
                                : design::unified::bg_00()};
    SkPoint pts[2] = {SkPoint::Make(bounds.fLeft, bounds.fTop),
                      SkPoint::Make(bounds.fLeft, bounds.fBottom)};
    paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                 SkTileMode::kClamp));
    canvas->drawRoundRect(bounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, paint);
    paint.setShader(nullptr);

    // Glow when active
    if (active) {
        SkPaint glowPaint;
        glowPaint.setColor(accentColor);
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
        glowPaint.setAlpha(100);
        canvas->drawRoundRect(bounds, design::dimensions::RADIUS_SM, design::dimensions::RADIUS_SM, glowPaint);
    }

    // Label
    SkFont font;
    font.setSize(14.0f);
    paint.setColor(active ? design::unified::text_inverse() : design::unified::text_primary());
    float textWidth =
        font.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, bounds.centerX() - textWidth / 2.0f,
                       bounds.centerY() + 5.0f, font, paint);
}

void CollabPanel::drawStatusIndicator(SkCanvas* canvas,
                                     CollaborationManager::ConnectionState state, float y,
                                     float w) {
    SkPaint paint;
    paint.setAntiAlias(true);

    juce::String statusText;
    SkColor statusColor;

    switch (state) {
        case CollaborationManager::ConnectionState::Disconnected:
            statusText = "Offline";
            statusColor = design::unified::text_tertiary();
            break;
        case CollaborationManager::ConnectionState::Registering:
            statusText = "Connecting to Server...";
            statusColor = design::unified::warning();
            break;
        case CollaborationManager::ConnectionState::Connecting:
            statusText = "Connecting to Peer...";
            statusColor = SkColorSetRGB(255, 150, 50);
            break;
        case CollaborationManager::ConnectionState::Punching:
            statusText = "Punching Firewall...";
            statusColor = design::unified::accent_secondary();
            break;
        case CollaborationManager::ConnectionState::Handshaking:
            statusText = "Authenticating Peer...";
            statusColor = design::unified::info();
            break;
        case CollaborationManager::ConnectionState::Hosting:
            statusText = "Hosting - Waiting for peers";
            statusColor = design::unified::accent_primary();
            break;
        case CollaborationManager::ConnectionState::Connected:
            statusText = "Connected!";
            statusColor = design::unified::success();
            break;
        case CollaborationManager::ConnectionState::Error:
            statusText = "Connection Failed";
            statusColor = design::unified::error();
            break;
        default:
            statusText = "Unknown";
            statusColor = design::unified::text_tertiary();
    }

    // Status dot with pulse animation
    float pulse = 1.0f + 0.2f * sin(juce::Time::currentTimeMillis() / 200.0f);
    paint.setColor(statusColor);
    canvas->drawCircle(30.0f, y, 5.0f * pulse, paint);

    // Status text
    SkFont font;
    font.setSize(13.0f);
    canvas->drawString(statusText.toRawUTF8(), 45.0f, y + 4.0f, font, paint);
}

} // namespace zenith
