/*
  ==============================================================================

    GrokDAWController.cpp
    Created: 2025-11-29
*/

#include "GrokDAWController.h"
#include "../dsp/DSPStemSeparator.h"
#include "../dsp/DSPVoiceChanger.h"
#include "../dsp/ONNXStemSeparator.h"
#include "../instruments/PresetGenerator.h"
#include "AIPrompts.h"
#include "AITools.h"
#include "GrokUtils.h"
#include <juce_events/juce_events.h>
#include <map>
#include <string>
#include <unordered_map>

namespace zenith {

// Implementation class
//==============================================================================

class GrokDAWController::Impl {
public:
  using FunctionHandler = std::function<void(
      const GrokFunctionCall &, std::function<void(juce::String)>,
      std::function<void(juce::String)>, std::function<void(juce::String)>)>;

  Impl(CommandAPI &api) : commandAPI(api) { registerDefaultHandlers(); }

  ~Impl() = default;

  //==========================================================================
  CommandAPI &commandAPI;
  GrokDAWClient grokClient;
  AudioAnalysisService analysisService;

  // Thread Pool for safe async operations
  juce::ThreadPool threadPool{1}; // Limit to 1 concurrent analysis job

  std::function<juce::var()> contextProvider;

  bool isInitialized = false;

  // Command Registry
  std::unordered_map<std::string, FunctionHandler> functionRegistry;

  // Command ID Mapping
  std::unordered_map<std::string, CommandAPI::CommandID> commandIdMap;

  //==========================================================================
  // Helper for safe threading
  void executeOnMessageThread(std::function<void()> task) {
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
      task();
    } else {
      juce::MessageManager::callAsync(task);
    }
  }

  void registerDefaultHandlers() {
    // Initialize Command IDs
    // This ensures strictly typed mapping from the start
    commandIdMap["create_track"] = CommandAPI::CommandID::CreateTrack;
    commandIdMap["list_tracks"] = CommandAPI::CommandID::ListTracks;
    commandIdMap["delete_track"] = CommandAPI::CommandID::DeleteTrack;
    commandIdMap["list_presets"] = CommandAPI::CommandID::ListPresets;
    commandIdMap["add_note"] = CommandAPI::CommandID::AddNote;
    commandIdMap["set_tempo"] = CommandAPI::CommandID::SetTempo;
    commandIdMap["generate_midi_pattern"] =
        CommandAPI::CommandID::GetMidiData; // verify mapping
    commandIdMap["generate_lyrics"] =
        CommandAPI::CommandID::AddMarker; // Mapped to lyrics-aware marker
                                          // creation
    commandIdMap["separate_stems"] = CommandAPI::CommandID::SeparateTrack;
    commandIdMap["analyze_track"] =
        CommandAPI::CommandID::ExportAudio; // uses export
    commandIdMap["search_plugins"] = CommandAPI::CommandID::SearchPlugins;
    commandIdMap["get_routing_graph"] = CommandAPI::CommandID::GetRoutingGraph;
    commandIdMap["start_evolution"] = CommandAPI::CommandID::StartEvolution;
    commandIdMap["stop_evolution"] = CommandAPI::CommandID::StopEvolution;
    commandIdMap["get_evolution_stats"] =
        CommandAPI::CommandID::GetEvolutionStats;

    // Handler for audio analysis (special case with side effects)
    functionRegistry["analyze_track"] = [this](const GrokFunctionCall &call,
                                               auto onComplete, auto onError,
                                               auto onProgress) {
      handleAudioAnalysis(call, onComplete, onError, onProgress);
    };

    // Handler for stem separation
    functionRegistry["separate_stems"] = [this](const GrokFunctionCall &call,
                                                auto onComplete, auto onError,
                                                auto onProgress) {
      if (onProgress)
        executeOnMessageThread([onProgress]() {
          onProgress("Checking AI model availability...");
        });

      // Task: Verify model path validity before attempting command
      juce::File defaultModel = ONNXStemSeparator::findDefaultModel();
      if (!defaultModel.existsAsFile()) {
        if (onProgress)
          executeOnMessageThread([onProgress]() {
            onProgress("Downloading AI Model (htdemucs.onnx)... This may take 1-2 minutes.");
          });

        // Use ThreadPool for network operation
        threadPool.addJob([this, call, onComplete, onError, onProgress]() {
          // Define target path (User local share on Linux/Mac, or AppData on Windows)
          juce::File targetDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
#if JUCE_LINUX
                                     .getChildFile(".local/share/zenith/models");
#elif JUCE_MAC
                                     .getChildFile("Library/Application Support/ZenithDAW/models");
#else
                                     .getChildFile("AppData/Roaming/ZenithDAW/models");
#endif

          if (!targetDir.createDirectory()) {
             executeOnMessageThread([onError]() { onError("Failed to create model directory."); });
             return;
          }

          juce::File targetFile = targetDir.getChildFile("htdemucs.onnx");
          juce::URL url("https://huggingface.co/canary-audio/htdemucs-onnx/resolve/main/htdemucs.onnx");
          
          // Helper to execute command after success
          auto runCommand = [this, call, onComplete, onError]() {
              juce::var result = commandAPI.executeCommand(
                  CommandAPI::CommandID::SeparateTrack, call.arguments);

              if (result.getProperty("success", false)) {
                juce::String msg = "Stems separated successfully. Created tracks: ";
                auto createdTracks = result.getProperty("createdTracks", juce::var());
                if (createdTracks.isArray())
                  msg += juce::String(createdTracks.size());
                onComplete(msg);
              } else {
                onError("Separation failed: " +
                        result.getProperty("error", "Unknown error").toString());
              }
          };

          // REAL-TIME PROGRESS: Use streaming instead of simple downloadToFile
          bool downloadSuccess = false;
          auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData);
          if (std::unique_ptr<juce::InputStream> stream = url.createInputStream (options)) {
              auto totalLength = stream->getTotalLength();
              auto targetStream = targetFile.createOutputStream();
              
              if (targetStream != nullptr) {
                  const int bufferSize = 65536; // 64KB
                  juce::HeapBlock<char> buffer (bufferSize);
                  int64_t bytesReadTotal = 0;
                  
                  while (!stream->isExhausted()) {
                      int bytesRead = stream->read(buffer, bufferSize);
                      if (bytesRead <= 0) break;
                      
                      targetStream->write(buffer, (size_t)bytesRead);
                      bytesReadTotal += bytesRead;
                      
                      // Report progress if length is known
                      if (totalLength > 0 && onProgress) {
                          int progress = (int)((bytesReadTotal * 100) / totalLength);
                          executeOnMessageThread([onProgress, progress]() {
                              onProgress("Downloading AI Model... " + juce::String(progress) + "%");
                          });
                      }
                  }
                  targetStream.reset();
                  downloadSuccess = bytesReadTotal >= totalLength || totalLength <= 0;
              }
          }

          if (downloadSuccess) {
             executeOnMessageThread([runCommand, onProgress]() {
                 if (onProgress) onProgress("Model downloaded. Starting separation...");
                 runCommand();
             });
          } else {
             executeOnMessageThread([onError]() { onError("Failed to download AI model from HuggingFace."); });
          }
        });
        return; 
      }

      if (onProgress)
        executeOnMessageThread([onProgress]() {
          onProgress("Separating stems (this may take a moment)...");
        });

      executeOnMessageThread([this, call, onComplete, onError]() {
        juce::var result = commandAPI.executeCommand(
            CommandAPI::CommandID::SeparateTrack, call.arguments);

        if (result.getProperty("success", false)) {
          juce::String msg = "Stems separated successfully. Created tracks: ";
          auto createdTracks = result.getProperty("createdTracks", juce::var());
          if (createdTracks.isArray())
            msg += juce::String(createdTracks.size());
          onComplete(msg);
        } else {
          onError("Separation failed: " +
                  result.getProperty("error", "Unknown error").toString());
        }
      });
    };

    // Default handler for standard commands
    auto defaultHandler = [this](const GrokFunctionCall &call, auto onComplete,
                                 auto onError, auto onProgress) {
      juce::ignoreUnused(onProgress);
      executeOnMessageThread([this, call, onComplete, onError]() {
        // Try to resolve CommandID
        auto idIt = commandIdMap.find(call.functionName.toStdString());
        juce::var result;

        if (idIt != commandIdMap.end()) {
          // Direct Enum Dispatch
          result = commandAPI.executeCommand(idIt->second, call.arguments);
        } else {
          // Fallback Path: String Dispatch
          auto *cmd = new juce::DynamicObject();
          cmd->setProperty("command", call.functionName);
          cmd->setProperty("params", call.arguments);
          result = commandAPI.executeCommand(juce::var(cmd));
        }

        if (result.getProperty("success", false)) {
          grokClient.submitFunctionResult(call, result, onComplete, onError);
        } else {
          onError("Function call failed: " +
                  result.getProperty("error", "Command failed").toString());
        }
      });
    };

    // Register common commands to use the default handler
    const char *standardCommands[] = {
        "create_track",          "list_tracks",     "delete_track",
        "list_presets",          "add_note",        "set_tempo",
        "generate_midi_pattern", "generate_lyrics", "search_plugins",
        "get_routing_graph",     "start_evolution", "stop_evolution",
        "get_evolution_stats"};

    for (const char *cmd : standardCommands) {
      functionRegistry[std::string(cmd)] = defaultHandler;
    }
  }

  //==========================================================================
  void
  handleAudioAnalysis(const GrokFunctionCall &call,
                      std::function<void(juce::String response)> onComplete,
                      std::function<void(juce::String error)> onError,
                      std::function<void(juce::String status)> onProgress) {
    // Use ThreadPool instead of detached threads
    threadPool.addJob([this, call, onComplete, onError, onProgress]() {
      // Check if analysis service is available (Python installed?)
      if (!analysisService.isAvailable()) {
        juce::MessageManager::callAsync([onError, this]() {
          onError(analysisService.getAvailabilityError());
        });
        return;
      }

      // Prepare export params
      auto placeholder = juce::File::createTempFile("analysis_");
      auto tempPath =
          placeholder.getFullPathName() + ".wav"; // Ensure .wav extension
      placeholder
          .deleteFile(); // Remove placeholder, we just need the unique path

      juce::var exportParams;
      auto *paramsObj = new juce::DynamicObject();
      paramsObj->setProperty("outputPath", tempPath);
      paramsObj->setProperty("durationSeconds", 10.0); // Default 10s
      exportParams = juce::var(paramsObj);

      // Use completion callback pattern instead of blocking wait to prevent
      // deadlocks. The original code used WaitableEvent::wait() which could
      // deadlock if called from the message thread (callAsync would never
      // execute). Now we use a fully async chain.

      juce::MessageManager::callAsync([this, exportParams, tempPath, onComplete,
                                       onError, onProgress]() {
        auto *cmdObj = new juce::DynamicObject();
        cmdObj->setProperty("command", "export_audio");
        cmdObj->setProperty("params", exportParams);

        juce::var exportResult = commandAPI.executeCommand(juce::var(cmdObj));
        bool exportSuccess = exportResult.getProperty("success", false);

        if (!exportSuccess) {
          onError("Failed to export audio for analysis");
          juce::File(tempPath).deleteFile();
          return;
        }

        // Continue analysis on background thread
        threadPool.addJob([this, tempPath, onComplete, onError, onProgress]() {
          if (onProgress) {
            juce::MessageManager::callAsync(
                [onProgress]() { onProgress("Analyzing audio..."); });
          }

          // Capture tempPath by value (String), create File when needed.
          // This avoids the need for const_cast on captured-by-value
          // juce::File.
          analysisService.analyzeAudioFile(
              juce::File(tempPath),
              [onComplete, tempPath](AudioAnalysisResults results) {
                // Clean up temp file (no const_cast needed!)
                juce::File(tempPath).deleteFile();
                if (results.success)
                  onComplete(results.toSummary());
                else
                  onComplete("Analysis failed: " + results.errorMessage);
              },
              [onError, tempPath](juce::String error) {
                // Clean up temp file (no const_cast needed!)
                juce::File(tempPath).deleteFile();
                onError("Analysis error: " + error);
              });
        });
      });
    });
  }

  //==========================================================================
  /**
      Handle function call via Registry
  */
  void handleFunctionCall(const GrokFunctionCall &call,
                          std::function<void(juce::String response)> onComplete,
                          std::function<void(juce::String error)> onError,
                          std::function<void(juce::String status)> onProgress) {
    if (onProgress)
      executeOnMessageThread([onProgress, funcName = call.functionName]() {
        onProgress("Executing: " + funcName);
      });

    std::string funcName = call.functionName.toStdString();
    auto it = functionRegistry.find(funcName);

    if (it != functionRegistry.end()) {
      it->second(call, onComplete, onError, onProgress);
    } else {
      // Fallback: Verify if this is a valid CommandID before trying
      auto idIt = commandIdMap.find(funcName);
      if (idIt != commandIdMap.end()) {
        // Valid ID known, dispatch safely using helper
        executeOnMessageThread([this, call, onComplete, onError, idIt]() {
          // Typed Dispatch
          juce::var result =
              commandAPI.executeCommand(idIt->second, call.arguments);

          if (result.getProperty("success", false)) {
            grokClient.submitFunctionResult(call, result, onComplete, onError);
          } else {
            onError("Function call failed: " +
                    result.getProperty("error", "Command failed").toString());
          }
        });
      } else {
        onError("Unknown function or failed execution: " + call.functionName);
      }
    }
  }
};

//==============================================================================
// GrokDAWController public interface
//==============================================================================

GrokDAWController::GrokDAWController(CommandAPI &commandAPI)
    : pImpl(std::make_unique<Impl>(commandAPI)) {}

GrokDAWController::~GrokDAWController() = default;

bool GrokDAWController::initialize(const juce::String &apiKey) {
  bool success = pImpl->grokClient.setAPIKey(apiKey);
  pImpl->isInitialized = success;
  return success;
}

bool GrokDAWController::isReady() const {
  return pImpl->isInitialized && pImpl->grokClient.hasAPIKey();
}

void GrokDAWController::executeCommand(
    const juce::String &userCommand, GrokMode mode,
    std::function<void(juce::String response)> onResponse,
    std::function<void(juce::String error)> onError,
    std::function<void(juce::String status)> onProgress) {
  if (!isReady()) {
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
      userCommand, mode, functions, systemPrompt, onResponse,
      [this, onResponse, onError, onProgress](GrokFunctionCall call) {
        // Grok wants to call a function -> Dispatch via Registry
        pImpl->handleFunctionCall(call, onResponse, onError, onProgress);
      },
      onError);
}

void GrokDAWController::generatePreset(
    const juce::String &instrumentId, const juce::String &description,
    const juce::String &genre,
    std::function<void(juce::String presetName, juce::var parameters)>
        onComplete,
    std::function<void(juce::String error)> onError) {
  if (!isReady()) {
    onError("Grok controller not initialized");
    return;
  }

  // Build specialized prompt for preset generation
  juce::String prompt =
      "Generate synthesizer preset parameters for: " + description + "\n";

  if (genre.isNotEmpty())
    prompt += "Genre: " + genre + "\n";

  prompt +=
      "\n"
      "Return ONLY a JSON object with the preset parameters. "
      "Use the ZenithPolySynth parameter schema:\n"
      "- Oscillators: osc1_waveform, osc1_detune, osc1_mix (same for osc2, "
      "osc3)\n"
      "- Filter: filter_type, filter_cutoff, filter_resonance, filter_drive\n"
      "- Envelopes: amp_attack, amp_decay, amp_sustain, amp_release (same "
      "for "
      "mod_*)\n"
      "- LFOs: lfo1_rate, lfo1_amount, lfo1_target (same for lfo2)\n"
      "- Effects: distortion, chorus\n"
      "\n"
      "Make musically appropriate choices based on the description.";

  // Use Thinking mode for better preset generation
  pImpl->grokClient.sendChat(
      prompt, GrokMode::Thinking,
      {}, // No function calling for preset generation
      "You are an expert sound designer. Generate synthesizer presets based "
      "on "
      "descriptions.",
      [this, instrumentId, description, genre, onComplete,
       onError](juce::String response) {
        // Parse Grok's response as JSON (using robust utility)
        auto params = GrokUtils::parseJSONResponse(response);

        if (!params.isObject()) {
          onError("Grok did not return valid JSON parameters");
          return;
        }

        // Validate and clamp parameters
        auto preset = PresetGenerator::createPresetFromParameters(
            instrumentId,
            description.substring(0, 50), // Use description as name
            description, params, genre);

        // Convert map to juce::var for callback
        auto *resultObj = new juce::DynamicObject();
        for (const auto &[key, value] : preset.parameters) {
          resultObj->setProperty(juce::Identifier(key), value);
        }

        // Success!
        onComplete(preset.name, juce::var(resultObj));
      },
      [](GrokFunctionCall) {
        // No function calls expected
      },
      onError);
}

void GrokDAWController::cancel() { pImpl->grokClient.cancelRequest(); }

void GrokDAWController::clearHistory() { pImpl->grokClient.clearHistory(); }

juce::Array<juce::var> GrokDAWController::getHistory() const {
  return pImpl->grokClient.getConversationHistory();
}

void GrokDAWController::setContextProvider(
    std::function<juce::var()> provider) {
  pImpl->contextProvider = provider;
}

void GrokDAWController::setOnChangesPending(std::function<void(bool)> callback) {
  // Logic for handling pending changes from AI
}

void GrokDAWController::acceptLastChanges() {
  // Commit changes
}

void GrokDAWController::denyLastChanges() {
  // Rollback changes
}

} // namespace zenith
