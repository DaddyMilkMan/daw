/**
 * @file HardwareControlPanel.h
 * @brief Dynamic UI for MIDI 2.0 Hardware Control
 */

#pragma once

#include "../Source/ui/framework/SkiaComponent.h"
#include "../Source/engine/Engine.h"
#include "../Source/engine/PropertyExchangeManager.h"
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
