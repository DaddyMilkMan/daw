//! main_play.zig — render the demo phrase and play it live through ALSA.

const std = @import("std");
const synth = @import("synth.zig");
const demo = @import("demo.zig");
const alsa = @import("audio_alsa.zig");

pub fn main() !void {
    const sr: u32 = 48000;
    const srf: f32 = @floatFromInt(sr);

    var s = synth.Synth{ .sample_rate = srf };

    const seconds: f32 = 4.0;
    const total: usize = @intFromFloat(seconds * srf);

    const allocator = std.heap.page_allocator;
    const buf = try allocator.alloc(f32, total);
    defer allocator.free(buf);
    @memset(buf, 0.0);

    demo.renderDemo(&s, buf, sr);

    // f32 [-1,1] -> interleaved 16-bit PCM (mono here)
    const pcm = try allocator.alloc(i16, total);
    defer allocator.free(pcm);
    for (buf, 0..) |v, idx| {
        const c = std.math.clamp(v, -1.0, 1.0);
        pcm[idx] = @intFromFloat(c * 32767.0);
    }

    const device = std.process.getEnvVarOwned(allocator, "ZENITH_PCM") catch null;
    defer if (device) |d| allocator.free(d);

    std.debug.print("Zenith (Zig) -> ALSA playback...\n", .{});
    if (device) |d| {
        const z = try allocator.dupeZ(u8, d);
        defer allocator.free(z);
        try alsa.playInterleavedS16(z, pcm, sr, 1);
    } else {
        try alsa.playInterleavedS16("default", pcm, sr, 1);
    }
    std.debug.print("done.\n", .{});
}
