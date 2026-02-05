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

    ZenithUI.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Master include for the new Zenith UI system.
    
    Usage:

        #include "ui/views2/ZenithUI.h"
        
        // In your MainComponent:
        zenith::ui::ZenithMainLayout mainLayout;
        addAndMakeVisible(mainLayout);

  ==============================================================================
*/

#pragma once

// Core navigation
#include "core/ViewSwitcher.h"

// Main views
#include "arranger/SkiaArrangementView.h"
#include "session/SkiaSessionView.h"
#include "ai-jam/SkiaAIJamView.h"

// Common components
#include "common/SkiaTransportBar.h"

// Main layout container
#include "ZenithMainLayout.h"

namespace zenith::ui {

/**
 * @brief Quick reference for view types
 */
using View = ViewType;

/**
 * @brief Quick reference for transport states
 */
using Transport = TransportState;

} // namespace zenith::ui
