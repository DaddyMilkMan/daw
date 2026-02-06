/*
  ==============================================================================

    ZenithUltraSynth_Example.cpp
    Created: [Date] Author: Claude AI
    Example usage of the ZenithUltraSynth in the Zenith ecosystem

  ==============================================================================
*/

#include "ZenithUltraSynth.h"
#include "../ZenithPolySynth.h"
#include "../../InstrumentTrack.h"

namespace Zenith
{

class ZenithUltraSynthIntegration : public InstrumentTrack
{
public:
    ZenithUltraSynthIntegration()
    {
        // Create the Ultra Synth processor
        ultraSynth_ = std::make_unique<ZenithUltraSynthProcessor>();

        // Initialize with default settings
        ultraSynth_->initialize(getSampleRate(), getBufferSize());

        // Set up MIDI processing
        ultraSynth_->enableMPE(true);

        // Load default preset
        ultraSynth_->loadPreset("default_ultra");

        // Set up audio processing callback
        setAudioProcessor(ultraSynth_.get());
    }

    ~ZenithUltraSynthIntegration()
    {
        // Clean up
        if (ultraSynth_)
        {
            ultraSynth_->shutdown();
        }
    }

    // Voice management
    void setVoiceCount(int voices) override
    {
        if (ultraSynth_)
        {
            ultraSynth_->setVoiceCount(voices);
        }
    }

    int getVoiceCount() const override
    {
        return ultraSynth_ ? ultraSynth_->getVoiceCount() : 0;
    }

    // Synthesis mode control
    void setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode mode)
    {
        if (ultraSynth_)
        {
            ultraSynth_->setSynthesisMode(mode);
        }
    }

    ZenithUltraSynthProcessor::SynthesisMode getSynthesisMode() const
    {
        return ultraSynth_ ? ultraSynth_->getSynthesisMode() : ZenithUltraSynthProcessor::SynthesisMode::Subtractive;
    }

    // Wavetable integration
    void loadWavetable(const juce::String& wavetableId)
    {
        if (auto* wavetableEngine = dynamic_cast<AdvancedWavetableEngine*>(ultraSynth_->getWavetableEngine()))
        {
            auto wavetable = wavetableEngine->getWavetableData(wavetableId);
            if (wavetable.isValid())
            {
                wavetableEngine->loadWavetable(wavetable);
            }
        }
    }

    // Visual feedback setup
    void setupVisualFeedback()
    {
        if (visualCanvas_ == nullptr && ultraSynth_)
        {
            visualCanvas_ = ultraSynth_->getVisualCanvas();
            if (visualCanvas_)
            {
                visualCanvas_->setAudioSource(getAudioBuffer());
                visualCanvas_->setViewMode(VisualSynthesisCanvas::ViewMode::Waveform);
            }
        }
    }

    // Macro control integration
    void setupMacroControls()
    {
        if (macroSystem_ == nullptr && ultraSynth_)
        {
            macroSystem_ = std::make_unique<MacroControlSystem>();
            macroSystem_->initialize(getSampleRate());

            // Set up macro assignments for common parameters
            MacroControlSystem::MacroAssignment assignment;
            assignment.parameterId = "oscillator_frequency";
            assignment.amount = 1.0f;
            assignment.minimum = 20.0f;
            assignment.maximum = 20000.0f;
            assignment.invert = false;
            assignment.curve = MacroControlSystem::ScalingCurve::Exponential;
            macroSystem_->addAssignment(0, assignment);
        }
    }

    // MIDI learn integration
    void enableMidiLearn(bool enabled)
    {
        if (ultraSynth_)
        {
            ultraSynth_->enableMPE(enabled);

            // Enable MIDI learn on macros
            if (macroSystem_)
            {
                for (int i = 0; i < MacroControlSystem::numMacros; ++i)
                {
                    macroSystem_->enableMidiLearn(i, enabled);
                }
            }
        }
    }

    // Performance monitoring
    float getCpuUsage() const override
    {
        return ultraSynth_ ? ultraSynth_->getCpuUsage() : 0.0f;
    }

    int getActiveVoices() const override
    {
        return ultraSynth_ ? ultraSynth_->getActiveVoices() : 0;
    }

    // Plugin editing
    std::unique_ptr<juce::Component> createEditor() override
    {
        auto editor = std::make_unique<juce::Component>();

        // Add synthesis mode selector
        auto modeSelector = std::make_unique<juce::ComboBox>();
        modeSelector->addItem("Subtractive", 1);
        modeSelector->addItem("Physical Modeling", 2);
        modeSelector->addItem("Neural Synthesis", 3);
        modeSelector->addItem("Wavetable", 4);
        modeSelector->addItem("Hybrid", 5);
        modeSelector->onChange = [this, modeSelector]()
        {
            int selected = modeSelector->getSelectedItemIndex();
            ZenithUltraSynthProcessor::SynthesisMode mode =
                static_cast<ZenithUltraSynthProcessor::SynthesisMode>(selected + 1);
            setSynthesisMode(mode);
        };
        editor->addChildComponent(modeSelector.get());

        // Add visual canvas
        if (visualCanvas_)
        {
            editor->addChildComponent(visualCanvas_.get());
        }

        // Add macro control panel
        if (macroSystem_)
        {
            auto macroPanel = createMacroPanel();
            editor->addChildComponent(macroPanel.get());
        }

        return editor;
    }

private:
    // Ultra Synth processor
    std::unique_ptr<ZenithUltraSynthProcessor> ultraSynth_;

    // Visual feedback
    std::unique_ptr<VisualSynthesisCanvas> visualCanvas_;

    // Macro control system
    std::unique_ptr<MacroControlSystem> macroSystem_;

    // Macro panel creation
    std::unique_ptr<juce::Component> createMacroPanel()
    {
        auto panel = std::make_unique<juce::Component>();

        // Create macro sliders
        for (int i = 0; i < MacroControlSystem::numMacros; ++i)
        {
            auto slider = std::make_unique<juce::Slider>();
            slider->setRange(0.0f, 1.0f);
            slider->setValue(0.5f);
            slider->onValueChange = [this, i](float value)
            {
                if (macroSystem_)
                {
                    macroSystem_->setMacroValue(i, value);
                }
            };
            panel->addChildComponent(slider.get());
        }

        return panel;
    }

    // Integration helper methods
    void initializeAudioProcessing()
    {
        if (ultraSynth_)
        {
            // Set up sample rate and buffer size
            ultraSynth_->setSampleRate(getSampleRate());
            ultraSynth_->setBufferSize(getBufferSize());

            // Set up voice stealing and polyphony
            ultraSynth_->setVoiceCount(64);
            ultraSynth_->setUnisonSize(1);
        }
    }

    void initializeParameterMapping()
    {
        if (macroSystem_)
        {
            // Map common parameters to macro controls
            std::vector<std::pair<juce::String, int>> parameterMappings = {
                {"oscillator_type", 0},
                {"cutoff_frequency", 1},
                {"resonance", 2},
                {"envelope_attack", 3},
                {"envelope_release", 4},
                {"lfo_rate", 5},
                {"lfo_amount", 6},
                {"reverb_amount", 7}
            };

            for (const auto& mapping : parameterMappings)
            {
                MacroControlSystem::MacroAssignment assignment;
                assignment.parameterId = mapping.first;
                assignment.amount = 1.0f;
                assignment.minimum = 0.0f;
                assignment.maximum = 1.0f;
                assignment.invert = false;
                assignment.curve = MacroControlSystem::ScalingCurve::Linear;
                assignment.enabled = true;

                macroSystem_->addAssignment(mapping.second, assignment);
            }
        }
    }
};

// Example usage in main application
class UltraSynthApplication : public juce::JUCEApplication
{
public:
    UltraSynthApplication() {}

    void initialise() override
    {
        // Create instrument track
        auto instrumentTrack = std::make_unique<ZenithUltraSynthIntegration>();

        // Set up instrument track
        instrumentTrack->setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode::Hybrid);
        instrumentTrack->setVoiceCount(128);
        instrumentTrack->enableMidiLearn(true);

        // Load presets
        instrumentTrack->loadPreset("neural_lead_01");

        // Set up visual feedback
        instrumentTrack->setupVisualFeedback();

        // Set up macro controls
        instrumentTrack->setupMacroControls();

        // Add to audio processing chain
        audioProcessorChain_.add(instrumentTrack.release());
    }

    void shutdown() override
    {
        audioProcessorChain_.clear();
    }

private:
    juce::OwnedArray<ZenithUltraSynthIntegration> audioProcessorChain_;
};

// Helper functions for integration
namespace UltraSynthHelpers
{
    // Create a preset from parameters
    juce::ValueTree createPreset(const juce::String& name,
                                ZenithUltraSynthProcessor::SynthesisMode mode,
                                const std::map<juce::String, float>& parameters)
    {
        auto preset = juce::ValueTree("Preset");
        preset.setProperty("name", name, nullptr);
        preset.setProperty("synthesisMode", juce::String(mode), nullptr);

        auto params = juce::ValueTree("Parameters");
        for (const auto& param : parameters)
        {
            params.setProperty(param.first, param.second, nullptr);
        }

        preset.addChild(params, -1, nullptr);
        return preset;
    }

    // Apply preset to processor
    void applyPreset(ZenithUltraSynthProcessor& processor, const juce::ValueTree& preset)
    {
        if (preset.hasProperty("synthesisMode"))
        {
            auto modeString = preset.getProperty("synthesisMode").toString();
            if (modeString == "Subtractive") processor.setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode::Subtractive);
            else if (modeString == "PhysicalModeling") processor.setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode::PhysicalModeling);
            else if (modeString == "Neural") processor.setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode::Neural);
            else if (modeString == "Wavetable") processor.setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode::Wavetable);
            else if (modeString == "Hybrid") processor.setSynthesisMode(ZenithUltraSynthProcessor::SynthesisMode::Hybrid);
        }

        if (preset.hasProperty("voiceCount"))
        {
            processor.setVoiceCount((int)preset.getProperty("voiceCount"));
        }

        if (preset.hasProperty("unisonSize"))
        {
            processor.setUnisonSize((int)preset.getProperty("unisonSize"));
        }
    }

    // Create morphing preset
    juce::ValueTree createMorphPreset(const juce::String& name,
                                     const juce::StringArray& presetIds,
                                     const std::vector<float>& morphAmounts)
    {
        auto morphPreset = juce::ValueTree("MorphPreset");
        morphPreset.setProperty("name", name, nullptr);

        auto presets = juce::ValueTree("Presets");
        for (size_t i = 0; i < presetIds.size(); ++i)
        {
            auto preset = juce::ValueTree("Preset");
            preset.setProperty("id", presetIds[i], nullptr);
            preset.setProperty("amount", morphAmounts[i], nullptr);
            presets.addChild(preset, -1, nullptr);
        }

        morphPreset.addChild(presets, -1, nullptr);
        return morphPreset;
    }
}

} // namespace Zenith