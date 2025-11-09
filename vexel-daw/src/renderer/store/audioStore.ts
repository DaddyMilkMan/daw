import { create } from 'zustand';
import { subscribeWithSelector } from 'zustand/middleware';
import {
  RecordingState,
  AudioSettings,
  MIDISettings,
  QuantizationSettings,
  AudioContextState,
  AudioInputState,
  MIDIInputState,
  ExtendedTrack,
  AudioClip,
  MIDIClip,
  InputLevel,
  ProjectMetadata,
  RecordingTrackState,
  MIDIEvent,
} from '../types/recording';

interface AudioStore {
  // Audio Context State
  audioContext: AudioContextState;
  audioInput: AudioInputState;
  midiInput: MIDIInputState;

  // Project State
  projectMetadata: ProjectMetadata;
  tracks: ExtendedTrack[];
  audioClips: AudioClip[];
  midiClips: MIDIClip[];
  masterVolume: number;
  masterPan: number;

  // Transport State
  isPlaying: boolean;
  currentBeat: number;
  tempo: number;
  timeSignature: {
    numerator: number;
    denominator: number;
  };
  looping: boolean;
  loopStart: number;
  loopEnd: number;

  // Recording State
  recording: RecordingState;
  inputLevels: Map<string, InputLevel>;

  // Settings
  audioSettings: AudioSettings;
  midiSettings: MIDISettings;
  quantizationSettings: QuantizationSettings;

  // Actions - Audio Context
  initializeAudioContext: () => Promise<void>;
  resumeAudioContext: () => Promise<void>;
  setMasterVolume: (volume: number) => void;

  // Actions - Recording
  armTrack: (trackId: string) => void;
  disarmTrack: (trackId: string) => void;
  toggleArm: (trackId: string) => void;
  startRecording: () => Promise<void>;
  stopRecording: () => Promise<void>;
  updateInputLevel: (trackId: string, level: InputLevel) => void;

  // Actions - Transport
  play: () => void;
  pause: () => void;
  stop: () => void;
  setTempo: (tempo: number) => void;
  setCurrentBeat: (beat: number) => void;

  // Actions - Tracks
  addTrack: (type: ExtendedTrack['type'], name?: string) => string;
  removeTrack: (trackId: string) => void;
  updateTrack: (trackId: string, updates: Partial<ExtendedTrack>) => void;

  // Actions - Clips
  addAudioClip: (clip: AudioClip) => void;
  addMIDIClip: (clip: MIDIClip) => void;
  removeAudioClip: (clipId: string) => void;
  removeMIDIClip: (clipId: string) => void;

  // Actions - Settings
  updateAudioSettings: (settings: Partial<AudioSettings>) => void;
  updateMIDISettings: (settings: Partial<MIDISettings>) => void;
  updateQuantizationSettings: (settings: Partial<QuantizationSettings>) => void;

  // Actions - MIDI
  initializeMIDI: () => Promise<void>;
  selectMIDIInput: (deviceId: string) => void;

  // Actions - Input Devices
  getInputDevices: () => Promise<MediaDeviceInfo[]>;
  selectInputDevice: (deviceId: string) => Promise<void>;
}

export const useAudioStore = create<AudioStore>()(
  subscribeWithSelector((set, get) => ({
    // Initial Audio Context State
    audioContext: {
      context: null,
      masterGain: null,
      analyser: null,
      compressor: null,
      initialized: false,
    },
    audioInput: {
      stream: null,
      source: null,
      processor: null,
      analyser: null,
    },
    midiInput: {
      access: null,
      inputs: new Map(),
      outputs: new Map(),
      selectedInput: null,
    },

    // Initial Project State
    projectMetadata: {
      name: 'Untitled Project',
      author: '',
      created: new Date(),
      modified: new Date(),
      tempo: 120,
      timeSignature: {
        numerator: 4,
        denominator: 4,
      },
      sampleRate: 48000,
      bitDepth: 32,
      key: 'C',
      genre: '',
      notes: '',
    },
    tracks: [],
    audioClips: [],
    midiClips: [],
    masterVolume: 0.8,
    masterPan: 0,

    // Initial Transport State
    isPlaying: false,
    currentBeat: 0,
    tempo: 120,
    timeSignature: {
      numerator: 4,
      denominator: 4,
    },
    looping: false,
    loopStart: 0,
    loopEnd: 16,

    // Initial Recording State
    recording: {
      isRecording: false,
      isArmed: false,
      armedTracks: new Set(),
      preCountBars: 2,
      preCountRemaining: 0,
      recordingTracks: new Map(),
      inputMonitoring: false,
      latencyCompensation: 0,
    },
    inputLevels: new Map(),

    // Initial Settings
    audioSettings: {
      sampleRate: 48000,
      bitDepth: 32,
      bufferSize: 128,
      inputDevice: null,
      outputDevice: null,
      inputLatency: 0,
      outputLatency: 0,
    },
    midiSettings: {
      inputDevices: [],
      outputDevices: [],
      midiThru: false,
    },
    quantizationSettings: {
      enabled: false,
      gridValue: 1 / 16, // 1/16 notes
      strength: 100,
      swing: 50,
      quantizeNoteStart: true,
      quantizeNoteEnd: false,
      scaleQuantization: {
        enabled: false,
        root: 0, // C
        scale: 'major',
      },
      triplets: false,
    },

    // Actions - Audio Context
    initializeAudioContext: async () => {
      const { audioContext, audioSettings } = get();

      if (audioContext.initialized) {
        return;
      }

      try {
        const context = new AudioContext({
          latencyHint: 'interactive',
          sampleRate: audioSettings.sampleRate,
        });

        const masterGain = context.createGain();
        masterGain.gain.value = get().masterVolume;
        masterGain.connect(context.destination);

        const analyser = context.createAnalyser();
        analyser.fftSize = 2048;
        analyser.connect(context.destination);

        const compressor = context.createDynamicsCompressor();
        compressor.threshold.value = -24;
        compressor.knee.value = 30;
        compressor.ratio.value = 12;
        compressor.attack.value = 0.003;
        compressor.release.value = 0.25;
        compressor.connect(masterGain);

        set({
          audioContext: {
            context,
            masterGain,
            analyser,
            compressor,
            initialized: true,
          },
        });

        console.log('AudioContext initialized:', {
          sampleRate: context.sampleRate,
          baseLatency: context.baseLatency * 1000, // ms
          outputLatency: context.outputLatency * 1000, // ms
        });
      } catch (error) {
        console.error('Failed to initialize AudioContext:', error);
        throw error;
      }
    },

    resumeAudioContext: async () => {
      const { audioContext } = get();
      if (audioContext.context && audioContext.context.state === 'suspended') {
        await audioContext.context.resume();
      }
    },

    setMasterVolume: (volume: number) => {
      const { audioContext } = get();
      set({ masterVolume: volume });
      if (audioContext.masterGain) {
        audioContext.masterGain.gain.setTargetAtTime(
          volume,
          audioContext.context!.currentTime,
          0.01
        );
      }
    },

    // Actions - Recording
    armTrack: (trackId: string) => {
      set((state) => ({
        tracks: state.tracks.map((track) =>
          track.id === trackId ? { ...track, armed: true } : track
        ),
        recording: {
          ...state.recording,
          armedTracks: new Set([...state.recording.armedTracks, trackId]),
          isArmed: true,
        },
      }));
    },

    disarmTrack: (trackId: string) => {
      set((state) => {
        const newArmedTracks = new Set(state.recording.armedTracks);
        newArmedTracks.delete(trackId);

        return {
          tracks: state.tracks.map((track) =>
            track.id === trackId ? { ...track, armed: false } : track
          ),
          recording: {
            ...state.recording,
            armedTracks: newArmedTracks,
            isArmed: newArmedTracks.size > 0,
          },
        };
      });
    },

    toggleArm: (trackId: string) => {
      const { tracks } = get();
      const track = tracks.find((t) => t.id === trackId);
      if (track) {
        if (track.armed) {
          get().disarmTrack(trackId);
        } else {
          get().armTrack(trackId);
        }
      }
    },

    startRecording: async () => {
      const { recording, audioContext, currentBeat } = get();

      if (!audioContext.initialized) {
        await get().initializeAudioContext();
      }

      if (recording.armedTracks.size === 0) {
        console.warn('No tracks armed for recording');
        return;
      }

      // Start pre-count
      set((state) => ({
        recording: {
          ...state.recording,
          preCountRemaining: state.recording.preCountBars,
        },
      }));

      // Countdown timer
      const countdownInterval = setInterval(() => {
        const { recording } = get();
        if (recording.preCountRemaining > 0) {
          set((state) => ({
            recording: {
              ...state.recording,
              preCountRemaining: state.recording.preCountRemaining - 1,
            },
          }));
        } else {
          clearInterval(countdownInterval);
          // Actually start recording
          get()._actuallyStartRecording();
        }
      }, (60 / get().tempo) * get().timeSignature.numerator * 1000); // One bar in ms
    },

    _actuallyStartRecording: () => {
      const { recording, audioContext, currentBeat, audioSettings } = get();

      const recordingTracks = new Map<string, RecordingTrackState>();

      recording.armedTracks.forEach((trackId) => {
        recordingTracks.set(trackId, {
          trackId,
          startTime: audioContext.context!.currentTime,
          startBeat: currentBeat,
          recordedData: [],
          sampleRate: audioSettings.sampleRate,
          channelCount: 2, // stereo
          midiEvents: [],
        });
      });

      set((state) => ({
        recording: {
          ...state.recording,
          isRecording: true,
          recordingTracks,
        },
      }));

      console.log('Recording started on tracks:', Array.from(recording.armedTracks));
    },

    stopRecording: async () => {
      const { recording, audioContext, audioSettings } = get();

      if (!recording.isRecording) {
        return;
      }

      // Process recorded data
      const newAudioClips: AudioClip[] = [];
      const newMIDIClips: MIDIClip[] = [];

      recording.recordingTracks.forEach((trackState, trackId) => {
        const track = get().tracks.find((t) => t.id === trackId);
        if (!track) return;

        if (track.type === 'audio') {
          // Combine all recorded chunks into a single AudioBuffer
          const totalSamples = trackState.recordedData.reduce(
            (sum, chunk) => sum + chunk.length,
            0
          );

          if (totalSamples > 0) {
            const audioBuffer = audioContext.context!.createBuffer(
              trackState.channelCount,
              totalSamples / trackState.channelCount,
              trackState.sampleRate
            );

            let offset = 0;
            trackState.recordedData.forEach((chunk) => {
              const channelData = audioBuffer.getChannelData(0);
              channelData.set(chunk, offset);
              offset += chunk.length;
            });

            const lengthInBeats =
              (audioBuffer.duration * get().tempo) / 60;

            newAudioClips.push({
              id: `audio-clip-${Date.now()}-${Math.random()}`,
              trackId,
              name: `Recording ${new Date().toLocaleTimeString()}`,
              start: trackState.startBeat,
              length: lengthInBeats,
              audioBuffer,
              waveformData: audioBuffer.getChannelData(0),
              fadeIn: 0,
              fadeOut: 0,
              gain: 1,
              offset: 0,
            });
          }
        } else if (track.type === 'midi') {
          // Process MIDI events
          if (trackState.midiEvents.length > 0) {
            const notes = convertMIDIEventsToNotes(trackState.midiEvents);

            newMIDIClips.push({
              id: `midi-clip-${Date.now()}-${Math.random()}`,
              trackId,
              name: `MIDI Recording ${new Date().toLocaleTimeString()}`,
              start: trackState.startBeat,
              length: 4, // Default length, will be adjusted
              notes,
            });
          }
        }
      });

      // Add clips to store
      set((state) => ({
        audioClips: [...state.audioClips, ...newAudioClips],
        midiClips: [...state.midiClips, ...newMIDIClips],
        recording: {
          ...state.recording,
          isRecording: false,
          recordingTracks: new Map(),
          preCountRemaining: 0,
        },
      }));

      console.log('Recording stopped. Created:', {
        audioClips: newAudioClips.length,
        midiClips: newMIDIClips.length,
      });
    },

    updateInputLevel: (trackId: string, level: InputLevel) => {
      set((state) => {
        const newLevels = new Map(state.inputLevels);
        newLevels.set(trackId, level);
        return { inputLevels: newLevels };
      });
    },

    // Actions - Transport
    play: () => {
      set({ isPlaying: true });
      get().resumeAudioContext();
    },

    pause: () => {
      set({ isPlaying: false });
    },

    stop: () => {
      set({ isPlaying: false, currentBeat: 0 });
    },

    setTempo: (tempo: number) => {
      set({ tempo });
    },

    setCurrentBeat: (beat: number) => {
      set({ currentBeat: beat });
    },

    // Actions - Tracks
    addTrack: (type, name) => {
      const id = `track-${Date.now()}-${Math.random()}`;
      const track: ExtendedTrack = {
        id,
        name: name || `${type.charAt(0).toUpperCase() + type.slice(1)} Track ${get().tracks.length + 1}`,
        type,
        volume: 0.8,
        pan: 0,
        muted: false,
        solo: false,
        armed: false,
        color: getRandomTrackColor(),
        height: 80,
        inputMonitoring: false,
        inputGain: 1.0,
        sends: [],
        inserts: [],
      };

      set((state) => ({
        tracks: [...state.tracks, track],
      }));

      return id;
    },

    removeTrack: (trackId: string) => {
      set((state) => ({
        tracks: state.tracks.filter((t) => t.id !== trackId),
        audioClips: state.audioClips.filter((c) => c.trackId !== trackId),
        midiClips: state.midiClips.filter((c) => c.trackId !== trackId),
      }));
    },

    updateTrack: (trackId: string, updates: Partial<ExtendedTrack>) => {
      set((state) => ({
        tracks: state.tracks.map((track) =>
          track.id === trackId ? { ...track, ...updates } : track
        ),
      }));
    },

    // Actions - Clips
    addAudioClip: (clip: AudioClip) => {
      set((state) => ({
        audioClips: [...state.audioClips, clip],
      }));
    },

    addMIDIClip: (clip: MIDIClip) => {
      set((state) => ({
        midiClips: [...state.midiClips, clip],
      }));
    },

    removeAudioClip: (clipId: string) => {
      set((state) => ({
        audioClips: state.audioClips.filter((c) => c.id !== clipId),
      }));
    },

    removeMIDIClip: (clipId: string) => {
      set((state) => ({
        midiClips: state.midiClips.filter((c) => c.id !== clipId),
      }));
    },

    // Actions - Settings
    updateAudioSettings: (settings: Partial<AudioSettings>) => {
      set((state) => ({
        audioSettings: { ...state.audioSettings, ...settings },
      }));
    },

    updateMIDISettings: (settings: Partial<MIDISettings>) => {
      set((state) => ({
        midiSettings: { ...state.midiSettings, ...settings },
      }));
    },

    updateQuantizationSettings: (settings: Partial<QuantizationSettings>) => {
      set((state) => ({
        quantizationSettings: { ...state.quantizationSettings, ...settings },
      }));
    },

    // Actions - MIDI
    initializeMIDI: async () => {
      try {
        const access = await navigator.requestMIDIAccess({ sysex: false });

        const inputs = new Map<string, MIDIInput>();
        const outputs = new Map<string, MIDIOutput>();

        access.inputs.forEach((input) => {
          inputs.set(input.id, input);
        });

        access.outputs.forEach((output) => {
          outputs.set(output.id, output);
        });

        set((state) => ({
          midiInput: {
            ...state.midiInput,
            access,
            inputs,
            outputs,
          },
        }));

        console.log('MIDI initialized:', {
          inputs: inputs.size,
          outputs: outputs.size,
        });
      } catch (error) {
        console.error('Failed to initialize MIDI:', error);
        throw error;
      }
    },

    selectMIDIInput: (deviceId: string) => {
      const { midiInput } = get();
      const input = midiInput.inputs.get(deviceId);

      if (input) {
        // Remove previous listener if any
        if (midiInput.selectedInput) {
          midiInput.selectedInput.onmidimessage = null;
        }

        // Set up new listener
        input.onmidimessage = (event) => {
          handleMIDIMessage(event, get);
        };

        set((state) => ({
          midiInput: {
            ...state.midiInput,
            selectedInput: input,
          },
        }));
      }
    },

    // Actions - Input Devices
    getInputDevices: async () => {
      const devices = await navigator.mediaDevices.enumerateDevices();
      return devices.filter((device) => device.kind === 'audioinput');
    },

    selectInputDevice: async (deviceId: string) => {
      const { audioContext, audioInput } = get();

      // Stop existing stream
      if (audioInput.stream) {
        audioInput.stream.getTracks().forEach((track) => track.stop());
      }

      try {
        const stream = await navigator.mediaDevices.getUserMedia({
          audio: {
            deviceId: { exact: deviceId },
            sampleRate: get().audioSettings.sampleRate,
            channelCount: 2,
            echoCancellation: false,
            noiseSuppression: false,
            autoGainControl: false,
          },
        });

        if (!audioContext.context) {
          await get().initializeAudioContext();
        }

        const source = audioContext.context!.createMediaStreamSource(stream);
        const analyser = audioContext.context!.createAnalyser();
        analyser.fftSize = 2048;

        source.connect(analyser);

        set((state) => ({
          audioInput: {
            ...state.audioInput,
            stream,
            source,
            analyser,
          },
          audioSettings: {
            ...state.audioSettings,
            inputDevice: deviceId,
          },
        }));

        console.log('Input device selected:', deviceId);
      } catch (error) {
        console.error('Failed to select input device:', error);
        throw error;
      }
    },
  }))
);

// Helper Functions

function getRandomTrackColor(): string {
  const colors = [
    '#EF4444', '#F59E0B', '#10B981', '#3B82F6',
    '#8B5CF6', '#EC4899', '#14B8A6', '#F97316'
  ];
  return colors[Math.floor(Math.random() * colors.length)];
}

function convertMIDIEventsToNotes(events: MIDIEvent[]): any[] {
  const noteOnMap = new Map<number, MIDIEvent>();
  const notes: any[] = [];

  events.forEach((event) => {
    if (event.type === 'noteOn') {
      noteOnMap.set(event.data1, event);
    } else if (event.type === 'noteOff') {
      const noteOn = noteOnMap.get(event.data1);
      if (noteOn) {
        notes.push({
          id: `note-${Date.now()}-${Math.random()}`,
          pitch: event.data1,
          start: noteOn.timestamp,
          length: event.timestamp - noteOn.timestamp,
          velocity: noteOn.data2 || 100,
          selected: false,
          muted: false,
        });
        noteOnMap.delete(event.data1);
      }
    }
  });

  return notes;
}

function handleMIDIMessage(event: WebMidi.MIDIMessageEvent, get: () => AudioStore) {
  const [status, data1, data2] = event.data;
  const { recording, currentBeat } = get();

  if (!recording.isRecording) {
    return;
  }

  const messageType = status & 0xf0;
  const channel = status & 0x0f;

  let eventType: MIDIEvent['type'];

  if (messageType === 0x90 && data2 > 0) {
    eventType = 'noteOn';
  } else if (messageType === 0x80 || (messageType === 0x90 && data2 === 0)) {
    eventType = 'noteOff';
  } else if (messageType === 0xb0) {
    eventType = 'cc';
  } else if (messageType === 0xe0) {
    eventType = 'pitchBend';
  } else if (messageType === 0xd0) {
    eventType = 'aftertouch';
  } else {
    return; // Unsupported message type
  }

  const midiEvent: MIDIEvent = {
    type: eventType,
    timestamp: currentBeat,
    data1,
    data2,
    channel,
  };

  // Add event to all armed MIDI tracks
  recording.recordingTracks.forEach((trackState, trackId) => {
    const track = get().tracks.find((t) => t.id === trackId);
    if (track && (track.type === 'midi' || track.type === 'instrument')) {
      trackState.midiEvents.push(midiEvent);
    }
  });
}
