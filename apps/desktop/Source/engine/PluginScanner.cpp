/*
  ==============================================================================

    PluginScanner.cpp
    Created: 2025-12-23
    Author:  Zenith DAW

    Standalone utility for out-of-process plugin scanning.
    Takes a plugin path and outputs PluginDescription as simple key-value pairs.

  ==============================================================================
*/

#include <iostream>
#include <csignal>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#include <sstream>
#include <regex>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace {
    std::atomic<bool> scanCancelled{false};

    void signalHandler(int signal) {
        std::cerr << "Error: Plugin scanner received signal " << signal << std::endl;
        scanCancelled.store(true);
    }
}

// TimeoutTimer class removed - using std::thread watchdog instead

// Simple output helper - key: value pairs
void outputPluginAsKeyValues(const juce::PluginDescription* desc) {
    std::cout << "STATUS=success" << std::endl;
    std::cout << "NAME=" << desc->name << std::endl;
    std::cout << "MANUFACTURER=" << desc->manufacturerName << std::endl;
    std::cout << "VERSION=" << desc->version << std::endl;
    std::cout << "UID=" << desc->uniqueId << std::endl;
    std::cout << "IS_INSTRUMENT=" << (desc->isInstrument ? "true" : "false") << std::endl;
    std::cout << "FORMAT=" << desc->pluginFormatName << std::endl;
}

int main(int argc, char *argv[]) {
  juce::ScopedJuceInitialiser_GUI initialiser;

  // Install signal handlers for crash detection
  std::signal(SIGSEGV, signalHandler);
  std::signal(SIGABRT, signalHandler);
  std::signal(SIGFPE, signalHandler);
#if defined(WIN32) || defined(_WIN32)
  std::signal(SIGILL, signalHandler);
#endif

  if (argc < 2) {
    std::cerr << "Usage: PluginScanner <plugin_path>" << std::endl;
    return 1;
  }

  juce::String path = argv[1];
  juce::File file(path);

  if (!file.exists()) {
    std::cerr << "Error: Plugin file does not exist: " << path.toStdString()
              << std::endl;
    return 2;
  }

  juce::AudioPluginFormatManager formatManager;
  formatManager.addFormat(new juce::VST3PluginFormat());

  // Attempt to identify plugin format
  juce::AudioPluginFormat *formatToUse = nullptr;
  for (int i = 0; i < formatManager.getNumFormats(); ++i) {
    auto *format = formatManager.getFormat(i);
    if (format->fileMightContainThisPluginType(file.getFullPathName())) {
      formatToUse = format;
      break;
    }
  }

  if (formatToUse == nullptr) {
    std::cerr << "Error: No suitable plugin format found for "
              << path.toStdString() << std::endl;
    return 3;
  }

  // Create watchdog thread (independent of message loop)
  // This ensures we can kill the process even if the main thread hangs in a plugin
  std::thread watchdog([&scanCancelled]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));
      if (!scanCancelled.load()) {
          std::cerr << "Error: Plugin scan timed out (Watchdog)" << std::endl;
          std::exit(5); // Force exit
      }
  });

  // Detach so it can run independently (we'll exit before it joins if successful)
  watchdog.detach();

  juce::OwnedArray<juce::PluginDescription> descriptions;

  try {
    formatToUse->findAllTypesForFile(descriptions, file.getFullPathName());
  } catch (const std::exception& e) {
    std::cerr << "Error: Exception during plugin scan: " << e.what() << std::endl;
    return 6;
  } catch (...) {
    std::cerr << "Error: Unknown exception during plugin scan" << std::endl;
    return 7;
  }

  // Signal successful completion to watchdog (though likely we'll just exit)
  scanCancelled.store(true);

  if (descriptions.size() == 0) {
    std::cerr << "Error: No plugin types found in " << path.toStdString()
              << std::endl;
    return 4;
  }

  // Success! Output descriptions as key-value pairs
  for (auto *desc : descriptions) {
    outputPluginAsKeyValues(desc);
  }

  return 0;
}