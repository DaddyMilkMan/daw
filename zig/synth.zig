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

// ---- modulation matrix: any source -> any destination, with depth ----------
pub const MAX_ROUTES = 64; // generous cap (RT-safe, no allocation on the audio path)
pub const MAX_MACROS = 16; // assignable macro knobs (mod sources)

pub const ModSource = enum(u8) {
    none,
    lfo1,
    lfo2,
    filt_env,
    amp_env,
    velocity,
    aftertouch,
    mod_wheel,
    keytrack,
    random, // a fixed per-voice random value (humanize)
    macro, // macro index lives in ModRoute.macro
};

pub const ModDest = enum(u8) {
    none,
    pitch, // semitones (depth)
    cutoff, // octaves
    resonance, // 0..1 added
    pan, // -1..1
    amp, // gain offset
    osc_mix, // 0..1 added
    pulse_width, // 0..1 added
};

/// One modulation route. `depth` is in the destination's natural units.
pub const ModRoute = struct {
    source: ModSource = .none,
    dest: ModDest = .none,
    macro: u8 = 0, // which macro, when source == .macro
    depth: f32 = 0,
};

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
    stereo: f32 = 0.6, // unison stereo spread width (0 = mono, 1 = full)
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
    // modulation matrix (additive on top of the built-in routings). n_routes can
    // grow up to MAX_ROUTES; the UI scrolls through them.
    routes: [MAX_ROUTES]ModRoute = [_]ModRoute{.{}} ** MAX_ROUTES,
    n_routes: u8 = 0,

    /// Append a route (no-op when full). Returns the new route count.
    pub fn addRoute(self: *Patch, r: ModRoute) u8 {
        if (self.n_routes < MAX_ROUTES) {
            self.routes[self.n_routes] = r;
            self.n_routes += 1;
        }
        return self.n_routes;
    }
    pub fn removeRoute(self: *Patch, idx: usize) void {
        if (idx >= self.n_routes) return;
        var i = idx;
        while (i + 1 < self.n_routes) : (i += 1) self.routes[i] = self.routes[i + 1];
        self.n_routes -= 1;
    }

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
    rnd_mod: f32 = 0.0, // fixed per-voice random (a mod-matrix source)
    // cached mod-matrix outputs (recomputed at control rate)
    cur_res: f32 = 0.2,
    cur_pan: f32 = 0.0,
    cur_amp: f32 = 1.0,
    cur_mix: f32 = 0.35,
    cur_pw: f32 = 0.5,
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
        self.rnd_mod = self.drand(); // fixed per-voice random source (-1..1)
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
    decimL: Decimator = .{},
    decimR: Decimator = .{},
    macro_bits: [MAX_MACROS]u32 = [_]u32{0} ** MAX_MACROS, // macro values 0..1 (mod sources)

    pub fn setPatch(self: *Synth, p: Patch) void {
        self.patch = p;
    }
    pub fn setMacro(self: *Synth, i: usize, v: f32) void { // UI thread -> audio
        if (i < MAX_MACROS) @atomicStore(u32, &self.macro_bits[i], @bitCast(v), .monotonic);
    }
    fn macro(self: *Synth, i: u8) f32 {
        return if (i < MAX_MACROS) @bitCast(@atomicLoad(u32, &self.macro_bits[i], .monotonic)) else 0;
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

    fn renderInner(self: *Synth, outL: []f32, outR: []f32, sr: f32) void {
        const p = &self.patch;
        const lfo1_inc = p.lfo1_rate / sr;
        const lfo2_inc = p.lfo2_rate / sr;
        const glide_coef: f32 = if (p.glide > 0.0) @exp(-1.0 / (p.glide * sr)) else 0.0;
        const un: usize = @max(@as(usize, 1), @min(@as(usize, p.unison), MAX_UNISON));
        const un_norm = 1.0 / @sqrt(@as(f32, @floatFromInt(un)));
        // per-block constants (hoisted out of the hot loop): oscB ratio + unison detunes
        const b_ratio = std.math.pow(f32, 2.0, (p.osc_b_semi + p.osc_b_fine / 100.0) / 12.0);
        var det_ratio: [MAX_UNISON]f32 = undefined;
        var pan_l: [MAX_UNISON]f32 = undefined;
        var pan_r: [MAX_UNISON]f32 = undefined;
        for (0..un) |ui| {
            const spread: f32 = if (un > 1)
                (@as(f32, @floatFromInt(ui)) / @as(f32, @floatFromInt(un - 1)) - 0.5) * 2.0
            else
                0.0;
            det_ratio[ui] = std.math.pow(f32, 2.0, (spread * p.unison_detune) / 1200.0);
            // stereo: spread the unison voices across the field (constant-power,
            // ×√2 so a centered voice is unity — keeps the mono sum level-matched)
            const ang = (spread * p.stereo * 0.5 + 0.5) * (std.math.pi / 2.0);
            pan_l[ui] = @cos(ang) * 1.4142135;
            pan_r[ui] = @sin(ang) * 1.4142135;
        }

        for (0..outL.len) |si| {
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

            var accL: f32 = 0.0;
            var accR: f32 = 0.0;
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
                    // --- modulation matrix: sum (source × depth) per destination ---
                    var dm_pitch: f32 = 0;
                    var dm_cut: f32 = 0;
                    var dm_res: f32 = 0;
                    var dm_pan: f32 = 0;
                    var dm_amp: f32 = 0;
                    var dm_mix: f32 = 0;
                    var dm_pw: f32 = 0;
                    var ri: usize = 0;
                    while (ri < p.n_routes) : (ri += 1) {
                        const rt = p.routes[ri];
                        const sv: f32 = switch (rt.source) {
                            .none => 0,
                            .lfo1 => lfo1,
                            .lfo2 => lfo2,
                            .filt_env => fenv,
                            .amp_env => amp,
                            .velocity => v.vel,
                            .aftertouch => std.math.clamp(self.pressure + v.note_press + self.chan_press[v.channel], 0.0, 1.0),
                            .mod_wheel => self.mod,
                            .keytrack => (@as(f32, @floatFromInt(v.note)) - 60.0) / 12.0,
                            .random => v.rnd_mod,
                            .macro => self.macro(rt.macro),
                        };
                        const d = sv * rt.depth;
                        switch (rt.dest) {
                            .none => {},
                            .pitch => dm_pitch += d,
                            .cutoff => dm_cut += d,
                            .resonance => dm_res += d,
                            .pan => dm_pan += d,
                            .amp => dm_amp += d,
                            .osc_mix => dm_mix += d,
                            .pulse_width => dm_pw += d,
                        }
                    }
                    // vibrato (lfo1 + mod wheel + aftertouch) + matrix pitch (semis -> cents)
                    const vib_cents = (p.lfo1_pitch * lfo1) +
                        30.0 * std.math.clamp(self.mod + self.pressure + v.note_press + self.chan_press[v.channel], 0.0, 1.0) * lfo1 +
                        dm_pitch * 100.0;
                    v.cur_vib = std.math.pow(f32, 2.0, vib_cents / 1200.0);
                    // cutoff: base * filter-env * keytrack * velocity * timbre * lfo2 * matrix
                    var cut = self.cutoff;
                    cut *= std.math.pow(f32, 2.0, p.filter_env_amt * fenv);
                    cut *= std.math.pow(f32, 2.0, p.keytrack * (@as(f32, @floatFromInt(v.note)) - 60.0) / 12.0);
                    cut *= (1.0 - p.vel_to_cutoff) + p.vel_to_cutoff * v.vel;
                    cut *= std.math.pow(f32, 2.0, p.lfo2_cutoff * lfo2);
                    cut *= std.math.pow(f32, 2.0, (v.note_timbre + self.chan_timbre[v.channel]) * 2.0);
                    cut *= std.math.pow(f32, 2.0, dm_cut); // matrix cutoff (octaves)
                    cut = std.math.clamp(cut, 40.0, 18000.0);
                    // cache the matrix-modulated per-voice params
                    v.cur_res = std.math.clamp(res + dm_res, 0.0, 1.0);
                    v.cur_pan = std.math.clamp(dm_pan, -1.0, 1.0);
                    v.cur_amp = std.math.clamp(1.0 + dm_amp, 0.0, 2.0);
                    v.cur_mix = std.math.clamp(p.osc_mix + dm_mix, 0.0, 1.0);
                    v.cur_pw = std.math.clamp(p.pulse_width + dm_pw, 0.05, 0.95);
                    v.filt.sr = sr; // honor the (possibly oversampled) render rate
                    v.filt.set(cut, v.cur_res, p.drive);
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

                // two oscillators (unison detune + drift baked into ra/rb), each unison
                // voice panned across the stereo field, per sample
                var sL: f32 = 0;
                var sR: f32 = 0;
                var ui: usize = 0;
                while (ui < un) : (ui += 1) {
                    const oa = v.osc_a[ui].next(p.osc_a, (base * v.ra[ui]) / sr, v.cur_pw);
                    const ob = v.osc_b[ui].next(p.osc_b, (base * v.rb[ui] * b_ratio) / sr, v.cur_pw);
                    const s = oa * (1.0 - v.cur_mix) + ob * v.cur_mix;
                    sL += s * pan_l[ui];
                    sR += s * pan_r[ui];
                }
                sL *= un_norm;
                sR *= un_norm;
                var center: f32 = 0;
                if (p.sub_level > 0.0) center += v.sub.next(.square, (base * 0.5) / sr, 0.5) * p.sub_level;
                if (p.noise_level > 0.0) center += v.noise() * p.noise_level;
                sL += center;
                sR += center;

                // mid/side: filter the mono mid (one filter per voice), keep the side
                // for stereo width — the high-frequency unison detune shimmer.
                const mid = (sL + sR) * 0.5;
                const side = (sL - sR) * 0.5;
                const fmid = v.filt.process(mid);
                // matrix amp + pan (balance) on top of the envelope/velocity
                const g = amp * v.vel * v.cur_amp;
                const lg: f32 = if (v.cur_pan > 0) 1.0 - v.cur_pan else 1.0;
                const rg: f32 = if (v.cur_pan < 0) 1.0 + v.cur_pan else 1.0;
                accL += (fmid + side) * g * lg;
                accR += (fmid - side) * g * rg;
            }
            const lvl = p.level * self.gain * self.expression;
            outL[si] = std.math.clamp(accL * lvl, -1.0, 1.0);
            outR[si] = std.math.clamp(accR * lvl, -1.0, 1.0);
        }
    }

    /// Render a STEREO block. When `oversample` > 1 the whole voice render
    /// (oscillators + the SATURATING filter) runs at N× the rate into scratch
    /// buffers, then steep decimators remove the aliases the nonlinearities create
    /// before folding back — the difference between "harsh digital" and "silky".
    pub fn renderStereo(self: *Synth, outL: []f32, outR: []f32) void {
        const os: usize = switch (self.oversample) {
            2 => 2,
            4 => 4,
            else => 1,
        };
        if (os == 1) return self.renderInner(outL, outR, self.sample_rate);
        const os_sr = self.sample_rate * @as(f32, @floatFromInt(os));
        self.decimL.configure(os_sr, self.sample_rate);
        self.decimR.configure(os_sr, self.sample_rate);
        var scratchL: [256 * 4]f32 = undefined;
        var scratchR: [256 * 4]f32 = undefined;
        var off: usize = 0;
        while (off < outL.len) {
            const chunk = @min(@as(usize, 256), outL.len - off);
            self.renderInner(scratchL[0 .. chunk * os], scratchR[0 .. chunk * os], os_sr);
            for (0..chunk) |i| {
                var yl: f32 = 0;
                var yr: f32 = 0;
                for (0..os) |k| { // filter every os sample, keep the last (decimate)
                    yl = self.decimL.process(scratchL[i * os + k]);
                    yr = self.decimR.process(scratchR[i * os + k]);
                }
                outL[off + i] = yl;
                outR[off + i] = yr;
            }
            off += chunk;
        }
    }

    /// Render a MONO block (the stereo render summed to center). Kept for callers
    /// (and tests) that don't need stereo.
    pub fn renderBlock(self: *Synth, out: []f32) void {
        var off: usize = 0;
        while (off < out.len) {
            const chunk = @min(@as(usize, 256), out.len - off);
            var tL: [256]f32 = undefined;
            var tR: [256]f32 = undefined;
            self.renderStereo(tL[0..chunk], tR[0..chunk]);
            for (0..chunk) |i| out[off + i] = (tL[i] + tR[i]) * 0.5;
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

test "unison produces a stereo image; a single oscillator stays mono" {
    var wide = Synth{ .oversample = 1 };
    wide.setPatch(.{ .unison = 7, .unison_detune = 14, .stereo = 1.0 });
    wide.noteOn(noteToFreq(57), 1.0);
    var lw: [2400]f32 = undefined;
    var rw: [2400]f32 = undefined;
    wide.renderStereo(&lw, &rw);
    var diff: f32 = 0;
    for (lw, rw) |l, r| diff += (l - r) * (l - r);
    try std.testing.expect(diff > 0.001); // L and R decorrelate -> stereo width

    var mono = Synth{ .oversample = 1 };
    mono.setPatch(.{ .unison = 1, .stereo = 1.0 });
    mono.noteOn(noteToFreq(57), 1.0);
    var lm: [2400]f32 = undefined;
    var rm: [2400]f32 = undefined;
    mono.renderStereo(&lm, &rm);
    var d2: f32 = 0;
    for (lm, rm) |l, r| d2 += @abs(l - r);
    try std.testing.expect(d2 < 1.0e-4); // single osc -> identical channels (centered)
}

fn peakOf(buf: []const f32) f32 {
    var p: f32 = 0;
    for (buf) |x| p = @max(p, @abs(x));
    return p;
}

test "mod matrix routes a macro to a destination (amp)" {
    var s = Synth{ .oversample = 1 };
    var patch = Patch.init_saw;
    _ = patch.addRoute(.{ .source = .macro, .macro = 0, .dest = .amp, .depth = -1.0 });
    s.setPatch(patch);
    var buf: [2400]f32 = undefined;
    // macro 0 = 0 -> amp unchanged, audible
    s.setMacro(0, 0.0);
    s.noteOn(noteToFreq(60), 1.0);
    s.renderBlock(&buf);
    try std.testing.expect(peakOf(&buf) > 0.01);
    // macro 0 = 1 -> amp offset -1 -> cur_amp 0 -> silence
    var s2 = Synth{ .oversample = 1 };
    s2.setPatch(patch);
    s2.setMacro(0, 1.0);
    s2.noteOn(noteToFreq(60), 1.0);
    s2.renderBlock(&buf);
    try std.testing.expect(peakOf(&buf) < 0.001);
}

test "mod matrix expands and a macro opens the filter" {
    var patch = Patch.init_saw;
    patch.filter_env_amt = 0; // isolate the matrix
    _ = patch.addRoute(.{ .source = .macro, .macro = 3, .dest = .cutoff, .depth = 4.0 }); // +4 octaves at full
    try std.testing.expectEqual(@as(u8, 1), patch.n_routes);
    // can keep adding routes up to the cap
    var i: usize = 0;
    while (i < 30) : (i += 1) _ = patch.addRoute(.{ .source = .lfo1, .dest = .pitch, .depth = 0.0 });
    try std.testing.expectEqual(@as(u8, 31), patch.n_routes);

    var dark = Synth{ .oversample = 1 };
    dark.setPatch(patch);
    dark.cutoff = 300;
    dark.setMacro(3, 0.0);
    dark.noteOn(noteToFreq(45), 1.0);
    var buf: [4800]f32 = undefined;
    dark.renderBlock(&buf);
    const e_dark = peakOf(&buf);
    var bright = Synth{ .oversample = 1 };
    bright.setPatch(patch);
    bright.cutoff = 300;
    bright.setMacro(3, 1.0); // macro opens the filter +4 octaves
    bright.noteOn(noteToFreq(45), 1.0);
    bright.renderBlock(&buf);
    const e_bright = peakOf(&buf);
    try std.testing.expect(e_bright > e_dark * 1.3); // a more-open filter passes more energy
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
