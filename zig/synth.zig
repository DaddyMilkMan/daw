//! synth.zig — a tiny subtractive synth, 100% Zig, no JUCE.
//!
//!   polyBLEP saw oscillator -> linear ADSR -> state-variable lowpass
//!
//! The seed of Zenith's native instrument engine. Everything here is owned:
//! no framework, no third-party DSP. It renders into a plain f32 buffer that
//! the platform layer (WAV writer or ALSA device) sends onward.

const std = @import("std");

pub const default_sample_rate: f32 = 48000.0;

/// polyBLEP residual — band-limits the saw's discontinuity to tame aliasing.
fn polyBlep(t: f32, dt: f32) f32 {
    if (t < dt) {
        const x = t / dt;
        return (x + x) - (x * x) - 1.0;
    } else if (t > 1.0 - dt) {
        const x = (t - 1.0) / dt;
        return (x * x) + (x + x) + 1.0;
    }
    return 0.0;
}

const Oscillator = struct {
    phase: f32 = 0.0,

    fn next(self: *Oscillator, freq: f32, sr: f32) f32 {
        const dt = freq / sr;
        const t = self.phase;
        var saw: f32 = (2.0 * t) - 1.0; // naive saw
        saw -= polyBlep(t, dt); // anti-aliased
        self.phase += dt;
        if (self.phase >= 1.0) self.phase -= 1.0;
        return saw;
    }
};

const Envelope = struct {
    const Stage = enum { idle, attack, decay, sustain, release };

    stage: Stage = .idle,
    value: f32 = 0.0,
    attack_rate: f32 = 0.01,
    decay_rate: f32 = 0.0001,
    sustain_level: f32 = 0.7,
    release_rate: f32 = 0.0001,

    fn setParams(self: *Envelope, a: f32, d: f32, s: f32, r: f32, sr: f32) void {
        self.attack_rate = 1.0 / @max(a * sr, 1.0);
        self.decay_rate = (1.0 - s) / @max(d * sr, 1.0);
        self.sustain_level = s;
        self.release_rate = s / @max(r * sr, 1.0);
    }
    fn noteOn(self: *Envelope) void {
        self.stage = .attack;
    }
    fn noteOff(self: *Envelope) void {
        if (self.stage != .idle) self.stage = .release;
    }
    fn isActive(self: *const Envelope) bool {
        return self.stage != .idle;
    }
    fn next(self: *Envelope) f32 {
        switch (self.stage) {
            .idle => {},
            .attack => {
                self.value += self.attack_rate;
                if (self.value >= 1.0) {
                    self.value = 1.0;
                    self.stage = .decay;
                }
            },
            .decay => {
                self.value -= self.decay_rate;
                if (self.value <= self.sustain_level) {
                    self.value = self.sustain_level;
                    self.stage = .sustain;
                }
            },
            .sustain => {},
            .release => {
                self.value -= self.release_rate;
                if (self.value <= 0.0) {
                    self.value = 0.0;
                    self.stage = .idle;
                }
            },
        }
        return self.value;
    }
};

const Voice = struct {
    osc: Oscillator = .{},
    env: Envelope = .{},
    svf_low: f32 = 0.0,
    svf_band: f32 = 0.0,
    freq: f32 = 0.0,
    vel: f32 = 1.0, // note-on velocity, normalized 0..1
    held: bool = false, // key still down (for sustain-pedal release deferral)
    in_use: bool = false,

    fn noteOn(self: *Voice, freq: f32, vel: f32, sr: f32) void {
        self.freq = freq;
        self.vel = std.math.clamp(vel, 0.0, 1.0);
        self.osc.phase = 0.0;
        self.svf_low = 0.0;
        self.svf_band = 0.0;
        self.env.setParams(0.005, 0.12, 0.6, 0.30, sr);
        self.env.noteOn();
        self.held = true;
        self.in_use = true;
    }
    fn noteOff(self: *Voice) void {
        self.held = false;
        self.env.noteOff();
    }
    /// `cutoff` is the base filter cutoff; `vb` (0..1) is how much velocity darkens
    /// soft notes (vel 1.0 always plays at full cutoff). `vib` is a per-sample pitch
    /// multiplier (vibrato). Amplitude scales with velocity.
    fn render(self: *Voice, cutoff: f32, res: f32, sr: f32, bend: f32, vb: f32, vib: f32) f32 {
        if (!self.env.isActive()) {
            self.in_use = false;
            return 0.0;
        }
        const raw = self.osc.next(self.freq * bend * vib, sr);
        const amp = self.env.next();

        // Velocity opens the filter: soft notes are darker, hard notes brighter.
        const eff_cut = cutoff * ((1.0 - vb) + vb * self.vel);
        // Chamberlin state-variable lowpass.
        const f = std.math.clamp(eff_cut / sr, 0.0001, 0.49);
        const q = res * 10.0 + 1.0;
        const r = 1.0 / q;
        self.svf_low += f * self.svf_band;
        const high = raw - self.svf_low - r * self.svf_band;
        self.svf_band += f * high;

        return self.svf_low * amp * self.vel;
    }
};

pub const Synth = struct {
    voices: [16]Voice = [_]Voice{.{}} ** 16,
    cutoff: f32 = 2200.0,
    resonance: f32 = 0.25,
    bend: f32 = 1.0, // global pitch multiplier (MIDI 2.0 pitch bend)
    sample_rate: f32 = default_sample_rate,
    gain: f32 = 0.18,
    // --- expression (driven by MIDI velocity + controllers) ---
    vel_brightness: f32 = 0.6, // how much velocity darkens soft notes (0 = none)
    mod: f32 = 0.0, // mod wheel (CC1) -> vibrato depth, 0..1
    pressure: f32 = 0.0, // channel pressure -> extra vibrato depth, 0..1
    expression: f32 = 1.0, // CC7 volume * CC11 expression, 0..1
    sustain: bool = false, // sustain pedal (CC64): defer note-off until released
    lfo_phase: f32 = 0.0, // vibrato LFO

    /// Trigger a note. `vel` is normalized velocity 0..1 (0 == note-off).
    pub fn noteOn(self: *Synth, freq: f32, vel: f32) void {
        if (vel <= 0.0) return self.noteOff(freq);
        for (&self.voices) |*v| {
            if (!v.in_use) {
                v.noteOn(freq, vel, self.sample_rate);
                return;
            }
        }
        self.voices[0].noteOn(freq, vel, self.sample_rate); // steal
    }

    pub fn noteOff(self: *Synth, freq: f32) void {
        for (&self.voices) |*v| {
            if (v.in_use and @abs(v.freq - freq) < 0.01) {
                v.held = false;
                if (!self.sustain) v.noteOff(); // else: held by the pedal
            }
        }
    }

    /// Sustain pedal (CC64). Releasing the pedal frees every key already lifted.
    pub fn setSustain(self: *Synth, on: bool) void {
        self.sustain = on;
        if (!on) {
            for (&self.voices) |*v| {
                if (v.in_use and !v.held) v.noteOff();
            }
        }
    }

    pub fn renderBlock(self: *Synth, out: []f32) void {
        const lfo_rate = 5.5; // Hz
        const lfo_inc = lfo_rate / self.sample_rate;
        // vibrato depth in cents: mod wheel + aftertouch, up to ~30 cents
        const depth_cents = std.math.clamp(self.mod + self.pressure, 0.0, 1.0) * 30.0;
        for (out) |*sample| {
            const vib = std.math.pow(f32, 2.0, (depth_cents / 1200.0) * @sin(self.lfo_phase * std.math.tau));
            self.lfo_phase += lfo_inc;
            if (self.lfo_phase >= 1.0) self.lfo_phase -= 1.0;
            var acc: f32 = 0.0;
            for (&self.voices) |*v| {
                if (v.in_use) acc += v.render(self.cutoff, self.resonance, self.sample_rate, self.bend, self.vel_brightness, vib);
            }
            sample.* = std.math.clamp(acc * self.gain * self.expression, -1.0, 1.0);
        }
    }
};

test "velocity scales output level" {
    var s = Synth{};
    var buf: [4800]f32 = undefined;
    // soft note
    s.noteOn(440.0, 0.2);
    s.renderBlock(&buf);
    var soft: f32 = 0;
    for (buf) |x| soft = @max(soft, @abs(x));
    // reset + hard note
    s = Synth{};
    s.noteOn(440.0, 1.0);
    s.renderBlock(&buf);
    var hard: f32 = 0;
    for (buf) |x| hard = @max(hard, @abs(x));
    try std.testing.expect(hard > soft * 2.0); // full velocity is much louder
}

test "sustain pedal holds a released note" {
    var s = Synth{};
    var buf: [256]f32 = undefined;
    s.setSustain(true);
    s.noteOn(440.0, 1.0);
    s.noteOff(440.0); // key up, but pedal is down -> keeps sounding
    s.renderBlock(&buf);
    var lvl: f32 = 0;
    for (buf) |x| lvl = @max(lvl, @abs(x));
    try std.testing.expect(lvl > 0.01); // still audible while pedal held
    try std.testing.expect(s.voices[0].in_use);
    s.setSustain(false); // pedal up -> the lifted key now releases
    try std.testing.expect(s.voices[0].env.stage == .release);
}
