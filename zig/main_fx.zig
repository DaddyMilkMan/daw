//! main_fx.zig — M(effects) demo: run the synth through EQ + delay + reverb, and
//! emit impulse responses for delay and reverb so the effects can be verified.

const std = @import("std");
const fx = @import("effects.zig");
const Synth = @import("synth.zig").Synth;
const demo = @import("demo.zig");
const wav = @import("wav.zig");

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const srf: f32 = @floatFromInt(sr);

    // --- synth phrase through EQ -> delay -> reverb ---
    const total: usize = @intFromFloat(5.0 * srf);
    const buf = try a.alloc(f32, total);
    defer a.free(buf);
    @memset(buf, 0.0);
    var syn = Synth{ .sample_rate = srf };
    demo.renderDemo(&syn, buf, sr);

    var reverb = try fx.Reverb.init(a, 0.7, 0.25);
    defer reverb.deinit();
    var delay = try fx.Delay.init(a, @intFromFloat(0.20 * srf));
    defer delay.deinit(a);
    delay.feedback = 0.45;
    delay.mix = 0.3;
    var eq = fx.Biquad.peaking(srf, 3000, 1.0, 4.0);

    for (buf) |*s| {
        var x = eq.process(s.*);
        x = delay.process(x);
        x = reverb.process(x);
        s.* = std.math.clamp(x, -1.0, 1.0);
    }
    try wav.writePcm16("fx_demo.wav", buf, sr, 1);

    // --- impulse -> delay (verify echoes) ---
    {
        var d = try fx.Delay.init(a, @intFromFloat(0.15 * srf));
        defer d.deinit(a);
        d.feedback = 0.6;
        d.mix = 1.0;
        const n: usize = @intFromFloat(2.0 * srf);
        const ib = try a.alloc(f32, n);
        defer a.free(ib);
        @memset(ib, 0.0);
        ib[0] = 1.0;
        for (ib) |*s| s.* = d.process(s.*);
        try wav.writePcm16("impulse_delay.wav", ib, sr, 1);
    }

    // --- impulse -> reverb (verify tail) ---
    {
        var rv = try fx.Reverb.init(a, 0.85, 0.2);
        defer rv.deinit();
        rv.mix = 1.0;
        const n: usize = @intFromFloat(3.0 * srf);
        const ib = try a.alloc(f32, n);
        defer a.free(ib);
        @memset(ib, 0.0);
        ib[0] = 1.0;
        for (ib) |*s| s.* = rv.process(s.*);
        try wav.writePcm16("impulse_reverb.wav", ib, sr, 1);
    }

    std.debug.print("fx: synth->EQ->delay->reverb -> fx_demo.wav; + impulse_delay.wav, impulse_reverb.wav\n", .{});
}
