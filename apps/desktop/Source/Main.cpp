/**
 * @file Main.cpp
 * @brief Zenith DAW - Main application entry point
 *
 * This file initializes the JUCE application and creates the main window.
 */

#include "MainWindow.h"
#include "Settings.h"
#include "utils/SampleGenerator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>


//==============================================================================
/**
 * @class ZenithApplication
 * @brief Main application class for Zenith DAW
 *
 * Handles application lifecycle events:
 * - Initialization
 * - Shutdown
 * - System commands
 */
class ZenithApplication : public juce::JUCEApplication {
public:
  //==========================================================================
  ZenithApplication() = default;

  //==========================================================================
  // JUCEApplication interface
  //==========================================================================

  const juce::String getApplicationName() override { return "Zenith DAW"; }
  const juce::String getApplicationVersion() override { return "0.1.0"; }
  bool moreThanOneInstanceAllowed() override { return true; } // Allow multiple instances for scanning

  //==========================================================================
  //==========================================================================
  void initialise(const juce::String &commandLine) override {
      // Check for scanning mode
      if (getCommandLineParameters().contains("--scan-plugin"))
      {
          runPluginScanningMode();
          quit();
          return;
      }

    // Input validation should be added here for production releases
    juce::ignoreUnused(commandLine);

    // Log startup
    DBG("Zenith DAW starting...");
    DBG("Version: " + getApplicationVersion());
    DBG("JUCE Version: " + juce::SystemStats::getJUCEVersion());

    // Log system info
    logSystemInfo();

    // Load user settings from disk
    zenith::Settings::getInstance().load();
    DBG("User settings loaded");

    // Ensure content validity (Generate missing samples if needed)
    zenith::SampleGenerator::generateMissingSamples();

    // Create main window
    mainWindow = std::make_unique<MainWindow>(getApplicationName());

    DBG("Zenith DAW initialized successfully!");
  }

  void shutdown() override {
    DBG("Zenith DAW shutting down...");

    // Close main window (releases all resources)
    mainWindow.reset();

    DBG("Zenith DAW shutdown complete.");
  }

  //==========================================================================
  void systemRequestedQuit() override {
    if (mainWindow != nullptr) {
      auto *projectState = mainWindow->getProjectState();
      if (projectState != nullptr && projectState->hasUnsavedChanges()) {
        int result = juce::NativeMessageBox::showYesNoCancelBox(
            juce::AlertWindow::WarningIcon, "Unsaved Changes",
            "You have unsaved changes. Do you want to save before quitting?",
            mainWindow.get(), nullptr);

        // JUCE NativeMessageBox return values:
        // 1 = Yes, 2 = No, 0 = Cancel
        const int RESULT_YES = 1;
        const int RESULT_NO = 2;
        const int RESULT_CANCEL = 0;

        if (result == RESULT_YES) // Yes
        {
          // Save and quit
          mainWindow->saveProject();
          quit();
        } else if (result == RESULT_NO) // No
        {
          // User explicitly consented to data loss (discard changes).
          quit();
        }
        // Cancel (result == RESULT_CANCEL) -> do nothing
      } else {
        quit();
      }
    } else {
      quit();
    }
  }

  void anotherInstanceStarted(const juce::String &commandLine) override {
    juce::ignoreUnused(commandLine);
  }

private:
  //==========================================================================
  void logSystemInfo() {
    DBG("========================================");
    DBG("System Information");
    DBG("========================================");
    DBG("OS: " + juce::SystemStats::getOperatingSystemName());
    DBG("CPU: " + juce::String(juce::SystemStats::getCpuSpeedInMegahertz()) +
        " MHz");
    DBG("CPU Cores: " + juce::String(juce::SystemStats::getNumCpus()));
    DBG("Memory: " +
        juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
    DBG("========================================");
  }

  //==========================================================================
  void runPluginScanningMode()
  {
      auto args = getCommandLineParameterArray();
      int dateIndex = args.indexOf("--scan-plugin");
      
      if (dateIndex == -1 || dateIndex + 1 >= args.size())
      {
          std::cout << "Error: Missing plugin path argument" << std::endl;
          juce::JUCEApplication::quit();
          return;
      }

      juce::String pluginPath = args[dateIndex + 1];
      // strip quotes if needed, though JUCE handles this mostly
      pluginPath = pluginPath.unquoted();
      
      juce::File pluginFile(pluginPath);

      if (!pluginFile.exists())
      {
          std::cerr << "Error: Plugin file does not exist: " << pluginPath.toRawUTF8() << std::endl;
          return;
      }

      juce::AudioPluginFormatManager formatManager;
      formatManager.addDefaultFormats();

      juce::PluginDescription description;
      bool found = false;

      // Iterate through formats
      for (int i = 0; i < formatManager.getNumFormats(); ++i)
      {
          auto* format = formatManager.getFormat(i);
          if (format->fileMightContainThisPluginType(pluginFile.getFullPathName()))
          {
              juce::OwnedArray<juce::PluginDescription> foundTypes;
              format->findAllTypesForFile(foundTypes, pluginFile.getFullPathName());
              
              if (foundTypes.size() > 0)
              {
                  description = *foundTypes[0];
                  found = true;
                  break;
              }
          }
      }

      if (!found)
      {
          std::cerr << "Error: No suitable plugin format found for " << pluginPath.toRawUTF8() << std::endl;
          return;
      }

      auto xml = description.createXml();
      if (xml != nullptr)
      {
          juce::DynamicObject::Ptr obj = new juce::DynamicObject();
          obj->setProperty("name", description.name);
          obj->setProperty("descriptiveName", description.descriptiveName);
          obj->setProperty("pluginFormatName", description.pluginFormatName);
          obj->setProperty("category", description.category);
          obj->setProperty("manufacturerName", description.manufacturerName);
          obj->setProperty("version", description.version);
          obj->setProperty("fileOrIdentifier", description.fileOrIdentifier);
          obj->setProperty("lastFileModTime", (juce::int64)description.lastFileModTime.toMilliseconds());
          obj->setProperty("lastInfoUpdateTime", (juce::int64)description.lastInfoUpdateTime.toMilliseconds());
          obj->setProperty("uniqueId", description.uniqueId);
          obj->setProperty("isInstrument", description.isInstrument);
          obj->setProperty("numInputChannels", description.numInputChannels);
          obj->setProperty("numOutputChannels", description.numOutputChannels);
          obj->setProperty("hasSharedContainer", description.hasSharedContainer);

          juce::String json = juce::JSON::toString(juce::var(obj.get()));
          std::cout << json.toRawUTF8() << std::endl;
      }
  }

  //==========================================================================
  std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(ZenithApplication)
