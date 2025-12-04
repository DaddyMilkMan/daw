/*
  ==============================================================================

    GeneratePolySynthPresetsStandalone.cpp
    Created: 2025-11-18
    Author:  Zenith DAW

    Standalone procedural preset generator for ZenithPolySynth.
    Generates 500+ high-quality presets across popular sound categories.
    This version has no dependencies and can be compiled standalone.

  ==============================================================================
*/

#include "PolySynthGenerators.h"

// Use the namespace for shared logic
using namespace PolySynthPresetGen;

#include <iostream> // Required for std::cerr, std::cout, std::endl
#include <fstream>  // Required for std::ofstream
#include <string>   // Required for std::string
#include <sstream>  // Required for std::ostringstream
#include <iomanip>  // Required for std::setprecision, std::fixed

//==============================================================================
/**
 * @brief Save preset to individual JSON file (Standalone implementation)
 */
bool savePresetToFile(const std::string& outputDir,
                      const PolySynthPresetGen::PresetMetadata& meta,
                      const PolySynthPresetGen::PresetParams& params)
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
    std::cout << "ZenithPolySynth Preset Generator (Standalone)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Determine output directory
    std::string outputDir = "zenith-core/Content/Presets/PolySynth_Generated";

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



















