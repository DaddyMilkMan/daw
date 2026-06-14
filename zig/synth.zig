//! synth.zig — Zenith's native polyphonic synthesizer, 100% Zig, no JUCE.
//!
//! A hybrid subtractive engine: two band-limited oscillators (+ sub + noise) with
//! unison detune, a zero-delay-feedback filter (`filter.zig`: clean TPT SVF or a
//! saturating Moog ladder) driven by its OWN ADSR (the defining subtractive move),
//! a separate amplitude ADSR, two LFOs, glide, and full per-note expression
//! so it voices MPE — each sounding note carries its own pitch bend / pressure /
//! timbre. Presets live in `Patch`. RT-safe: no allocation on the render path.

const std = @import("std");
const filter = @import("filter.zig");
const effects = @import("effects.zig");

pub const default_sample_rate: f32 = 48000.0;

pub const Wave = enum { sine, triangle, saw, square };

/// polyBLEP residual — band-limits a discontinuity to tame aliasing.
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

/// One band-limited oscillator phase. `pw` is pulse width for the square.
const Osc = struct {
    phase: f32 = 0.0,

    fn next(self: *Osc, wave: Wave, dt: f32, pw: f32) f32 {
        const t = self.phase;
        var out: f32 = 0;
        switch (wave) {
            .sine => out = @sin(t * std.math.tau),
            .triangle => out = 4.0 * @abs(t - 0.5) - 1.0, // -1..1 triangle
            .saw => {
                out = (2.0 * t) - 1.0;
                out -= polyBlep(t, dt);
            },
            .square => {
                out = if (t < pw) 1.0 else -1.0;
                out += polyBlep(t, dt);
                var t2 = t - pw;
                if (t2 < 0) t2 += 1.0;
                out -= polyBlep(t2, dt);
            },
        }
        self.phase += dt;
        if (self.phase >= 1.0) self.phase -= 1.0;
        return out;
    }
};

/// Exponential-segment ADSR (one-pole approach to each stage target — sounds far
/// more natural than linear ramps).
pub const Adsr = struct {
    a: f32 = 0.005,
    d: f32 = 0.12,
    s: f32 = 0.7,
    r: f32 = 0.25,
    stage: Stage = .idle,
    value: f32 = 0.0,

    pub const Stage = enum { idle, attack, decay, sustain, release };

    fn coef(time: f32, sr: f32) f32 {
        return @exp(-1.0 / (@max(time, 1.0e-4) * sr));
    }
    fn gateOn(self: *Adsr) void {
        self.stage = .attack;
    }
    fn gateOff(self: *Adsr) void {
        if (self.stage != .idle) self.stage = .release;
    }
    fn active(self: *const Adsr) bool {
        return self.stage != .idle;
    }
    fn process(self: *Adsr, sr: f32) f32 {
        switch (self.stage) {
            .idle => {},
            .attack => {
                const c = coef(self.a, sr);
                self.value = 1.0 + (self.value - 1.0) * c; // approach 1
                if (self.value >= 0.99) {
                    self.value = 1.0;
                    self.stage = .decay;
                }
            },
            .decay => {
                const c = coef(self.d, sr);
                self.value = self.s + (self.value - self.s) * c;
                if (@abs(self.value - self.s) < 0.001) {
                    self.value = self.s;
                    self.stage = .sustain;
                }
            },
            .sustain => self.value = self.s,
            .release => {
                const c = coef(self.r, sr);
                self.value *= c;
                if (self.value <= 1.0e-4) {
                    self.value = 0.0;
                    self.stage = .idle;
                }
            },
        }
        return self.value;
    }
};

/// All the timbral parameters of a sound — a preset.
pub const Patch = struct {
    osc_a: Wave = .saw,
    osc_b: Wave = .saw,
    osc_b_semi: f32 = 0.0, // oscB transpose, semitones
    osc_b_fine: f32 = 0.0, // oscB fine, cents
    osc_mix: f32 = 0.35, // 0 = all A, 1 = all B
    pulse_width: f32 = 0.5,
    sub_level: f32 = 0.0, // square sub one octave below
    noise_level: f32 = 0.0,
    unison: u8 = 1, // 1..7 detuned copies of each osc
    unison_detune: f32 = 8.0, // cents spread across the unison stack
    // analog imperfection — what makes it sound alive instead of static
    drift: f32 = 4.0, // peak cents of slow per-oscillator pitch drift (free-running VCOs)
    tune_spread: f32 = 2.0, // peak cents of fixed per-voice detune (component tolerance)
    // filter (zero-delay: svf = clean/stable multimode, ladder = analog growl)
    filter_model: filter.Model = .ladder,
    filter_mode: filter.Mode = .lowpass,
    drive: f32 = 1.0, // filter input drive (1 = clean, >1 saturates)
    resonance: f32 = 0.2,
    filter_env_amt: f32 = 0.0, // octaves of cutoff swept by the filter envelope
    keytrack: f32 = 0.0, // 0..1 cutoff follows pitch
    vel_to_cutoff: f32 = 0.5, // velocity opens the filter
    // envelopes
    amp_env: Adsr = .{ .a = 0.005, .d = 0.12, .s = 0.7, .r = 0.25 },
    filt_env: Adsr = .{ .a = 0.005, .d = 0.20, .s = 0.0, .r = 0.25 },
    // LFOs (lfo1 -> pitch vibrato, lfo2 -> cutoff)
    lfo1_rate: f32 = 5.5,
    lfo1_pitch: f32 = 0.0, // cents of vibrato (also summed with mod wheel)
    lfo2_rate: f32 = 0.4,
    lfo2_cutoff: f32 = 0.0, // octaves
    glide: f32 = 0.0, // portamento time, seconds
    level: f32 = 0.18, // patch output gain

    pub const init_saw = Patch{};
    pub const fat_bass = Patch{
        .osc_a = .saw, .osc_b = .square, .osc_b_semi = -12, .osc_mix = 0.4,
        .sub_level = 0.5, .unison = 3, .unison_detune = 6,
        .filter_model = .ladder, .drive = 1.8, // saturated ladder = analog bass grit
        .resonance = 0.35, .filter_env_amt = 2.2, .vel_to_cutoff = 0.6,
        .amp_env = .{ .a = 0.002, .d = 0.18, .s = 0.6, .r = 0.12 },
        .filt_env = .{ .a = 0.002, .d = 0.16, .s = 0.0, .r = 0.12 },
        .level = 0.20,
    };
    pub const super_lead = Patch{
        .osc_a = .saw, .osc_b = .saw, .osc_b_fine = 12, .osc_mix = 0.5,
        .unison = 7, .unison_detune = 14,
        .filter_model = .ladder, .drive = 1.3, .resonance = 0.18,
        .filter_env_amt = 1.4, .keytrack = 0.5, .lfo1_pitch = 6,
        .amp_env = .{ .a = 0.01, .d = 0.3, .s = 0.8, .r = 0.25 },
        .level = 0.14,
    };
    pub const warm_pad = Patch{
        .osc_a = .triangle, .osc_b = .saw, .osc_b_semi = 12, .osc_mix = 0.45,
        .unison = 5, .unison_detune = 10,
        .filter_model = .svf, .resonance = 0.12, // clean SVF keeps pads smooth
        .filter_env_amt = 0.8, .keytrack = 0.3, .lfo2_cutoff = 0.4, .lfo2_rate = 0.25,
        .amp_env = .{ .a = 0.4, .d = 0.6, .s = 0.85, .r = 0.8 },
        .filt_env = .{ .a = 0.5, .d = 0.7, .s = 0.4, .r = 0.8 },
        .level = 0.12,
    };
};

const MAX_UNISON = 7;
const CTL = 16; // control-rate divisor: recompute heavy modulation every 16 samples

const Voice = struct {
    osc_a: [MAX_UNISON]Osc = [_]Osc{.{}} ** MAX_UNISON,
    osc_b: [MAX_UNISON]Osc = [_]Osc{.{}} ** MAX_UNISON,
    sub: Osc = .{},
    amp_env: Adsr = .{},
    filt_env: Adsr = .{},
    filt: filter.Filter = .{},
    rng: u32 = 0x2545F491, // per-voice white noise
    // analog drift: per-oscillator random-walk pitch (cents `w*` -> cached ratio `r*`)
    wa: [MAX_UNISON]f32 = [_]f32{0} ** MAX_UNISON,
    wb: [MAX_UNISON]f32 = [_]f32{0} ** MAX_UNISON,
    ra: [MAX_UNISON]f32 = [_]f32{1} ** MAX_UNISON,
    rb: [MAX_UNISON]f32 = [_]f32{1} ** MAX_UNISON,
    drng: u32 = 0x9E3779B9, // drift RNG (independent of the noise RNG)
    tune: f32 = 1.0, // fixed per-voice tuning offset (component tolerance)
    freq: f32 = 0.0, // target frequency
    cur_freq: f32 = 0.0, // glided frequency
    cur_vib: f32 = 1.0, // vibrato pitch multiplier (updated at control rate)
    note: u8 = 0,
    channel: u4 = 0,
    vel: f32 = 1.0,
    held: bool = false,
    in_use: bool = false,
    // per-note (MPE) expression overrides
    note_bend: f32 = 1.0, // pitch multiplier
    note_press: f32 = 0.0, // 0..1
    note_timbre: f32 = 0.0, // -1..1 cutoff offset

    fn noteOn(self: *Voice, freq: f32, vel: f32, note: u8, channel: u4, p: *const Patch, sr: f32) void {
        self.freq = freq;
        // start from the current glided freq if still sounding (legato glide)
        if (!self.in_use or p.glide <= 0.0) self.cur_freq = freq;
        self.note = note;
        self.channel = channel;
        self.vel = std.math.clamp(vel, 0.0, 1.0);
        // analog imperfection: a fixed per-voice tuning offset + fresh drift state
        self.drng = (0x9E3779B9 ^ (@as(u32, note) *% 2654435761)) | 1;
        self.tune = std.math.pow(f32, 2.0, (self.drand() * p.tune_spread) / 1200.0);
        self.wa = [_]f32{0} ** MAX_UNISON;
        self.wb = [_]f32{0} ** MAX_UNISON;
        self.ra = [_]f32{1} ** MAX_UNISON;
        self.rb = [_]f32{1} ** MAX_UNISON;
        self.filt = .{ .model = p.filter_model, .mode = p.filter_mode, .sr = sr };
        self.amp_env = p.amp_env;
        self.filt_env = p.filt_env;
        self.amp_env.gateOn();
        self.filt_env.gateOn();
        self.note_bend = 1.0;
        self.note_press = 0.0;
        self.note_timbre = 0.0;
        self.held = true;
        self.in_use = true;
    }
    fn noteOff(self: *Voice) void {
        self.held = false;
        self.amp_env.gateOff();
        self.filt_env.gateOff();
    }
    fn noise(self: *Voice) f32 {
        // xorshift -> -1..1
        self.rng ^= self.rng << 13;
        self.rng ^= self.rng >> 17;
        self.rng ^= self.rng << 5;
        return (@as(f32, @floatFromInt(self.rng)) / 2147483648.0) - 1.0;
    }
    fn drand(self: *Voice) f32 { // independent xorshift -> -1..1 (drift/tuning)
        self.drng ^= self.drng << 13;
        self.drng ^= self.drng >> 17;
        self.drng ^= self.drng << 5;
        return (@as(f32, @floatFromInt(self.drng)) / 2147483648.0) - 1.0;
    }
};

pub const Synth = struct {
    voices: [16]Voice = [_]Voice{.{}} ** 16,
    patch: Patch = .{},
    // --- live performance controls (the engine writes these per block) ---
    cutoff: f32 = 2200.0, // base filter cutoff Hz (CC74 / default)
    resonance: f32 = 0.2,
    bend: f32 = 1.0, // global pitch multiplier (channel-wide bend)
    mod: f32 = 0.0, // mod wheel -> vibrato depth, 0..1
    pressure: f32 = 0.0, // channel pressure (non-MPE global), 0..1
    expression: f32 = 1.0, // CC7/CC11 gain, 0..1
    vel_brightness: f32 = 0.5,
    sustain: bool = false,
    sample_rate: f32 = default_sample_rate,
    gain: f32 = 1.0, // master trim (the engine's own gain is separate)
    // --- per-channel (MPE) expression: each note's channel carries its own ---
    chan_bend: [16]f32 = [_]f32{1.0} ** 16,
    chan_press: [16]f32 = [_]f32{0.0} ** 16,
    chan_timbre: [16]f32 = [_]f32{0.0} ** 16,
    lfo1_phase: f32 = 0.0,
    lfo2_phase: f32 = 0.0,
    oversample: u8 = 2, // 1 / 2 / 4 — anti-alias the saturating filter path
    decim: Decimator = .{},

    pub fn setPatch(self: *Synth, p: Patch) void {
        self.patch = p;
    }

    /// Trigger a note (channel 0). `vel` is normalized velocity 0..1.
    pub fn noteOn(self: *Synth, freq: f32, vel: f32) void {
        self.noteOnMpe(freq, vel, 0);
    }

    /// Trigger a note on a specific MIDI channel (for MPE, one note per channel).
    pub fn noteOnMpe(self: *Synth, freq: f32, vel: f32, channel: u4) void {
        if (vel <= 0.0) return self.noteOff(freq);
        const note = freqToNote(freq);
        for (&self.voices) |*v| {
            if (!v.in_use) {
                v.noteOn(freq, vel, note, channel, &self.patch, self.sample_rate);
                return;
            }
        }
        // steal the quietest voice
        var qi: usize = 0;
        var q: f32 = 1e9;
        for (&self.voices, 0..) |*v, i| {
            if (v.amp_env.value < q) {
                q = v.amp_env.value;
                qi = i;
            }
        }
        self.voices[qi].noteOn(freq, vel, note, channel, &self.patch, self.sample_rate);
    }

    pub fn noteOff(self: *Synth, freq: f32) void {
        const note = freqToNote(freq);
        for (&self.voices) |*v| {
            if (v.in_use and v.note == note) {
                v.held = false;
                if (!self.sustain) v.noteOff();
            }
        }
    }

    pub fn setSustain(self: *Synth, on: bool) void {
        self.sustain = on;
        if (!on) {
            for (&self.voices) |*v| {
                if (v.in_use and !v.held) v.noteOff();
            }
        }
    }

    // --- MPE / per-channel + per-note expression setters ---
    pub fn setChannelBend(self: *Synth, ch: u4, ratio: f32) void {
        self.chan_bend[ch] = ratio;
    }
    pub fn setChannelPressure(self: *Synth, ch: u4, v: f32) void {
        self.chan_press[ch] = v;
    }
    pub fn setChannelTimbre(self: *Synth, ch: u4, v: f32) void {
        self.chan_timbre[ch] = v;
    }
    pub fn setNoteBend(self: *Synth, note: u8, ratio: f32) void {
        for (&self.voices) |*v| if (v.in_use and v.note == note) {
            v.note_bend = ratio;
        };
    }
    pub fn setNotePressure(self: *Synth, note: u8, val: f32) void {
        for (&self.voices) |*v| if (v.in_use and v.note == note) {
            v.note_press = val;
        };
    }
    pub fn setNoteTimbre(self: *Synth, note: u8, val: f32) void { // MPE per-note CC74
        for (&self.voices) |*v| if (v.in_use and v.note == note) {
            v.note_timbre = val;
        };
    }

    fn renderInner(self: *Synth, out: []f32, sr: f32) void {
        const p = &self.patch;
        const lfo1_inc = p.lfo1_rate / sr;
        const lfo2_inc = p.lfo2_rate / sr;
        const glide_coef: f32 = if (p.glide > 0.0) @exp(-1.0 / (p.glide * sr)) else 0.0;
        const un: usize = @max(@as(usize, 1), @min(@as(usize, p.unison), MAX_UNISON));
        const un_norm = 1.0 / @sqrt(@as(f32, @floatFromInt(un)));
        // per-block constants (hoisted out of the hot loop): oscB ratio + unison detunes
        const b_ratio = std.math.pow(f32, 2.0, (p.osc_b_semi + p.osc_b_fine / 100.0) / 12.0);
        var det_ratio: [MAX_UNISON]f32 = undefined;
        for (0..un) |ui| {
            const spread: f32 = if (un > 1)
                (@as(f32, @floatFromInt(ui)) / @as(f32, @floatFromInt(un - 1)) - 0.5) * 2.0
            else
                0.0;
            det_ratio[ui] = std.math.pow(f32, 2.0, (spread * p.unison_detune) / 1200.0);
        }

        for (out, 0..) |*sample, si| {
            const lfo1 = @sin(self.lfo1_phase * std.math.tau);
            const lfo2 = @sin(self.lfo2_phase * std.math.tau);
            self.lfo1_phase += lfo1_inc;
            if (self.lfo1_phase >= 1.0) self.lfo1_phase -= 1.0;
            self.lfo2_phase += lfo2_inc;
            if (self.lfo2_phase >= 1.0) self.lfo2_phase -= 1.0;

            // recompute the heavy modulation (vibrato + cutoff + filter coeffs,
            // i.e. all the pow()/tan() calls) every CTL samples — "control rate".
            // Envelopes + oscillators still run per sample, so audio stays smooth.
            const ctl = (si & (CTL - 1)) == 0;
            const res = std.math.clamp(p.resonance + (self.resonance - 0.25), 0.0, 1.0);

            var acc: f32 = 0.0;
            for (&self.voices) |*v| {
                if (!v.in_use) continue;
                const amp = v.amp_env.process(sr);
                if (!v.amp_env.active()) {
                    v.in_use = false;
                    continue;
                }
                const fenv = v.filt_env.process(sr);

                // glide toward the target frequency (per sample — cheap)
                if (glide_coef > 0.0) {
                    v.cur_freq = v.freq + (v.cur_freq - v.freq) * glide_coef;
                } else v.cur_freq = v.freq;

                if (ctl) {
                    // vibrato (lfo1 + mod wheel + aftertouch), cents -> ratio
                    const vib_cents = (p.lfo1_pitch * lfo1) +
                        30.0 * std.math.clamp(self.mod + self.pressure + v.note_press + self.chan_press[v.channel], 0.0, 1.0) * lfo1;
                    v.cur_vib = std.math.pow(f32, 2.0, vib_cents / 1200.0);
                    // cutoff: base * filter-env * keytrack * velocity * timbre * lfo2
                    var cut = self.cutoff;
                    cut *= std.math.pow(f32, 2.0, p.filter_env_amt * fenv);
                    cut *= std.math.pow(f32, 2.0, p.keytrack * (@as(f32, @floatFromInt(v.note)) - 60.0) / 12.0);
                    cut *= (1.0 - p.vel_to_cutoff) + p.vel_to_cutoff * v.vel;
                    cut *= std.math.pow(f32, 2.0, p.lfo2_cutoff * lfo2);
                    cut *= std.math.pow(f32, 2.0, (v.note_timbre + self.chan_timbre[v.channel]) * 2.0);
                    cut = std.math.clamp(cut, 40.0, 18000.0);
                    v.filt.sr = sr; // honor the (possibly oversampled) render rate
                    v.filt.set(cut, res, p.drive);
                    // analog drift: random-walk each oscillator's pitch (free-running
                    // VCOs slowly wander), fold into the cached unison-detune ratios
                    if (p.drift > 0.0) {
                        // 2^(cents/1200) ~= 1 + cents*ln2/1200 (drift is tiny, so this
                        // is exact to <0.001% and avoids a pow() on the hot path)
                        const c = p.drift * 0.000577623; // ln(2)/1200
                        var di: usize = 0;
                        while (di < un) : (di += 1) {
                            v.wa[di] = v.wa[di] * 0.99 + v.drand() * 0.06;
                            v.wb[di] = v.wb[di] * 0.99 + v.drand() * 0.06;
                            v.ra[di] = det_ratio[di] * (1.0 + v.wa[di] * c);
                            v.rb[di] = det_ratio[di] * (1.0 + v.wb[di] * c);
                        }
                    } else {
                        var di: usize = 0;
                        while (di < un) : (di += 1) {
                            v.ra[di] = det_ratio[di];
                            v.rb[di] = det_ratio[di];
                        }
                    }
                }
                const base = v.cur_freq * self.bend * self.chan_bend[v.channel] * v.note_bend * v.cur_vib * v.tune;

                // two oscillators (unison detune + drift baked into ra/rb), per sample
                var oa: f32 = 0;
                var ob: f32 = 0;
                var ui: usize = 0;
                while (ui < un) : (ui += 1) {
                    oa += v.osc_a[ui].next(p.osc_a, (base * v.ra[ui]) / sr, p.pulse_width);
                    ob += v.osc_b[ui].next(p.osc_b, (base * v.rb[ui] * b_ratio) / sr, p.pulse_width);
                }
                oa *= un_norm;
                ob *= un_norm;
                var sig = oa * (1.0 - p.osc_mix) + ob * p.osc_mix;
                if (p.sub_level > 0.0) sig += v.sub.next(.square, (base * 0.5) / sr, 0.5) * p.sub_level;
                if (p.noise_level > 0.0) sig += v.noise() * p.noise_level;

                acc += v.filt.process(sig) * amp * v.vel;
            }
            sample.* = std.math.clamp(acc * p.level * self.gain * self.expression, -1.0, 1.0);
        }
    }

    /// Render a block. When `oversample` > 1 the whole voice render (oscillators +
    /// the SATURATING filter) runs at N× the rate into a scratch buffer, then a
    /// steep decimator removes the aliases the nonlinearities create before folding
    /// back to the base rate — the difference between "harsh digital" and "silky".
    pub fn renderBlock(self: *Synth, out: []f32) void {
        const os: usize = switch (self.oversample) {
            2 => 2,
            4 => 4,
            else => 1,
        };
        if (os == 1) return self.renderInner(out, self.sample_rate);
        const os_sr = self.sample_rate * @as(f32, @floatFromInt(os));
        self.decim.configure(os_sr, self.sample_rate);
        var scratch: [256 * 4]f32 = undefined;
        var off: usize = 0;
        while (off < out.len) {
            const chunk = @min(@as(usize, 256), out.len - off);
            self.renderInner(scratch[0 .. chunk * os], os_sr);
            for (0..chunk) |i| {
                var y: f32 = 0;
                for (0..os) |k| y = self.decim.process(scratch[i * os + k]); // filter every os sample, keep the last
                out[off + i] = y;
            }
            off += chunk;
        }
    }
};

/// Anti-aliasing decimator: a 6-pole Butterworth low-pass (3 cascaded biquads) at
/// ~0.45×base-Nyquist, run at the oversampled rate; downsampling just keeps 1-of-N.
const Decimator = struct {
    stages: [3]effects.Biquad = .{ .{}, .{}, .{} },
    cfg_sr: f32 = 0,

    pub fn configure(self: *Decimator, os_sr: f32, base_sr: f32) void {
        if (self.cfg_sr == os_sr) return; // already set up for this rate
        const fc = base_sr * 0.45;
        const q = [3]f32{ 0.51763809, 0.70710678, 1.9318517 }; // 6th-order Butterworth Qs
        for (&self.stages, 0..) |*s, i| s.* = effects.Biquad.lowpass(os_sr, fc, q[i]);
        self.cfg_sr = os_sr;
    }
    pub fn process(self: *Decimator, x: f32) f32 {
        var y = x;
        for (&self.stages) |*s| y = s.process(y);
        return y;
    }
};

pub fn noteToFreq(note: u8) f32 {
    return 440.0 * std.math.pow(f32, 2.0, (@as(f32, @floatFromInt(note)) - 69.0) / 12.0);
}
fn freqToNote(freq: f32) u8 {
    if (freq <= 0) return 0;
    const n = 69.0 + 12.0 * std.math.log2(freq / 440.0);
    return @intFromFloat(std.math.clamp(@round(n), 0.0, 127.0));
}

test "velocity scales output level" {
    var s = Synth{};
    var buf: [4800]f32 = undefined;
    s.noteOn(440.0, 0.2);
    s.renderBlock(&buf);
    var soft: f32 = 0;
    for (buf) |x| soft = @max(soft, @abs(x));
    s = Synth{};
    s.noteOn(440.0, 1.0);
    s.renderBlock(&buf);
    var hard: f32 = 0;
    for (buf) |x| hard = @max(hard, @abs(x));
    try std.testing.expect(hard > soft * 1.8);
}

test "sustain pedal holds a released note" {
    var s = Synth{};
    var buf: [256]f32 = undefined;
    s.setSustain(true);
    s.noteOn(440.0, 1.0);
    s.noteOff(440.0);
    s.renderBlock(&buf);
    var lvl: f32 = 0;
    for (buf) |x| lvl = @max(lvl, @abs(x));
    try std.testing.expect(lvl > 0.01);
    try std.testing.expect(s.voices[0].in_use);
    s.setSustain(false);
    try std.testing.expect(s.voices[0].amp_env.stage == .release);
}

test "filter envelope opens the cutoff (onset brighter than sustain)" {
    var s = Synth{};
    s.setPatch(.{ .filter_env_amt = 3.0, .filt_env = .{ .a = 0.001, .d = 0.03, .s = 0.0, .r = 0.2 } });
    s.cutoff = 400; // low base so the env sweep is audible
    var buf: [9600]f32 = undefined; // 200 ms
    s.noteOn(220.0, 1.0);
    s.renderBlock(&buf);
    // energy in the first 30 ms (env open) vs the last 30 ms (env closed)
    var onset: f32 = 0;
    for (buf[0..1440]) |x| onset += x * x;
    var tail: f32 = 0;
    for (buf[8160..]) |x| tail += x * x;
    try std.testing.expect(onset > tail); // the filter sweep makes the onset brighter/louder
}

test "MPE per-note bend detunes only its own voice" {
    var s = Synth{};
    s.noteOnMpe(noteToFreq(60), 1.0, 0);
    s.noteOnMpe(noteToFreq(64), 1.0, 1);
    s.setNoteBend(64, 1.05); // bend only note 64
    try std.testing.expectEqual(@as(f32, 1.0), s.voices[0].note_bend);
    try std.testing.expectEqual(@as(f32, 1.05), s.voices[1].note_bend);
}

fn aliasEnergy2(patch: Patch, os: u8, hz: f32, fund: f32) f32 {
    var s = Synth{ .oversample = os };
    s.setPatch(patch);
    s.cutoff = 18000; // filter wide open so saturation harmonics pass
    s.noteOn(fund, 1.0);
    var buf: [8192]f32 = undefined;
    s.renderBlock(&buf); // settle
    s.renderBlock(&buf);
    var bp = effects.Biquad.bandpass(48000, hz, 8.0);
    var acc: f32 = 0;
    for (buf) |x| {
        const y = bp.process(x);
        acc += y * y;
    }
    return @sqrt(acc / @as(f32, @floatFromInt(buf.len)));
}

test "oversampling reduces aliasing from filter saturation" {
    // a saturated sine at 11 kHz: its strong 3rd harmonic (33 kHz) folds to 15 kHz
    // at the base rate. Oversampling renders/decimates it away; 1x leaves the alias.
    const patch = Patch{ .osc_a = .sine, .osc_mix = 0.0, .filter_model = .ladder, .drive = 4.0, .resonance = 0.1 };
    const at_1x = aliasEnergy2(patch, 1, 15000.0, 11000.0);
    const at_4x = aliasEnergy2(patch, 4, 15000.0, 11000.0);
    try std.testing.expect(at_4x < at_1x * 0.6); // 4x oversampling cuts the alias
}

test "analog drift moves pitch + voices detune individually" {
    var s = Synth{}; // default patch: drift = 4 cents, tune_spread = 2 cents
    s.oversample = 1;
    s.noteOn(noteToFreq(60), 1.0);
    s.noteOn(noteToFreq(67), 1.0);
    var buf: [2400]f32 = undefined;
    s.renderBlock(&buf);
    try std.testing.expect(s.voices[0].in_use and s.voices[1].in_use);
    // each voice got its own fixed tuning offset (component tolerance)
    try std.testing.expect(s.voices[0].tune != s.voices[1].tune);
    // drift walked the oscillator ratio off its nominal 1.0 (unison = 1 -> detune 1)
    try std.testing.expect(@abs(s.voices[0].ra[0] - 1.0) > 1.0e-5);
}

test "presets render without NaN or runaway" {
    inline for (.{ Patch.init_saw, Patch.fat_bass, Patch.super_lead, Patch.warm_pad }) |patch| {
        var s = Synth{};
        s.setPatch(patch);
        var buf: [4800]f32 = undefined;
        s.noteOn(noteToFreq(57), 0.9);
        s.renderBlock(&buf);
        var peak: f32 = 0;
        for (buf) |x| {
            try std.testing.expect(!std.math.isNan(x));
            peak = @max(peak, @abs(x));
        }
        try std.testing.expect(peak <= 1.0);
    }
}
