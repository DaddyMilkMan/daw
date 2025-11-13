/**
 * Real-time Collaboration Service
 * Multi-user DAW session with conflict-free synchronization
 *
 * Features:
 * - WebRTC peer-to-peer communication
 * - Yjs CRDT for conflict-free synchronization
 * - Presence awareness (who's online)
 * - Real-time cursor tracking
 * - Audio sync (challenging)
 * - Permission management
 *
 * Based on:
 * - Yjs CRDT library (October 2025)
 * - WebRTC data channels
 * - Y-WebRTC provider for signaling
 *
 * Challenges:
 * - Audio playback synchronization across network
 * - Large audio file distribution
 * - Latency compensation
 */

import * as Y from 'yjs';
import { WebrtcProvider } from 'y-webrtc';
import { useAudioStore } from '../store/audioStore';
import { ExtendedTrack, AudioClip, MIDIClip } from '../types/recording';

export interface CollaborationUser {
  id: string;
  name: string;
  color: string;
  isOnline: boolean;
  cursor?: {
    beat: number;
    trackId: string;
  };
}

export interface CollaborationSession {
  id: string;
  name: string;
  createdAt: number;
  hostId: string;
  users: CollaborationUser[];
  permissions: CollaborationPermissions;
}

export interface CollaborationPermissions {
  allowEdit: boolean; // Can edit project
  allowRecord: boolean; // Can record audio/MIDI
  allowEffects: boolean; // Can add/modify effects
  allowMixing: boolean; // Can adjust volume/pan
}

export class CollaborationService {
  private static instance: CollaborationService | null = null;

  private ydoc: Y.Doc | null = null;
  private provider: WebrtcProvider | null = null;
  private sessionId: string | null = null;
  private localUserId: string | null = null;

  // Yjs shared types
  private sharedTracks: Y.Array<any> | null = null;
  private sharedAudioClips: Y.Array<any> | null = null;
  private sharedMIDIClips: Y.Array<any> | null = null;
  private sharedState: Y.Map<any> | null = null;
  private awareness: any | null = null;

  private constructor() {}

  static getInstance(): CollaborationService {
    if (!CollaborationService.instance) {
      CollaborationService.instance = new CollaborationService();
    }
    return CollaborationService.instance;
  }

  /**
   * Start a new collaboration session
   */
  async startSession(
    sessionName: string,
    userName: string,
    signalingServer?: string
  ): Promise<CollaborationSession> {
    console.log(`Starting collaboration session: ${sessionName}`);

    // Create Yjs document
    this.ydoc = new Y.Doc();
    this.sessionId = `zenith-daw-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
    this.localUserId = `user-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

    // Initialize shared types
    this.sharedTracks = this.ydoc.getArray('tracks');
    this.sharedAudioClips = this.ydoc.getArray('audioClips');
    this.sharedMIDIClips = this.ydoc.getArray('midiClips');
    this.sharedState = this.ydoc.getMap('state');

    // Set up WebRTC provider for signaling
    // Note: You'll need to configure a signaling server or use public servers
    const signalingServers = signalingServer
      ? [signalingServer]
      : [
          'wss://signaling.yjs.dev', // Public Yjs signaling server
          'wss://y-webrtc-signaling-eu.herokuapp.com',
          'wss://y-webrtc-signaling-us.herokuapp.com',
        ];

    this.provider = new WebrtcProvider(this.sessionId, this.ydoc, {
      signaling: signalingServers,
      password: null, // Add password for private sessions
      awareness: this.ydoc.getArray('awareness') as any,
      maxConns: 20,
      filterBcConns: true,
    });

    this.awareness = this.provider.awareness;

    // Set local user state
    this.awareness.setLocalState({
      user: {
        id: this.localUserId,
        name: userName,
        color: this.generateUserColor(),
      },
    });

    // Listen for remote changes
    this.setupSyncListeners();

    // Sync local state to Yjs
    this.syncLocalToYjs();

    const session: CollaborationSession = {
      id: this.sessionId,
      name: sessionName,
      createdAt: Date.now(),
      hostId: this.localUserId,
      users: [
        {
          id: this.localUserId,
          name: userName,
          color: this.generateUserColor(),
          isOnline: true,
        },
      ],
      permissions: {
        allowEdit: true,
        allowRecord: true,
        allowEffects: true,
        allowMixing: true,
      },
    };

    console.log(`Session started: ${this.sessionId}`);

    return session;
  }

  /**
   * Join an existing collaboration session
   */
  async joinSession(
    sessionId: string,
    userName: string,
    signalingServer?: string
  ): Promise<CollaborationSession> {
    console.log(`Joining collaboration session: ${sessionId}`);

    this.ydoc = new Y.Doc();
    this.sessionId = sessionId;
    this.localUserId = `user-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

    // Initialize shared types
    this.sharedTracks = this.ydoc.getArray('tracks');
    this.sharedAudioClips = this.ydoc.getArray('audioClips');
    this.sharedMIDIClips = this.ydoc.getArray('midiClips');
    this.sharedState = this.ydoc.getMap('state');

    const signalingServers = signalingServer
      ? [signalingServer]
      : [
          'wss://signaling.yjs.dev',
          'wss://y-webrtc-signaling-eu.herokuapp.com',
          'wss://y-webrtc-signaling-us.herokuapp.com',
        ];

    this.provider = new WebrtcProvider(this.sessionId, this.ydoc, {
      signaling: signalingServers,
      password: null,
      awareness: this.ydoc.getArray('awareness') as any,
      maxConns: 20,
      filterBcConns: true,
    });

    this.awareness = this.provider.awareness;

    this.awareness.setLocalState({
      user: {
        id: this.localUserId,
        name: userName,
        color: this.generateUserColor(),
      },
    });

    // Listen for changes
    this.setupSyncListeners();

    // Wait for initial sync
    await this.waitForInitialSync();

    // Sync Yjs to local state
    this.syncYjsToLocal();

    const users = this.getOnlineUsers();

    const session: CollaborationSession = {
      id: this.sessionId,
      name: 'Collaborative Session',
      createdAt: Date.now(),
      hostId: users[0]?.id || '',
      users,
      permissions: {
        allowEdit: true,
        allowRecord: true,
        allowEffects: true,
        allowMixing: true,
      },
    };

    console.log(`Joined session: ${this.sessionId}`);

    return session;
  }

  /**
   * Leave collaboration session
   */
  leaveSession(): void {
    if (this.provider) {
      this.provider.destroy();
      this.provider = null;
    }

    if (this.ydoc) {
      this.ydoc.destroy();
      this.ydoc = null;
    }

    this.sessionId = null;
    this.localUserId = null;

    console.log('Left collaboration session');
  }

  /**
   * Setup sync listeners
   */
  private setupSyncListeners(): void {
    if (!this.ydoc) return;

    // Listen for track changes
    this.sharedTracks?.observe((event) => {
      console.log('Tracks changed remotely');
      this.syncYjsToLocal();
    });

    // Listen for audio clip changes
    this.sharedAudioClips?.observe((event) => {
      console.log('Audio clips changed remotely');
      this.syncYjsToLocal();
    });

    // Listen for MIDI clip changes
    this.sharedMIDIClips?.observe((event) => {
      console.log('MIDI clips changed remotely');
      this.syncYjsToLocal();
    });

    // Listen for state changes
    this.sharedState?.observe((event) => {
      console.log('State changed remotely');
      this.syncYjsToLocal();
    });

    // Listen for awareness changes (presence)
    this.awareness?.on('change', () => {
      console.log('User presence changed');
      // Trigger UI update for online users
    });
  }

  /**
   * Sync local state to Yjs (when local changes occur)
   */
  syncLocalToYjs(): void {
    if (!this.ydoc || !this.sharedTracks || !this.sharedAudioClips || !this.sharedMIDIClips) {
      return;
    }

    const store = useAudioStore.getState();

    this.ydoc.transact(() => {
      // Sync tracks
      this.sharedTracks!.delete(0, this.sharedTracks!.length);
      store.tracks.forEach((track) => {
        this.sharedTracks!.push([this.serializeTrack(track)]);
      });

      // Sync audio clips
      this.sharedAudioClips!.delete(0, this.sharedAudioClips!.length);
      store.audioClips.forEach((clip) => {
        this.sharedAudioClips!.push([this.serializeAudioClip(clip)]);
      });

      // Sync MIDI clips
      this.sharedMIDIClips!.delete(0, this.sharedMIDIClips!.length);
      store.midiClips.forEach((clip) => {
        this.sharedMIDIClips!.push([clip]);
      });

      // Sync playback state
      this.sharedState!.set('isPlaying', store.isPlaying);
      this.sharedState!.set('currentBeat', store.currentBeat);
      this.sharedState!.set('tempo', store.tempo);
    });

    console.log('Synced local state to Yjs');
  }

  /**
   * Sync Yjs to local state (when remote changes occur)
   */
  private syncYjsToLocal(): void {
    if (!this.sharedTracks || !this.sharedAudioClips || !this.sharedMIDIClips || !this.sharedState) {
      return;
    }

    const store = useAudioStore.getState();

    // Sync tracks
    const tracks = this.sharedTracks.toArray().map((t) => this.deserializeTrack(t));
    useAudioStore.setState({ tracks });

    // Sync audio clips
    const audioClips = this.sharedAudioClips.toArray().map((c) => this.deserializeAudioClip(c));
    useAudioStore.setState({ audioClips });

    // Sync MIDI clips
    const midiClips = this.sharedMIDIClips.toArray();
    useAudioStore.setState({ midiClips });

    // Sync state
    const isPlaying = this.sharedState.get('isPlaying');
    const currentBeat = this.sharedState.get('currentBeat');
    const tempo = this.sharedState.get('tempo');

    if (isPlaying !== undefined) useAudioStore.setState({ isPlaying });
    if (currentBeat !== undefined) useAudioStore.setState({ currentBeat });
    if (tempo !== undefined) useAudioStore.setState({ tempo });

    console.log('Synced Yjs to local state');
  }

  /**
   * Get online users
   */
  getOnlineUsers(): CollaborationUser[] {
    if (!this.awareness) {
      return [];
    }

    const users: CollaborationUser[] = [];
    const states = this.awareness.getStates();

    states.forEach((state: any, clientId: number) => {
      if (state.user) {
        users.push({
          id: state.user.id,
          name: state.user.name,
          color: state.user.color,
          isOnline: true,
          cursor: state.cursor,
        });
      }
    });

    return users;
  }

  /**
   * Update local cursor position
   */
  updateCursor(beat: number, trackId: string): void {
    if (!this.awareness) return;

    const currentState = this.awareness.getLocalState();
    this.awareness.setLocalState({
      ...currentState,
      cursor: { beat, trackId },
    });
  }

  /**
   * Serialize/deserialize methods for complex types
   */
  private serializeTrack(track: ExtendedTrack): any {
    return {
      ...track,
      // Remove non-serializable properties
    };
  }

  private deserializeTrack(data: any): ExtendedTrack {
    return data as ExtendedTrack;
  }

  private serializeAudioClip(clip: AudioClip): any {
    return {
      ...clip,
      // Audio buffers cannot be serialized directly
      // Need to send audio file URL or use separate file transfer
      audioBuffer: null,
    };
  }

  private deserializeAudioClip(data: any): AudioClip {
    return data as AudioClip;
  }

  /**
   * Generate random user color
   */
  private generateUserColor(): string {
    const colors = [
      '#FF6B6B',
      '#4ECDC4',
      '#45B7D1',
      '#FFA07A',
      '#98D8C8',
      '#F7DC6F',
      '#BB8FCE',
      '#85C1E2',
    ];
    return colors[Math.floor(Math.random() * colors.length)];
  }

  /**
   * Wait for initial sync
   */
  private async waitForInitialSync(): Promise<void> {
    return new Promise((resolve) => {
      if (!this.provider) {
        resolve();
        return;
      }

      const checkSync = () => {
        if (this.provider && this.provider.connected) {
          resolve();
        } else {
          setTimeout(checkSync, 100);
        }
      };

      checkSync();
    });
  }
}

export const collaborationService = CollaborationService.getInstance();
