/**
 * Local storage utilities for persisting DAW settings
 */

export interface MetronomeSettings {
  volume: number;
  preCountEnabled: boolean;
  preCountBars: number;
}

export interface ProjectSettings {
  tempo: number;
  timeSignature: {
    numerator: number;
    denominator: number;
  };
  metronome: MetronomeSettings;
}

const STORAGE_KEYS = {
  METRONOME_SETTINGS: 'vexel-daw:metronome-settings',
  PROJECT_SETTINGS: 'vexel-daw:project-settings',
};

/**
 * Load metronome settings from localStorage
 */
export function loadMetronomeSettings(): MetronomeSettings {
  try {
    const stored = localStorage.getItem(STORAGE_KEYS.METRONOME_SETTINGS);
    if (stored) {
      return JSON.parse(stored);
    }
  } catch (error) {
    console.error('Failed to load metronome settings:', error);
  }

  // Return defaults
  return {
    volume: 0.5,
    preCountEnabled: false,
    preCountBars: 2,
  };
}

/**
 * Save metronome settings to localStorage
 */
export function saveMetronomeSettings(settings: MetronomeSettings): void {
  try {
    localStorage.setItem(STORAGE_KEYS.METRONOME_SETTINGS, JSON.stringify(settings));
  } catch (error) {
    console.error('Failed to save metronome settings:', error);
  }
}

/**
 * Load project settings from localStorage
 */
export function loadProjectSettings(): Partial<ProjectSettings> {
  try {
    const stored = localStorage.getItem(STORAGE_KEYS.PROJECT_SETTINGS);
    if (stored) {
      return JSON.parse(stored);
    }
  } catch (error) {
    console.error('Failed to load project settings:', error);
  }

  // Return defaults
  return {
    tempo: 120,
    timeSignature: {
      numerator: 4,
      denominator: 4,
    },
  };
}

/**
 * Save project settings to localStorage
 */
export function saveProjectSettings(settings: Partial<ProjectSettings>): void {
  try {
    const current = loadProjectSettings();
    const updated = { ...current, ...settings };
    localStorage.setItem(STORAGE_KEYS.PROJECT_SETTINGS, JSON.stringify(updated));
  } catch (error) {
    console.error('Failed to save project settings:', error);
  }
}

/**
 * Clear all stored settings
 */
export function clearAllSettings(): void {
  try {
    localStorage.removeItem(STORAGE_KEYS.METRONOME_SETTINGS);
    localStorage.removeItem(STORAGE_KEYS.PROJECT_SETTINGS);
  } catch (error) {
    console.error('Failed to clear settings:', error);
  }
}
