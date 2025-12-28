/*
  ==============================================================================

    PluginScanner.cpp
    Created: 2025-12-23
    Author:  Zenith DAW

    Standalone utility for out-of-process plugin scanning.
    Takes a plugin path and outputs the PluginDescription as XML to stdout.

  ==============================================================================
*/

#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

int main(int argc, char *argv[]) {
  // Use ScopedJuceInitialiser_GUI for messaging and other essentials
  juce::ScopedJuceInitialiser_GUI initialiser;

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
  formatManager.addDefaultFormats();

  // Attempt to identify the plugin format
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

  juce::OwnedArray<juce::PluginDescription> descriptions;
  formatToUse->findAllTypesForFile(descriptions, file.getFullPathName());

  if (descriptions.size() == 0) {
    std::cerr << "Error: No plugin types found in " << path.toStdString()
              << std::endl;
    return 4;
  }

  // Success! Output descriptions as XML
  juce::XmlElement results("PLUGINS");
  for (auto *desc : descriptions) {
    results.addChildElement(desc->createXml().release());
  }

  std::cout << results.toString(juce::XmlElement::TextFormat().withoutHeader())
                   .toStdString()
            << std::endl;

  return 0;
}
