/**
 * MIDISequencer.ts
 * MIDI note sequencing and playback using Web Audio API
 */

import { MIDINote } from './AudioEngine';

/**
 * Simple polyphonic synthesizer for MIDI playback
 */
export class SimpleSynth {
  private context: AudioContext;
  private masterGain: GainNode;
  private activeNotes: Map<number, { oscillator: OscillatorNode; gain: GainNode }>;
  private waveform: OscillatorType;

  constructor(context: AudioContext, output: AudioNode) {
    this.context = context;
    this.activeNotes = new Map();
    this.waveform = 'sawtooth';

    // Create master gain
    this.masterGain = context.createGain();
    this.masterGain.gain.value = 0.3; // Prevent clipping
    this.masterGain.connect(output);
  }

  /**
   * Play a MIDI note
   */
  noteOn(pitch: number, velocity: number) {
    // If note is already playing, stop it first
    this.noteOff(pitch);

    // Calculate frequency from MIDI note number
    const frequency = this.midiToFrequency(pitch);

    // Create oscillator
    const oscillator = this.context.createOscillator();
    oscillator.type = this.waveform;
    oscillator.frequency.value = frequency;

    // Create gain envelope
    const gain = this.context.createGain();
    gain.gain.value = 0;

    // Connect: oscillator -> gain -> master
    oscillator.connect(gain);
    gain.connect(this.masterGain);

    // Attack envelope
    const now = this.context.currentTime;
    const normalizedVelocity = velocity / 127;
    const attackTime = 0.01;

    gain.gain.setValueAtTime(0, now);
    gain.gain.linearRampToValueAtTime(normalizedVelocity * 0.3, now + attackTime);

    // Start oscillator
    oscillator.start(now);

    // Store active note
    this.activeNotes.set(pitch, { oscillator, gain });
  }

  /**
   * Stop a MIDI note
   */
  noteOff(pitch: number) {
    const note = this.activeNotes.get(pitch);
    if (!note) return;

    const now = this.context.currentTime;
    const releaseTime = 0.1;

    // Release envelope
    note.gain.gain.cancelScheduledValues(now);
    note.gain.gain.setValueAtTime(note.gain.gain.value, now);
    note.gain.gain.linearRampToValueAtTime(0, now + releaseTime);

    // Stop oscillator after release
    note.oscillator.stop(now + releaseTime);

    // Cleanup
    this.activeNotes.delete(pitch);
  }

  /**
   * Stop all notes
   */
  allNotesOff() {
    this.activeNotes.forEach((_, pitch) => {
      this.noteOff(pitch);
    });
  }

  /**
   * Set waveform type
   */
  setWaveform(waveform: OscillatorType) {
    this.waveform = waveform;
  }

  /**
   * Convert MIDI note number to frequency (Hz)
   */
  private midiToFrequency(note: number): number {
    return 440 * Math.pow(2, (note - 69) / 12);
  }

  /**
   * Cleanup
   */
  dispose() {
    this.allNotesOff();
    this.masterGain.disconnect();
  }
}

/**
 * MIDI sequencer for scheduling and playing back MIDI notes
 */
export class MIDISequencer {
  private context: AudioContext;
  private synth: SimpleSynth;
  private notes: MIDINote[];
  private scheduledNotes: Set<number>;
  private tempo: number;
  private isPlaying: boolean;
  private startTime: number;
  private currentTime: number;
  private scheduleAheadTime: number; // How far ahead to schedule (seconds)
  private scheduleInterval: number; // How often to schedule (ms)
  private schedulerTimer: number | null;

  constructor(context: AudioContext, output: AudioNode) {
    this.context = context;
    this.synth = new SimpleSynth(context, output);
    this.notes = [];
    this.scheduledNotes = new Set();
    this.tempo = 120;
    this.isPlaying = false;
    this.startTime = 0;
    this.currentTime = 0;
    this.scheduleAheadTime = 0.1; // Schedule 100ms ahead
    this.scheduleInterval = 25; // Check every 25ms
    this.schedulerTimer = null;
  }

  /**
   * Load MIDI notes into sequencer
   */
  loadNotes(notes: MIDINote[]) {
    this.notes = [...notes].sort((a, b) => a.startTime - b.startTime);
  }

  /**
   * Add MIDI notes to sequencer
   */
  addNotes(notes: MIDINote[]) {
    this.notes.push(...notes);
    this.notes.sort((a, b) => a.startTime - b.startTime);
  }

  /**
   * Clear all notes
   */
  clearNotes() {
    this.notes = [];
    this.scheduledNotes.clear();
  }

  /**
   * Start playback
   */
  start(fromBeat: number = 0) {
    if (this.isPlaying) return;

    this.isPlaying = true;
    this.currentTime = fromBeat;
    this.startTime = this.context.currentTime - this.beatsToSeconds(fromBeat);
    this.scheduledNotes.clear();

    // Start scheduler
    this.startScheduler();
  }

  /**
   * Stop playback
   */
  stop() {
    if (!this.isPlaying) return;

    this.isPlaying = false;
    this.stopScheduler();
    this.synth.allNotesOff();
    this.scheduledNotes.clear();
  }

  /**
   * Set tempo in BPM
   */
  setTempo(bpm: number) {
    this.tempo = bpm;
  }

  /**
   * Set synthesizer waveform
   */
  setWaveform(waveform: OscillatorType) {
    this.synth.setWaveform(waveform);
  }

  /**
   * Start the scheduler
   */
  private startScheduler() {
    this.schedulerTimer = window.setInterval(() => {
      this.schedule();
    }, this.scheduleInterval);
  }

  /**
   * Stop the scheduler
   */
  private stopScheduler() {
    if (this.schedulerTimer !== null) {
      clearInterval(this.schedulerTimer);
      this.schedulerTimer = null;
    }
  }

  /**
   * Schedule notes that need to play soon
   */
  private schedule() {
    if (!this.isPlaying) return;

    // Update current time
    this.currentTime = this.secondsToBeats(this.context.currentTime - this.startTime);

    // Schedule ahead window
    const scheduleUntilBeat = this.currentTime + this.secondsToBeats(this.scheduleAheadTime);

    // Find notes that need to be scheduled
    for (let i = 0; i < this.notes.length; i++) {
      const note = this.notes[i];

      // Skip if already scheduled
      if (this.scheduledNotes.has(i)) continue;

      // Skip if note hasn't started yet
      if (note.startTime > scheduleUntilBeat) break;

      // Skip if note has already ended
      if (note.startTime + note.duration < this.currentTime) {
        this.scheduledNotes.add(i);
        continue;
      }

      // Schedule note on
      const noteOnTime = this.startTime + this.beatsToSeconds(note.startTime);
      this.scheduleNoteOn(note, noteOnTime);

      // Schedule note off
      const noteOffTime = noteOnTime + this.beatsToSeconds(note.duration);
      this.scheduleNoteOff(note, noteOffTime);

      // Mark as scheduled
      this.scheduledNotes.add(i);
    }
  }

  /**
   * Schedule a note on event
   */
  private scheduleNoteOn(note: MIDINote, time: number) {
    // Schedule with a small delay to ensure it happens at the right time
    const delay = Math.max(0, (time - this.context.currentTime) * 1000);

    setTimeout(() => {
      if (this.isPlaying) {
        this.synth.noteOn(note.pitch, note.velocity);
      }
    }, delay);
  }

  /**
   * Schedule a note off event
   */
  private scheduleNoteOff(note: MIDINote, time: number) {
    const delay = Math.max(0, (time - this.context.currentTime) * 1000);

    setTimeout(() => {
      if (this.isPlaying) {
        this.synth.noteOff(note.pitch);
      }
    }, delay);
  }

  /**
   * Convert beats to seconds
   */
  private beatsToSeconds(beats: number): number {
    return (beats * 60) / this.tempo;
  }

  /**
   * Convert seconds to beats
   */
  private secondsToBeats(seconds: number): number {
    return (seconds * this.tempo) / 60;
  }

  /**
   * Cleanup
   */
  dispose() {
    this.stop();
    this.synth.dispose();
  }
}
