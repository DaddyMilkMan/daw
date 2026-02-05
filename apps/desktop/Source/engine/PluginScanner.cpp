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

// Simple output helper - YAML-style key: value pairs (matches PluginHost parser)
void outputPluginAsYAML(const juce::PluginDescription* desc) {
    std::cout << "status: success" << std::endl;
    std::cout << "name: " << desc->name.toStdString() << std::endl;
    std::cout << "manufacturer: " << desc->manufacturerName.toStdString() << std::endl;
    std::cout << "version: " << desc->version.toStdString() << std::endl;
    std::cout << "uid: " << desc->uniqueId << std::endl;
    std::cout << "is_instrument: " << (desc->isInstrument ? "true" : "false") << std::endl;
    std::cout << "format: " << desc->pluginFormatName.toStdString() << std::endl;
}

// Output error in YAML format
void outputError(const char* errorType, const char* message) {
    std::cout << "status: error" << std::endl;
    std::cout << "error_type: " << errorType << std::endl;
    std::cout << "message: " << message << std::endl;
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
    outputError("usage", "Usage: PluginScanner <plugin_path>");
    return 1;
  }

  juce::String path = argv[1];
  juce::File file(path);

  if (!file.exists()) {
    outputError("file_not_found", ("Plugin file does not exist: " + path).toStdString().c_str());
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
    outputError("format_not_supported", ("No suitable plugin format found for " + path).toStdString().c_str());
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
    outputError("exception", e.what());
    return 6;
  } catch (...) {
    outputError("unknown_exception", "Unknown exception during plugin scan");
    return 7;
  }

  // Signal successful completion to watchdog (though likely we'll just exit)
  scanCancelled.store(true);

  if (descriptions.size() == 0) {
    outputError("no_plugins_found", ("No plugin types found in " + path).toStdString().c_str());
    return 4;
  }

  // Success! Output descriptions as YAML
  for (auto *desc : descriptions) {
    outputPluginAsYAML(desc);
  }

  return 0;
}