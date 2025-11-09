/**
 * MIDI Clock Implementation
 * Sends MIDI clock messages for external synchronization
 * Standard: 24 PPQN (Pulses Per Quarter Note)
 */

import { MIDIClockSettings } from '../types/metronome';

// MIDI System Real-Time Messages
const MIDI_CLOCK = 0xF8; // Timing Clock (24 PPQN)
const MIDI_START = 0xFA; // Start
const MIDI_CONTINUE = 0xFB; // Continue
const MIDI_STOP = 0xFC; // Stop

// MIDI System Common Messages
const MIDI_SONG_POSITION_POINTER = 0xF2; // Song Position Pointer

const PPQN = 24; // Pulses Per Quarter Note (MIDI standard)

export class MIDIClock {
  private settings: MIDIClockSettings;
  private midiAccess: MIDIAccess | null = null;
  private midiOutputs: MIDIOutput[] = [];

  private clockPulseCount: number = 0;
  private songPosition: number = 0; // In MIDI beats (1 MIDI beat = 6 MIDI clocks)

  constructor(settings: MIDIClockSettings) {
    this.settings = settings;
    this.initializeMIDI();
  }

  /**
   * Initialize Web MIDI API
   */
  private async initializeMIDI(): Promise<void> {
    if (!navigator.requestMIDIAccess) {
      console.warn('⚠️ Web MIDI API not supported in this browser');
      return;
    }

    try {
      this.midiAccess = await navigator.requestMIDIAccess({ sysex: false });
      this.updateOutputs();

      // Listen for device changes
      this.midiAccess.onstatechange = () => {
        this.updateOutputs();
      };

      console.log('✅ MIDI Clock initialized');
    } catch (error) {
      console.error('❌ Failed to initialize MIDI:', error);
    }
  }

  /**
   * Update list of MIDI outputs
   */
  private updateOutputs(): void {
    if (!this.midiAccess) return;

    this.midiOutputs = [];
    const outputs = this.midiAccess.outputs.values();

    for (const output of outputs) {
      this.midiOutputs.push(output);
      console.log(`🎹 MIDI Output: ${output.name}`);
    }
  }

  /**
   * Send MIDI Start message
   */
  sendStart(): void {
    if (!this.settings.enabled || !this.settings.sendStart) return;

    this.clockPulseCount = 0;
    this.songPosition = 0;

    this.sendToAll([MIDI_START]);
    console.log('▶️ MIDI Start sent');
  }

  /**
   * Send MIDI Stop message
   */
  sendStop(): void {
    if (!this.settings.enabled || !this.settings.sendStop) return;

    this.sendToAll([MIDI_STOP]);
    console.log('⏹️ MIDI Stop sent');
  }

  /**
   * Send MIDI Continue message
   */
  sendContinue(): void {
    if (!this.settings.enabled || !this.settings.sendContinue) return;

    this.sendToAll([MIDI_CONTINUE]);
    console.log('▶️ MIDI Continue sent');
  }

  /**
   * Send Song Position Pointer
   * @param position - Position in MIDI beats (1 MIDI beat = 6 MIDI clocks)
   */
  sendSongPositionPointer(position: number): void {
    if (!this.settings.enabled || !this.settings.sendSongPosition) return;

    this.songPosition = position;

    // SPP is 14-bit value, sent as two 7-bit bytes (LSB first)
    const lsb = position & 0x7F;
    const msb = (position >> 7) & 0x7F;

    this.sendToAll([MIDI_SONG_POSITION_POINTER, lsb, msb]);
    console.log(`📍 Song Position: ${position} MIDI beats`);
  }

  /**
   * Send MIDI Clock pulse
   * Should be called 24 times per quarter note
   * @param time - AudioContext time (for future scheduling if needed)
   * @param tempo - Current tempo in BPM
   */
  sendClockPulse(time: number, tempo: number): void {
    if (!this.settings.enabled) return;

    // In a real implementation, we would send one clock pulse here
    // For a full beat, this would be called 24 times
    // However, since our metronome triggers once per beat, we'll send
    // all 24 pulses for that beat

    for (let i = 0; i < PPQN; i++) {
      this.sendToAll([MIDI_CLOCK]);
      this.clockPulseCount++;
    }

    // Update song position (every 6 clocks = 1 MIDI beat)
    if (this.clockPulseCount % 6 === 0) {
      this.songPosition = Math.floor(this.clockPulseCount / 6);
    }
  }

  /**
   * Send MIDI message to all outputs
   */
  private sendToAll(data: number[]): void {
    if (!this.settings.enabled) return;

    this.midiOutputs.forEach(output => {
      try {
        output.send(data);
      } catch (error) {
        console.error(`❌ Failed to send MIDI to ${output.name}:`, error);
      }
    });
  }

  /**
   * Update settings
   */
  updateSettings(settings: MIDIClockSettings): void {
    this.settings = settings;
  }

  /**
   * Get list of available MIDI outputs
   */
  getOutputs(): { id: string; name: string }[] {
    return this.midiOutputs.map(output => ({
      id: output.id || '',
      name: output.name || 'Unknown Device'
    }));
  }

  /**
   * Reset clock state
   */
  reset(): void {
    this.clockPulseCount = 0;
    this.songPosition = 0;
  }

  /**
   * Clean up resources
   */
  destroy(): void {
    this.midiOutputs = [];
    if (this.midiAccess) {
      this.midiAccess.onstatechange = null;
    }
    this.midiAccess = null;
  }
}
