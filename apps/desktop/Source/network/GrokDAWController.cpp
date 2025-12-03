/*
  ==============================================================================

    GrokDAWController.cpp
    Created: 2025-11-29
*/

#include "GrokDAWController.h"
#include "GrokUtils.h"
#include "AITools.h"
#include "AIPrompts.h"
#include <juce_events/juce_events.h>
#include "../dsp/ONNXStemSeparator.h"
#include "../dsp/DSPStemSeparator.h"
#include "../dsp/DSPVoiceChanger.h"
#include "../instruments/PresetGenerator.h"
#include <map>

namespace zenith {

// Implementation class
//==============================================================================

class GrokDAWController::Impl
{
public:
    using FunctionHandler = std::function<void(const GrokFunctionCall&, 
                                             std::function<void(juce::String)>, 
                                             std::function<void(juce::String)>, 
                                             std::function<void(juce::String)>)>;

    Impl(CommandAPI& api)
        : commandAPI(api)
    {
        registerDefaultHandlers();
    }
    
    ~Impl() = default;
    
    //==========================================================================
    CommandAPI& commandAPI;
    GrokAPIClient grokClient;
    AudioAnalysisService analysisService;
    
    // Thread Pool for safe async operations (Complaint #8 Fix)
    juce::ThreadPool threadPool{1}; // Limit to 1 concurrent analysis job for now
    
    std::function<juce::var()> contextProvider;
    
    bool isInitialized = false;
    
    // Command Registry (Complaint #2 Fix)
    std::map<juce::String, FunctionHandler> functionRegistry;

    //==========================================================================
    void registerDefaultHandlers()
    {
        // ... (handlers remain the same)
        // Handler for audio analysis (special case with side effects)
        functionRegistry["analyze_track"] = [this](const GrokFunctionCall& call, auto onComplete, auto onError, auto onProgress) {
            handleAudioAnalysis(call, onComplete, onError, onProgress);
        };

        // Handler for stem separation
        functionRegistry["separate_stems"] = [this](const GrokFunctionCall& call, auto onComplete, auto onError, auto onProgress) {
            if (onProgress) onProgress("Separating stems (this may take a moment)...");
            
            // Delegate to CommandAPI
            auto* cmd = new juce::DynamicObject();
            cmd->setProperty("command", "separate_track");
            cmd->setProperty("params", call.arguments);
            
            juce::var result = commandAPI.executeCommand(juce::var(cmd));
            
            if (result.getProperty("success", false))
            {
                juce::String msg = "Stems separated successfully. Created tracks: ";
                auto createdTracks = result.getProperty("createdTracks", juce::var());
                if (createdTracks.isArray()) msg += juce::String(createdTracks.size());
                onComplete(msg);
            }
            else
            {
                onError("Separation failed: " + result.getProperty("error", "Unknown error").toString());
            }
        };

        // Default handler for all other CommandAPI commands
        auto defaultHandler = [this](const GrokFunctionCall& call, auto onComplete, auto onError, auto onProgress) {
            auto* cmd = new juce::DynamicObject();
            cmd->setProperty("command", call.functionName);
            cmd->setProperty("params", call.arguments);
            
            juce::var result = commandAPI.executeCommand(juce::var(cmd));
            
            if (result.getProperty("success", false))
            {
                grokClient.submitFunctionResult(call, result, onComplete, onError);
            }
            else
            {
                onError("Function call failed: " + result.getProperty("error", "Command failed").toString());
            }
        };

        // Register common commands to use the default handler
        const char* standardCommands[] = {
            "create_track", "list_tracks", "delete_track", 
            "list_presets", "add_note", "set_tempo", 
            "generate_midi_pattern", "generate_lyrics"
        };

        for (const char* cmd : standardCommands) {
            functionRegistry[cmd] = defaultHandler;
        }
    }

    //==========================================================================
    void handleAudioAnalysis(
        const GrokFunctionCall& call,
        std::function<void(juce::String response)> onComplete,
        std::function<void(juce::String error)> onError,
        std::function<void(juce::String status)> onProgress)
    {
        // Use ThreadPool instead of detached threads (Complaint #8 Fix)
        threadPool.addJob([this, call, onComplete, onError, onProgress]()
        {
            // Check if analysis service is available (Python installed?)
            if (!analysisService.isAvailable())
            {
                juce::MessageManager::callAsync([onError, this]()
                { 
                    onError(analysisService.getAvailabilityError()); 
                });
                return;
            }

            // Prepare export params
            juce::var exportParams;
            auto* paramsObj = new juce::DynamicObject();
            paramsObj->setProperty("outputPath", juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("analysis_temp.wav").getFullPathName());
            paramsObj->setProperty("durationSeconds", 10.0); // Default 10s
            exportParams = juce::var(paramsObj);

            // Execute export command (must be done on Message Thread for safety with current Engine)
            bool exportSuccess = false;
            juce::WaitableEvent exportFinished;
            
            juce::MessageManager::callAsync([this, exportParams, &exportSuccess, &exportFinished]() {
                auto* cmdObj = new juce::DynamicObject();
                cmdObj->setProperty("command", "export_audio");
                cmdObj->setProperty("params", exportParams);
                
                juce::var exportResult = commandAPI.executeCommand(juce::var(cmdObj));
                exportSuccess = exportResult.getProperty("success", false);
                exportFinished.signal();
            });
            
            exportFinished.wait();
            
            if (!exportSuccess)
            {
                juce::MessageManager::callAsync([onError](){ onError("Failed to export audio for analysis"); });
                return;
            }

            // 2. Analyze audio (Safe to run on background thread)
            if (onProgress) 
            {
                 juce::MessageManager::callAsync([onProgress](){ onProgress("Analyzing audio..."); });
            }

            juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("analysis_temp.wav");

            analysisService.analyzeAudioFile(
                tempFile,
                [onComplete, tempFile](AudioAnalysisResults results) {
                    const_cast<juce::File&>(tempFile).deleteFile();
                    if (results.success)
                        onComplete(results.toSummary());
                    else
                        onComplete("Analysis failed: " + results.errorMessage);
                },
                [onError, tempFile](juce::String error) {
                    const_cast<juce::File&>(tempFile).deleteFile();
                    onError("Analysis error: " + error);
                }
            );
        });
    }

    //==========================================================================
    /**
        Handle function call via Registry
    */
    void handleFunctionCall(
        const GrokFunctionCall& call,
        std::function<void(juce::String response)> onComplete,
        std::function<void(juce::String error)> onError,
        std::function<void(juce::String status)> onProgress)
    {
        if (onProgress)
            onProgress("Executing: " + call.functionName);
            
        auto it = functionRegistry.find(call.functionName);
        if (it != functionRegistry.end())
        {
            it->second(call, onComplete, onError, onProgress);
        }
        else
        {
            // Fallback: Try to execute as a generic command even if not explicitly registered
            // This allows CommandAPI to expand without updating this registry every time
            auto* cmd = new juce::DynamicObject();
            cmd->setProperty("command", call.functionName);
            cmd->setProperty("params", call.arguments);
            
            juce::var result = commandAPI.executeCommand(juce::var(cmd));
            
            if (result.getProperty("success", false))
            {
                grokClient.submitFunctionResult(call, result, onComplete, onError);
            }
            else
            {
                onError("Unknown function or failed execution: " + call.functionName);
            }
        }
    }
};

//==============================================================================
// GrokDAWController public interface
//==============================================================================

GrokDAWController::GrokDAWController(CommandAPI& commandAPI)
    : pImpl(std::make_unique<Impl>(commandAPI))
{
}

GrokDAWController::~GrokDAWController() = default;

bool GrokDAWController::initialize(const juce::String& apiKey)
{
    bool success = pImpl->grokClient.setAPIKey(apiKey);
    pImpl->isInitialized = success;
    return success;
}

bool GrokDAWController::isReady() const
{
    return pImpl->isInitialized && pImpl->grokClient.hasAPIKey();
}

void GrokDAWController::executeCommand(
    const juce::String& userCommand,
    GrokMode mode,
    std::function<void(juce::String response)> onResponse,
    std::function<void(juce::String error)> onError,
    std::function<void(juce::String status)> onProgress)
{
    if (!isReady())
    {
        onError("Grok controller not initialized");
        return;
    }
    
    if (onProgress)
        onProgress("Processing command...");
    
    // Task 4: Use helper for system prompt
    auto systemPrompt = AIPrompts::buildSystemPrompt(pImpl->contextProvider);
    
    // Task 4: Use helper for tool definitions
    auto functions = AITools::getAvailableFunctions();
    
    // Send to Grok
    pImpl->grokClient.sendChat(
        userCommand,
        mode,
        functions,
        systemPrompt,
        onResponse,
        [this, onResponse, onError, onProgress](GrokFunctionCall call)
        {
            // Grok wants to call a function -> Dispatch via Registry
            pImpl->handleFunctionCall(call, onResponse, onError, onProgress);
        },
        onError
    );
}

void GrokDAWController::generatePreset(
    const juce::String& instrumentId,
    const juce::String& description,
    const juce::String& genre,
    std::function<void(juce::String presetName, juce::var parameters)> onComplete,
    std::function<void(juce::String error)> onError)
{
    if (!isReady())
    {
        onError("Grok controller not initialized");
        return;
    }
    
    // Build specialized prompt for preset generation
    juce::String prompt = 
        "Generate synthesizer preset parameters for: " + description + "\n";
    
    if (genre.isNotEmpty())
        prompt += "Genre: " + genre + "\n";
    
    prompt += "\n"
        "Return ONLY a JSON object with the preset parameters. "
        "Use the ZenithPolySynth parameter schema:\n"
        "- Oscillators: osc1_waveform, osc1_detune, osc1_mix (same for osc2, osc3)\n"
        "- Filter: filter_type, filter_cutoff, filter_resonance, filter_drive\n"
        "- Envelopes: amp_attack, amp_decay, amp_sustain, amp_release (same for mod_*)\n"
        "- LFOs: lfo1_rate, lfo1_amount, lfo1_target (same for lfo2)\n"
        "- Effects: distortion, chorus\n"
        "\n"
        "Make musically appropriate choices based on the description.";
    
    // Use Thinking mode for better preset generation
    pImpl->grokClient.sendChat(
        prompt,
        GrokMode::Thinking,
        {}, // No function calling for preset generation
        "You are an expert sound designer. Generate synthesizer presets based on descriptions.",
        [this, instrumentId, description, genre, onComplete, onError](juce::String response)
        {
            // Parse Grok's response as JSON (using robust utility)
            auto params = GrokUtils::parseJSONResponse(response);
            
            if (!params.isObject())
            {
                onError("Grok did not return valid JSON parameters");
                return;
            }
            
            // Validate and clamp parameters
            auto preset = PresetGenerator::createPresetFromParameters(
                instrumentId,
                description.substring(0, 50), // Use description as name
                description,
                params,
                genre
            );
            
            // Convert map to juce::var for callback
            auto* resultObj = new juce::DynamicObject();
            for (const auto& [key, value] : preset.parameters)
            {
                resultObj->setProperty(juce::Identifier(key), value);
            }
            
            // Success!
            onComplete(preset.name, juce::var(resultObj));
        },
        [](GrokFunctionCall) {
            // No function calls expected
        },
        onError
    );
}

void GrokDAWController::cancel()
{
    pImpl->grokClient.cancelRequest();
}

void GrokDAWController::clearHistory()
{
    pImpl->grokClient.clearHistory();
}

juce::Array<juce::var> GrokDAWController::getHistory() const
{
    return pImpl->grokClient.getConversationHistory();
}

void GrokDAWController::setContextProvider(std::function<juce::var()> provider)
{
    pImpl->contextProvider = provider;
}

} // namespace zenith
