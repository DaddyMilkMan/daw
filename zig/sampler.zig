//! sampler.zig — a polyphonic sample-playback instrument. Loads a mono sample
//! and pitches it per MIDI note (rate = 2^((note-root)/12)), with a quick
//! attack/release envelope. Triggered exactly like the synth.

const std = @import("std");
const resample = @import("resample.zig");

const Voice = struct {
    const State = enum { idle, playing, releasing };
    pos: f64 = 0.0,
    rate: f64 = 1.0,
    amp: f32 = 0.0,
    state: State = .idle,
    note: u8 = 0,
};

pub const Sampler = struct {
    sample: []const f32 = &.{}, // mono source
    sample_rate: f64 = 48000.0,
    out_sample_rate: f64 = 48000.0,
    root_note: f32 = 60.0, // MIDI note at which the sample plays at native speed
    gain: f32 = 0.7,
    attack_inc: f32 = 0.005,
    release_dec: f32 = 0.001,
    voices: [16]Voice = [_]Voice{.{}} ** 16,

    pub fn init(sample: []const f32, sample_rate: f64, root_note: f32, out_sample_rate: f64) Sampler {
        return .{
            .sample = sample,
            .sample_rate = sample_rate,
            .out_sample_rate = out_sample_rate,
            .root_note = root_note,
            .attack_inc = @floatCast(1.0 / (0.003 * out_sample_rate)), // ~3 ms
            .release_dec = @floatCast(1.0 / (0.08 * out_sample_rate)), // ~80 ms
        };
    }

    pub fn noteOn(self: *Sampler, note: u8) void {
        const semis = @as(f64, @floatFromInt(note)) - self.root_note;
        const pitch = std.math.pow(f64, 2.0, semis / 12.0);
        const rate = pitch * (self.sample_rate / self.out_sample_rate);
        for (&self.voices) |*v| {
            if (v.state == .idle) {
                v.* = .{ .pos = 0, .rate = rate, .amp = 0, .state = .playing, .note = note };
                return;
            }
        }
        self.voices[0] = .{ .pos = 0, .rate = rate, .amp = 0, .state = .playing, .note = note }; // steal
    }

    pub fn noteOff(self: *Sampler, note: u8) void {
        for (&self.voices) |*v| {
            if (v.state == .playing and v.note == note) v.state = .releasing;
        }
    }

    pub fn renderBlock(self: *Sampler, out: []f32) void {
        const last: f64 = @floatFromInt(self.sample.len);
        for (out) |*o| {
            var acc: f32 = 0.0;
            for (&self.voices) |*v| {
                if (v.state == .idle) continue;

                if (v.state == .playing) {
                    if (v.amp < 1.0) v.amp = @min(1.0, v.amp + self.attack_inc);
                } else {
                    v.amp -= self.release_dec;
                    if (v.amp <= 0.0) {
                        v.state = .idle;
                        continue;
                    }
                }

                acc += resample.hermite(self.sample, v.pos) * v.amp;
                v.pos += v.rate;
                if (v.pos >= last - 1.0) v.state = .idle; // ran off the end of the sample
            }
            o.* = std.math.clamp(acc * self.gain, -1.0, 1.0);
        }
    }
};
