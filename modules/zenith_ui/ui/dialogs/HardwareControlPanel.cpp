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

 * @file HardwareControlPanel.cpp
 * @brief Implementation of Hardware Control Panel
 */



namespace zenith {

HardwareControlPanel::HardwareControlPanel(Engine& engine)
    : engine_(engine)
{
    // Listen for property changes from the discovery service
    if (auto* discovery = engine_.getMidi2DiscoveryService())
    {
        discovery->getPEManager().onPropertiesChanged = [this](uint32_t muid) {
            // A+ Standard: Use SafePointer to avoid crashes if component is deleted
            auto safe = safeThis;
            juce::MessageManager::callAsync([safe, muid]() {
                if (safe != nullptr) 
                {
                    safe->activeDeviceMuid_ = muid;
                    safe->refreshUI(false); // Delta update
                }
            });
        };
    }
    
    refreshUI(true); // Initial full build
}

HardwareControlPanel::~HardwareControlPanel()
{
    if (auto* discovery = engine_.getMidi2DiscoveryService())
    {
        discovery->getPEManager().onPropertiesChanged = nullptr;
    }
}

void HardwareControlPanel::refreshUI(bool forceRebuild)
{
    auto* discovery = engine_.getMidi2DiscoveryService();
    if (!discovery || activeDeviceMuid_ == 0)
    {
        controls_.clear();
        return;
    }
        
    auto properties = discovery->getPEManager().getProperties(activeDeviceMuid_);
    bool layoutChanged = false;

    if (forceRebuild || properties.size() != controls_.size())
    {
        controls_.clear();
        for (const auto& prop : properties)
        {
            auto control = std::make_unique<PropertyControl>();
            control->id = prop.id;
            
            control->slider = std::make_unique<SkiaSlider>(prop.name, design::unified::accent_primary());
            control->slider->setRange(prop.min, prop.max, prop.value);
            control->slider->setValue(prop.value);
            
            control->slider->onValueChange = [this, propId = prop.id](float val) {
                if (auto* disc = engine_.getMidi2DiscoveryService())
                    disc->getPEManager().setProperty(activeDeviceMuid_, propId, val);
            };
            
            control->label = std::make_unique<SkiaLabel>(prop.name);
            
            addAndMakeVisible(control->slider.get());
            addAndMakeVisible(control->label.get());
            
            controls_.push_back(std::move(control));
        }
        layoutChanged = true;
    }
    else
    {
        // Delta update: Only update values if they differ
        for (size_t i = 0; i < properties.size(); ++i)
        {
            if (controls_[i]->slider->getValue() != properties[i].value)
            {
                controls_[i]->slider->setValue(properties[i].value);
            }
        }
    }
    
    if (layoutChanged)
        resized();
        
    markDirty();
}

void HardwareControlPanel::resized()
{
    int y = 60;
    int margin = 20;
    int itemHeight = 60;
    int width = getWidth() - (margin * 2);
    
    for (auto& ctrl : controls_)
    {
        ctrl->label->setBounds(margin, y, width, 20);
        ctrl->slider->setBounds(margin, y + 25, width, 25);
        y += itemHeight;
    }
}

void HardwareControlPanel::drawSkia(SkCanvas* canvas)
{
    SkPaint textPaint;
    textPaint.setColor(SK_ColorWHITE);
    textPaint.setAntiAlias(true);

    SkFont headerFont;
    headerFont.setSize(24.0f);
    headerFont.setEmbolden(true);

    SkFont statusFont;
    statusFont.setSize(14.0f);

    canvas->drawString("Hardware Total Recall", 20, 35, headerFont, textPaint);
    
    if (activeDeviceMuid_ == 0)
    {
        juce::Colour dimmed = zenith::ZenithTheme::Colors::text_secondary;
        textPaint.setColor(SkColorSetARGB(dimmed.getAlpha(), dimmed.getRed(), dimmed.getGreen(), dimmed.getBlue()));
        canvas->drawString("Waiting for MIDI-CI device discovery...", 20, 80, statusFont, textPaint);
    }
    else
    {
        juce::String status = "Connected to MUID: " + juce::String((int)activeDeviceMuid_);
        juce::Colour active = zenith::ZenithTheme::Colors::accent_primary;
        textPaint.setColor(SkColorSetARGB(active.getAlpha(), active.getRed(), active.getGreen(), active.getBlue()));
        canvas->drawString(status.toStdString().c_str(), 20, 55, statusFont, textPaint);
    }
}

} // namespace zenith
