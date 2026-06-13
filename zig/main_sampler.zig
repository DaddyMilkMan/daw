//! main_sampler.zig — M4 demo: generate a sample, write it, read it back, load
//! it into the sampler, and play a pitched phrase from that one sample.

const std = @import("std");
const wav = @import("wav.zig");
const Sampler = @import("sampler.zig").Sampler;

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const srf: f64 = @floatFromInt(sr);
    const tau = 2.0 * std.math.pi;

    // 1. Generate a plucked source tone: A3 (220 Hz, MIDI 57), 0.6s, exp decay.
    const src_len: usize = @intFromFloat(0.6 * srf);
    const src = try a.alloc(f32, src_len);
    defer a.free(src);
    {
        const freq = 220.0;
        var i: usize = 0;
        while (i < src_len) : (i += 1) {
            const t = @as(f64, @floatFromInt(i)) / srf;
            const env = std.math.exp(-5.0 * t);
            const s = std.math.sin(tau * freq * t) * 0.7 +
                std.math.sin(tau * freq * 2.0 * t) * 0.2 +
                std.math.sin(tau * freq * 3.0 * t) * 0.1;
            src[i] = @floatCast(s * env * 0.9);
        }
    }
    try wav.writePcm16("sampler_source.wav", src, sr, 1);
    std.debug.print("wrote sampler_source.wav ({d} frames)\n", .{src_len});

    // 2. Read it back — exercises the WAV reader end to end.
    var loaded = try wav.readPcm(a, "sampler_source.wav");
    defer loaded.deinit(a);
    std.debug.print("read back: {d} ch, {d} Hz, {d} frames\n", .{ loaded.channels, loaded.sample_rate, loaded.frames() });

    // 3. Sampler from the loaded sample (root = A3 = MIDI 57).
    var smp = Sampler.init(loaded.samples, @floatFromInt(loaded.sample_rate), 57.0, srf);

    // 4. Play a phrase, all pitched from that single sample.
    const seconds: f32 = 4.0;
    const total: usize = @intFromFloat(seconds * srf);
    const buf = try a.alloc(f32, total);
    defer a.free(buf);
    @memset(buf, 0.0);

    const notes = [_]u8{ 57, 60, 64, 69 }; // A3 C4 E4 A4 — pitched up from the root
    const step: usize = @intFromFloat(0.4 * srf);
    const block: usize = 256;
    var i: usize = 0;
    var note_i: usize = 0;
    var next: usize = 0;
    var chord = false;
    while (i < total) {
        if (i >= next) {
            if (note_i < notes.len) {
                smp.noteOn(notes[note_i]);
                note_i += 1;
                next = i + step;
            } else if (!chord) {
                smp.noteOn(57);
                smp.noteOn(61);
                smp.noteOn(64); // A major triad
                chord = true;
                next = total;
            }
        }
        const n = @min(block, total - i);
        smp.renderBlock(buf[i .. i + n]);
        i += n;
    }
    try wav.writePcm16("sampler_demo.wav", buf, sr, 1);
    std.debug.print("rendered sampler_demo.wav ({d:.1}s) — one sample, many pitches\n", .{seconds});
}
