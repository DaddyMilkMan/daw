//! ai_midi.zig — algorithmic MIDI pattern generation (clean-room Zig port of the
//! legacy `ai_client/MIDIPatternGenerator.cpp`).
//!
//! Pure music theory: 50+ scales, per-style drum/bass/chord/melody/arp
//! generators, swing + humanize + quantize. No LLM, no JUCE — deterministic
//! given a seed. The AI wedge exposes this as the `generate_pattern` tool so the
//! model can lay real notes with one call (no API cost, no hallucinated MIDI).
//!
//! Output is `GenNote` in beats; the tool layer converts beats→frames using the
//! project tempo + sample rate before writing into a clip.

const std = @import("std");

pub const Style = enum { trap, house, jazz, lofi, rock, edm, rnb, pop };
pub const Kind = enum { drums, bass, chords, melody, arp };

/// A generated note in musical time (beats), before the frame conversion.
pub const GenNote = struct {
    pitch: i32,
    start_beats: f64,
    length_beats: f64,
    velocity: i32,
};

pub const Spec = struct {
    kind: Kind = .drums,
    style: Style = .trap,
    key: []const u8 = "C",
    scale: []const u8 = "minor",
    bars: u32 = 4,
    complexity: f32 = 0.5,
    swing: f32 = 0.0,
    humanize: f32 = 0.1,
    variation: f32 = 0.5,
    seed: u64 = 0,
    quantize_grid: []const u8 = "1/16",
    quantize_strength: f32 = 1.0,
};

// General-MIDI drum map.
const KICK = 36;
const SNARE = 38;
const CLAP = 39;
const CLOSED_HAT = 42;
const OPEN_HAT = 46;
const RIDE = 51;

// ---------------------------------------------------------------------------
// Seeded RNG + the humanize/variation helpers.
// ---------------------------------------------------------------------------

const Rng = struct {
    prng: std.Random.DefaultPrng,

    fn init(seed: u64) Rng {
        return .{ .prng = std.Random.DefaultPrng.init(seed) };
    }
    fn r(self: *Rng) std.Random {
        return self.prng.random();
    }
    fn f01(self: *Rng) f32 {
        return self.r().float(f32);
    }
    /// base ± up to 0.3, scaled by variation, clamped to [0,1].
    fn varyProbability(self: *Rng, base: f32, variation: f32) f32 {
        const d = self.r().float(f32) * 0.6 - 0.3;
        return std.math.clamp(base + d * variation, 0.0, 1.0);
    }
    fn applyHumanizeVel(self: *Rng, vel: i32, amount: f32) i32 {
        const d: f32 = @floatFromInt(self.r().intRangeAtMost(i32, -15, 15));
        const v = vel + @as(i32, @intFromFloat(d * amount));
        return std.math.clamp(v, 1, 127);
    }
    fn applyTimingHumanize(self: *Rng, pos: f64, amount: f32) f64 {
        const d = self.r().float(f64) * 0.06 - 0.03;
        return pos + d * @as(f64, amount);
    }
};

fn applySwing(position: f64, swing: f32) f64 {
    const step = @mod(@as(i64, @intFromFloat(position * 4)), 4);
    if (step == 1 or step == 3) return position + @as(f64, swing) * 0.08;
    return position;
}

// ---------------------------------------------------------------------------
// Scales.
// ---------------------------------------------------------------------------

const scale_db = std.StaticStringMap([]const i8).initComptime(.{
    .{ "ionian", &[_]i8{ 0, 2, 4, 5, 7, 9, 11 } },
    .{ "major", &[_]i8{ 0, 2, 4, 5, 7, 9, 11 } },
    .{ "dorian", &[_]i8{ 0, 2, 3, 5, 7, 9, 10 } },
    .{ "phrygian", &[_]i8{ 0, 1, 3, 5, 7, 8, 10 } },
    .{ "lydian", &[_]i8{ 0, 2, 4, 6, 7, 9, 11 } },
    .{ "mixolydian", &[_]i8{ 0, 2, 4, 5, 7, 9, 10 } },
    .{ "aeolian", &[_]i8{ 0, 2, 3, 5, 7, 8, 10 } },
    .{ "natural_minor", &[_]i8{ 0, 2, 3, 5, 7, 8, 10 } },
    .{ "minor", &[_]i8{ 0, 2, 3, 5, 7, 8, 10 } },
    .{ "locrian", &[_]i8{ 0, 1, 3, 5, 6, 8, 10 } },
    .{ "harmonic_minor", &[_]i8{ 0, 2, 3, 5, 7, 8, 11 } },
    .{ "melodic_minor", &[_]i8{ 0, 2, 3, 5, 7, 9, 11 } },
    .{ "hungarian_minor", &[_]i8{ 0, 2, 3, 6, 7, 8, 11 } },
    .{ "neapolitan_minor", &[_]i8{ 0, 1, 3, 5, 7, 8, 11 } },
    .{ "harmonic_major", &[_]i8{ 0, 2, 4, 5, 7, 8, 11 } },
    .{ "double_harmonic", &[_]i8{ 0, 1, 4, 5, 7, 8, 11 } },
    .{ "pentatonic_major", &[_]i8{ 0, 2, 4, 7, 9 } },
    .{ "pentatonic_minor", &[_]i8{ 0, 3, 5, 7, 10 } },
    .{ "pentatonic", &[_]i8{ 0, 3, 5, 7, 10 } },
    .{ "blues", &[_]i8{ 0, 3, 5, 6, 7, 10 } },
    .{ "blues_major", &[_]i8{ 0, 2, 3, 4, 7, 9 } },
    .{ "bebop_dominant", &[_]i8{ 0, 2, 4, 5, 7, 9, 10, 11 } },
    .{ "bebop_major", &[_]i8{ 0, 2, 4, 5, 7, 8, 9, 11 } },
    .{ "arabic", &[_]i8{ 0, 1, 4, 5, 7, 8, 11 } },
    .{ "persian", &[_]i8{ 0, 1, 4, 5, 6, 8, 11 } },
    .{ "japanese", &[_]i8{ 0, 1, 5, 7, 8 } },
    .{ "hirajoshi", &[_]i8{ 0, 2, 3, 7, 8 } },
    .{ "gypsy", &[_]i8{ 0, 2, 3, 6, 7, 8, 10 } },
    .{ "spanish", &[_]i8{ 0, 1, 4, 5, 7, 8, 10 } },
    .{ "egyptian", &[_]i8{ 0, 2, 5, 7, 10 } },
    .{ "chinese", &[_]i8{ 0, 4, 6, 7, 11 } },
    .{ "chromatic", &[_]i8{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
    .{ "whole_tone", &[_]i8{ 0, 2, 4, 6, 8, 10 } },
    .{ "diminished", &[_]i8{ 0, 2, 3, 5, 6, 8, 9, 11 } },
    .{ "augmented", &[_]i8{ 0, 3, 4, 7, 8, 11 } },
    .{ "lydian_dominant", &[_]i8{ 0, 2, 4, 6, 7, 9, 10 } },
    .{ "altered", &[_]i8{ 0, 1, 3, 4, 6, 8, 10 } },
    .{ "phrygian_dominant", &[_]i8{ 0, 1, 4, 5, 7, 8, 10 } },
    .{ "prometheus", &[_]i8{ 0, 2, 4, 6, 9, 10 } },
    .{ "enigmatic", &[_]i8{ 0, 1, 4, 6, 8, 10, 11 } },
    .{ "flamenco", &[_]i8{ 0, 1, 4, 5, 7, 8, 11 } },
    .{ "byzantine", &[_]i8{ 0, 1, 4, 5, 7, 8, 11 } },
    .{ "kumoi", &[_]i8{ 0, 2, 3, 7, 9 } },
    .{ "iwato", &[_]i8{ 0, 1, 5, 6, 10 } },
    .{ "yo", &[_]i8{ 0, 2, 5, 7, 9 } },
});

/// Intervals for `name` (case-insensitive); defaults to minor if unknown.
pub fn scaleIntervals(name: []const u8) []const i8 {
    var buf: [40]u8 = undefined;
    const lower = std.ascii.lowerString(buf[0..@min(name.len, buf.len)], name[0..@min(name.len, buf.len)]);
    return scale_db.get(lower) orelse scale_db.get("minor").?;
}

/// MIDI pitch classes (0-11) of a scale rooted at `root` (e.g. "C", "F#").
fn scaleNotes(root: []const u8, scale: []const u8, out: *[12]i32) usize {
    const intervals = scaleIntervals(scale);
    const root_pc = @mod(noteNameToMidi(root, 0), 12);
    for (intervals, 0..) |iv, i| {
        out[i] = @mod(root_pc + @as(i32, iv), 12);
    }
    return intervals.len;
}

/// Parse a note name ("C", "C#", "Eb", "F#3") to a MIDI number. If the name has
/// no trailing octave digit, `default_octave` is used.
pub fn noteNameToMidi(name: []const u8, default_octave: i32) i32 {
    // split trailing digits / '-'
    var end = name.len;
    while (end > 0 and (std.ascii.isDigit(name[end - 1]) or name[end - 1] == '-')) end -= 1;
    const letter = name[0..end];
    var octave: i32 = default_octave;
    if (end < name.len) {
        octave = std.fmt.parseInt(i32, name[end..], 10) catch default_octave;
    }
    const pc: i32 = blk: {
        const Pair = struct { []const u8, i32 };
        const map = [_]Pair{
            .{ "C", 0 }, .{ "C#", 1 }, .{ "Db", 1 }, .{ "D", 2 },  .{ "D#", 3 },  .{ "Eb", 3 },
            .{ "E", 4 }, .{ "Fb", 4 }, .{ "E#", 5 }, .{ "F", 5 },  .{ "F#", 6 },  .{ "Gb", 6 },
            .{ "G", 7 }, .{ "G#", 8 }, .{ "Ab", 8 }, .{ "A", 9 },  .{ "A#", 10 }, .{ "Bb", 10 },
            .{ "B", 11 }, .{ "Cb", 11 }, .{ "B#", 0 },
        };
        for (map) |p| {
            if (std.mem.eql(u8, letter, p[0])) break :blk p[1];
        }
        break :blk 0;
    };
    return (octave + 1) * 12 + pc;
}

// ---------------------------------------------------------------------------
// Quantize.
// ---------------------------------------------------------------------------

fn gridSize(grid: []const u8) f64 {
    const m = std.StaticStringMap(f64).initComptime(.{
        .{ "1/4", 1.0 },     .{ "1/8", 0.5 },        .{ "1/16", 0.25 },
        .{ "1/32", 0.125 },  .{ "1/64", 0.0625 },    .{ "1/8T", 1.0 / 3.0 },
        .{ "1/16T", 1.0 / 6.0 }, .{ "1/4T", 2.0 / 3.0 },
    });
    return m.get(grid) orelse 0.25;
}

fn applyQuantize(notes: []GenNote, spec: Spec) void {
    if (spec.quantize_strength <= 0.0) return;
    const g = gridSize(spec.quantize_grid);
    for (notes) |*n| {
        const q = @round(n.start_beats / g) * g;
        n.start_beats = n.start_beats + (q - n.start_beats) * spec.quantize_strength;
        if (n.start_beats < 0) n.start_beats = 0;
    }
}

// ---------------------------------------------------------------------------
// Drum templates.
// ---------------------------------------------------------------------------

const DrumTemplate = struct {
    kick: [16]f32,
    snare: [16]f32,
    hat: [16]f32,
    open: [16]f32,
    hat_vel_var: f32 = 0.2,
};

fn varyRow(rng: *Rng, base: [16]f32, variation: f32) [16]f32 {
    var out: [16]f32 = undefined;
    for (base, 0..) |v, i| out[i] = rng.varyProbability(v, variation);
    return out;
}

fn drumTemplate(style: Style, variation: f32, rng: *Rng) DrumTemplate {
    const z = [_]f32{0} ** 16;
    switch (style) {
        .trap => {
            var t = DrumTemplate{
                .kick = varyRow(rng, .{ 1, 0, 0, 0.3, 0, 0, 0.7, 0, 0, 0, 0.4, 0, 0, 0, 0.2, 0 }, variation),
                .snare = varyRow(rng, .{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0.3 }, variation),
                .hat = varyRow(rng, .{ 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9 }, variation),
                .open = varyRow(rng, .{ 0, 0, 0, 0, 0, 0, 0, 0.3, 0, 0, 0, 0, 0, 0, 0, 0.3 }, variation),
                .hat_vel_var = 0.3 + variation * 0.2,
            };
            const extra: usize = @intFromFloat(variation * 4);
            var i: usize = 0;
            while (i < extra) : (i += 1) t.kick[rng.r().intRangeLessThan(usize, 0, 16)] = rng.varyProbability(0.3, variation);
            return t;
        },
        .house => {
            var t = DrumTemplate{
                .kick = varyRow(rng, .{ 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 }, variation),
                .snare = varyRow(rng, .{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, variation),
                .hat = varyRow(rng, .{ 0, 0, 0.9, 0, 0, 0, 0.9, 0, 0, 0, 0.9, 0, 0, 0, 0.9, 0 }, variation),
                .open = varyRow(rng, .{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.6, 0 }, variation),
            };
            if (rng.f01() < variation) t.kick[6] = rng.varyProbability(0.5, variation);
            if (rng.f01() < variation) t.kick[14] = rng.varyProbability(0.4, variation);
            return t;
        },
        .jazz => {
            var t = DrumTemplate{
                .kick = varyRow(rng, .{ 0.6, 0, 0, 0.3, 0, 0, 0.4, 0, 0, 0.2, 0, 0, 0.5, 0, 0, 0 }, variation),
                .snare = varyRow(rng, .{ 0, 0, 0.2, 0, 0, 0, 0.3, 0, 0, 0, 0.2, 0, 0, 0, 0.2, 0 }, variation),
                .hat = varyRow(rng, .{ 0.9, 0, 0.5, 0.9, 0, 0.5, 0.9, 0, 0.5, 0.9, 0, 0.5, 0.9, 0, 0.5, 0 }, variation),
                .open = z,
                .hat_vel_var = 0.25,
            };
            const ghosts: usize = @intFromFloat(variation * 6);
            var i: usize = 0;
            while (i < ghosts) : (i += 1) {
                const s = rng.r().intRangeLessThan(usize, 0, 16);
                t.snare[s] = @max(t.snare[s], rng.varyProbability(0.2, variation));
            }
            return t;
        },
        .lofi => {
            var t = DrumTemplate{
                .kick = varyRow(rng, .{ 1, 0, 0, 0, 0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0.4, 0, 0 }, variation),
                .snare = varyRow(rng, .{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0.2 }, variation),
                .hat = varyRow(rng, .{ 0.7, 0, 0.7, 0, 0.7, 0, 0.7, 0, 0.7, 0, 0.7, 0, 0.7, 0, 0.7, 0 }, variation),
                .open = z,
                .hat_vel_var = 0.2,
            };
            t.open[7] = rng.varyProbability(0.4, variation);
            return t;
        },
        .rock => return DrumTemplate{
            .kick = varyRow(rng, .{ 1, 0, 0, 0, 0, 0, 0.8, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, variation),
            .snare = varyRow(rng, .{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, variation),
            .hat = varyRow(rng, .{ 0.9, 0, 0.9, 0, 0.9, 0, 0.9, 0, 0.9, 0, 0.9, 0, 0.9, 0, 0.9, 0 }, variation),
            .open = z,
        },
        .edm => {
            var t = DrumTemplate{
                .kick = varyRow(rng, .{ 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.5, 0 }, variation),
                .snare = varyRow(rng, .{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, variation),
                .hat = varyRow(rng, .{ 1, 0.5, 1, 0.5, 1, 0.5, 1, 0.5, 1, 0.5, 1, 0.5, 1, 0.5, 1, 0.5 }, variation),
                .open = z,
                .hat_vel_var = 0.15,
            };
            if (rng.f01() < variation * 0.5) {
                var i: usize = 12;
                while (i < 16) : (i += 1) t.snare[i] = rng.varyProbability(0.7, variation);
            }
            return t;
        },
        .rnb => return DrumTemplate{
            .kick = varyRow(rng, .{ 1, 0, 0, 0.3, 0, 0, 0.6, 0, 0, 0, 0.4, 0, 0, 0, 0, 0 }, variation),
            .snare = varyRow(rng, .{ 0, 0, 0, 0, 1, 0, 0, 0.2, 0, 0, 0, 0, 1, 0, 0, 0 }, variation),
            .hat = varyRow(rng, .{ 0.8, 0.4, 0.8, 0.4, 0.8, 0.4, 0.8, 0.4, 0.8, 0.4, 0.8, 0.4, 0.8, 0.4, 0.8, 0.4 }, variation),
            .open = z,
            .hat_vel_var = 0.25,
        },
        .pop => return DrumTemplate{
            .kick = varyRow(rng, .{ 1, 0, 0, 0, 0, 0, 0.5, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, variation),
            .snare = varyRow(rng, .{ 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, variation),
            .hat = varyRow(rng, .{ 0.8, 0, 0.8, 0, 0.8, 0, 0.8, 0, 0.8, 0, 0.8, 0, 0.8, 0, 0.8, 0 }, variation),
            .open = z,
        },
    }
}

// ---------------------------------------------------------------------------
// Chord progressions.
// ---------------------------------------------------------------------------

const Chord = struct {
    notes: [6]i32 = undefined,
    len: u8 = 0,
};
const Prog = [4][]const i32;

fn progTable(style: Style) []const Prog {
    return switch (style) {
        .trap, .lofi => &[_]Prog{
            .{ &.{ 0, 3, 7 }, &.{ 8, 0, 3 }, &.{ 3, 7, 10 }, &.{ 10, 2, 5 } },
            .{ &.{ 0, 3, 7 }, &.{ 5, 8, 0 }, &.{ 7, 10, 2 }, &.{ 3, 7, 10 } },
            .{ &.{ 0, 3, 7 }, &.{ 10, 2, 5 }, &.{ 8, 0, 3 }, &.{ 5, 8, 0 } },
            .{ &.{ 0, 3, 7 }, &.{ 3, 7, 10 }, &.{ 5, 8, 0 }, &.{ 7, 10, 2 } },
        },
        .house, .edm => &[_]Prog{
            .{ &.{ 0, 3, 7 }, &.{ 5, 8, 0 }, &.{ 10, 2, 5 }, &.{ 3, 7, 10 } },
            .{ &.{ 0, 4, 7 }, &.{ 7, 11, 2 }, &.{ 9, 0, 4 }, &.{ 5, 9, 0 } },
            .{ &.{ 0, 3, 7 }, &.{ 8, 0, 3 }, &.{ 5, 8, 0 }, &.{ 10, 2, 5 } },
        },
        .jazz => &[_]Prog{
            .{ &.{ 2, 5, 9 }, &.{ 7, 11, 2 }, &.{ 0, 4, 7 }, &.{ 9, 0, 4 } },
            .{ &.{ 0, 4, 7 }, &.{ 9, 0, 4 }, &.{ 2, 5, 9 }, &.{ 7, 11, 2 } },
            .{ &.{ 0, 4, 7, 11 }, &.{ 5, 9, 0, 4 }, &.{ 2, 5, 9, 0 }, &.{ 7, 11, 2, 5 } },
        },
        .rock => &[_]Prog{
            .{ &.{ 0, 4, 7 }, &.{ 5, 9, 0 }, &.{ 7, 11, 2 }, &.{ 0, 4, 7 } },
            .{ &.{ 0, 4, 7 }, &.{ 7, 11, 2 }, &.{ 5, 9, 0 }, &.{ 0, 4, 7 } },
            .{ &.{ 9, 0, 4 }, &.{ 5, 9, 0 }, &.{ 0, 4, 7 }, &.{ 7, 11, 2 } },
        },
        .rnb => &[_]Prog{
            .{ &.{ 0, 4, 7, 11 }, &.{ 9, 0, 4, 7 }, &.{ 2, 5, 9, 0 }, &.{ 7, 11, 2, 5 } },
            .{ &.{ 0, 3, 7, 10 }, &.{ 5, 8, 0, 3 }, &.{ 10, 2, 5, 8 }, &.{ 3, 7, 10, 2 } },
        },
        .pop => &[_]Prog{
            .{ &.{ 0, 4, 7 }, &.{ 7, 11, 2 }, &.{ 9, 0, 4 }, &.{ 5, 9, 0 } },
            .{ &.{ 0, 4, 7 }, &.{ 5, 9, 0 }, &.{ 9, 0, 4 }, &.{ 7, 11, 2 } },
            .{ &.{ 9, 0, 4 }, &.{ 5, 9, 0 }, &.{ 0, 4, 7 }, &.{ 7, 11, 2 } },
        },
    };
}

fn chordProgression(style: Style, variation: f32, rng: *Rng) [4]Chord {
    const table = progTable(style);
    const sel = rng.r().intRangeLessThan(usize, 0, table.len);
    const chosen = table[sel];

    var out: [4]Chord = undefined;
    for (chosen, 0..) |ch, ci| {
        var c = Chord{ .len = @intCast(ch.len) };
        for (ch, 0..) |n, i| c.notes[i] = n;
        // Random inversion: rotate the lowest note up an octave.
        if (variation > 0.3 and rng.f01() < variation) {
            const inv = rng.r().intRangeAtMost(usize, 0, 2);
            var k: usize = 0;
            while (k < inv and c.len > 1) : (k += 1) {
                const bottom = c.notes[0] + 12;
                std.mem.copyForwards(i32, c.notes[0 .. c.len - 1], c.notes[1..c.len]);
                c.notes[c.len - 1] = bottom;
            }
        }
        out[ci] = c;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Generators.
// ---------------------------------------------------------------------------

/// Generate a pattern. Caller owns the returned slice.
pub fn generate(alloc: std.mem.Allocator, spec: Spec) ![]GenNote {
    var rng = Rng.init(spec.seed);
    var notes = std.ArrayList(GenNote).init(alloc);
    errdefer notes.deinit();

    switch (spec.kind) {
        .drums => try genDrums(&notes, spec, &rng),
        .bass => try genBass(&notes, spec, &rng),
        .chords => try genChords(&notes, spec, &rng),
        .melody => try genMelody(&notes, spec, &rng),
        .arp => try genArp(&notes, spec, &rng),
    }

    const slice = try notes.toOwnedSlice();
    applyQuantize(slice, spec);
    return slice;
}

fn genDrums(notes: *std.ArrayList(GenNote), spec: Spec, rng: *Rng) !void {
    const t = drumTemplate(spec.style, spec.variation, rng);
    const total: usize = 16 * spec.bars;
    var step: usize = 0;
    while (step < total) : (step += 1) {
        const ps = step % 16;
        var beat = @as(f64, @floatFromInt(step)) / 4.0;
        beat = applySwing(beat, spec.swing);
        const clap_styles = spec.style == .trap or spec.style == .house or spec.style == .edm;

        if (rng.f01() < t.kick[ps] * (0.5 + spec.complexity * 0.5))
            try notes.append(.{ .pitch = KICK, .start_beats = rng.applyTimingHumanize(beat, spec.humanize), .length_beats = 0.25, .velocity = rng.applyHumanizeVel(100, spec.humanize) });

        if (rng.f01() < t.snare[ps])
            try notes.append(.{ .pitch = if (clap_styles) CLAP else SNARE, .start_beats = rng.applyTimingHumanize(beat, spec.humanize), .length_beats = 0.25, .velocity = rng.applyHumanizeVel(110, spec.humanize) });

        if (rng.f01() < t.hat[ps] * (0.7 + spec.complexity * 0.3)) {
            const base_vel: i32 = if (ps % 4 == 0) 100 else 70;
            try notes.append(.{ .pitch = if (spec.style == .jazz) RIDE else CLOSED_HAT, .start_beats = rng.applyTimingHumanize(beat, spec.humanize), .length_beats = 0.125, .velocity = rng.applyHumanizeVel(base_vel, spec.humanize + t.hat_vel_var) });
        }

        if (t.open[ps] > 0 and rng.f01() < t.open[ps])
            try notes.append(.{ .pitch = OPEN_HAT, .start_beats = rng.applyTimingHumanize(beat, spec.humanize), .length_beats = 0.5, .velocity = rng.applyHumanizeVel(80, spec.humanize) });

        // Trap hi-hat rolls.
        if (spec.style == .trap and spec.complexity > 0.6 and (ps == 14 or ps == 15)) {
            var rr: usize = 0;
            while (rr < 2) : (rr += 1) {
                if (rng.f01() < spec.complexity * 0.5)
                    try notes.append(.{ .pitch = CLOSED_HAT, .start_beats = beat + @as(f64, @floatFromInt(rr)) * 0.0625, .length_beats = 0.0625, .velocity = rng.applyHumanizeVel(60 + @as(i32, @intCast(rr)) * 10, spec.humanize) });
            }
        }
    }
}

fn genBass(notes: *std.ArrayList(GenNote), spec: Spec, rng: *Rng) !void {
    const root_midi = noteNameToMidi(spec.key, 2);
    const rhythm = bassRhythm(spec.style, spec.variation, rng);
    const chords = chordProgression(spec.style, spec.variation, rng);
    const total: usize = 16 * spec.bars;
    var step: usize = 0;
    while (step < total) : (step += 1) {
        const ps = step % 16;
        const bar = step / 16;
        const beat = @as(f64, @floatFromInt(step)) / 4.0;
        const ci = bar % 4;
        const prob = rhythm.hit[ps] * (0.6 + spec.complexity * 0.4);
        if (rng.f01() < prob and rhythm.len[ps] > 0) {
            const chord_root = chords[ci].notes[0];
            const options: [5]i32 = .{ chord_root, chord_root, @mod(chord_root + 7, 12), chord_root + 12, @mod(chord_root + 5, 12) };
            var n_opts: usize = 4;
            if (spec.variation > 0.3) n_opts = 5;
            const interval = options[rng.r().intRangeLessThan(usize, 0, n_opts)];
            try notes.append(.{ .pitch = root_midi + interval, .start_beats = rng.applyTimingHumanize(beat, spec.humanize), .length_beats = rhythm.len[ps], .velocity = rng.applyHumanizeVel(90, spec.humanize) });
        }
    }
}

const BassRhythm = struct { hit: [16]f32, len: [16]f64 };
fn bassRhythm(style: Style, variation: f32, rng: *Rng) BassRhythm {
    var br: BassRhythm = undefined;
    switch (style) {
        .trap, .lofi => {
            const base = [16]f32{ 1, 0, 0, 0.3, 0, 0, 0.5, 0, 0, 0, 0.3, 0, 0, 0, 0.4, 0 };
            const lens = [16]f64{ 2, 0, 0, 1, 0, 0, 1.5, 0, 0, 0, 1, 0, 0, 0, 0.5, 0 };
            for (0..16) |i| {
                br.hit[i] = rng.varyProbability(base[i], variation);
                br.len[i] = lens[i] * (0.8 + rng.f01() * 0.4);
            }
        },
        .house, .edm => {
            const base = [16]f32{ 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.5, 0 };
            for (0..16) |i| {
                br.hit[i] = rng.varyProbability(base[i], variation);
                br.len[i] = 0.5;
            }
        },
        .jazz => {
            const base = [16]f32{ 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
            for (0..16) |i| {
                br.hit[i] = rng.varyProbability(base[i], variation);
                br.len[i] = 0.9;
            }
        },
        else => {
            const base = [16]f32{ 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
            for (0..16) |i| {
                br.hit[i] = rng.varyProbability(base[i], variation);
                br.len[i] = 0.4;
            }
        },
    }
    return br;
}

fn genChords(notes: *std.ArrayList(GenNote), spec: Spec, rng: *Rng) !void {
    const root_midi = noteNameToMidi(spec.key, 4);
    const chords = chordProgression(spec.style, spec.variation, rng);
    const rhythm = chordRhythm(spec.style, spec.variation, rng);
    const total: usize = 16 * spec.bars;
    var step: usize = 0;
    while (step < total) : (step += 1) {
        const ps = step % 16;
        const bar = step / 16;
        const beat = @as(f64, @floatFromInt(step)) / 4.0;
        const ci = bar % 4;
        if (rng.f01() < rhythm[ps]) {
            const len: f64 = switch (spec.style) {
                .trap, .lofi => 4.0,
                .house, .edm => 0.5,
                else => 1.0,
            };
            const c = chords[ci];
            for (0..c.len) |k| {
                try notes.append(.{ .pitch = root_midi + c.notes[k], .start_beats = rng.applyTimingHumanize(beat, spec.humanize * 0.3), .length_beats = len, .velocity = rng.applyHumanizeVel(75, spec.humanize) });
            }
        }
    }
}

fn chordRhythm(style: Style, variation: f32, rng: *Rng) [16]f32 {
    const base: [16]f32 = switch (style) {
        .trap, .lofi => .{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        .house, .edm => .{ 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0 },
        .jazz => .{ 0, 0, 0.8, 0, 0, 0.5, 0, 0, 0, 0, 0, 0.7, 0, 0, 0.5, 0 },
        else => .{ 1, 0, 0, 0, 0.5, 0, 0, 0, 1, 0, 0, 0, 0.5, 0, 0, 0 },
    };
    var out: [16]f32 = undefined;
    for (0..16) |i| out[i] = rng.varyProbability(base[i], variation);
    return out;
}

fn genMelody(notes: *std.ArrayList(GenNote), spec: Spec, rng: *Rng) !void {
    var scale_buf: [12]i32 = undefined;
    const n_scale = scaleNotes(spec.key, spec.scale, &scale_buf);
    const sc = scale_buf[0..n_scale];
    const root_midi = noteNameToMidi(spec.key, 5);

    // seed motif (3-5 notes)
    var motif_buf: [5]i32 = undefined;
    const motif_len: usize = @intCast(rng.r().intRangeAtMost(i32, 3, 5));
    {
        var idx: usize = rng.r().intRangeLessThan(usize, 0, sc.len);
        for (0..motif_len) |i| {
            motif_buf[i] = sc[idx];
            const mv = rng.r().intRangeAtMost(i32, -1, 1);
            idx = @intCast(std.math.clamp(@as(i32, @intCast(idx)) + mv, 0, @as(i32, @intCast(sc.len - 1))));
        }
    }

    const rhythm = melodyRhythm(spec.style, spec.variation, rng);
    const total: usize = 16 * spec.bars;
    var cur_idx: i32 = @intCast(rng.r().intRangeLessThan(usize, 0, sc.len));
    var direction: i32 = 1;
    var motif_i: usize = 0;
    var use_motif = true;

    var step: usize = 0;
    while (step < total) : (step += 1) {
        const ps = step % 16;
        const bar = step / 16;
        const beat = @as(f64, @floatFromInt(step)) / 4.0;

        if (ps == 0 and bar > 0) {
            developMotif(motif_buf[0..motif_len], spec.variation, rng);
            motif_i = 0;
        }

        if (rng.f01() < rhythm[ps] * spec.complexity) {
            var pitch: i32 = undefined;
            if (use_motif and motif_i < motif_len) {
                pitch = root_midi + motif_buf[motif_i];
                motif_i += 1;
                if (motif_i >= motif_len) {
                    use_motif = rng.f01() < 0.6;
                    motif_i = 0;
                }
            } else {
                if (rng.f01() < 0.7) {
                    cur_idx += direction;
                    if (cur_idx >= @as(i32, @intCast(sc.len))) {
                        cur_idx = @intCast(sc.len - 2);
                        direction = -1;
                    } else if (cur_idx < 0) {
                        cur_idx = 1;
                        direction = 1;
                    }
                } else {
                    cur_idx = @intCast(rng.r().intRangeLessThan(usize, 0, sc.len));
                    direction = rng.r().intRangeAtMost(i32, -1, 1);
                    if (direction == 0) direction = 1;
                }
                pitch = root_midi + sc[@intCast(cur_idx)];
            }
            const len_choice = rng.r().intRangeAtMost(i32, 1, 4);
            try notes.append(.{ .pitch = pitch, .start_beats = rng.applyTimingHumanize(beat, spec.humanize), .length_beats = @as(f64, @floatFromInt(len_choice)) * 0.25, .velocity = rng.applyHumanizeVel(85, spec.humanize) });
        }
    }
}

fn melodyRhythm(style: Style, variation: f32, rng: *Rng) [16]f32 {
    const base: [16]f32 = switch (style) {
        .trap => .{ 0.8, 0, 0.6, 0.3, 0.7, 0, 0.5, 0, 0.6, 0, 0.4, 0.3, 0.7, 0, 0.5, 0.4 },
        .lofi => .{ 0.6, 0, 0, 0.4, 0, 0, 0.5, 0, 0, 0.3, 0, 0, 0.4, 0, 0, 0.3 },
        .jazz => .{ 0.7, 0, 0.5, 0.4, 0, 0.6, 0.3, 0, 0.5, 0, 0.4, 0.3, 0, 0.5, 0.4, 0 },
        else => .{ 0.8, 0, 0.5, 0, 0.7, 0, 0.4, 0, 0.6, 0, 0.5, 0, 0.7, 0, 0.4, 0.3 },
    };
    var out: [16]f32 = undefined;
    for (0..16) |i| out[i] = rng.varyProbability(base[i], variation);
    return out;
}

fn developMotif(motif: []i32, variation: f32, rng: *Rng) void {
    if (rng.f01() < variation) {
        const t = rng.r().intRangeAtMost(i32, -5, 7);
        for (motif) |*n| n.* = @mod(n.* + t, 12);
    }
    if (rng.f01() < variation * 0.5) std.mem.reverse(i32, motif);
    for (motif) |*n| {
        if (rng.f01() < variation * 0.3) n.* = @mod(n.* + (if (rng.f01() < 0.5) @as(i32, 1) else -1), 12);
    }
}

fn genArp(notes: *std.ArrayList(GenNote), spec: Spec, rng: *Rng) !void {
    const chords = chordProgression(spec.style, spec.variation, rng);
    const root_midi = noteNameToMidi(spec.key, 4);
    const pattern = rng.r().intRangeAtMost(u32, 0, 3); // up/down/updown/random
    const skip: usize = rng.r().intRangeAtMost(usize, 0, 3);
    const total: usize = 16 * spec.bars;
    var step: usize = 0;
    while (step < total) : (step += 1) {
        const sib = step % 16;
        const bar = step / 16;
        const beat = @as(f64, @floatFromInt(step)) / 4.0;
        if (skip > 0 and sib % (skip + 1) == skip) continue;

        const c = chords[bar % 4];
        const cs: usize = c.len;
        var ni: usize = switch (pattern) {
            0 => sib % cs, // up
            1 => (cs - 1) - (sib % cs), // down
            2 => blk: { // updown
                const cycle = sib % (cs * 2 - 2);
                break :blk if (cycle < cs) cycle else cs - 2 - (cycle - cs);
            },
            else => rng.r().intRangeLessThan(usize, 0, cs), // random
        };
        if (ni >= cs) ni = cs - 1;

        var pitch = root_midi + c.notes[ni];
        if (sib >= 8 and rng.f01() < 0.3 * spec.variation) pitch += 12;
        if (rng.f01() < 0.1 * spec.variation) pitch -= 12;

        try notes.append(.{ .pitch = pitch, .start_beats = rng.applyTimingHumanize(beat, spec.humanize), .length_beats = 0.2 + rng.f01() * 0.1 * spec.variation, .velocity = rng.applyHumanizeVel(70 + (if (ni == 0) @as(i32, 15) else 0), spec.humanize) });
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test "scale lookup is case-insensitive and defaults to minor" {
    try std.testing.expectEqualSlices(i8, &[_]i8{ 0, 2, 4, 5, 7, 9, 11 }, scaleIntervals("Major"));
    try std.testing.expectEqualSlices(i8, &[_]i8{ 0, 2, 3, 5, 7, 8, 10 }, scaleIntervals("not_a_scale"));
}

test "noteNameToMidi parses names and octaves" {
    try std.testing.expectEqual(@as(i32, 60), noteNameToMidi("C", 4));
    try std.testing.expectEqual(@as(i32, 61), noteNameToMidi("C#4", 4));
    try std.testing.expectEqual(@as(i32, 63), noteNameToMidi("Eb4", 4));
    try std.testing.expectEqual(@as(i32, 36), noteNameToMidi("C2", 4));
}

test "each generator produces in-range notes" {
    const a = std.testing.allocator;
    inline for (.{ .drums, .bass, .chords, .melody, .arp }) |k| {
        const notes = try generate(a, .{ .kind = k, .style = .trap, .key = "C", .scale = "minor", .bars = 4, .complexity = 0.8, .seed = 42 });
        defer a.free(notes);
        try std.testing.expect(notes.len > 0);
        for (notes) |n| {
            try std.testing.expect(n.pitch >= 0 and n.pitch <= 127);
            try std.testing.expect(n.velocity >= 1 and n.velocity <= 127);
            try std.testing.expect(n.start_beats >= 0);
            try std.testing.expect(n.length_beats > 0);
        }
    }
}

test "generation is deterministic per seed" {
    const a = std.testing.allocator;
    const n1 = try generate(a, .{ .kind = .melody, .style = .jazz, .seed = 123, .bars = 2 });
    defer a.free(n1);
    const n2 = try generate(a, .{ .kind = .melody, .style = .jazz, .seed = 123, .bars = 2 });
    defer a.free(n2);
    try std.testing.expectEqual(n1.len, n2.len);
    for (n1, n2) |x, y| {
        try std.testing.expectEqual(x.pitch, y.pitch);
        try std.testing.expectEqual(x.velocity, y.velocity);
    }
}

test "bass notes sit in the scale's low register" {
    const a = std.testing.allocator;
    const notes = try generate(a, .{ .kind = .bass, .style = .house, .key = "C", .seed = 7, .bars = 2 });
    defer a.free(notes);
    for (notes) |n| try std.testing.expect(n.pitch >= 24 and n.pitch <= 60);
}
