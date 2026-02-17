/*
  ==============================================================================
    PluginCard.h
    Skia-based marketplace plugin card declaration
  ==============================================================================
*/

#pragma once

#include "PluginInfo.h"
#include <zenith_ui/ui/framework/SkiaComponent.h>
#include <zenith_ui/ui/controls/SkiaButton.h>
#include <zenith_ui/ui/controls/SkiaLabel.h>
#include <memory>

namespace zenith {
namespace marketplace {

class PluginCard : public zenith::SkiaComponent {
public:
    explicit PluginCard(const PluginInfo& pluginInfo);
    ~PluginCard() override;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    const PluginInfo& getPluginInfo() const { return pluginInfo_; }

private:
    PluginInfo pluginInfo_;

    std::unique_ptr<SkiaLabel> nameLabel_;
    std::unique_ptr<SkiaLabel> developerLabel_;
    std::unique_ptr<SkiaLabel> priceLabel_;
    std::unique_ptr<SkiaLabel> ratingLabel_;
    std::unique_ptr<SkiaButton> downloadButton_;
    std::unique_ptr<SkiaButton> trialButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCard)
};

} // namespace marketplace
} // namespace zenith
