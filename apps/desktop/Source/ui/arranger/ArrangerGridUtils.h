/**
 * @file ArrangerGridUtils.h
 * @brief Coordinate conversion, grid snapping, and waveform cache utilities for ArrangerComponent
 * 
 * This module handles all coordinate math, time conversion, and waveform caching
 * for the arranger view. It provides utilities for converting between beats/pixels,
 * track indices/Y coordinates, and manages pre-computed waveform peak data.
 */
#pragma once

#include <juce_core/juce_core.h>
#include <unordered_map>
#include <vector>

namespace zenith {

// Forward declarations
class ArrangerComponent;
class Engine;
class ProjectState;

//==============================================================================
/**
 * @struct WaveformCache
 * @brief Pre-computed peak data for fast waveform rendering
 * 
 * Stores downsampled min/max peaks for efficient waveform thumbnail display.
 * Built lazily when clips are first rendered.
 */
struct WaveformCache {
    juce::String audioFilePath;       ///< Source audio file path
    std::vector<float> minPeaks;      ///< Downsampled minimum peaks
    std::vector<float> maxPeaks;      ///< Downsampled maximum peaks
    int samplesPerPixel = 512;        ///< Resolution (samples per peak entry)
    bool isValid = false;             ///< Whether cache data is valid
};

//==============================================================================
/**
 * @class ArrangerGridUtils
 * @brief Coordinate conversion and grid utilities for the arranger view
 * 
 * Provides methods for:
 * - Converting between beats and pixel X coordinates
 * - Converting between track indices and pixel Y coordinates
 * - Grid snapping calculations
 * - Bar/Beat/Tick time formatting
 * - Waveform cache management for audio clips
 */
class ArrangerGridUtils {
public:
    /**
     * @brief Construct grid utilities for an arranger component
     * @param owner Reference to the owning ArrangerComponent
     * @param engine Reference to the audio engine
     * @param projectState Reference to the project state
     */
    ArrangerGridUtils(ArrangerComponent& owner, Engine& engine, ProjectState& projectState);

    //==========================================================================
    // Coordinate Conversion
    //==========================================================================
    
    /**
     * @brief Convert a beat position to screen X coordinate
     * @param beats Beat position in the timeline
     * @return X coordinate in pixels
     */
    float beatsToX(double beats) const;
    
    /**
     * @brief Convert screen X coordinate to beat position
     * @param x X coordinate in pixels
     * @return Beat position in the timeline
     */
    double xToBeats(float x) const;
    
    /**
     * @brief Convert track index to screen Y coordinate
     * @param trackIndex Zero-based track index
     * @return Y coordinate of the track's top edge
     */
    float trackIndexToY(int trackIndex) const;
    
    /**
     * @brief Convert screen Y coordinate to track index
     * @param y Y coordinate in pixels
     * @return Track index, or -1 if outside valid track area
     */
    int yToTrackIndex(float y) const;
    
    /**
     * @brief Snap a beat position to the current grid resolution
     * @param beats Input beat position
     * @return Snapped beat position
     */
    double snapToGrid(double beats) const;

    //==========================================================================
    // Time Conversion & Formatting
    //==========================================================================
    
    /**
     * @brief Convert sample position to beat position
     * @param samples Sample position
     * @return Beat position (accounting for tempo and sample rate)
     */
    double samplesToBeats(juce::int64 samples) const;
    
    /**
     * @brief Format beat position as Bar.Beat.Tick string
     * @param beats Beat position
     * @return Formatted string (e.g., "1.1.00", "5.3.45")
     */
    juce::String formatBarBeatTick(double beats) const;
    
    /**
     * @brief Get beats per bar from current time signature
     * @return Number of beats per bar (default 4 for 4/4)
     */
    int getBeatsPerBar() const;

    //==========================================================================
    // Waveform Cache Management
    //==========================================================================
    
    /**
     * @brief Build waveform cache for an audio file
     * 
     * Computes downsampled peak data for fast rendering. Cache is built
     * lazily when first needed and stored for reuse.
     * 
     * @param audioFilePath Path to the audio file
     */
    void buildWaveformCache(const juce::String& audioFilePath);
    
    /**
     * @brief Get cached waveform data for an audio file
     * @param audioFilePath Path to the audio file
     * @return Pointer to cache entry, or nullptr if not cached/invalid
     */
    const WaveformCache* getWaveformCache(const juce::String& audioFilePath) const;

private:
    ArrangerComponent& owner_;         ///< Owning component (for view state access)
    Engine& engine_;                   ///< Audio engine reference
    ProjectState& projectState_;       ///< Project state reference
    
    /// Waveform cache keyed by file path
    std::unordered_map<juce::String, WaveformCache> waveformCache_;
};

} // namespace zenith
