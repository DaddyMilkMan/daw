/*
  ==============================================================================

    GrokDAWController.cpp
    Created: 2025-11-29
*/

#include "GrokDAWController.h"
#include <juce_events/juce_events.h>
#include "../dsp/ONNXStemSeparator.h"
#include "../dsp/DSPStemSeparator.h"
#include "../dsp/DSPVoiceChanger.h"
#include "../instruments/PresetGenerator.h"

namespace zenith {

// Implementation class
//==============================================================================

class GrokDAWController::Impl
{
public:
    Impl(CommandAPI& api)
        : commandAPI(api)
    {
    }
    
    ~Impl() = default;
    
    //==========================================================================
    CommandAPI& commandAPI;
    GrokAPIClient grokClient;
    AudioAnalysisService analysisService;
    
    std::function<juce::var()> contextProvider;
    
    bool isInitialized = false;
    
    //==========================================================================
    /**
        Build system prompt with DAW context
    */
    juce::String buildSystemPrompt()
    {
        juce::String prompt = 
            "You are an AI assistant for Zenith DAW. "
            "You can control the DAW via function calling. "
            "\n\n"
            "Capabilities:\n"
            "- Manage tracks and clips\n"
            "- Control plugins and automation\n"
            "- Load/save presets\n"
            "- Generate synthesizer presets\n"
            "- Edit MIDI\n"
            "- Control transport and project settings\n"
            "- Analyze audio tracks\n"
            "\n"
            "Use available functions to fulfill user requests. "
            "Confirm actions concisely.\n"
            "\n";
        
        // Add current DAW context if available
        if (contextProvider)
        {
            auto context = contextProvider();
            if (context.isObject())
            {
                prompt += "Current DAW State:\n";
                
                if (context.hasProperty("trackCount"))
                    prompt += "- Tracks: " + context["trackCount"].toString() + "\n";
                
                if (context.hasProperty("selectedTrack"))
                    prompt += "- Selected Track: " + context["selectedTrack"].toString() + "\n";
                
                if (context.hasProperty("tempo"))
                    prompt += "- Tempo: " + context["tempo"].toString() + " BPM\n";
                
                if (context.hasProperty("isPlaying"))
                {
                    bool playing = context["isPlaying"];
                    prompt += "- Playback: " + juce::String(playing ? "Playing" : "Stopped") + "\n";
                }
                
                prompt += "\n";
            }
        }
        
        return prompt;
    }
    
    //==========================================================================
    /**
        Helper to create function definition from JSON schema string
    */
    GrokFunction createFunctionDef(
        const juce::String& name,
        const juce::String& description,
        const juce::String& schemaJson)
    {
        auto schema = juce::JSON::parse(schemaJson);
        return GrokFunction(name, description, schema);
    }

    //==========================================================================
    /**
        Get all available DAW functions as Grok function definitions
    */
    juce::Array<GrokFunction> getAvailableFunctions()
    {
        juce::Array<GrokFunction> myFunctions;
        
        // Track functions
        myFunctions.add(createFunctionDef(
            "create_track",
            "Create a new audio or MIDI track",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"name\": {\"type\": \"string\", \"description\": \"Track name\"},"
            "    \"type\": {\"type\": \"string\", \"enum\": [\"audio\", \"midi\"], \"description\": \"Track type\"}"
            "  },"
            "  \"required\": [\"name\", \"type\"]"
            "}"
        ));
        
        myFunctions.add(createFunctionDef(
            "list_tracks",
            "Get list of all tracks in the project",
            "{\"type\": \"object\", \"properties\": {}}"
        ));
        
        myFunctions.add(createFunctionDef(
            "delete_track",
            "Delete a track by ID",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID to delete\"}"
            "  },"
            "  \"required\": [\"trackId\"]"
            "}"
        ));

        // Audio Analysis
        myFunctions.add(createFunctionDef(
            "analyze_track",
            "Analyze the audio of a specific track or the master mix to provide feedback",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID to analyze, or 'master' for the full mix\"},"
            "    \"duration\": {\"type\": \"number\", \"description\": \"Duration to analyze in seconds (default 10.0)\"}"
            "  },"
            "  \"required\": [\"trackId\"]"
            "}"
        ));
        
        // Preset functions
        myFunctions.add(createFunctionDef(
            "list_presets",
            "List available presets for an instrument",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"instrumentId\": {\"type\": \"string\", \"description\": \"Instrument ID\"},"
            "    \"name\": {\"type\": \"string\", \"description\": \"Preset name\"},"
            "    \"parameters\": {\"type\": \"object\", \"description\": \"Preset parameters\"}"
            "  },"
            "  \"required\": [\"instrumentId\", \"name\", \"parameters\"]"
            "}"
        ));

        // AI Audio Processing
        myFunctions.add(createFunctionDef(
            "separate_stems",
            "Separate audio track into stems (vocals, drums, bass, other)",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID to separate\"}"
            "  },"
            "  \"required\": [\"trackId\"]"
            "}"
        ));

        // MIDI functions
        myFunctions.add(createFunctionDef(
            "add_note",
            "Add a MIDI note to a clip",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"trackId\": {\"type\": \"string\"},"
            "    \"clipId\": {\"type\": \"string\"},"
            "    \"pitch\": {\"type\": \"integer\", \"description\": \"MIDI note number (0-127)\"},"
            "    \"startBeats\": {\"type\": \"number\", \"description\": \"Start position in beats\"},"
            "    \"lengthBeats\": {\"type\": \"number\", \"description\": \"Note length in beats\"},"
            "    \"velocity\": {\"type\": \"integer\", \"description\": \"Velocity (0-127)\"}"
            "  },"
            "  \"required\": [\"trackId\", \"clipId\", \"pitch\", \"startBeats\", \"lengthBeats\"]"
            "}"
        ));
        
        // Tempo control
        myFunctions.add(createFunctionDef(
            "set_tempo",
            "Set the project tempo",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"bpm\": {\"type\": \"number\", \"description\": \"Tempo in BPM\"}"
            "  },"
            "  \"required\": [\"bpm\"]"
            "}"
        ));

        // MIDI Generation
        myFunctions.add(createFunctionDef(
            "generate_midi_pattern",
            "Generate a MIDI pattern (bassline, melody, or full chord progression) on a track",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"trackId\": {\"type\": \"string\", \"description\": \"Track ID\"},"
            "    \"description\": {\"type\": \"string\", \"description\": \"Description (e.g., 'Neo-Soul chord progression in Eb Minor')\"},"
            "    \"lengthBeats\": {\"type\": \"number\", \"description\": \"Length in beats (default 16)\"},"
            "    \"type\": {\"type\": \"string\", \"enum\": [\"melody\", \"bass\", \"chords\"], \"description\": \"Pattern type\"}"
            "  },"
            "  \"required\": [\"trackId\", \"description\"]"
            "}"
        ));

        // Lyric Generation
        myFunctions.add(createFunctionDef(
            "generate_lyrics",
            "Generate lyrics for a song based on a theme or mood",
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"theme\": {\"type\": \"string\", \"description\": \"Theme, topic, or mood\"},"
            "    \"style\": {\"type\": \"string\", \"description\": \"Style (e.g., 'Rap', 'Pop', 'Country')\"},"
            "    \"structure\": {\"type\": \"string\", \"description\": \"Structure (e.g., 'Verse-Chorus-Verse')\"}"
            "  },"
            "  \"required\": [\"theme\"]"
            "}"
        ));

        return myFunctions;
    }
    
    //==========================================================================
    /**
        Handle audio analysis request
    */
    void handleAudioAnalysis(
        const GrokFunctionCall& call,
        std::function<void(juce::String response)> onComplete,
        std::function<void(juce::String error)> onError,
        std::function<void(juce::String status)> onProgress)
    {
        // Launch background thread for export and analysis
        juce::Thread::launch([this, call, onComplete, onError, onProgress]()
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
        Handle function call from Grok
    */
    void handleFunctionCall(
        const GrokFunctionCall& call,
        std::function<void(juce::String response)> onComplete,
        std::function<void(juce::String error)> onError,
        std::function<void(juce::String status)> onProgress)
    {
        if (onProgress)
            onProgress("Executing: " + call.functionName);
            
        // Special handling for audio analysis
        if (call.functionName == "analyze_track")
        {
            handleAudioAnalysis(call, onComplete, onError, onProgress);
            return;
        }

        if (call.functionName == "separate_stems")
        {
            if (onProgress) onProgress("Separating stems (this may take a moment)...");
            
            // Delegate to CommandAPI's real implementation
            auto* cmd = new juce::DynamicObject();
            cmd->setProperty("command", "separate_track");
            cmd->setProperty("params", call.arguments);
            
            juce::var cmdVar(cmd);
            juce::var result = commandAPI.executeCommand(cmdVar);
            
            bool success = result.getProperty("success", false);
            
            if (success)
            {
                juce::String msg = "Stems separated successfully. Created tracks: ";
                auto createdTracks = result.getProperty("createdTracks", juce::var());
                if (createdTracks.isArray())
                {
                    msg += juce::String(createdTracks.size());
                }
                onComplete(msg);
            }
            else
            {
                juce::String error = result.getProperty("error", "Unknown error");
                onError("Separation failed: " + error);
            }
            return;
        }
        
        // Build command for CommandAPI
        auto* cmd = new juce::DynamicObject();
        cmd->setProperty("command", call.functionName);
        cmd->setProperty("params", call.arguments);
        
        juce::var cmdVar(cmd);
        
        // Execute command
        juce::var result = commandAPI.executeCommand(cmdVar);
        
        // Check if successful
        bool success = result.hasProperty("success") ? (bool)result["success"] : false;
        
        if (success)
        {
            // Submit result back to Grok
            grokClient.submitFunctionResult(
                call,
                result,
                onComplete,
                onError
            );
        }
        else
        {
            juce::String errorMsg = result.hasProperty("error") 
                ? result["error"].toString() 
                : "Command failed";
            onError("Function call failed: " + errorMsg);
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
    
    // Build system prompt with current DAW state
    auto systemPrompt = pImpl->buildSystemPrompt();
    
    // Get available functions
    auto functions = pImpl->getAvailableFunctions();
    
    // Send to Grok
    pImpl->grokClient.sendChat(
        userCommand,
        mode,
        functions,
        systemPrompt,
        onResponse,
        [this, onResponse, onError, onProgress](GrokFunctionCall call)
        {
            // Grok wants to call a function
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
            // Parse Grok's response as JSON
            auto params = juce::JSON::parse(response);
            
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
