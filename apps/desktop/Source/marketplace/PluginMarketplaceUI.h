/*
  ==============================================================================
    PluginMarketplaceUI.h
    Skia-based marketplace UI declaration
  ==============================================================================
*/

#pragma once

#include "PluginMarketplace.h"
#include <zenith_ui/ui/framework/SkiaComponent.h>
#include <zenith_ui/ui/controls/SkiaButton.h>
#include <zenith_ui/ui/controls/SkiaLabel.h>
#include <zenith_ui/ui/controls/SkiaTextEditor.h>
#include <zenith_ui/ui/controls/SkiaComboBox.h>
#include <memory>
#include <vector>

namespace zenith {
namespace marketplace {

class PluginMarketplaceUI : public zenith::SkiaComponent,
                            public PluginMarketplace::Listener {
public:
    PluginMarketplaceUI();
    ~PluginMarketplaceUI() override;

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setMarketplace(PluginMarketplace* marketplace);
    PluginMarketplace* getMarketplace() const { return marketplace_; }

    void showPluginDetails(const juce::String& pluginId);
    void showDownloads();
    void showInstalled();
    void showWishlist();

    void loginStatusChanged(bool isLoggedIn) override;
    void catalogUpdated() override;
    void pluginDownloadProgress(const juce::String& pluginId, float progress) override;
    void pluginDownloadCompleted(const juce::String& pluginId) override;
    void updateAvailable(const juce::String& pluginId) override;

private:
    PluginMarketplace* marketplace_ = nullptr;
    std::vector<PluginInfo> displayedPlugins_;

    std::unique_ptr<SkiaButton> loginButton_;
    std::unique_ptr<SkiaButton> accountButton_;
    std::unique_ptr<SkiaButton> refreshButton_;
    std::unique_ptr<SkiaLabel> userLabel_;
    std::unique_ptr<SkiaLabel> statusLabel_;
    std::unique_ptr<SkiaTextEditor> searchEditor_;
    std::unique_ptr<SkiaComboBox> categoryComboBox_;
    std::unique_ptr<SkiaComboBox> sortByComboBox_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginMarketplaceUI)
};

} // namespace marketplace
} // namespace zenith
