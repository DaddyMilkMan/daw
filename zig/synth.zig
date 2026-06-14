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
    in_use: bool = false,

    fn noteOn(self: *Voice, freq: f32, sr: f32) void {
        self.freq = freq;
        self.osc.phase = 0.0;
        self.svf_low = 0.0;
        self.svf_band = 0.0;
        self.env.setParams(0.005, 0.12, 0.6, 0.30, sr);
        self.env.noteOn();
        self.in_use = true;
    }
    fn noteOff(self: *Voice) void {
        self.env.noteOff();
    }
    fn render(self: *Voice, cutoff: f32, res: f32, sr: f32, bend: f32) f32 {
        if (!self.env.isActive()) {
            self.in_use = false;
            return 0.0;
        }
        const raw = self.osc.next(self.freq * bend, sr);
        const amp = self.env.next();

        // Chamberlin state-variable lowpass.
        const f = std.math.clamp(cutoff / sr, 0.0001, 0.49);
        const q = res * 10.0 + 1.0;
        const r = 1.0 / q;
        self.svf_low += f * self.svf_band;
        const high = raw - self.svf_low - r * self.svf_band;
        self.svf_band += f * high;

        return self.svf_low * amp;
    }
};

pub const Synth = struct {
    voices: [16]Voice = [_]Voice{.{}} ** 16,
    cutoff: f32 = 2200.0,
    resonance: f32 = 0.25,
    bend: f32 = 1.0, // global pitch multiplier (MIDI 2.0 pitch bend)
    sample_rate: f32 = default_sample_rate,
    gain: f32 = 0.18,

    pub fn noteOn(self: *Synth, freq: f32) void {
        for (&self.voices) |*v| {
            if (!v.in_use) {
                v.noteOn(freq, self.sample_rate);
                return;
            }
        }
        self.voices[0].noteOn(freq, self.sample_rate); // steal
    }

    pub fn noteOff(self: *Synth, freq: f32) void {
        for (&self.voices) |*v| {
            if (v.in_use and @abs(v.freq - freq) < 0.01) v.noteOff();
        }
    }

    pub fn renderBlock(self: *Synth, out: []f32) void {
        for (out) |*sample| {
            var acc: f32 = 0.0;
            for (&self.voices) |*v| {
                if (v.in_use) acc += v.render(self.cutoff, self.resonance, self.sample_rate, self.bend);
            }
            sample.* = std.math.clamp(acc * self.gain, -1.0, 1.0);
        }
    }
};
