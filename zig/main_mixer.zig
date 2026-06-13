//! main_mixer.zig — M6 demo: synth and sampler on two tracks, panned and mixed
//! into a stereo master. The first time two instruments play together in Zenith.

const std = @import("std");
const Synth = @import("synth.zig").Synth;
const Sampler = @import("sampler.zig").Sampler;
const demo = @import("demo.zig");
const mix = @import("mixer.zig");
const wav = @import("wav.zig");

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const srf: f64 = @floatFromInt(sr);
    const tau = 2.0 * std.math.pi;
    const seconds: f32 = 4.0;
    const frames: usize = @intFromFloat(seconds * srf);
    const blk: usize = 256;

    // Track 0 — synth phrase (the demo arpeggio + chord).
    var syn = Synth{ .sample_rate = @floatCast(srf) };
    const buf0 = try a.alloc(f32, frames);
    defer a.free(buf0);
    @memset(buf0, 0.0);
    demo.renderDemo(&syn, buf0, sr);

    // Track 1 — sampler playing a descending line from a generated sample.
    const src_len: usize = @intFromFloat(0.5 * srf);
    const src = try a.alloc(f32, src_len);
    defer a.free(src);
    {
        var i: usize = 0;
        while (i < src_len) : (i += 1) {
            const t = @as(f64, @floatFromInt(i)) / srf;
            src[i] = @floatCast(std.math.sin(tau * 330.0 * t) * std.math.exp(-6.0 * t) * 0.9);
        }
    }
    var smp = Sampler.init(src, srf, 64.0, srf); // root E4 = MIDI 64 = 330 Hz
    const buf1 = try a.alloc(f32, frames);
    defer a.free(buf1);
    @memset(buf1, 0.0);
    {
        const seq = [_]u8{ 72, 71, 69, 67, 72, 71, 69, 67 };
        const step: usize = @intFromFloat(0.5 * srf);
        var i: usize = 0;
        var ni: usize = 0;
        var nx: usize = @intFromFloat(0.25 * srf);
        while (i < frames) {
            if (i >= nx and ni < seq.len) {
                smp.noteOn(seq[ni]);
                ni += 1;
                nx += step;
            }
            const n = @min(blk, frames - i);
            smp.renderBlock(buf1[i .. i + n]);
            i += n;
        }
    }

    // Mix: synth left, sampler right, into a stereo master.
    var mixer = mix.Mixer{ .master_gain = 0.9 };
    const tracks = [_]mix.Track{
        .{ .gain = 0.8, .pan = -0.5 }, // synth
        .{ .gain = 0.9, .pan = 0.5 }, // sampler
    };
    const bufs = [_][]const f32{ buf0, buf1 };
    const out = try a.alloc(f32, frames * 2);
    defer a.free(out);
    mixer.mixToStereo(&tracks, &bufs, out);

    try wav.writePcm16("mixer_demo.wav", out, sr, 2);
    std.debug.print("mixed 2 tracks (synth L / sampler R) -> mixer_demo.wav (stereo)\n", .{});
}
