//! main_live.zig — real-time, MIDI-driven Zenith.
//!
//!   live mode (default): open an ALSA "Zenith" MIDI input port + stream audio.
//!     Connect a keyboard (aconnect) and play. Optional ZENITH_REC=path records.
//!   self-test (ZENITH_SELFTEST=1): inject a scripted note sequence through the
//!     SAME synth path and render to ZENITH_REC (no device/keyboard needed).
//!
//! Env: ZENITH_SECONDS (default 20), ZENITH_REC (wav path), ZENITH_PCM (device),
//!      ZENITH_SELFTEST (deterministic offline render).

const std = @import("std");
const synth = @import("synth.zig");
const alsa = @import("audio_alsa.zig");
const midi = @import("midi_alsa.zig");
const demo = @import("demo.zig");
const wav = @import("wav.zig");

const Scripted = struct { at: usize, on: bool, note: u8 };

fn envFlag(allocator: std.mem.Allocator, name: []const u8) bool {
    if (std.process.getEnvVarOwned(allocator, name)) |v| {
        allocator.free(v);
        return true;
    } else |_| return false;
}

pub fn main() !void {
    const allocator = std.heap.page_allocator;
    const sr: u32 = 48000;
    const block: usize = 256;

    var s = synth.Synth{ .sample_rate = @floatFromInt(sr) };

    const seconds: f32 = blk: {
        if (std.process.getEnvVarOwned(allocator, "ZENITH_SECONDS")) |v| {
            defer allocator.free(v);
            break :blk std.fmt.parseFloat(f32, v) catch 20.0;
        } else |_| break :blk 20.0;
    };
    const selftest = envFlag(allocator, "ZENITH_SELFTEST");
    const rec_path = std.process.getEnvVarOwned(allocator, "ZENITH_REC") catch null;
    defer if (rec_path) |p| allocator.free(p);

    const total_blocks: usize = @intFromFloat(@as(f32, @floatFromInt(sr)) * seconds / @as(f32, @floatFromInt(block)));

    var fbuf: [block]f32 = undefined;
    var ibuf: [block]i16 = undefined;
    var events: [64]midi.MidiEvent = undefined;

    var rec = std.ArrayList(f32).init(allocator);
    defer rec.deinit();

    // Scripted phrase for self-test (block indices): C, E, G, then a C-major chord.
    const script = [_]Scripted{
        .{ .at = 2, .on = true, .note = 60 },  .{ .at = 28, .on = false, .note = 60 },
        .{ .at = 32, .on = true, .note = 64 }, .{ .at = 58, .on = false, .note = 64 },
        .{ .at = 62, .on = true, .note = 67 }, .{ .at = 88, .on = false, .note = 67 },
        .{ .at = 92, .on = true, .note = 60 }, .{ .at = 92, .on = true, .note = 64 },
        .{ .at = 92, .on = true, .note = 67 },
    };

    // --- self-test: deterministic offline render, no device/keyboard ---
    if (selftest) {
        std.debug.print("Zenith self-test: scripted MIDI -> synth -> {s}\n", .{rec_path orelse "(no ZENITH_REC set)"});
        var b: usize = 0;
        while (b < total_blocks) : (b += 1) {
            for (script) |sc| {
                if (sc.at == b) {
                    const f = demo.midiToFreq(@floatFromInt(sc.note));
                    if (sc.on) s.noteOn(f, 1.0) else s.noteOff(f);
                }
            }
            s.renderBlock(&fbuf);
            if (rec_path != null) try rec.appendSlice(&fbuf);
        }
        if (rec_path) |p| {
            try wav.writePcm16(p, rec.items, sr, 1);
            std.debug.print("self-test recorded {d} samples -> {s}\n", .{ rec.items.len, p });
        }
        return;
    }

    // --- live: ALSA seq MIDI in + streaming audio out ---
    var input = midi.MidiInput.open("Zenith", "Zenith MIDI In") catch |e| {
        std.debug.print("MIDI open failed: {any}\n", .{e});
        return e;
    };
    defer input.close();
    std.debug.print("Zenith MIDI in ready — client {d}, port {d}. Connect with:  aconnect <src> {d}:{d}\n", .{ input.client, input.port, input.client, input.port });

    const dev_name = std.process.getEnvVarOwned(allocator, "ZENITH_PCM") catch null;
    defer if (dev_name) |d| allocator.free(d);
    const devZ = try allocator.dupeZ(u8, dev_name orelse "default");
    defer allocator.free(devZ);

    var out = try alsa.StreamOut.open(devZ.ptr, sr, 1, 30_000);
    defer out.close();
    std.debug.print("streaming audio for {d:.0}s...\n", .{seconds});

    var b: usize = 0;
    while (b < total_blocks) : (b += 1) {
        const nev = input.poll(&events);
        for (events[0..nev]) |e| {
            const f = demo.midiToFreq(@floatFromInt(e.note));
            switch (e.kind) {
                .note_on => s.noteOn(f, 1.0),
                .note_off => s.noteOff(f),
                else => {},
            }
        }
        s.renderBlock(&fbuf);
        for (fbuf, 0..) |v, i| ibuf[i] = @intFromFloat(std.math.clamp(v, -1.0, 1.0) * 32767.0);
        try out.writeBlock(&ibuf);
        if (rec_path != null) try rec.appendSlice(&fbuf);
    }

    if (rec_path) |p| {
        try wav.writePcm16(p, rec.items, sr, 1);
        std.debug.print("recorded -> {s}\n", .{p});
    }
}
