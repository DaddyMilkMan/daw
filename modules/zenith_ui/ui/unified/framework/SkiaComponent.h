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

#pragma once

// SkiaComponent.h


#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_opengl/juce_opengl.h>

namespace zenith {

/**
 * @class SkiaComponent
 * @brief Base Skia component for unified UI framework
 *
 * This component provides GPU-accelerated rendering capabilities using Skia.
 * It's part of the unified UI framework that replaces the dual JUCE/Skia system.
 */
class SkiaComponent : public juce::Component {
public:
    //==============================================================================
    // Construction
    //==============================================================================

    SkiaComponent();
    ~SkiaComponent() override;

    //==============================================================================
    // Rendering
    //==============================================================================

    void paint(juce::Graphics& g) override;

    //==============================================================================
    // OpenGL Context
    //==============================================================================

    void openGLContextCreated();
    void openGLContextClosing();
    void renderOpenGL();

    //==============================================================================
    // Configuration
    //==============================================================================

    void setUseSkiaRendering(bool shouldUseSkia);
    bool isUsingSkiaRendering() const;

private:
    //==============================================================================
    // Members
    //==============================================================================

    bool useSkiaRendering_{true};
    std::unique_ptr<juce::OpenGLContext> openGLContext_;
    std::unique_ptr<juce::OpenGLRenderer> openGLRenderer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaComponent)
};

} // namespace zenith