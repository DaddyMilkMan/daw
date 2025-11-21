/**
 * @file SkiaSettingsManager.cpp
 * @brief Implementation of Skia settings manager
 */

#include "SkiaSettingsManager.h"

namespace zenith {

SkiaSettingsManager::SkiaSettingsManager()
{
    // Initialize GPU settings with defaults
    gpuSettings_.targetFPS = 60;
    gpuSettings_.adaptiveFPS = true;
    gpuSettings_.prioritizeQuality = true;
    gpuSettings_.waveformDetailLevel = 4;
    gpuSettings_.enableAntialiasing = true;
    gpuSettings_.msaaSamples = 4;
    gpuSettings_.enableMipmaps = true;

    // Try to load saved settings
    loadSettings();
}

SkiaSettingsManager& SkiaSettingsManager::getInstance()
{
    static SkiaSettingsManager instance;
    return instance;
}

void SkiaSettingsManager::setThemeMode(ThemeMode mode)
{
    if (themeMode_ != mode)
    {
        themeMode_ = mode;
        SkiaTheme::getInstance().setThemeMode(mode);
        saveSettings();
    }
}

void SkiaSettingsManager::setButtonPhysics(const SpringPhysicsSettings& settings)
{
    buttonPhysics_ = settings;
    SkiaTheme::getInstance().setButtonPhysics(settings);
    saveSettings();
}

void SkiaSettingsManager::setSliderPhysics(const SpringPhysicsSettings& settings)
{
    sliderPhysics_ = settings;
    SkiaTheme::getInstance().setSliderPhysics(settings);
    saveSettings();
}

void SkiaSettingsManager::setKnobPhysics(const SpringPhysicsSettings& settings)
{
    knobPhysics_ = settings;
    SkiaTheme::getInstance().setKnobPhysics(settings);
    saveSettings();
}

void SkiaSettingsManager::setWaveformPhysics(const SpringPhysicsSettings& settings)
{
    waveformPhysics_ = settings;
    SkiaTheme::getInstance().setWaveformPhysics(settings);
    saveSettings();
}

void SkiaSettingsManager::setTimelinePhysics(const SpringPhysicsSettings& settings)
{
    timelinePhysics_ = settings;
    SkiaTheme::getInstance().setTimelinePhysics(settings);
    saveSettings();
}

void SkiaSettingsManager::setGPUSettings(const SkiaTheme::GPUSettings& settings)
{
    gpuSettings_ = settings;
    SkiaTheme::getInstance().setGPUSettings(settings);
    saveSettings();
}

void SkiaSettingsManager::setTargetFPS(int fps)
{
    int clampedFPS = juce::jlimit(24, 240, fps);
    if (gpuSettings_.targetFPS != clampedFPS)
    {
        gpuSettings_.targetFPS = clampedFPS;
        SkiaTheme::getInstance().setGPUSettings(gpuSettings_);
        saveSettings();
    }
}

void SkiaSettingsManager::setAdaptiveFPS(bool enabled)
{
    if (gpuSettings_.adaptiveFPS != enabled)
    {
        gpuSettings_.adaptiveFPS = enabled;
        SkiaTheme::getInstance().setGPUSettings(gpuSettings_);
        saveSettings();
    }
}

void SkiaSettingsManager::setDepthStyle(const DepthStyle& style)
{
    depthStyle_ = style;
    SkiaTheme::getInstance().setDepthStyle(style);
    saveSettings();
}

bool SkiaSettingsManager::loadSettings()
{
    try
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "ZenithDAW";
        options.filenameSuffix = "properties";
        options.osxLibrarySubFolder = "Application Support";
        options.commonToAllUsers = false;
        options.ignoreCaseOfKeyNames = false;

        loadFromProperties(options);
        return true;
    }
    catch (const std::exception& e)
    {
        DBG("Failed to load Skia settings: " << e.what());
        return false;
    }
}

bool SkiaSettingsManager::saveSettings()
{
    try
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "ZenithDAW";
        options.filenameSuffix = "properties";
        options.osxLibrarySubFolder = "Application Support";
        options.commonToAllUsers = false;
        options.ignoreCaseOfKeyNames = false;

        saveToProperties(options);
        return true;
    }
    catch (const std::exception& e)
    {
        DBG("Failed to save Skia settings: " << e.what());
        return false;
    }
}

void SkiaSettingsManager::resetToDefaults()
{
    themeMode_ = ThemeMode::Dark;
    buttonPhysics_ = SpringPhysicsSettings::snappy();
    sliderPhysics_ = SpringPhysicsSettings::smooth();
    knobPhysics_ = SpringPhysicsSettings::smooth();
    waveformPhysics_ = SpringPhysicsSettings::precise();
    timelinePhysics_ = SpringPhysicsSettings::smooth();
    depthStyle_ = DepthStyle::moderate();

    gpuSettings_.targetFPS = 60;
    gpuSettings_.adaptiveFPS = true;
    gpuSettings_.prioritizeQuality = true;
    gpuSettings_.waveformDetailLevel = 4;
    gpuSettings_.enableAntialiasing = true;
    gpuSettings_.msaaSamples = 4;
    gpuSettings_.enableMipmaps = true;

    saveSettings();
}

juce::File SkiaSettingsManager::getSettingsFile() const
{
    juce::PropertiesFile::Options options;
    options.applicationName = "ZenithDAW";
    options.filenameSuffix = "properties";
    options.osxLibrarySubFolder = "Application Support";
    options.commonToAllUsers = false;

    return options.getDefaultFile();
}

void SkiaSettingsManager::loadFromProperties(const juce::PropertiesFile::Options& options)
{
    auto props = std::make_unique<juce::PropertiesFile>(options);

    // Load theme
    auto themeName = props->getValue("theme", "dark");
    themeMode_ = (themeName == "light") ? ThemeMode::Light : ThemeMode::Dark;

    // Load button physics
    buttonPhysics_.stiffness = props->getDoubleValue("buttonPhysics.stiffness", 450.0);
    buttonPhysics_.damping = props->getDoubleValue("buttonPhysics.damping", 30.0);
    buttonPhysics_.fps = props->getDoubleValue("buttonPhysics.fps", 60.0);

    // Load slider physics
    sliderPhysics_.stiffness = props->getDoubleValue("sliderPhysics.stiffness", 350.0);
    sliderPhysics_.damping = props->getDoubleValue("sliderPhysics.damping", 25.0);
    sliderPhysics_.fps = props->getDoubleValue("sliderPhysics.fps", 60.0);

    // Load knob physics
    knobPhysics_.stiffness = props->getDoubleValue("knobPhysics.stiffness", 350.0);
    knobPhysics_.damping = props->getDoubleValue("knobPhysics.damping", 25.0);
    knobPhysics_.fps = props->getDoubleValue("knobPhysics.fps", 60.0);

    // Load waveform physics
    waveformPhysics_.stiffness = props->getDoubleValue("waveformPhysics.stiffness", 500.0);
    waveformPhysics_.damping = props->getDoubleValue("waveformPhysics.damping", 35.0);
    waveformPhysics_.fps = props->getDoubleValue("waveformPhysics.fps", 120.0);

    // Load timeline physics
    timelinePhysics_.stiffness = props->getDoubleValue("timelinePhysics.stiffness", 350.0);
    timelinePhysics_.damping = props->getDoubleValue("timelinePhysics.damping", 25.0);
    timelinePhysics_.fps = props->getDoubleValue("timelinePhysics.fps", 60.0);

    // Load GPU settings
    gpuSettings_.targetFPS = props->getIntValue("gpu.targetFPS", 60);
    gpuSettings_.adaptiveFPS = props->getBoolValue("gpu.adaptiveFPS", true);
    gpuSettings_.prioritizeQuality = props->getBoolValue("gpu.prioritizeQuality", true);
    gpuSettings_.waveformDetailLevel = props->getIntValue("gpu.waveformDetail", 4);
    gpuSettings_.enableAntialiasing = props->getBoolValue("gpu.antialiasing", true);
    gpuSettings_.msaaSamples = props->getIntValue("gpu.msaaSamples", 4);
    gpuSettings_.enableMipmaps = props->getBoolValue("gpu.mipmaps", true);

    // Load depth style
    depthStyle_.shadowBlur = props->getDoubleValue("depth.shadowBlur", 8.0);
    depthStyle_.shadowOffsetY = props->getDoubleValue("depth.shadowOffsetY", 3.0);
    depthStyle_.shadowOpacity = props->getDoubleValue("depth.shadowOpacity", 0.4);
    depthStyle_.highlightOpacity = props->getDoubleValue("depth.highlightOpacity", 0.25);
    depthStyle_.innerShadowSize = props->getDoubleValue("depth.innerShadowSize", 2.0);
    depthStyle_.useGradients = props->getBoolValue("depth.useGradients", true);

    DBG("Skia settings loaded successfully");
}

void SkiaSettingsManager::saveToProperties(const juce::PropertiesFile::Options& options)
{
    auto props = std::make_unique<juce::PropertiesFile>(options);

    // Save theme
    props->setValue("theme", (themeMode_ == ThemeMode::Dark) ? "dark" : "light");

    // Save button physics
    props->setValue("buttonPhysics.stiffness", static_cast<double>(buttonPhysics_.stiffness));
    props->setValue("buttonPhysics.damping", static_cast<double>(buttonPhysics_.damping));
    props->setValue("buttonPhysics.fps", static_cast<double>(buttonPhysics_.fps));

    // Save slider physics
    props->setValue("sliderPhysics.stiffness", static_cast<double>(sliderPhysics_.stiffness));
    props->setValue("sliderPhysics.damping", static_cast<double>(sliderPhysics_.damping));
    props->setValue("sliderPhysics.fps", static_cast<double>(sliderPhysics_.fps));

    // Save knob physics
    props->setValue("knobPhysics.stiffness", static_cast<double>(knobPhysics_.stiffness));
    props->setValue("knobPhysics.damping", static_cast<double>(knobPhysics_.damping));
    props->setValue("knobPhysics.fps", static_cast<double>(knobPhysics_.fps));

    // Save waveform physics
    props->setValue("waveformPhysics.stiffness", static_cast<double>(waveformPhysics_.stiffness));
    props->setValue("waveformPhysics.damping", static_cast<double>(waveformPhysics_.damping));
    props->setValue("waveformPhysics.fps", static_cast<double>(waveformPhysics_.fps));

    // Save timeline physics
    props->setValue("timelinePhysics.stiffness", static_cast<double>(timelinePhysics_.stiffness));
    props->setValue("timelinePhysics.damping", static_cast<double>(timelinePhysics_.damping));
    props->setValue("timelinePhysics.fps", static_cast<double>(timelinePhysics_.fps));

    // Save GPU settings
    props->setValue("gpu.targetFPS", gpuSettings_.targetFPS);
    props->setValue("gpu.adaptiveFPS", gpuSettings_.adaptiveFPS);
    props->setValue("gpu.prioritizeQuality", gpuSettings_.prioritizeQuality);
    props->setValue("gpu.waveformDetail", gpuSettings_.waveformDetailLevel);
    props->setValue("gpu.antialiasing", gpuSettings_.enableAntialiasing);
    props->setValue("gpu.msaaSamples", gpuSettings_.msaaSamples);
    props->setValue("gpu.mipmaps", gpuSettings_.enableMipmaps);

    // Save depth style
    props->setValue("depth.shadowBlur", static_cast<double>(depthStyle_.shadowBlur));
    props->setValue("depth.shadowOffsetY", static_cast<double>(depthStyle_.shadowOffsetY));
    props->setValue("depth.shadowOpacity", static_cast<double>(depthStyle_.shadowOpacity));
    props->setValue("depth.highlightOpacity", static_cast<double>(depthStyle_.highlightOpacity));
    props->setValue("depth.innerShadowSize", static_cast<double>(depthStyle_.innerShadowSize));
    props->setValue("depth.useGradients", depthStyle_.useGradients);

    props->saveIfNeeded();
    DBG("Skia settings saved successfully");
}

} // namespace zenith
