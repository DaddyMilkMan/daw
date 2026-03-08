/*
    Advanced Modulation System for Zenith Ultra Synth
    Features: Visual matrix, drag-drop routing, modulatable parameters, depth control, curve selection
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace zenith {

//==============================================================================
// Modulation Source
//==============================================================================

class ModulationSource {
public:
    enum class Type {
        LFO1,
        LFO2,
        LFO3,
        LFO4,
        ENV1,           // Amplitude envelope
        ENV2,           // Filter envelope
        ENV3,           // Mod envelope 1
        ENV4,           // Mod envelope 2
        Macro1,
        Macro2,
        Macro3,
        Macro4,
        Macro5,
        Macro6,
        Macro7,
        Macro8,
        ModWheel,
        PitchBend,
        Aftertouch,
        KeyTrack,
        Velocity,
        ReleaseVelocity,
        NoteNumber,
        Random,
        Clock,
        VelocityCurve,
        Custom
    };

    ModulationSource() = default;
    ModulationSource(Type t, const juce::String& n) : type(t), name(n) {}

    Type type = Type::LFO1;
    juce::String name;
    juce::String id;

    float currentValue = 0.0f;        // Current output (normalized)
    float bipolarValue = 0.0f;        // -1 to 1 output
    float unipolarValue = 0.0f;       // 0 to 1 output

    bool isBipolar = true;
    bool isActive = false;

    // Smoothing
    float smoothingTime = 0.0f;

    // For visualization
    juce::Colour color = juce::Colours::blue;
    float lastUpdate = 0.0f;

    static juce::String getTypeName(Type type) {
        switch (type) {
            case Type::LFO1: return "LFO 1";
            case Type::LFO2: return "LFO 2";
            case Type::LFO3: return "LFO 3";
            case Type::LFO4: return "LFO 4";
            case Type::ENV1: return "Envelope 1";
            case Type::ENV2: return "Envelope 2";
            case Type::ENV3: return "Envelope 3";
            case Type::ENV4: return "Envelope 4";
            case Type::Macro1: return "Macro 1";
            case Type::Macro2: return "Macro 2";
            case Type::Macro3: return "Macro 3";
            case Type::Macro4: return "Macro 4";
            case Type::Macro5: return "Macro 5";
            case Type::Macro6: return "Macro 6";
            case Type::Macro7: return "Macro 7";
            case Type::Macro8: return "Macro 8";
            case Type::ModWheel: return "Mod Wheel";
            case Type::PitchBend: return "Pitch Bend";
            case Type::Aftertouch: return "Aftertouch";
            case Type::KeyTrack: return "Key Track";
            case Type::Velocity: return "Velocity";
            case Type::ReleaseVelocity: return "Release Velocity";
            case Type::NoteNumber: return "Note Number";
            case Type::Random: return "Random";
            case Type::Clock: return "Clock";
            case Type::VelocityCurve: return "Velocity Curve";
            case Type::Custom: return "Custom";
            default: return "Unknown";
        }
    }

    static juce::Colour getDefaultColor(Type type) {
        switch (type) {
            case Type::LFO1:
            case Type::LFO2:
            case Type::LFO3:
            case Type::LFO4:
                return juce::Colour(0xFF2196F3);  // Blue

            case Type::ENV1:
            case Type::ENV2:
            case Type::ENV3:
            case Type::ENV4:
                return juce::Colour(0xFFFF9800);  // Orange

            case Type::Macro1: return juce::Colour(0xFF2196F3);  // Blue
            case Type::Macro2: return juce::Colour(0xFF03A9F4);  // Light Blue
            case Type::Macro3: return juce::Colour(0xFF00BCD4);  // Cyan
            case Type::Macro4: return juce::Colour(0xFF009688);  // Teal
            case Type::Macro5: return juce::Colour(0xFF4CAF50);  // Green
            case Type::Macro6: return juce::Colour(0xFF8BC34A);  // Light Green
            case Type::Macro7: return juce::Colour(0xFFCDDC39);  // Lime
            case Type::Macro8: return juce::Colour(0xFFFFEB3B);  // Yellow

            case Type::ModWheel:
            case Type::PitchBend:
            case Type::Aftertouch:
                return juce::Colour(0xFF9C27B0);  // Purple

            default:
                return juce::Colour(0xFFFFFFFF);  // White
        }
    }
};

//==============================================================================
// Modulation Destination
//==============================================================================

class ModulationDestination {
public:
    enum class Type {
        // Oscillator 1
        OSC1_Pitch,
        OSC1_PulseWidth,
        OSC1_Mix,
        OSC1_Detune,
        OSC1_Phase,
        OSC1_Pan,
        OSC1_FM,

        // Oscillator 2
        OSC2_Pitch,
        OSC2_PulseWidth,
        OSC2_Mix,
        OSC2_Detune,
        OSC2_Phase,
        OSC2_Pan,

        // Sub Oscillator
        SUB_Level,
        SUB_Octave,

        // Filter
        Filter_Cutoff,
        Filter_Resonance,
        Filter_Drive,
        Filter_EnvAmount,
        Filter_KeyTrack,

        // Envelope 1 (Amplitude)
        ENV1_Attack,
        ENV1_Decay,
        ENV1_Sustain,
        ENV1_Release,
        ENV1_VelocitySens,

        // Envelope 2 (Filter)
        ENV2_Attack,
        ENV2_Decay,
        ENV2_Sustain,
        ENV2_Release,

        // LFOs
        LFO1_Rate,
        LFO1_Depth,
        LFO2_Rate,
        LFO2_Depth,

        // Effects
        FX_Distortion,
        FX_Chorus,
        FX_ChorusRate,
        FX_ChorusDepth,
        FX_Reverb,
        FX_Delay,
        FX_DelayTime,
        FX_DelayFeedback,

        // Master
        Master_Volume,
        Master_Pan,
        Master_Pitch,

        // Global
        Global_Tune,
        Global_Vibrato
    };

    Type type = Type::OSC1_Pitch;
    juce::String name;
    juce::String id;

    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.5f;

    float currentValue = 0.5f;      // Base value
    float modulatedValue = 0.5f;     // After all modulations

    bool isModulatable = true;
    bool isAutomated = false;
    bool hasMidiAssignment = false;

    // For parameter normalization
    bool usesRawValue = false;        // If true, modulation is in raw units
    float rawMin = 0.0f;
    float rawMax = 1.0f;

    static juce::String getTypeName(Type type) {
        switch (type) {
            // OSC 1
            case Type::OSC1_Pitch: return "OSC 1 Pitch";
            case Type::OSC1_PulseWidth: return "OSC 1 Pulse Width";
            case Type::OSC1_Mix: return "OSC 1 Mix";
            case Type::OSC1_Detune: return "OSC 1 Detune";
            case Type::OSC1_Phase: return "OSC 1 Phase";
            case Type::OSC1_Pan: return "OSC 1 Pan";
            case Type::OSC1_FM: return "OSC 1 FM";

            // OSC 2
            case Type::OSC2_Pitch: return "OSC 2 Pitch";
            case Type::OSC2_PulseWidth: return "OSC 2 Pulse Width";
            case Type::OSC2_Mix: return "OSC 2 Mix";
            case Type::OSC2_Detune: return "OSC 2 Detune";
            case Type::OSC2_Phase: return "OSC 2 Phase";
            case Type::OSC2_Pan: return "OSC 2 Pan";

            // Sub
            case Type::SUB_Level: return "Sub Level";
            case Type::SUB_Octave: return "Sub Octave";

            // Filter
            case Type::Filter_Cutoff: return "Filter Cutoff";
            case Type::Filter_Resonance: return "Filter Resonance";
            case Type::Filter_Drive: return "Filter Drive";
            case Type::Filter_EnvAmount: return "Filter Env Amount";
            case Type::Filter_KeyTrack: return "Filter Key Track";

            // ENV 1
            case Type::ENV1_Attack: return "Env 1 Attack";
            case Type::ENV1_Decay: return "Env 1 Decay";
            case Type::ENV1_Sustain: return "Env 1 Sustain";
            case Type::ENV1_Release: return "Env 1 Release";
            case Type::ENV1_VelocitySens: return "Env 1 Vel Sens";

            // ENV 2
            case Type::ENV2_Attack: return "Env 2 Attack";
            case Type::ENV2_Decay: return "Env 2 Decay";
            case Type::ENV2_Sustain: return "Env 2 Sustain";
            case Type::ENV2_Release: return "Env 2 Release";

            // LFOs
            case Type::LFO1_Rate: return "LFO 1 Rate";
            case Type::LFO1_Depth: return "LFO 1 Depth";
            case Type::LFO2_Rate: return "LFO 2 Rate";
            case Type::LFO2_Depth: return "LFO 2 Depth";

            // FX
            case Type::FX_Distortion: return "Distortion";
            case Type::FX_Chorus: return "Chorus";
            case Type::FX_ChorusRate: return "Chorus Rate";
            case Type::FX_ChorusDepth: return "Chorus Depth";
            case Type::FX_Reverb: return "Reverb";
            case Type::FX_Delay: return "Delay";
            case Type::FX_DelayTime: return "Delay Time";
            case Type::FX_DelayFeedback: return "Delay Feedback";

            // Master
            case Type::Master_Volume: return "Master Volume";
            case Type::Master_Pan: return "Master Pan";
            case Type::Master_Pitch: return "Master Pitch";

            // Global
            case Type::Global_Tune: return "Global Tune";
            case Type::Global_Vibrato: return "Global Vibrato";

            default: return "Unknown";
        }
    }

    static bool isBipolar(Type type) {
        switch (type) {
            case Type::OSC1_Pitch:
            case Type::OSC2_Pitch:
            case Type::OSC1_Pan:
            case Type::OSC2_Pan:
            case Type::Master_Pan:
            case Type::Global_Vibrato:
                return true;

            default:
                return false;
        }
    }

    static juce::String getCategory(Type type) {
        if (type >= Type::OSC1_Pitch && type <= Type::OSC1_FM) return "Oscillator 1";
        if (type >= Type::OSC2_Pitch && type <= Type::OSC2_Pan) return "Oscillator 2";
        if (type >= Type::SUB_Level && type <= Type::SUB_Octave) return "Sub";
        if (type >= Type::Filter_Cutoff && type <= Type::Filter_KeyTrack) return "Filter";
        if (type >= Type::ENV1_Attack && type <= Type::ENV1_VelocitySens) return "Envelope 1";
        if (type >= Type::ENV2_Attack && type <= Type::ENV2_Release) return "Envelope 2";
        if (type >= Type::LFO1_Rate && type <= Type::LFO2_Depth) return "LFO";
        if (type >= Type::FX_Distortion && type <= Type::FX_DelayFeedback) return "Effects";
        if (type >= Type::Master_Volume && type <= Type::Master_Pitch) return "Master";
        return "Global";
    }
};

//==============================================================================
// Modulation Curve
//==============================================================================

class ModulationCurve {
public:
    enum class Type {
        Linear,         // y = x
        Exponential,    // y = x^2 (or similar)
        Logarithmic,    // y = log(x)
        Sine,           // Smooth sine curve
        EaseIn,         // Slow start, fast end
        EaseOut,        // Fast start, slow end
        EaseInOut,      // S-curve
        Step,           // Hard step
        Custom          // User-drawn
    };

    ModulationCurve() = default;

    void setType(Type type) {
        type_ = type;
    }

    void setInvert(bool invert) {
        invert_ = invert;
    }

    void setCustomCurve(const std::vector<float>& curve) {
        customCurve_ = curve;
    }

    // Process value through curve
    float process(float input) const {
        // Input is -1 to 1 (bipolar) or 0 to 1 (unipolar)
        float normalized = juce::jlimit(-1.0f, 1.0f, input);

        // For unipolar-like processing
        float uniInput = (normalized + 1.0f) * 0.5f;

        float output = 0.0f;

        switch (type_) {
            case Type::Linear:
                output = normalized;
                break;

            case Type::Exponential:
                output = normalized * std::abs(normalized);
                break;

            case Type::Logarithmic:
                output = std::copysign(std::sqrt(std::abs(normalized)), normalized);
                break;

            case Type::Sine:
                output = std::sin(normalized * juce::MathConstants<float>::pi * 0.5f);
                break;

            case Type::EaseIn:
                output = uniInput * uniInput * 2.0f - 1.0f;
                break;

            case Type::EaseOut:
                output = (1.0f - (1.0f - uniInput) * (1.0f - uniInput)) * 2.0f - 1.0f;
                break;

            case Type::EaseInOut:
                output = (uniInput < 0.5f)
                    ? uniInput * uniInput * 4.0f * 2.0f - 1.0f
                    : 1.0f - std::pow(2.0f - uniInput * 2.0f, 2.0f) * 0.5f;
                break;

            case Type::Step:
                output = (normalized > 0.0f) ? 1.0f : -1.0f;
                break;

            case Type::Custom:
                if (!customCurve_.empty()) {
                    int index = juce::jmin(static_cast<int>(uniInput * (customCurve_.size() - 1)),
                                          static_cast<int>(customCurve_.size() - 1));
                    output = customCurve_[index] * 2.0f - 1.0f;
                } else {
                    output = normalized;
                }
                break;

            default:
                output = normalized;
                break;
        }

        // Apply inversion
        if (invert_) {
            output = -output;
        }

        return output;
    }

    juce::String getTypeName() const {
        switch (type_) {
            case Type::Linear: return "Linear";
            case Type::Exponential: return "Exponential";
            case Type::Logarithmic: return "Logarithmic";
            case Type::Sine: return "Sine";
            case Type::EaseIn: return "Ease In";
            case Type::EaseOut: return "Ease Out";
            case Type::EaseInOut: return "Ease In/Out";
            case Type::Step: return "Step";
            case Type::Custom: return "Custom";
            default: return "Unknown";
        }
    }

private:
    Type type_ = Type::Linear;
    bool invert_ = false;
    std::vector<float> customCurve_;
};

//==============================================================================
// Modulation Routing
//==============================================================================

class ModulationRouting {
public:
    ModulationRouting() = default;

    ModulationRouting(const ModulationSource* src, const ModulationDestination* dest)
        : source(src), destination(dest) {}

    const ModulationSource* source = nullptr;
    const ModulationDestination* destination = nullptr;

    float amount = 0.0f;             // Depth of modulation
    float minimum = -1.0f;            // Minimum modulation amount
    float maximum = 1.0f;             // Maximum modulation amount

    ModulationCurve curve;

    bool isActive = false;            // Routing is enabled
    bool isInverted = false;          // Invert modulation

    // For macro smart mapping
    bool isSmartMapping = false;
    float smartAmount = 0.0f;         // Auto-calculated amount

    // For visualization
    float currentOutput = 0.0f;       // Current modulated value
    bool isHighlighted = false;       // UI highlight

    // Get the modulated value
    float getModulatedValue(float sourceValue) const {
        if (!source || !isActive) {
            return 0.0f;
        }

        // Apply curve
        float curved = curve.process(sourceValue);

        // Apply amount
        float modulated = curved * amount;

        // Clamp to range
        modulated = juce::jlimit(minimum, maximum, modulated);

        return modulated;
    }

    juce::String getDescription() const {
        if (!source || !destination) {
            return {};
        }

        return source->name + " → " + destination->name;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationRouting)
};

//==============================================================================
// Quick Mod (Shift+Drag on knobs)
//==============================================================================

class QuickMod {
public:
    struct Assignment {
        ModulationRouting* routing;
        float previousAmount;
    };

    // Called when shift+drag starts on a parameter
    static void beginMod(ModulationDestination* dest, ModulationSource* source) {
        // Create or find existing routing
        // Set up for interactive adjustment
    }

    // Called during shift+drag
    static void updateMod(float delta) {
        // Adjust the most recently created modulation routing
    }

    // Called when shift+drag ends
    static void endMod() {
        // Finalize the routing
    }

private:
    static ModulationDestination* currentDestination_;
    static float startAmount_;
};

//==============================================================================
// Macro Control with Smart Mapping
//==============================================================================

class MacroControl {
public:
    int index = 0;
    juce::String name = "Macro";
    juce::String id;

    float value = 0.5f;
    float defaultValue = 0.5f;
    float minValue = 0.0f;
    float maxValue = 1.0f;

    juce::Colour color = juce::Colours::blue;

    // Modulation smoothing
    float smoothingTime = 20.0f;      // ms
    float currentValue = 0.5f;        // Smoothed value

    // Assignments
    std::vector<ModulationRouting*> assignments;

    // Smart mapping
    void smartMap(const std::vector<ModulationDestination*>& availableDestinations) {
        // Auto-assign related parameters
        // Example: Macro 1 could map to all oscillator detunes
    }

    void addAssignment(ModulationRouting* routing) {
        if (routing) {
            assignments.push_back(routing);
        }
    }

    void removeAssignment(ModulationRouting* routing) {
        auto it = std::find(assignments.begin(), assignments.end(), routing);
        if (it != assignments.end()) {
            assignments.erase(it);
        }
    }

    void clearAssignments() {
        assignments.clear();
    }

    // Presets
    struct Preset {
        juce::String name;
        juce::String assignedParams;  // Comma-separated
        float amounts;
    };

    static std::vector<Preset> getPresets() {
        return {
            {"Oscillator Detune", "OSC1_Detune,OSC2_Detune", 0.5f},
            {"Filter Sweep", "Filter_Cutoff,Filter_Resonance", 0.7f},
            {"Envelope Times", "ENV1_Attack,ENV1_Decay,ENV2_Attack,ENV2_Decay", 0.5f},
            {"FX Mix", "FX_Distortion,FX_Chorus,FX_Reverb,FX_Delay", 0.5f},
            {"Vibrato", "OSC1_Pitch,OSC2_Pitch", 0.3f},
            {"Master", "Master_Volume,Master_Pan", 0.5f}
        };
    }
};

//==============================================================================
// Modulation Matrix
//==============================================================================

class ModulationMatrix {
public:
    ModulationMatrix() {
        initializeDefaultSources();
        initializeDefaultDestinations();
    }

    // Source management
    const std::vector<std::unique_ptr<ModulationSource>>& getSources() const {
        return sources_;
    }

    ModulationSource* getSource(const juce::String& id) {
        for (auto& source : sources_) {
            if (source->id == id) {
                return source.get();
            }
        }
        return nullptr;
    }

    ModulationSource* getSource(int index) {
        if (index >= 0 && index < static_cast<int>(sources_.size())) {
            return sources_[index].get();
        }
        return nullptr;
    }

    // Destination management
    const std::vector<std::unique_ptr<ModulationDestination>>& getDestinations() const {
        return destinations_;
    }

    ModulationDestination* getDestination(const juce::String& id) {
        for (auto& dest : destinations_) {
            if (dest->id == id) {
                return dest.get();
            }
        }
        return nullptr;
    }

    // Routing management
    ModulationRouting* addRouting(const juce::String& sourceId, const juce::String& destId) {
        auto* source = getSource(sourceId);
        auto* dest = getDestination(destId);

        if (!source || !dest) {
            return nullptr;
        }

        // Check if routing already exists
        for (auto& routing : routings_) {
            if (routing->source == source && routing->destination == dest) {
                return routing.get();
            }
        }

        // Create new routing
        auto routing = std::make_unique<ModulationRouting>(source, dest);
        routing->isActive = true;
        routing->amount = 0.5f;

        ModulationRouting* ptr = routing.get();
        routings_.push_back(std::move(routing));

        return ptr;
    }

    bool removeRouting(const juce::String& sourceId, const juce::String& destId) {
        auto it = std::remove_if(routings_.begin(), routings_.end(),
            [&sourceId, &destId](const std::unique_ptr<ModulationRouting>& r) {
                return r->source && r->source->id == sourceId &&
                       r->destination && r->destination->id == destId;
            });

        if (it != routings_.end()) {
            routings_.erase(it, routings_.end());
            return true;
        }

        return false;
    }

    void clearAllRoutings() {
        routings_.clear();
    }

    const std::vector<std::unique_ptr<ModulationRouting>>& getRoutings() const {
        return routings_;
    }

    std::vector<ModulationRouting*> getRoutingsForDestination(const ModulationDestination* dest) {
        std::vector<ModulationRouting*> result;

        for (auto& routing : routings_) {
            if (routing->destination == dest && routing->isActive) {
                result.push_back(routing.get());
            }
        }

        return result;
    }

    std::vector<ModulationRouting*> getRoutingsForSource(const ModulationSource* source) {
        std::vector<ModulationRouting*> result;

        for (auto& routing : routings_) {
            if (routing->source == source && routing->isActive) {
                result.push_back(routing.get());
            }
        }

        return result;
    }

    // Process all modulations
    void process() {
        // Reset all destination modulated values to base
        for (auto& dest : destinations_) {
            dest->modulatedValue = dest->currentValue;
        }

        // Apply each routing
        for (auto& routing : routings_) {
            if (routing->isActive && routing->source && routing->destination) {
                float sourceValue = routing->source->bipolarValue;
                float modulated = routing->getModulatedValue(sourceValue);

                // Add to destination
                routing->destination->modulatedValue += modulated;
                routing->currentOutput = modulated;

                // Clamp to destination range
                routing->destination->modulatedValue = juce::jlimit(
                    routing->destination->minValue,
                    routing->destination->maxValue,
                    routing->destination->modulatedValue
                );
            }
        }
    }

    // Presets
    struct Preset {
        juce::String name;
        juce::String description;
        std::vector<std::pair<juce::String, juce::String>> routings;  // source->dest pairs
        std::vector<float> amounts;
    };

    void loadPreset(const Preset& preset) {
        clearAllRoutings();

        for (size_t i = 0; i < preset.routings.size() && i < preset.amounts.size(); ++i) {
            auto& [sourceId, destId] = preset.routings[i];
            auto* routing = addRouting(sourceId, destId);
            if (routing) {
                routing->amount = preset.amounts[i];
            }
        }
    }

    Preset savePreset(const juce::String& name) const {
        Preset preset;
        preset.name = name;

        for (const auto& routing : routings_) {
            if (routing->isActive && routing->source && routing->destination) {
                preset.routings.push_back({routing->source->id, routing->destination->id});
                preset.amounts.push_back(routing->amount);
            }
        }

        return preset;
    }

    static std::vector<Preset> getFactoryPresets() {
        return {
            {
                "LFO vibrato",
                "LFO 1 modulates oscillator pitch",
                {{"LFO1", "OSC1_Pitch"}, {"LFO1", "OSC2_Pitch"}},
                {0.1f, 0.1f}
            },
            {
                "Filter sweep",
                "Envelope 2 modulates filter cutoff",
                {{"ENV2", "Filter_Cutoff"}},
                {0.8f}
            },
            {
                "Velocity to filter",
                "Velocity modulates filter resonance",
                {{"Velocity", "Filter_Resonance"}},
                {0.5f}
            },
            {
                "Mod wheel vibrato",
                "Mod wheel modulates all oscillator pitch",
                {{"ModWheel", "OSC1_Pitch"}, {"ModWheel", "OSC2_Pitch"}},
                {0.2f, 0.2f}
            },
            {
                "Aftertouch brightness",
                "Channel pressure modulates filter cutoff",
                {{"Aftertouch", "Filter_Cutoff"}},
                {0.6f}
            }
        };
    }

    // Macro controls
    std::array<MacroControl, 8>& getMacros() {
        return macros_;
    }

    MacroControl* getMacro(int index) {
        if (index >= 0 && index < 8) {
            return &macros_[index];
        }
        return nullptr;
    }

    // Search/Filter
    std::vector<ModulationRouting*> searchRoutings(const juce::String& query) {
        std::vector<ModulationRouting*> results;
        juce::String lowerQuery = query.toLowerCase();

        for (auto& routing : routings_) {
            if (!routing->isActive) continue;

            juce::String desc = routing->getDescription().toLowerCase();

            if (desc.contains(lowerQuery)) {
                results.push_back(routing.get());
            }
        }

        return results;
    }

    // Sort routings
    enum class SortBy {
        Source,
        Destination,
        Amount
    };

    void sortRoutings(SortBy sortBy) {
        std::sort(routings_.begin(), routings_.end(),
            [sortBy](const std::unique_ptr<ModulationRouting>& a,
                     const std::unique_ptr<ModulationRouting>& b) {
                if (!a->source || !b->source) return false;
                if (!a->destination || !b->destination) return false;

                switch (sortBy) {
                    case SortBy::Source:
                        return a->source->name < b->source->name;
                    case SortBy::Destination:
                        return a->destination->name < b->destination->name;
                    case SortBy::Amount:
                        return a->amount < b->amount;
                }
                return false;
            });
    }

    // Reset
    void reset() {
        for (auto& routing : routings_) {
            routing->isActive = false;
        }

        for (auto& macro : macros_) {
            macro.value = macro.defaultValue;
            macro.currentValue = macro.defaultValue;
        }
    }

private:
    void initializeDefaultSources() {
        sources_.clear();

        // LFOs
        for (int i = 0; i < 4; ++i) {
            auto source = std::make_unique<ModulationSource>();
            source->type = static_cast<ModulationSource::Type>(
                static_cast<int>(ModulationSource::Type::LFO1) + i);
            source->name = ModulationSource::getTypeName(source->type);
            source->id = "LFO" + juce::String(i + 1);
            source->color = ModulationSource::getDefaultColor(source->type);
            source->isBipolar = true;
            sources_.push_back(std::move(source));
        }

        // Envelopes
        for (int i = 0; i < 4; ++i) {
            auto source = std::make_unique<ModulationSource>();
            source->type = static_cast<ModulationSource::Type>(
                static_cast<int>(ModulationSource::Type::ENV1) + i);
            source->name = ModulationSource::getTypeName(source->type);
            source->id = "ENV" + juce::String(i + 1);
            source->color = ModulationSource::getDefaultColor(source->type);
            source->isBipolar = false;
            sources_.push_back(std::move(source));
        }

        // Macros
        for (int i = 0; i < 8; ++i) {
            auto source = std::make_unique<ModulationSource>();
            source->type = static_cast<ModulationSource::Type>(
                static_cast<int>(ModulationSource::Type::Macro1) + i);
            source->name = ModulationSource::getTypeName(source->type);
            source->id = "MACRO" + juce::String(i + 1);
            source->color = ModulationSource::getDefaultColor(source->type);
            source->isBipolar = true;
            sources_.push_back(std::move(source));
        }

        // MIDI sources
        std::vector<ModulationSource::Type> midiTypes = {
            ModulationSource::Type::ModWheel,
            ModulationSource::Type::PitchBend,
            ModulationSource::Type::Aftertouch,
            ModulationSource::Type::Velocity,
            ModulationSource::Type::KeyTrack,
            ModulationSource::Type::ReleaseVelocity
        };

        for (auto type : midiTypes) {
            auto source = std::make_unique<ModulationSource>();
            source->type = type;
            source->name = ModulationSource::getTypeName(type);
            source->id = source->name.toUpperCase().replaceCharacters(" ", "_");
            source->color = ModulationSource::getDefaultColor(type);

            // Set bipolar based on type
            source->isBipolar = (type == ModulationSource::Type::PitchBend ||
                                 type == ModulationSource::Type::ModWheel);

            sources_.push_back(std::move(source));
        }

        // Other sources
        std::vector<ModulationSource::Type> otherTypes = {
            ModulationSource::Type::Random,
            ModulationSource::Type::Clock,
            ModulationSource::Type::NoteNumber
        };

        for (auto type : otherTypes) {
            auto source = std::make_unique<ModulationSource>();
            source->type = type;
            source->name = ModulationSource::getTypeName(type);
            source->id = source->name.toUpperCase().replaceCharacters(" ", "_");
            source->color = ModulationSource::getDefaultColor(type);
            source->isBipolar = (type == ModulationSource::Type::Random);
            sources_.push_back(std::move(source));
        }
    }

    void initializeDefaultDestinations() {
        destinations_.clear();

        // Add all destination types
        auto allTypes = {
            ModulationDestination::Type::OSC1_Pitch,
            ModulationDestination::Type::OSC1_PulseWidth,
            ModulationDestination::Type::OSC1_Mix,
            ModulationDestination::Type::OSC1_Detune,
            ModulationDestination::Type::OSC1_Phase,
            ModulationDestination::Type::OSC1_Pan,
            ModulationDestination::Type::OSC1_FM,
            ModulationDestination::Type::OSC2_Pitch,
            ModulationDestination::Type::OSC2_PulseWidth,
            ModulationDestination::Type::OSC2_Mix,
            ModulationDestination::Type::OSC2_Detune,
            ModulationDestination::Type::OSC2_Phase,
            ModulationDestination::Type::OSC2_Pan,
            ModulationDestination::Type::SUB_Level,
            ModulationDestination::Type::SUB_Octave,
            ModulationDestination::Type::Filter_Cutoff,
            ModulationDestination::Type::Filter_Resonance,
            ModulationDestination::Type::Filter_Drive,
            ModulationDestination::Type::Filter_EnvAmount,
            ModulationDestination::Type::Filter_KeyTrack,
            ModulationDestination::Type::ENV1_Attack,
            ModulationDestination::Type::ENV1_Decay,
            ModulationDestination::Type::ENV1_Sustain,
            ModulationDestination::Type::ENV1_Release,
            ModulationDestination::Type::ENV2_Attack,
            ModulationDestination::Type::ENV2_Decay,
            ModulationDestination::Type::ENV2_Sustain,
            ModulationDestination::Type::ENV2_Release,
            ModulationDestination::Type::LFO1_Rate,
            ModulationDestination::Type::LFO1_Depth,
            ModulationDestination::Type::LFO2_Rate,
            ModulationDestination::Type::LFO2_Depth,
            ModulationDestination::Type::FX_Distortion,
            ModulationDestination::Type::FX_Chorus,
            ModulationDestination::Type::FX_Reverb,
            ModulationDestination::Type::FX_Delay,
            ModulationDestination::Type::Master_Volume,
            ModulationDestination::Type::Master_Pan
        };

        for (auto type : allTypes) {
            auto dest = std::make_unique<ModulationDestination>();
            dest->type = type;
            dest->name = ModulationDestination::getTypeName(type);
            dest->id = dest->name.toUpperCase().replaceCharacters(" ", "_");
            dest->isModulatable = true;

            // Set ranges based on type
            setDestinationRanges(dest.get());

            destinations_.push_back(std::move(dest));
        }
    }

    void setDestinationRanges(ModulationDestination* dest) {
        if (!dest) return;

        bool bipolar = ModulationDestination::isBipolar(dest->type);

        if (bipolar) {
            dest->minValue = -1.0f;
            dest->maxValue = 1.0f;
            dest->defaultValue = 0.0f;
        } else {
            dest->minValue = 0.0f;
            dest->maxValue = 1.0f;
            dest->defaultValue = 0.5f;
        }

        dest->currentValue = dest->defaultValue;
        dest->modulatedValue = dest->defaultValue;
    }

    std::vector<std::unique_ptr<ModulationSource>> sources_;
    std::vector<std::unique_ptr<ModulationDestination>> destinations_;
    std::vector<std::unique_ptr<ModulationRouting>> routings_;
    std::array<MacroControl, 8> macros_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrix)
};

//==============================================================================
// Modulation Manager (Main interface)
//==============================================================================

class ModulationManager {
public:
    ModulationManager() {
        matrix_ = std::make_unique<ModulationMatrix>();
    }

    ModulationMatrix& getMatrix() {
        return *matrix_;
    }

    const ModulationMatrix& getMatrix() const {
        return *matrix_;
    }

    // Update all source values
    void updateSourceValue(ModulationSource::Type type, float value) {
        if (auto* source = matrix_->getSource(static_cast<int>(type))) {
            source->currentValue = value;
            source->unipolarValue = (value + 1.0f) * 0.5f;
            source->bipolarValue = value;
            source->isActive = true;
        }
    }

    // Process all modulations
    void process() {
        matrix_->process();
    }

    // Get modulated value for a destination
    float getModulatedValue(ModulationDestination::Type type) {
        for (auto& dest : matrix_->getDestinations()) {
            if (dest->type == type) {
                return dest->modulatedValue;
            }
        }
        return 0.5f;
    }

private:
    std::unique_ptr<ModulationMatrix> matrix_;
};

} // namespace zenith
