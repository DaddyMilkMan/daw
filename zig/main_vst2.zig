//! main_vst2.zig — host a VST2 plugin end-to-end and pull audio through it.
//! Loads the .so, drives the AEffect lifecycle, runs processReplacing(), writes
//! the output to a WAV, and asserts the plugin produced non-silent audio.

const std = @import("std");
const v = @import("vst2_abi.zig");
const host = @import("vst2_host.zig");
const wav = @import("wav.zig");
const dsp = @import("dsp.zig");

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

    // Send a MIDI note-on for A4 (MIDI 69 -> 440 Hz) before processing.
    var midi = std.mem.zeroes(v.VstMidiEvent);
    midi.type = v.kVstMidiType;
    midi.byte_size = 24; // size excluding type+byteSize, per the VST2 convention
    midi.midi_data = .{ 0x90, 69, 100, 0 }; // note-on, ch0, vel 100
    const ev: *v.VstEvent = @ptrCast(&midi);
    var events = v.VstEvents{ .num_events = 1, .reserved = 0, .events = .{ ev, null } };
    plug.sendEvents(&events);

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

    try wav.writePcm16("vst2_demo.wav", rec.items, sr, 1);
    const freq = try dominantFreq(a, rec.items, sr);
    var peak: f32 = 0;
    for (rec.items) |s| peak = @max(peak, @abs(s));
    std.debug.print("instrument played MIDI 69 -> dominant {d:.1} Hz (peak {d:.3}) -> vst2_demo.wav\n", .{ freq, peak });

    // State round-trip via chunks: setChunk(gain=0.42) then getChunk -> confirm.
    {
        var blob: [4]u8 = undefined;
        std.mem.writeInt(u32, &blob, @bitCast(@as(f32, 0.42)), .little);
        plug.setChunk(&blob);
        const got = plug.getChunk();
        const restored: f32 = @bitCast(std.mem.readInt(u32, got[0..4], .little));
        std.debug.print("state: setChunk(gain=0.42) -> getChunk read back {d:.2}\n", .{restored});
        if (@abs(restored - 0.42) > 1e-6) return error.StateRoundTripFailed;
    }

    if (peak < 0.1) return error.SilentOutput;
    if (@abs(freq - 440.0) > 15.0) return error.WrongPitch;
    std.debug.print("OK: VST2 instrument hosted — MIDI note delivered, synthesized 440 Hz, state round-trips.\n", .{});
}

/// Hann-windowed FFT peak -> dominant frequency, from a steady-state slice.
fn dominantFreq(a: std.mem.Allocator, samples: []const f32, sr: u32) !f32 {
    const off: usize = @min(samples.len / 2, 8192);
    const avail = samples.len - off;
    var n: usize = 1;
    while (n * 2 <= avail and n < 8192) n *= 2;
    if (n < 8) return 0;
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
    return @as(f32, @floatFromInt(max_bin)) * @as(f32, @floatFromInt(sr)) / @as(f32, @floatFromInt(n));
}
