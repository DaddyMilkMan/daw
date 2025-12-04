/*
  ==============================================================================

    GeneratePolySynthPresets.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Procedural preset generator for ZenithPolySynth.
    Generates 500+ high-quality presets across popular sound categories.

  ==============================================================================
*/

#include <JuceHeader.h>
#include <fstream>
#include "PolySynthGenerators.h" // Include the new shared header

// Use the namespace for shared logic
using namespace PolySynthPresetGen;























bool savePresetToFile(const std::string& outputDir,
                      const PresetMetadata& meta,
                      const PresetParams& params)
{
    std::string categoryDir = outputDir + "/" + meta.category;

    // Create category directory
    PolySynthPresetGen::createDirectory(categoryDir);

    std::string filename = categoryDir + "/" + meta.id + ".json";
    std::string jsonContent = PolySynthPresetGen::serializePresetToJSON(meta, params);

    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    file << jsonContent;
    file.close();

    return true;
}

//==============================================================================
/**
 * @brief Main entry point
 */
int main(int argc, char* argv[])
{
    std::cout << "========================================" << std::endl;
    std::cout << "ZenithPolySynth Preset Generator" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Determine output directory
    std::string outputDir = "/home/user/daw/zenith-core/Content/Presets/PolySynth_Generated";

    if (argc > 1)
        outputDir = argv[1];

    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << std::endl;

    // Create output directory
    PolySynthPresetGen::createDirectory(outputDir);

    // Generate presets
    PolySynthPresetGen::PresetGenerator generator;
    auto allPresets = generator.generateAllPresets();

    std::cout << std::endl;
    std::cout << "Saving presets to disk..." << std::endl;

    // Save all presets
    int savedCount = 0;
    for (const auto& [meta, params] : allPresets)
    {
        if (savePresetToFile(outputDir, meta, params))
            savedCount++;
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Successfully generated " << savedCount << " presets!" << std::endl;
    std::cout << "Presets saved to: " << outputDir << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
