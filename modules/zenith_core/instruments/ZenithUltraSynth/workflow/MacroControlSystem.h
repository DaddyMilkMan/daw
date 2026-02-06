/*
  ==============================================================================

    MacroControlSystem.h
    Created: [Date] Author: Claude AI
    Advanced macro controls with smart assignments and modulation

  ==============================================================================
*/

#pragma once

#include "../../../JuceLibraryCode/JuceHeader.h"

namespace Zenith
{

class MacroControlSystem
{
public:
    // Macro system configuration
    static constexpr int numMacros = 8;
    static constexpr int numAssignments = 16;   // Per macro
    static constexpr int maxModulationSources = 8;
    static constexpr int maxModulationTargets = 32;

    // Scaling curve types
    enum class ScalingCurve
    {
        Linear,      // Linear mapping
        Exponential, // Exponential mapping
        Logarithmic, // Logarithmic mapping
        Power,      // Power law mapping
        Sine,       // Sine wave mapping
        Custom      // Custom curve mapping
    };

    // Modulation source types
    enum class ModulationSource
    {
        None,           // No modulation
        Macro,          // Another macro control
        LFO,            // LFO modulation
        Envelope,       // Envelope modulation
        Velocity,       // Velocity modulation
        Aftertouch,     // Aftertouch modulation
        PitchBend,      // Pitch bend modulation
        Random,         // Random modulation
        MIDI CC,        // MIDI CC modulation
        Audio,          // Audio level modulation
        Sidechain,      // Sidechain modulation
        Sequencer,      // Sequencer modulation
        External        // External input modulation
    };

    // Assignment structure
    struct MacroAssignment
    {
        juce::String parameterId;          // Target parameter ID
        float amount;                      // Modulation depth (-1 to 1)
        float minimum;                     // Parameter minimum value
        float maximum;                     // Parameter maximum value
        bool invert;                       // Inverted polarity
        ScalingCurve curve;               // Scaling curve
        ModulationSource source;           // Modulation source
        int sourceIndex;                   // Source index (e.g., LFO index)
        float sourceAmount;                // Source modulation amount
        float smoothingTime;              // Parameter smoothing time
        bool enabled;                      // Assignment enabled
        bool bipolar;                      // Bipolar modulation
        float range;                       // Modulation range
        float offset;                      // Modulation offset
        juce::StringArray automationPaths; // Associated automation paths
        juce::Colour color;                // Visual color coding
        juce::String name;                // Assignment name
        juce::String description;          // Assignment description
        juce::uint64 lastModified;        // Last modification time
    };

    // Macro structure
    struct Macro
    {
        float value;                       // Current macro value (0-1)
        float targetValue;                 // Target macro value
        float smoothedValue;              // Smoothed macro value
        bool isMuted;                      // Mute state
        bool isSoloed;                     // Solo state
        bool isModulating;                 // Currently modulating
        float modulationDepth;            // Overall modulation depth
        juce::String name;                 // Macro name
        juce::String description;         // Macro description
        juce::Colour color;                // Visual color coding
        juce::Array<float> history;       // Value history for display
        float automationRate;              // Automation recording rate
        bool isRecording;                  // Recording automation
        std::vector<MacroAssignment> assignments; // Parameter assignments
        juce::uint64 lastActive;          // Last activity time
        juce::StringArray midiCCs;         // Associated MIDI CCs
        bool midiLearn;                    // MIDI learn mode
        int assignedMidiCC;               // Assigned MIDI CC
    };

    // Modulation matrix
    struct ModulationMatrix
    {
        struct Connection
        {
            MacroAssignment assignment;
            float currentValue;
            float smoothedValue;
            bool active;
            juce::uint64 lastUpdated;
        };

        std::vector<Connection> connections;
        float matrixGain;
        bool enabled;
        bool normalize;
        juce::StringArray outputBusses;
    };

    // Modulation sources
    struct ModulationSources
    {
        // LFO sources
        struct LFO
        {
            float rate;                    // Rate (Hz)
            float amount;                  // Amount
            float phase;                   // Phase (0-1)
            int waveform;                  // Waveform type
            bool smooth;                   // Smooth output
            float smoothingTime;           // Smoothing time
        };
        std::array<LFO, maxModulationSources> lfos;

        // Envelope sources
        struct Envelope
        {
            struct ADSR
            {
                float attack;             // Attack time
                float decay;               // Decay time
                float sustain;             // Sustain level
                float release;              // Release time
                float curve;               // Curve shape
                float value;               // Current value
                float phase;               // Current phase
            };
            ADSR adsr;
            bool retrigger;               // Retrigger on new note
            bool noteTracking;           // Note tracking enabled
        };
        std::array<Envelope, maxModulationSources> envelopes;

        // Random sources
        struct Random
        {
            float rate;                   // Random rate
            float amount;                 // Random amount
            float seed;                   // Random seed
            int distribution;              // Distribution type
            bool smooth;                  // Smooth output
            float smoothingTime;          // Smoothing time
        };
        std::array<Random, maxModulationSources> randomSources;

        // External sources
        struct External
        {
            float value;                  // Current value
            float amount;                 // Modulation amount
            int channel;                  // MIDI channel
            int cc;                       // CC number
            bool active;                  // Active state
        };
        std::array<External, maxModulationSources> externalSources;
    };

    // System state
    struct SystemState
    {
        std::array<Macro, numMacros> macros;
        ModulationMatrix matrix;
        ModulationSources sources;
        float globalGain;                 // Global gain
        float sampleRate;                 // Sample rate
        bool initialized;                 // System initialized
        juce::uint64 lastUpdate;         // Last update time
    };

    MacroControlSystem();
    ~MacroControlSystem();

    // Initialization
    void initialize(double sampleRate);
    void shutdown();
    void setSampleRate(double sampleRate);

    // Macro control
    void setMacroValue(int macroIndex, float value);
    float getMacroValue(int macroIndex) const;
    void setMacroTarget(int macroIndex, float value);
    float getMacroTarget(int macroIndex) const;
    void setMacroSmoothing(int macroIndex, float timeMs);
    float getMacroSmoothing(int macroIndex) const;

    // Macro state
    void setMacroMute(int macroIndex, bool muted);
    bool isMacroMuted(int macroIndex) const;
    void setMacroSolo(int macroIndex, bool soloed);
    bool isMacroSoloed(int macroIndex) const;
    void setMacroName(int macroIndex, const juce::String& name);
    juce::String getMacroName(int macroIndex) const;
    void setMacroColor(int macroIndex, const juce::Colour& color);
    juce::Colour getMacroColor(int macroIndex) const;

    // Assignment management
    bool addAssignment(int macroIndex, const MacroAssignment& assignment);
    bool removeAssignment(int macroIndex, int assignmentIndex);
    bool updateAssignment(int macroIndex, int assignmentIndex, const MacroAssignment& assignment);
    MacroAssignment getAssignment(int macroIndex, int assignmentIndex) const;
    std::vector<MacroAssignment> getAssignments(int macroIndex) const;
    int getAssignmentCount(int macroIndex) const;

    // Smart assignments
    void suggestAssignments(int macroIndex, const juce::String& context);
    void suggestAssignments(int macroIndex, const std::vector<juce::String>& parameterIds);
    void autoAssign(int macroIndex, const juce::StringArray& parameterIds);
    void optimizeAssignments(int macroIndex);
    std::vector<juce::String> getRecommendedAssignments(int macroIndex) const;

    // Modulation sources
    void setLFORate(int sourceIndex, float rate);
    float getLFORate(int sourceIndex) const;
    void setLFOAmount(int sourceIndex, float amount);
    float getLFOAmount(int sourceIndex) const;
    void setLFOWaveform(int sourceIndex, int waveform);
    int getLFOWaveform(int sourceIndex) const;

    void setEnvelopeASR(int sourceIndex, float attack, float sustain, float release);
    void setEnvelopeADSR(int sourceIndex, float attack, float decay, float sustain, float release);
    float getEnvelopeValue(int sourceIndex) const;

    void setRandomRate(int sourceIndex, float rate);
    float getRandomRate(int sourceIndex) const;
    void setRandomAmount(int sourceIndex, float amount);
    float getRandomAmount(int sourceIndex) const;

    // Matrix control
    void setMatrixEnabled(bool enabled);
    bool isMatrixEnabled() const;
    void setMatrixNormalize(bool normalize);
    bool isMatrixNormalized() const;
    void setMatrixGain(float gain);
    float getMatrixGain() const;

    // Real-time processing
    void processMidiEvent(const juce::MidiMessage& message);
    void processAudioLevel(int macroIndex, float level);
    void updateModulation(float deltaTime);
    void processModulationMatrix();
    void applyModulationToParameters();

    // Automation recording
    void startAutomationRecording(int macroIndex);
    void stopAutomationRecording(int macroIndex);
    bool isAutomationRecording(int macroIndex) const;
    void setAutomationRate(int macroIndex, float rate);
    float getAutomationRate(int macroIndex) const;

    // MIDI learn
    void enableMidiLearn(int macroIndex, bool enabled);
    bool isMidiLearnEnabled(int macroIndex) const;
    void assignMidiCC(int macroIndex, int channel, int cc);
    int getAssignedMidiCC(int macroIndex) const;
    void clearMidiAssignment(int macroIndex);

    // Analysis and monitoring
    float getModulationDepth(int macroIndex) const;
    float getModulationFrequency(int macroIndex) const;
    std::vector<float> getModulationOutput(int macroIndex) const;
    std::vector<float> getMacroHistory(int macroIndex) const;
    juce::Array<float> getModulationLevels() const;

    // State management
    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& state);
    void saveState(const juce::File& file);
    void loadState(const juce::File& file);

    // Export/Import
    bool exportAssignments(const juce::File& file, int macroIndex);
    bool importAssignments(const juce::File& file, int macroIndex);
    bool exportPresets(const juce::File& file);
    bool importPresets(const juce::File& file);

    // Callbacks
    void setMacroChangeListener(std::function<void(int macroIndex)> callback);
    void setAssignmentChangeListener(std::function<void(int macroIndex, int assignmentIndex)> callback);
    void setModulationListener(std::function<void(float value, int macroIndex)> callback);

    // Performance optimization
    void setUpdateRate(float rate);
    float getUpdateRate() const;
    void setSmoothingEnabled(bool enabled);
    bool isSmoothingEnabled() const;
    void setBatchProcessing(bool enabled);
    bool isBatchProcessing() const;

private:
    // System state
    SystemState state_;
    double sampleRate_;
    bool initialized_;

    // Processing
    float updateRate_;
    bool smoothingEnabled_;
    bool batchProcessing_;

    // Timing
    juce::uint64 lastUpdateTime_;
    float deltaTime_;

    // Callbacks
    std::function<void(int macroIndex)> macroChangeListener_;
    std::function<void(int macroIndex, int assignmentIndex)> assignmentChangeListener_;
    std::function<void(float value, int macroIndex)> modulationListener_;

    // Smoothed values
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, numMacros> smoothedMacros_;

    // MIDI state
    std::array<int, numMacros> assignedMidiCCs_;
    std::array<bool, numMacros> midiLearnEnabled_;

    // Automation recording
    struct AutomationRecording
    {
        bool isRecording;
        std::vector<juce::var> values;
        std::vector<juce::uint64> timestamps;
        float rate;
    };
    std::array<AutomationRecording, numMacros> automationRecording_;

    // Private helper methods
    void initializeModulationSources();
    void initializeModulationMatrix();
    void updateSmoothing(float deltaTime);
    void processMacroModulation(int macroIndex, float deltaTime);
    void processAssignments(int macroIndex, float deltaTime);
    void calculateModulationMatrix();

    // Assignment processing
    float processAssignment(const MacroAssignment& assignment, float macroValue) const;
    float applyScalingCurve(float value, ScalingCurve curve, float min, float max) const;
    float applyModulationSource(ModulationSource source, int sourceIndex) const;
    float calculateModulationDepth(const MacroAssignment& assignment) const;

    // MIDI processing
    void processMidiCC(int channel, int cc, float value);
    void processPitchBend(int channel, float value);
    void processAftertouch(int channel, float value);
    void processVelocity(int channel, float value);

    // Automation recording
    void recordAutomationValue(int macroIndex, float value);
    void saveAutomationRecording(int macroIndex);
    void loadAutomationRecording(int macroIndex, const std::vector<juce::var>& values);

    // Smart assignment algorithms
    std::vector<juce::String> analyzeParameters(const juce::StringArray& parameterIds) const;
    std::vector<juce::String> findCompatibleParameters(const MacroAssignment& assignment) const;
    float calculateAssignmentScore(const MacroAssignment& assignment) const;
    void optimizeAssignmentOrder(int macroIndex);
    void removeRedundantAssignments(int macroIndex);

    // Export/Import helpers
    juce::ValueTree createValueTreeFromMacro(const Macro& macro) const;
    Macro createMacroFromValueTree(const juce::ValueTree& tree) const;
    juce::ValueTree createValueTreeFromAssignment(const MacroAssignment& assignment) const;
    MacroAssignment createAssignmentFromValueTree(const juce::ValueTree& tree) const;

    // Utility methods
    juce::String generateAssignmentId() const;
    juce::String generateMacroId() const;
    bool validateAssignment(const MacroAssignment& assignment) const;
    bool validateMacroIndex(int index) const;
    bool validateAssignmentIndex(int macroIndex, int assignmentIndex) const;

    // Thread safety
    juce::CriticalSection systemLock_;
    juce::ScopedLock scopedLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroControlSystem)
};

} // namespace Zenith