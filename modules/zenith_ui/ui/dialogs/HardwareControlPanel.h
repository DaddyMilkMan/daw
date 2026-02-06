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


 * @file HardwareControlPanel.h
 * @brief Dynamic UI for MIDI 2.0 Hardware Control


#include "../Source/ui/controls/SkiaSlider.h"
#include "../Source/ui/controls/SkiaLabel.h"
#include <vector>
#include <memory>

namespace zenith {

/**
 * @class HardwareControlPanel
 * @brief Dynamically generated UI for hardware parameters
 */
class HardwareControlPanel : public SkiaComponent {
public:
    HardwareControlPanel(Engine& engine);
    ~HardwareControlPanel() override;

    void resized() override;
    void drawSkia(SkCanvas* canvas) override;

    /**
     * @brief Refresh the UI based on discovered properties
     * @param forceRebuild If true, clears and recreates all controls.
     */
    void refreshUI(bool forceRebuild = false);

private:
    Engine& engine_;
    uint32_t activeDeviceMuid_ = 0;
    
    struct PropertyControl {
        juce::String id;
        std::unique_ptr<SkiaSlider> slider;
        std::unique_ptr<SkiaLabel> label;
    };
    
    std::vector<std::unique_ptr<PropertyControl>> controls_;
    
    // Safely track this component for async callbacks
    juce::Component::SafePointer<HardwareControlPanel> safeThis { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardwareControlPanel)
};

} // namespace zenith
