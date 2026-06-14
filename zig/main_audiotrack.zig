//! main_audiotrack.zig — end-to-end audio-track + recording verification.
//! 1) Build a 2-track audio timeline (clips placed at frames, panned) and render
//!    it to a stereo WAV; check the right material lands at the right time.
//! 2) Simulate recording: feed captured frames into a Recorder, finalize into a
//!    timeline clip, render it back, and FFT-verify the pitch survived.

const std = @import("std");
const at = @import("audio_track.zig");
const dsp = @import("dsp.zig");
const wav = @import("wav.zig");

fn toneClip(a: std.mem.Allocator, freq: f32, secs: f32, start: u64, sr: u32) !at.AudioClip {
    const n: usize = @intFromFloat(secs * @as(f32, @floatFromInt(sr)));
    const s = try a.alloc(f32, n);
    for (s, 0..) |*v, i| v.* = 0.6 * @sin(2.0 * std.math.pi * freq * @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(sr)));
    return at.AudioClip.fromOwned(a, s, 1, start, sr);
}

fn renderTimeline(a: std.mem.Allocator, tracks: []const at.AudioTrack, total: u64) !struct { l: []f32, r: []f32 } {
    const ln = try a.alloc(f32, @intCast(total));
    const rn = try a.alloc(f32, @intCast(total));
    const block: usize = 512;
    var ph: u64 = 0;
    while (ph < total) {
        const n: usize = @intCast(@min(@as(u64, block), total - ph));
        at.mixTracks(tracks, ln[@intCast(ph)..][0..n], rn[@intCast(ph)..][0..n], ph);
        ph += n;
    }
    return .{ .l = ln, .r = rn };
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const a = gpa.allocator();
    const sr: u32 = 48000;

    // --- 1) multi-track audio timeline ---
    var t1 = at.AudioTrack.init(a);
    defer t1.deinit();
    try t1.name.appendSlice("Drums");
    t1.gain = 1.0;
    try t1.addClip(try toneClip(a, 220, 0.5, 0, sr)); // A3 from frame 0

    var t2 = at.AudioTrack.init(a);
    defer t2.deinit();
    try t2.name.appendSlice("Lead");
    t2.gain = 1.0;
    t2.pan = 0.8; // panned right
    try t2.addClip(try toneClip(a, 880, 0.25, sr / 4, sr)); // A5 enters at 0.25s

    const tracks = [_]at.AudioTrack{ t1, t2 };
    const total = @max(t1.lengthFrames(), t2.lengthFrames());
    const mix = try renderTimeline(a, &tracks, total);
    defer a.free(mix.l);
    defer a.free(mix.r);

    // interleave + write
    const inter = try a.alloc(f32, mix.l.len * 2);
    defer a.free(inter);
    for (0..mix.l.len) |i| {
        inter[i * 2] = mix.l[i];
        inter[i * 2 + 1] = mix.r[i];
    }
    try wav.writeFloat32("audiotrack_demo.wav", inter, sr, 2);

    // before the lead enters (frame < sr/4) the right channel should be ~quiet
    // relative to left (lead is panned right); after, the lead adds 880 Hz.
    const before_l = dominantFreq(a, mix.l[0 .. sr / 4]) catch 0;
    std.debug.print("timeline: {d} frames, 2 tracks; pre-lead dominant {d:.0} Hz (expect ~220)\n", .{ total, before_l * @as(f32, @floatFromInt(sr)) });

    var lead_peak: f32 = 0;
    var pre_peak: f32 = 0;
    for (mix.r[0 .. sr / 4]) |s| pre_peak = @max(pre_peak, @abs(s)); // before lead enters
    for (mix.r[sr / 4 ..]) |s| lead_peak = @max(lead_peak, @abs(s)); // lead active [0.25s, 0.5s)
    std.debug.print("right channel: pre-lead peak {d:.3}, after-lead peak {d:.3}\n", .{ pre_peak, lead_peak });
    if (!(lead_peak > pre_peak + 0.2)) return error.LeadNotPlacedRight;

    // --- 2) recording round-trip ---
    var rec = at.Recorder.init(a, 1, sr);
    defer rec.deinit();
    rec.start_at(0);
    // simulate capturing a 660 Hz input over many blocks
    const block: usize = 256;
    var captured: usize = 0;
    var phase: f32 = 0;
    const inc = 2.0 * std.math.pi * 660.0 / @as(f32, @floatFromInt(sr));
    while (captured < sr / 2) : (captured += block) {
        var blk: [256]f32 = undefined;
        const n = @min(block, sr / 2 - captured);
        for (0..n) |i| {
            blk[i] = 0.5 * @sin(phase);
            phase += inc;
        }
        try rec.feed(blk[0..n]);
    }
    var clip = try rec.finish();
    // place the recorded take on a fresh track and render it
    var t3 = at.AudioTrack.init(a);
    defer t3.deinit();
    t3.gain = 1.0;
    try t3.addClip(clip);
    const take = try renderTimeline(a, &[_]at.AudioTrack{t3}, t3.lengthFrames());
    defer a.free(take.l);
    defer a.free(take.r);
    const got = (try dominantFreq(a, take.l)) * @as(f32, @floatFromInt(sr));
    std.debug.print("recording: captured {d} frames -> clip -> rendered; dominant {d:.0} Hz (expect ~660)\n", .{ clip.frames(), got });
    if (@abs(got - 660.0) > 20.0) return error.RecordingPitchWrong;

    std.debug.print("OK: audio tracks place clips sample-accurately + recording round-trips to the timeline.\n", .{});
}

/// Dominant frequency as a fraction of sample-rate (Hann-windowed FFT peak).
fn dominantFreq(a: std.mem.Allocator, samples: []const f32) !f32 {
    var n: usize = 1;
    while (n * 2 <= samples.len and n < 8192) n *= 2;
    if (n < 8) return 0;
    const off = (samples.len - n) / 2;
    const buf = try a.alloc(dsp.Complex, n);
    defer a.free(buf);
    for (buf, 0..) |*c, k| {
        const w = 0.5 - 0.5 * @cos(2.0 * std.math.pi * @as(f32, @floatFromInt(k)) / @as(f32, @floatFromInt(n - 1)));
        c.* = .{ .re = samples[off + k] * w, .im = 0 };
    }
    dsp.fft(buf, false);
    var max_bin: usize = 1;
    var max_mag: f32 = 0;
    for (buf[1 .. n / 2], 1..) |c, k| {
        const m = c.mag();
        if (m > max_mag) {
            max_mag = m;
            max_bin = k;
        }
    }
    return @as(f32, @floatFromInt(max_bin)) / @as(f32, @floatFromInt(n));
}
