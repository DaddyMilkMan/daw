/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

 * @file CommandAPI.h
 * @brief JSON command API for Wingman/AI integration
 */


#include <map>
#include <memory>
#include <string>
#include <juce_core/juce_core.h>

// Forward declarations for helper classes
namespace zenith {
    class TrackCommands;
    class ClipCommands;
    class TransportCommands;
    class WingmanSynthBridge;
    namespace ai { class UXDirectorAgent; class PresetGeneticistAgent; }
}

namespace zenith {

class CommandAPI {
public:
  enum class CommandID {
      ListTracks, CreateTrack, DeleteTrack, RenameTrack, SetTrackVolume, SetTrackPan,
      ExportAudio, ExportProjectAdvanced, SeparateTrack,
      FreezeTrack, UnfreezeTrack,
      ListClips, CreateClip, DeleteClip, SplitClip, MoveClip, ResizeClip,
      Play, Stop, Record, Rewind, SetLoop, SetTempo, SetTimeSignature,
      GetSessionGraph, Undo, Redo, History,
      DescribeInstrument, AddPlugin, RemovePlugin, SetPluginParam, GetPluginParams, ListPlugins,
      AddAutomationPoint, ClearAutomation, GetAutomation,
      AddTempoChange, GetTempoMap,
      AddMarker, GetMarkers, DeleteMarker, GotoMarker,
      AddNote, DeleteNote, MoveNote, GetNotes, SetNoteVelocity, SetNoteLength, GetMidiData,
      SetClipNotes,
      ListPresets, LoadPreset, SavePreset, CreatePreset, DeletePreset, GeneratePreset,
      GetInstrumentParameters, SetInstrumentParameter, GetInstrumentParameterSchema,
      SetTrackSend, SetTrackEQ, SetTrackCompressor,
      // Aux Bus Commands
      CreateAuxBus, RemoveAuxBus, SetAuxBusVolume, SetAuxBusPan, SetAuxBusMute, GetAuxBuses,
      // Vision Command
      GetUIState,
      SearchPlugins,
      StartEvolution,
      StopEvolution,
      GetEvolutionStats,
      // Routing Graph Commands
      GetRoutingGraph, ConnectNodes, DisconnectNodes,
      // Synth Control Commands (Wingman → ZenithPolySynth)
      SetSynthParameter, SetSynthOscillatorWave, SetSynthOscillatorDetune, SetSynthOscillatorMix,
      SetSynthFilterType, SetSynthFilterCutoff, SetSynthFilterResonance, SetSynthFilterDrive,
      SetSynthAmpEnvelope, SetSynthFilterEnvelope, SetSynthLFORate, SetSynthLFOAmount,
      SetSynthDistortion, SetSynthChorus, SetSynthReverb, SetSynthDelay, SetSynthModulation,
      ApplySynthPreset, RandomizeSynthPatch, AnalyzeSynthPatch,
      // NEW: Unison, Arpeggiator, Step LFO Commands
      SetSynthUnisonVoices, SetSynthUnisonDetune, SetSynthUnisonSpread, SetSynthUnisonPanRandom,
      SetSynthArpEnable, SetSynthArpMode, SetSynthArpRate, SetSynthArpGate, SetSynthArpSwing, SetSynthArpHold,
      SetSynthStepLFO1Enable, SetSynthStepLFO1Steps, SetSynthStepLFO1Rate, SetSynthStepLFO1Smoothing,
      SetSynthStepLFO2Enable, SetSynthStepLFO2Steps, SetSynthStepLFO2Rate, SetSynthStepLFO2Smoothing,
      SetSynthStepLFO3Enable, SetSynthStepLFO3Steps, SetSynthStepLFO3Rate, SetSynthStepLFO3Smoothing,
      SetSynthStepLFO4Enable, SetSynthStepLFO4Steps, SetSynthStepLFO4Rate, SetSynthStepLFO4Smoothing,
      // AI MIDI Pattern Generation Commands
      GenerateDrums, GenerateBass, GenerateChords, GenerateMelody, GenerateArpeggio,
      GenerateFullPattern
  };

  //==========================================================================
  // Command API (Step 2)
  //==========================================================================
  
  // The UI or AI calls this
  void setTrackVolume(int trackIndex, float newVolume) {
      auto tracks = projectState.getState().getChildWithName(ProjectState::ID_TRACKS);
      auto track = tracks.getChild(trackIndex);
      
      if (track.isValid()) {
          track.setProperty(ProjectState::PROP_VOLUME, newVolume, &projectState.getUndoManager());
      }
  }

  void undo() { projectState.getUndoManager().undo(); }
  void redo() { projectState.getUndoManager().redo(); }

  ProjectState& getProjectState() { return projectState; }

  bool performAction(std::unique_ptr<juce::UndoableAction> action) {
      if (action == nullptr) return false;
      return projectState.getUndoManager().perform(action.release());
  }

  //==========================================================================
  CommandAPI(ProjectState &projectState, Engine &engine);
  ~CommandAPI();

  //==========================================================================
  juce::var executeCommand(const juce::var &request);
  juce::var executeCommand(CommandID id, const juce::var &params);
  juce::String executeCommand(const juce::String &jsonRequest);
  
  juce::var executeBatch(const juce::Array<juce::var>& commands, const juce::String& batchName);
  


  using CommandHandler = std::function<juce::var(const juce::var &params)>;
  void registerCommand(const juce::String &commandName, CommandHandler handler);

  void setUXDirector(ai::UXDirectorAgent* agent) { uxDirector_ = agent; }
  void setPresetGeneticist(ai::PresetGeneticistAgent* agent) { presetGeneticist_ = agent; }

  juce::var startEvolution(const juce::var& params);
  juce::var stopEvolution(const juce::var& params);
  juce::var getEvolutionStats(const juce::var& params);

private:
  void initializeCommandMap();

  // Member functions matching cpp implementation
  juce::var exportAudio(const juce::var& params);
  juce::var exportProjectAdvanced(const juce::var& params);
  juce::var getSessionGraph(const juce::var& params);
  juce::var undo(const juce::var& params);
  juce::var redo(const juce::var& params);
  juce::var history(const juce::var& params);
  juce::var describeInstrument(const juce::var& params);
  juce::var addPlugin(const juce::var& params);
  juce::var removePlugin(const juce::var& params);
  juce::var listPlugins(const juce::var& params);
  juce::var searchPlugins(const juce::var& params);
  juce::var setPluginParam(const juce::var& params);
  juce::var getPluginParams(const juce::var& params);
  juce::var addAutomationPoint(const juce::var& params);
  juce::var clearAutomation(const juce::var& params);
  juce::var getAutomation(const juce::var& params);
  juce::var addMarker(const juce::var& params);
  juce::var getMarkers(const juce::var& params);
  juce::var deleteMarker(const juce::var& params);
  juce::var gotoMarker(const juce::var& params);
  juce::var addNote(const juce::var& params);
  juce::var deleteNote(const juce::var& params);
  juce::var moveNote(const juce::var& params);
  juce::var getNotes(const juce::var& params);
  juce::var setNoteVelocity(const juce::var& params);
  juce::var setNoteLength(const juce::var& params);
  juce::var getMidiData(const juce::var& params);
  
  // Instrument/Preset methods (assuming they exist in cpp)
  juce::var listPresets(const juce::var& params);
  juce::var loadPreset(const juce::var& params);
  juce::var savePreset(const juce::var& params);
  juce::var createPreset(const juce::var& params);
  juce::var deletePreset(const juce::var& params);
  juce::var generatePreset(const juce::var& params);
  juce::var getInstrumentParameters(const juce::var& params);
  juce::var setInstrumentParameter(const juce::var& params);
  juce::var getInstrumentParameterSchema(const juce::var& params);

  // Aux Bus Handlers
  juce::var createAuxBus(const juce::var& params);
  juce::var removeAuxBus(const juce::var& params);
  juce::var setAuxBusVolume(const juce::var& params);
  juce::var setAuxBusPan(const juce::var& params);
  juce::var setAuxBusMute(const juce::var& params);
  juce::var getAuxBuses(const juce::var& params);

  // Vision Handlers
  juce::var getUIHealth(const juce::var& params);
  juce::var getUIState(const juce::var& params);

  // Routing Graph Handlers
  juce::var getRoutingGraph(const juce::var& params);
  juce::var connectNodes(const juce::var& params);
  juce::var disconnectNodes(const juce::var& params);

  // Synth Control Handlers (Wingman → ZenithPolySynth)
  juce::var setSynthParameter(const juce::var& params);
  juce::var setSynthOscillatorWave(const juce::var& params);
  juce::var setSynthOscillatorDetune(const juce::var& params);
  juce::var setSynthOscillatorMix(const juce::var& params);
  juce::var setSynthOscillatorShape(const juce::var& params);
  juce::var setSynthFilterType(const juce::var& params);
  juce::var setSynthFilterCutoff(const juce::var& params);
  juce::var setSynthFilterResonance(const juce::var& params);
  juce::var setSynthFilterDrive(const juce::var& params);
  juce::var setSynthAmpEnvelope(const juce::var& params);
  juce::var setSynthFilterEnvelope(const juce::var& params);
  juce::var setSynthLFORate(const juce::var& params);
  juce::var setSynthLFOAmount(const juce::var& params);
  juce::var setSynthLFOWaveform(const juce::var& params);
  juce::var setSynthDistortion(const juce::var& params);
  juce::var setSynthChorus(const juce::var& params);
  juce::var setSynthReverb(const juce::var& params);
  juce::var setSynthDelay(const juce::var& params);
  juce::var setSynthModulation(const juce::var& params);
  juce::var applySynthPreset(const juce::var& params);
  juce::var randomizeSynthPatch(const juce::var& params);
  juce::var analyzeSynthPatch(const juce::var& params);

  // NEW: Unison Commands
  juce::var setSynthUnisonVoices(const juce::var& params);
  juce::var setSynthUnisonDetune(const juce::var& params);
  juce::var setSynthUnisonSpread(const juce::var& params);
  juce::var setSynthUnisonPanRandom(const juce::var& params);

  // NEW: Arpeggiator Commands
  juce::var setSynthArpEnable(const juce::var& params);
  juce::var setSynthArpMode(const juce::var& params);
  juce::var setSynthArpRate(const juce::var& params);
  juce::var setSynthArpGate(const juce::var& params);
  juce::var setSynthArpSwing(const juce::var& params);
  juce::var setSynthArpHold(const juce::var& params);

  // NEW: Step LFO Commands
  juce::var setSynthStepLFO1Enable(const juce::var& params);
  juce::var setSynthStepLFO1Steps(const juce::var& params);
  juce::var setSynthStepLFO1Rate(const juce::var& params);
  juce::var setSynthStepLFO1Smoothing(const juce::var& params);
  juce::var setSynthStepLFO2Enable(const juce::var& params);
  juce::var setSynthStepLFO2Steps(const juce::var& params);
  juce::var setSynthStepLFO2Rate(const juce::var& params);
  juce::var setSynthStepLFO2Smoothing(const juce::var& params);
  juce::var setSynthStepLFO3Enable(const juce::var& params);
  juce::var setSynthStepLFO3Steps(const juce::var& params);
  juce::var setSynthStepLFO3Rate(const juce::var& params);
  juce::var setSynthStepLFO3Smoothing(const juce::var& params);
  juce::var setSynthStepLFO4Enable(const juce::var& params);
  juce::var setSynthStepLFO4Steps(const juce::var& params);
  juce::var setSynthStepLFO4Rate(const juce::var& params);
  juce::var setSynthStepLFO4Smoothing(const juce::var& params);

  // AI MIDI Pattern Generation Commands
  juce::var generateDrums(const juce::var& params);
  juce::var generateBass(const juce::var& params);
  juce::var generateChords(const juce::var& params);
  juce::var generateMelody(const juce::var& params);
  juce::var generateArpeggio(const juce::var& params);
  juce::var generateFullPattern(const juce::var& params);

  // Helpers
  juce::String createResponse(const juce::var &data) const;
  juce::String createErrorResponse(const juce::String &errorMessage) const;
  juce::var createSuccessResponse(const juce::var& result = juce::var()) const;
  bool validateParam(const juce::var &params, const juce::String &paramName, juce::String &errorOut) const;

  // Members
  ProjectState &projectState;
  Engine &engine;

  std::unique_ptr<TrackCommands> trackCommands;
  std::unique_ptr<ClipCommands> clipCommands;
  std::unique_ptr<TransportCommands> transportCommands;

  std::map<std::string, CommandID> commandMap;
  std::map<juce::String, CommandHandler> commandHandlers;

  ai::UXDirectorAgent* uxDirector_ = nullptr;
  ai::PresetGeneticistAgent* presetGeneticist_ = nullptr;

  // Helper to get WingmanSynthBridge for active track
  WingmanSynthBridge* getSynthBridgeForActiveTrack();
  WingmanSynthBridge* getSynthBridgeForTrack(const juce::String& trackId);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandAPI)
};

} // namespace zenith
