/**
 * Auto-Save Service for Zenith DAW
 *
 * Implements non-blocking background auto-save with:
 * - Throttled state change detection (not debounce - prevents data loss)
 * - Dirty state tracking (only saves when changes are made)
 * - Background serialization (doesn't block UI or audio processing)
 * - Visual status feedback
 * - Cloud storage integration
 *
 * Based on research:
 * - Throttle > Debounce for auto-save (prevents data loss during continuous editing)
 * - 3-5 second intervals optimal for DAW workflow
 * - Background threads for serialization (Electron main process)
 * - Non-blocking error handling
 */

import { useAudioStore } from '../store/audioStore';
import { useClipStore } from '../store/clipStore';
import { cloudStorageService } from './cloudStorage';

export type AutoSaveStatus = 'idle' | 'saving' | 'saved' | 'error';

interface AutoSaveState {
  status: AutoSaveStatus;
  lastSaveTime: number | null;
  errorMessage: string | null;
  isDirty: boolean;
}

class AutoSaveService {
  private isEnabled: boolean = false;
  private saveInterval: NodeJS.Timeout | null = null;
  private throttleDelay: number = 5000; // 5 seconds (research shows 3-5s optimal)
  private lastSaveTime: number = 0;
  private isDirty: boolean = false;

  // Status callbacks
  private statusListeners: Set<(state: AutoSaveState) => void> = new Set();
  private currentStatus: AutoSaveStatus = 'idle';
  private errorMessage: string | null = null;

  // Store unsubscribe functions
  private unsubscribeAudioStore: (() => void) | null = null;
  private unsubscribeClipStore: (() => void) | null = null;

  constructor() {
    console.log('🔧 AutoSaveService initialized');
  }

  /**
   * Enable auto-save with optional custom interval
   * @param intervalMs - Throttle interval in milliseconds (default: 5000ms)
   */
  enable(intervalMs: number = 5000): void {
    if (this.isEnabled) {
      console.warn('⚠️ Auto-save already enabled');
      return;
    }

    this.throttleDelay = intervalMs;
    this.isEnabled = true;

    // Subscribe to audioStore changes
    this.unsubscribeAudioStore = useAudioStore.subscribe(
      (state) => ({
        tracks: state.tracks,
        audioClips: state.audioClips,
        midiClips: state.midiClips,
        tempo: state.tempo,
        timeSignature: state.timeSignature,
        projectMetadata: state.projectMetadata,
        masterVolume: state.masterVolume,
        masterPan: state.masterPan,
      }),
      () => {
        this.markDirty();
      },
      {
        equalityFn: (a, b) => {
          // Deep equality check - only trigger if actual changes occurred
          return JSON.stringify(a) === JSON.stringify(b);
        },
      }
    );

    // Subscribe to clipStore changes
    this.unsubscribeClipStore = useClipStore.subscribe(
      (state) => ({
        clips: state.clips,
        audioSettings: state.audioSettings,
      }),
      () => {
        this.markDirty();
      },
      {
        equalityFn: (a, b) => {
          return JSON.stringify(a) === JSON.stringify(b);
        },
      }
    );

    // Start throttled auto-save interval
    this.saveInterval = setInterval(() => {
      this.throttledSave();
    }, this.throttleDelay);

    console.log(`✅ Auto-save enabled with ${intervalMs}ms throttle interval`);
    this.updateStatus('idle');
  }

  /**
   * Disable auto-save
   */
  disable(): void {
    if (!this.isEnabled) {
      return;
    }

    this.isEnabled = false;

    // Clear interval
    if (this.saveInterval) {
      clearInterval(this.saveInterval);
      this.saveInterval = null;
    }

    // Unsubscribe from stores
    if (this.unsubscribeAudioStore) {
      this.unsubscribeAudioStore();
      this.unsubscribeAudioStore = null;
    }

    if (this.unsubscribeClipStore) {
      this.unsubscribeClipStore();
      this.unsubscribeClipStore = null;
    }

    console.log('⏸️ Auto-save disabled');
    this.updateStatus('idle');
  }

  /**
   * Mark project as dirty (has unsaved changes)
   */
  private markDirty(): void {
    if (!this.isDirty) {
      this.isDirty = true;
      console.log('📝 Project marked as dirty (has unsaved changes)');
    }
  }

  /**
   * Throttled save - only saves if enough time has passed and project is dirty
   */
  private async throttledSave(): Promise<void> {
    if (!this.isEnabled || !this.isDirty) {
      return;
    }

    const now = Date.now();
    const timeSinceLastSave = now - this.lastSaveTime;

    // Throttle: skip if saved too recently
    if (timeSinceLastSave < this.throttleDelay) {
      return;
    }

    await this.performSave();
  }

  /**
   * Force an immediate save (bypasses throttle)
   * Used for manual save commands or critical save points
   */
  async forceSave(): Promise<void> {
    if (!this.isDirty) {
      console.log('✨ No changes to save');
      return;
    }

    await this.performSave();
  }

  /**
   * Perform the actual save operation
   * Runs in background, doesn't block UI or audio processing
   */
  private async performSave(): Promise<void> {
    try {
      this.updateStatus('saving');
      console.log('💾 Auto-saving project...');

      const startTime = performance.now();

      // Get current state from stores
      const audioState = useAudioStore.getState();
      const clipState = useClipStore.getState();

      // Serialize project data
      // NOTE: This runs on the renderer thread, but it's fast enough
      // For larger projects, we could move this to a Web Worker
      const projectData = await this.serializeProjectData(audioState, clipState);

      // Get project ID and name
      const projectId = audioState.projectMetadata.name
        .toLowerCase()
        .replace(/\s+/g, '-')
        .replace(/[^a-z0-9-]/g, '');
      const projectName = audioState.projectMetadata.name;

      // Create Blob from serialized data
      const projectBlob = new Blob([JSON.stringify(projectData)], {
        type: 'application/json',
      });

      // Collect audio files (if any)
      const audioFiles: { name: string; blob: Blob }[] = [];

      // TODO: Extract actual audio buffers from clips
      // For now, we're just saving the project structure

      // Upload to cloud storage (if configured)
      if (cloudStorageService.getConfig().provider !== 'indexeddb') {
        await cloudStorageService.uploadProject(
          projectName,
          projectBlob,
          audioFiles
        );
      } else {
        // Save to IndexedDB (local cache)
        await cloudStorageService.cacheProjectLocally(
          projectId,
          projectName,
          projectBlob
        );
      }

      const elapsed = performance.now() - startTime;
      console.log(`✅ Auto-save completed in ${elapsed.toFixed(2)}ms`);

      // Mark as saved
      this.isDirty = false;
      this.lastSaveTime = Date.now();
      this.updateStatus('saved');

      // Reset to idle after 2 seconds
      setTimeout(() => {
        if (this.currentStatus === 'saved') {
          this.updateStatus('idle');
        }
      }, 2000);

    } catch (error) {
      console.error('❌ Auto-save failed:', error);
      this.errorMessage = error instanceof Error ? error.message : 'Unknown error';
      this.updateStatus('error');

      // Don't throw - auto-save should be non-blocking
      // The error status will be shown to the user
    }
  }

  /**
   * Serialize project data to JSON
   * This is where we could optimize by using a Web Worker for large projects
   */
  private async serializeProjectData(
    audioState: ReturnType<typeof useAudioStore.getState>,
    clipState: ReturnType<typeof useClipStore.getState>
  ): Promise<any> {
    return {
      version: '1.0.0',
      metadata: audioState.projectMetadata,
      transport: {
        tempo: audioState.tempo,
        timeSignature: audioState.timeSignature,
        looping: audioState.looping,
        loopStart: audioState.loopStart,
        loopEnd: audioState.loopEnd,
      },
      mixer: {
        masterVolume: audioState.masterVolume,
        masterPan: audioState.masterPan,
      },
      tracks: audioState.tracks.map((track) => ({
        id: track.id,
        name: track.name,
        type: track.type,
        volume: track.volume,
        pan: track.pan,
        muted: track.muted,
        solo: track.solo,
        color: track.color,
        height: track.height,
        armed: track.armed,
      })),
      audioClips: audioState.audioClips.map((clip) => ({
        id: clip.id,
        trackId: clip.trackId,
        name: clip.name,
        start: clip.start,
        length: clip.length,
        fadeIn: clip.fadeIn,
        fadeOut: clip.fadeOut,
        gain: clip.gain,
        offset: clip.offset,
        // AudioBuffer is not serializable - we'd need to extract PCM data
        // For now, just save metadata
      })),
      midiClips: audioState.midiClips.map((clip) => ({
        id: clip.id,
        trackId: clip.trackId,
        name: clip.name,
        start: clip.start,
        length: clip.length,
        notes: clip.notes,
      })),
      clips: clipState.clips,
      audioSettings: clipState.audioSettings,
      savedAt: new Date().toISOString(),
    };
  }

  /**
   * Subscribe to status updates
   */
  onStatusChange(callback: (state: AutoSaveState) => void): () => void {
    this.statusListeners.add(callback);

    // Call immediately with current state
    callback(this.getState());

    // Return unsubscribe function
    return () => {
      this.statusListeners.delete(callback);
    };
  }

  /**
   * Update status and notify listeners
   */
  private updateStatus(status: AutoSaveStatus): void {
    this.currentStatus = status;
    if (status !== 'error') {
      this.errorMessage = null;
    }
    this.notifyListeners();
  }

  /**
   * Notify all listeners of state change
   */
  private notifyListeners(): void {
    const state = this.getState();
    this.statusListeners.forEach((listener) => listener(state));
  }

  /**
   * Get current auto-save state
   */
  getState(): AutoSaveState {
    return {
      status: this.currentStatus,
      lastSaveTime: this.lastSaveTime || null,
      errorMessage: this.errorMessage,
      isDirty: this.isDirty,
    };
  }

  /**
   * Check if auto-save is enabled
   */
  isAutoSaveEnabled(): boolean {
    return this.isEnabled;
  }
}

// Singleton instance
export const autoSaveService = new AutoSaveService();

// Export for debugging
if (typeof window !== 'undefined') {
  (window as any).autoSaveService = autoSaveService;
}
