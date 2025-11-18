#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

/**
 * @brief Cross-platform content path resolver for Zenith instruments
 *
 * Manages the directory structure for samples, presets, and instrument content.
 *
 * Directory structure:
 *   <ContentRoot>/
 *     Instruments/
 *       ZenithSampler/
 *         <InstrumentName>/
 *           Samples/*.wav
 *           <InstrumentName>.zpatch
 *
 * Platform defaults:
 *   - macOS: ~/Music/Zenith/
 *   - Windows: %USERPROFILE%\Music\Zenith\
 *   - Linux: ~/Music/Zenith/
 *
 * Thread-safe for reading. All file I/O must happen off the audio thread.
 */
class ContentPaths
{
public:
    /**
     * @brief Get the singleton instance
     */
    static ContentPaths& getInstance()
    {
        static ContentPaths instance;
        return instance;
    }

    /**
     * @brief Get the root content directory
     *
     * @return juce::File The root directory for all Zenith content
     */
    juce::File getContentRoot() const
    {
        if (customContentRoot.exists())
            return customContentRoot;

        return getDefaultContentRoot();
    }

    /**
     * @brief Set a custom content root directory
     *
     * @param newRoot The new content root directory
     */
    void setContentRoot(const juce::File& newRoot)
    {
        customContentRoot = newRoot;
    }

    /**
     * @brief Get the instruments directory
     *
     * @return juce::File Path to Instruments/ directory
     */
    juce::File getInstrumentsDirectory() const
    {
        return getContentRoot().getChildFile("Instruments");
    }

    /**
     * @brief Get the directory for a specific instrument
     *
     * @param instrumentType Type of instrument (e.g., "ZenithSampler")
     * @return juce::File Path to instrument type directory
     */
    juce::File getInstrumentTypeDirectory(const juce::String& instrumentType) const
    {
        return getInstrumentsDirectory().getChildFile(instrumentType);
    }

    /**
     * @brief Get the directory for a specific instrument patch
     *
     * @param instrumentType Type of instrument (e.g., "ZenithSampler")
     * @param patchName Name of the patch (e.g., "Piano")
     * @return juce::File Path to patch directory
     */
    juce::File getPatchDirectory(const juce::String& instrumentType,
                                 const juce::String& patchName) const
    {
        return getInstrumentTypeDirectory(instrumentType).getChildFile(patchName);
    }

    /**
     * @brief Get the samples directory for a specific patch
     *
     * @param instrumentType Type of instrument
     * @param patchName Name of the patch
     * @return juce::File Path to Samples/ directory
     */
    juce::File getSamplesDirectory(const juce::String& instrumentType,
                                   const juce::String& patchName) const
    {
        return getPatchDirectory(instrumentType, patchName).getChildFile("Samples");
    }

    /**
     * @brief Get the patch file for a specific instrument
     *
     * @param instrumentType Type of instrument
     * @param patchName Name of the patch
     * @return juce::File Path to .zpatch file
     */
    juce::File getPatchFile(const juce::String& instrumentType,
                           const juce::String& patchName) const
    {
        return getPatchDirectory(instrumentType, patchName)
            .getChildFile(patchName + ".zpatch");
    }

    /**
     * @brief Ensure the content directory structure exists
     *
     * Creates all necessary directories if they don't exist.
     *
     * @return bool True if successful or directories already exist
     */
    bool ensureDirectoryStructureExists() const
    {
        auto root = getContentRoot();
        auto instruments = getInstrumentsDirectory();

        // Create root directories
        if (!root.exists() && !root.createDirectory())
            return false;

        if (!instruments.exists() && !instruments.createDirectory())
            return false;

        // Create ZenithSampler directory
        auto samplerDir = getInstrumentTypeDirectory("ZenithSampler");
        if (!samplerDir.exists() && !samplerDir.createDirectory())
            return false;

        return true;
    }

    /**
     * @brief Get all available patches for a specific instrument type
     *
     * @param instrumentType Type of instrument (e.g., "ZenithSampler")
     * @return juce::StringArray List of patch names
     */
    juce::StringArray getAvailablePatches(const juce::String& instrumentType) const
    {
        juce::StringArray patches;
        auto instrumentDir = getInstrumentTypeDirectory(instrumentType);

        if (!instrumentDir.exists())
            return patches;

        // Find all subdirectories
        juce::Array<juce::File> subdirs;
        instrumentDir.findChildFiles(subdirs,
                                     juce::File::findDirectories,
                                     false);

        for (const auto& dir : subdirs)
        {
            // Check if a .zpatch file exists
            auto patchFile = dir.getChildFile(dir.getFileName() + ".zpatch");
            if (patchFile.existsAsFile())
            {
                patches.add(dir.getFileName());
            }
        }

        return patches;
    }

private:
    ContentPaths() = default;
    ~ContentPaths() = default;

    // Prevent copying
    ContentPaths(const ContentPaths&) = delete;
    ContentPaths& operator=(const ContentPaths&) = delete;

    /**
     * @brief Get the platform-specific default content root
     */
    static juce::File getDefaultContentRoot()
    {
        // Get the user's Music directory
        auto musicDir = juce::File::getSpecialLocation(
            juce::File::userMusicDirectory);

        // Create Zenith subdirectory
        return musicDir.getChildFile("Zenith");
    }

    juce::File customContentRoot;
};


} // namespace zenith
