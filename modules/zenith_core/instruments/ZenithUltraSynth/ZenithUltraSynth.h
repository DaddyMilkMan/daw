/*
  ==============================================================================

    ZenithUltraSynth.h
    Created: [Date] Author: Claude AI
    Based on: ZenithPolySynth architecture

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "physical_modeling/StringModel.h"
#include "physical_modeling/WindModel.h"
#include "physical_modeling/PercussionModel.h"
#include "neural_synthesis/NeuralSynthEngine.h"
#include "neural_synthesis/TimbreTransfer.h"
#include "neural_synthesis/SoundDesignAssistant.h"
#include "wavetable/AdvancedWavetableEngine.h"
#include "wavetable/WavetableEditor.h"
#include "wavetable/WavetableMorpher.h"
#include "wavetable/WavetableLibrary.h"
#include "workflow/VisualSynthesisCanvas.h"
#include "workflow/MacroControlSystem.h"
#include "workflow/PresetMorpher.h"
#include "workflow/RandomizationEngine.h"

namespace Zenith
{

class ZenithUltraSynthProcessor : public juce::MPESynthesiser
{
public:
    // Synthesis engine modes
    enum class SynthesisMode
    {
        Subtractive,      // Classic ZenithPolySynth
        PhysicalModeling, // Physical modeling synthesis
        Neural,           // AI-powered neural synthesis
        Wavetable,        // Advanced wavetable synthesis
        Hybrid            // Combine multiple engines
    };

    // Voice management
    static constexpr int maxVoices = 256;  // Massive polyphony
    static constexpr int maxUnison = 16;   // Per-oscillator unison

    ZenithUltraSynthProcessor();
    ~ZenithUltraSynthProcessor();

    // Core synthesis engine control
    void setSynthesisMode(SynthesisMode mode);
    SynthesisMode getSynthesisMode() const;

    // Voice count control
    void setVoiceCount(int voices);
    int getVoiceCount() const;

    // Unison control
    void setUnisonSize(int size);
    int getUnisonSize() const;

    // Preset management
    void loadPreset(const juce::String& presetId);
    void savePreset(const juce::String& name);
    juce::StringArray getPresetList() const;

    // Audio processing
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

    // Sample rate management
    void setSampleRate(double newSampleRate) override;

    // MPE support
    void enableMPE(bool enabled);
    bool isMPEEnabled() const;

    // Reset and clear
    void reset();
    void clearAllVoices();

    // Audio buffer management
    void setBufferSize(int bufferSize);
    int getBufferSize() const;

    // CPU monitoring
    float getCpuUsage() const;
    int getActiveVoices() const;

    // Component getters for UI
    std::unique_ptr<juce::Component> createEditor();
    WavetableEditor* getWavetableEditor();
    VisualSynthesisCanvas* getVisualCanvas();

private:
    // Component engines
    std::unique_ptr<PhysicalModelingEngine> physicalModelingEngine_;
    std::unique_ptr<NeuralSynthEngine> neuralSynthEngine_;
    std::unique_ptr<AdvancedWavetableEngine> wavetableEngine_;
    std::unique_ptr<SubtractiveEngine> subtractiveEngine_;  // From ZenithPolySynth

    // Engine management
    SynthesisMode synthesisMode_;
    int voiceCount_;
    int unisonSize_;
    bool mpeEnabled_;

    // Audio processing parameters
    double sampleRate_;
    int bufferSize_;

    // CPU monitoring
    juce::ScopedCPUUsageMeter cpuMeter_;
    float lastCpuUsage_;

    // Preset management
    juce::HashMap<juce::String, juce::ValueTree> presetDatabase_;
    juce::String currentPresetId_;

    // UI components (lazy-loaded)
    std::unique_ptr<WavetableEditor> wavetableEditor_;
    std::unique_ptr<VisualSynthesisCanvas> visualCanvas_;

    // Private helper methods
    void initializeEngines();
    void updateVoiceParameters();
    void processGlobalModulation(juce::AudioBuffer<float>& buffer, int numSamples);
};

} // namespace Zenith