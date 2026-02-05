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
    CollabPanel.h
    Skia-based Real-Time Collaboration UI Panel
  ==============================================================================
*/


#include <juce_core/juce_core.h>

namespace zenith {

class CollabPanel : public SkiaComponent, public juce::ChangeListener {
public:
    CollabPanel();
    ~CollabPanel() override;

    void changeListenerCallback(juce::ChangeBroadcaster *) override;
    void timerCallback() override;
    void drawSkia(SkCanvas *canvas) override;

    void mouseDown(const juce::MouseEvent &e) override;
    bool keyPressed(const juce::KeyPress &key) override;
    void resized() override;

private:
    enum class Field { None, Name, Code };
    Field activeField_ = Field::None;

    juce::String userName_ = "User";
    juce::String sessionCode_;

    SkRect nameInputBounds_;
    SkRect hostButtonBounds_;
    SkRect codeInputBounds_;
    SkRect joinButtonBounds_;
    SkRect disconnectButtonBounds_;

    void drawSection(SkCanvas *canvas, const char *title, float y, float w);
    void drawTextField(SkCanvas *canvas, const SkRect &bounds,
                     const juce::String &text, const char *placeholder,
                     bool focused);
    void drawButton(SkCanvas *canvas, const SkRect &bounds, const char *label,
                  bool active, SkColor accentColor);
    void drawStatusIndicator(SkCanvas *canvas,
                           CollaborationManager::ConnectionState state, float y,
                           float w);
};

} // namespace zenith
