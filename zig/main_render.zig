//! main_render.zig — render the demo phrase to a WAV file. Verifiable anywhere.

const std = @import("std");
const synth = @import("synth.zig");
const demo = @import("demo.zig");
const wav = @import("wav.zig");

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

    try wav.writePcm16("zenith_hello.wav", buf, sr, 1);
    std.debug.print("Zenith (Zig) rendered {d:.1}s -> zenith_hello.wav ({d} samples)\n", .{ seconds, total });
}
