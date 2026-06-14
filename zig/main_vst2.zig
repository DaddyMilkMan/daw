//! main_vst2.zig — host a VST2 plugin end-to-end and pull audio through it.
//! Loads the .so, drives the AEffect lifecycle, runs processReplacing(), writes
//! the output to a WAV, and asserts the plugin produced non-silent audio.

const std = @import("std");
const v = @import("vst2_abi.zig");
const host = @import("vst2_host.zig");
const wav = @import("wav.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const a = gpa.allocator();

    var args = std.process.args();
    _ = args.next();
    const path = args.next() orelse "zig-out/lib/libzenith_vst2_test.so";

    const sr: u32 = 48000;
    const block: u32 = 256;

    var plug = try host.Plugin.open(path);
    defer plug.close();

    var nb: [64]u8 = undefined;
    std.debug.print("loaded '{s}': name='{s}' inputs={d} outputs={d} params={d} synth={}\n", .{
        path,
        plug.effectName(&nb),
        plug.effect.numInputs,
        plug.effect.numOutputs,
        plug.effect.numParams,
        (plug.effect.flags & v.effFlagsIsSynth) != 0,
    });

    plug.start(@floatFromInt(sr), @intCast(block));
    defer plug.stop();

    const nch: usize = @intCast(plug.effect.numOutputs);
    const chbufs = try a.alloc([]f32, nch);
    defer {
        for (chbufs) |c| a.free(c);
        a.free(chbufs);
    }
    const chptrs = try a.alloc([*]f32, nch);
    defer a.free(chptrs);
    for (chbufs, 0..) |*cb, i| {
        cb.* = try a.alloc(f32, block);
        chptrs[i] = cb.*.ptr;
    }
    // VST2 always passes valid input/output channel arrays; inputs is empty here.
    var no_inputs = [_][*]f32{};

    var rec = std.ArrayList(f32).init(a);
    defer rec.deinit();

    const total: usize = @intFromFloat(0.5 * @as(f64, @floatFromInt(sr)));
    var done: usize = 0;
    while (done < total) {
        const n: u32 = @intCast(@min(@as(usize, block), total - done));
        plug.process(&no_inputs, chptrs.ptr, @intCast(n));
        try rec.appendSlice(chbufs[0][0..n]);
        done += n;
    }

    var peak: f32 = 0;
    for (rec.items) |s| peak = @max(peak, @abs(s));
    try wav.writePcm16("vst2_demo.wav", rec.items, sr, 1);
    std.debug.print("processed {d} samples through the VST2 plugin -> vst2_demo.wav (peak {d:.3})\n", .{ rec.items.len, peak });

    if (peak < 0.1) return error.SilentOutput;
    std.debug.print("OK: VST2 host loaded, opened, and pulled non-silent audio.\n", .{});
}
