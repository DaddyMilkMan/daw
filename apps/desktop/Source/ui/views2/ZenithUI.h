/*
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
