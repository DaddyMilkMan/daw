/**
 * Centralized UI State Store (Zustand)
 *
 * Single source of truth for all UI state.
 * Updated ONLY by engine events via engineClient.onEvent()
 *
 * Components read from this store but NEVER mutate it directly.
 * All mutations go through engineClient.sendCommand()
 */

import { create } from 'zustand';
import type { AudioState, Track } from '../types/audio';

// Extended state beyond what the engine tracks
export interface UIState {
  // Core engine state (synced from engine)
  projectState: AudioState;

  // Transport state (subset of projectState for convenience)
  transportState: {
    isPlaying: boolean;
    tempo: number;
    currentBar: number;
    timeSignature: {
      numerator: number;
      denominator: number;
    };
  };

  // Mixer state (per-track)
  mixer: {
    tracks: Record<string, {
      volume: number;
      pan: number;
      muted: boolean;
      solo: boolean;
      level: number; // Current audio level (0-100)
      peak: number;  // Peak level (0-100)
    }>;
    master: {
      volume: number;
      level: number;
      peak: number;
    };
  };

  // Automation (local UI state until engine supports it)
  automation: {
    expandedTracks: Set<string>; // Tracks with automation lanes visible
    lanes: Map<string, Array<{
      id: string;
      parameter: string;
      points: Array<{ time: number; value: number }>;
    }>>;
  };

  // Session View (local UI state)
  sessionView: {
    clips: Array<{
      id: string;
      trackId: string;
      sceneIndex: number;
      name: string;
      color: string;
      length: number;
    }>;
  };

  // Record arm state (local UI state)
  recordArmed: Set<string>; // Track IDs

  // Piano Roll state (when opened)
  pianoRoll: {
    trackId: string | null;
    notes: Array<{
      id: string;
      pitch: number;
      time: number;
      duration: number;
      velocity: number;
    }>;
  } | null;

  // Browser state (entirely local)
  browser: {
    searchQuery: string;
    selectedCategory: string;
    selectedTags: string[];
    favorites: Set<string>;
  };

  // UI preferences
  preferences: {
    view: 'session' | 'arrangement';
    wingmanOpen: boolean;
    wingmanPosition: 'left' | 'right';
  };

  // Connection state
  engineConnected: boolean;
  engineMode: 'mock' | 'ipc';
}

// Actions (all state mutations happen here)
interface StoreActions {
  // Engine state updates (called by engineClient event listener)
  updateProjectState: (state: AudioState) => void;

  // Mixer updates
  setTrackVolume: (trackId: string, volume: number) => void;
  setTrackPan: (trackId: string, pan: number) => void;
  setTrackMute: (trackId: string, muted: boolean) => void;
  setTrackSolo: (trackId: string, solo: boolean) => void;
  setTrackLevel: (trackId: string, level: number, peak: number) => void;
  setMasterVolume: (volume: number) => void;
  setMasterLevel: (level: number, peak: number) => void;

  // Automation
  toggleAutomationLane: (trackId: string) => void;
  addAutomationLane: (trackId: string, parameter: string) => void;
  removeAutomationLane: (trackId: string, laneId: string) => void;
  updateAutomationPoints: (trackId: string, laneId: string, points: Array<{ time: number; value: number }>) => void;

  // Record arm
  toggleRecordArm: (trackId: string) => void;

  // Piano Roll
  openPianoRoll: (trackId: string, notes?: Array<any>) => void;
  closePianoRoll: () => void;
  updatePianoRollNotes: (notes: Array<any>) => void;

  // Browser
  setBrowserSearch: (query: string) => void;
  setBrowserCategory: (category: string) => void;
  toggleBrowserFavorite: (itemId: string) => void;

  // UI preferences
  setView: (view: 'session' | 'arrangement') => void;
  setWingmanOpen: (open: boolean) => void;
  setWingmanPosition: (position: 'left' | 'right') => void;

  // Connection
  setEngineConnected: (connected: boolean) => void;
  setEngineMode: (mode: 'mock' | 'ipc') => void;

  // Utility
  reset: () => void;
}

const initialState: UIState = {
  projectState: {
    tempo: 120,
    timeSignature: { numerator: 4, denominator: 4 },
    isPlaying: false,
    currentBar: 0,
    tracks: [],
  },

  transportState: {
    isPlaying: false,
    tempo: 120,
    currentBar: 0,
    timeSignature: { numerator: 4, denominator: 4 },
  },

  mixer: {
    tracks: {},
    master: {
      volume: 75,
      level: 0,
      peak: 0,
    },
  },

  automation: {
    expandedTracks: new Set(),
    lanes: new Map(),
  },

  sessionView: {
    clips: [],
  },

  recordArmed: new Set(),

  pianoRoll: null,

  browser: {
    searchQuery: '',
    selectedCategory: 'samples',
    selectedTags: [],
    favorites: new Set(),
  },

  preferences: {
    view: 'arrangement',
    wingmanOpen: false,
    wingmanPosition: 'right',
  },

  engineConnected: false,
  engineMode: 'mock',
};

export const useStore = create<UIState & StoreActions>((set, get) => ({
  ...initialState,

  // Engine state updates
  updateProjectState: (state: AudioState) => {
    set({
      projectState: state,
      transportState: {
        isPlaying: state.isPlaying,
        tempo: state.tempo,
        currentBar: state.currentBar,
        timeSignature: state.timeSignature,
      },
    });

    // Initialize mixer state for new tracks
    const currentMixerTracks = get().mixer.tracks;
    const newMixerTracks = { ...currentMixerTracks };

    state.tracks.forEach((track) => {
      if (!newMixerTracks[track.id]) {
        newMixerTracks[track.id] = {
          volume: track.volume ?? 75,
          pan: track.pan ?? 0,
          muted: track.muted ?? false,
          solo: track.solo ?? false,
          level: 0,
          peak: 0,
        };
      }
    });

    set((state) => ({
      mixer: {
        ...state.mixer,
        tracks: newMixerTracks,
      },
    }));
  },

  // Mixer updates
  setTrackVolume: (trackId, volume) =>
    set((state) => ({
      mixer: {
        ...state.mixer,
        tracks: {
          ...state.mixer.tracks,
          [trackId]: {
            ...state.mixer.tracks[trackId],
            volume,
          },
        },
      },
    })),

  setTrackPan: (trackId, pan) =>
    set((state) => ({
      mixer: {
        ...state.mixer,
        tracks: {
          ...state.mixer.tracks,
          [trackId]: {
            ...state.mixer.tracks[trackId],
            pan,
          },
        },
      },
    })),

  setTrackMute: (trackId, muted) =>
    set((state) => ({
      mixer: {
        ...state.mixer,
        tracks: {
          ...state.mixer.tracks,
          [trackId]: {
            ...state.mixer.tracks[trackId],
            muted,
          },
        },
      },
    })),

  setTrackSolo: (trackId, solo) =>
    set((state) => ({
      mixer: {
        ...state.mixer,
        tracks: {
          ...state.mixer.tracks,
          [trackId]: {
            ...state.mixer.tracks[trackId],
            solo,
          },
        },
      },
    })),

  setTrackLevel: (trackId, level, peak) =>
    set((state) => ({
      mixer: {
        ...state.mixer,
        tracks: {
          ...state.mixer.tracks,
          [trackId]: {
            ...state.mixer.tracks[trackId],
            level,
            peak,
          },
        },
      },
    })),

  setMasterVolume: (volume) =>
    set((state) => ({
      mixer: {
        ...state.mixer,
        master: {
          ...state.mixer.master,
          volume,
        },
      },
    })),

  setMasterLevel: (level, peak) =>
    set((state) => ({
      mixer: {
        ...state.mixer,
        master: {
          ...state.mixer.master,
          level,
          peak,
        },
      },
    })),

  // Automation
  toggleAutomationLane: (trackId) =>
    set((state) => {
      const newExpanded = new Set(state.automation.expandedTracks);
      if (newExpanded.has(trackId)) {
        newExpanded.delete(trackId);
      } else {
        newExpanded.add(trackId);
      }
      return {
        automation: {
          ...state.automation,
          expandedTracks: newExpanded,
        },
      };
    }),

  addAutomationLane: (trackId, parameter) =>
    set((state) => {
      const newLanes = new Map(state.automation.lanes);
      const trackLanes = newLanes.get(trackId) || [];
      const newLane = {
        id: `${trackId}-${parameter}-${Date.now()}`,
        parameter,
        points: [],
      };
      newLanes.set(trackId, [...trackLanes, newLane]);

      return {
        automation: {
          ...state.automation,
          lanes: newLanes,
        },
      };
    }),

  removeAutomationLane: (trackId, laneId) =>
    set((state) => {
      const newLanes = new Map(state.automation.lanes);
      const trackLanes = newLanes.get(trackId) || [];
      newLanes.set(
        trackId,
        trackLanes.filter((lane) => lane.id !== laneId)
      );

      return {
        automation: {
          ...state.automation,
          lanes: newLanes,
        },
      };
    }),

  updateAutomationPoints: (trackId, laneId, points) =>
    set((state) => {
      const newLanes = new Map(state.automation.lanes);
      const trackLanes = newLanes.get(trackId) || [];
      newLanes.set(
        trackId,
        trackLanes.map((lane) =>
          lane.id === laneId ? { ...lane, points } : lane
        )
      );

      return {
        automation: {
          ...state.automation,
          lanes: newLanes,
        },
      };
    }),

  // Record arm
  toggleRecordArm: (trackId) =>
    set((state) => {
      const newRecordArmed = new Set(state.recordArmed);
      if (newRecordArmed.has(trackId)) {
        newRecordArmed.delete(trackId);
      } else {
        newRecordArmed.add(trackId);
      }
      return { recordArmed: newRecordArmed };
    }),

  // Piano Roll
  openPianoRoll: (trackId, notes = []) =>
    set({
      pianoRoll: {
        trackId,
        notes,
      },
    }),

  closePianoRoll: () => set({ pianoRoll: null }),

  updatePianoRollNotes: (notes) =>
    set((state) => ({
      pianoRoll: state.pianoRoll
        ? {
            ...state.pianoRoll,
            notes,
          }
        : null,
    })),

  // Browser
  setBrowserSearch: (query) =>
    set((state) => ({
      browser: {
        ...state.browser,
        searchQuery: query,
      },
    })),

  setBrowserCategory: (category) =>
    set((state) => ({
      browser: {
        ...state.browser,
        selectedCategory: category,
      },
    })),

  toggleBrowserFavorite: (itemId) =>
    set((state) => {
      const newFavorites = new Set(state.browser.favorites);
      if (newFavorites.has(itemId)) {
        newFavorites.delete(itemId);
      } else {
        newFavorites.add(itemId);
      }
      return {
        browser: {
          ...state.browser,
          favorites: newFavorites,
        },
      };
    }),

  // UI preferences
  setView: (view) =>
    set((state) => ({
      preferences: {
        ...state.preferences,
        view,
      },
    })),

  setWingmanOpen: (open) =>
    set((state) => ({
      preferences: {
        ...state.preferences,
        wingmanOpen: open,
      },
    })),

  setWingmanPosition: (position) =>
    set((state) => ({
      preferences: {
        ...state.preferences,
        wingmanPosition: position,
      },
    })),

  // Connection
  setEngineConnected: (connected) => set({ engineConnected: connected }),

  setEngineMode: (mode) => set({ engineMode: mode }),

  // Utility
  reset: () => set(initialState),
}));

// Convenience selectors
export const useProjectState = () => useStore((state) => state.projectState);
export const useTransportState = () => useStore((state) => state.transportState);
export const useTracks = () => useStore((state) => state.projectState.tracks);
export const useMixer = () => useStore((state) => state.mixer);
export const useAutomation = () => useStore((state) => state.automation);
export const useRecordArmed = () => useStore((state) => state.recordArmed);
export const usePianoRoll = () => useStore((state) => state.pianoRoll);
export const useBrowser = () => useStore((state) => state.browser);
export const usePreferences = () => useStore((state) => state.preferences);
export const useEngineStatus = () => useStore((state) => ({
  connected: state.engineConnected,
  mode: state.engineMode,
}));
