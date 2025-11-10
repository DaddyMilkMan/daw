/**
 * @file Main.cpp
 * @brief Zenith DAW - Main application entry point
 *
 * This file initializes the JUCE application and creates the main window.
 *
 * Phase 0: Foundation
 * - Application lifecycle management
 * - Main window initialization
 * - System initialization
 */

#include <JuceHeader.h>
#include "../include/MainWindow.h"

//==============================================================================
/**
 * @class ZenithApplication
 * @brief Main application class for Zenith DAW
 *
 * Handles application lifecycle events:
 * - Initialization
 * - Shutdown
 * - System commands
 * - Anotherinstance
 */
class ZenithApplication : public juce::JUCEApplication
{
public:
    //==========================================================================
    ZenithApplication() = default;

    //==========================================================================
    // JUCEApplication interface
    //==========================================================================

    /**
     * @brief Returns the application name
     */
    const juce::String getApplicationName() override
    {
        return "Zenith DAW";
    }

    /**
     * @brief Returns the application version
     */
    const juce::String getApplicationVersion() override
    {
        return "0.1.0";
    }

    /**
     * @brief Returns whether multiple instances are allowed
     */
    bool moreThanOneInstanceAllowed() override
    {
        return false;  // Single instance for now
    }

    //==========================================================================
    /**
     * @brief Called when the application starts
     *
     * This is where we:
     * 1. Check for command line arguments
     * 2. Initialize the audio engine
     * 3. Create and show the main window
     */
    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);

        // Log startup
        DBG("Zenith DAW starting...");
        DBG("Version: " + getApplicationVersion());
        DBG("JUCE Version: " + juce::SystemStats::getJUCEVersion());

        // Log system info
        logSystemInfo();

        // Create main window
        mainWindow = std::make_unique<MainWindow>(getApplicationName());

        DBG("Zenith DAW initialized successfully!");
    }

    /**
     * @brief Called when the application is about to quit
     *
     * Clean up resources:
     * 1. Close audio device
     * 2. Save preferences
     * 3. Close main window
     */
    void shutdown() override
    {
        DBG("Zenith DAW shutting down...");

        // Close main window (releases all resources)
        mainWindow.reset();

        DBG("Zenith DAW shutdown complete.");
    }

    //==========================================================================
    /**
     * @brief Called when the system wants to quit
     */
    void systemRequestedQuit() override
    {
        // TODO: Check for unsaved changes
        // TODO: Show "Save changes?" dialog if needed

        quit();
    }

    /**
     * @brief Called when another instance is launched (if allowed)
     */
    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        // Not called since moreThanOneInstanceAllowed() returns false
    }

private:
    //==========================================================================
    /**
     * @brief Log system information for debugging
     */
    void logSystemInfo()
    {
        DBG("========================================");
        DBG("System Information");
        DBG("========================================");
        DBG("OS: " + juce::SystemStats::getOperatingSystemName());
        DBG("CPU: " + juce::String(juce::SystemStats::getCpuSpeedInMegahertz()) + " MHz");
        DBG("CPU Cores: " + juce::String(juce::SystemStats::getNumCpus()));
        DBG("CPU Vendor: " + juce::SystemStats::getCpuVendor());
        DBG("Memory: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
        DBG("Display DPI: " + juce::String(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->dpi));
        DBG("========================================");
    }

    //==========================================================================
    // Member variables
    //==========================================================================

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
/**
 * @brief JUCE macro that creates the application instance
 *
 * This macro generates the platform-specific entry point (main/WinMain)
 * and creates an instance of our application class.
 */
START_JUCE_APPLICATION(ZenithApplication)
