//! main_loop.zig — Zenith live looper (M3).
//!
//! A looping transport + MIDI recorder. You play notes; they sound live AND get
//! recorded into the loop; on every later pass the loop replays them. Overdub by
//! playing more. The first real "DAW" feature on top of the live instrument.
//!
//!   live:  zig build loop      (opens "Zenith Looper In"; connect a keyboard)
//!   test:  ZENITH_SELFTEST=1 ZENITH_REC=loop.wav ZENITH_SECONDS=8 ./zig-out/bin/zenith_loop
//!
//! Env: ZENITH_SECONDS (default 8), ZENITH_BPM (120), ZENITH_BEATS (4),
//!      ZENITH_REC (wav path), ZENITH_SELFTEST (scripted, offline).

const std = @import("std");
const synth = @import("synth.zig");
const Transport = @import("transport.zig").Transport;
const seqmod = @import("sequence.zig");
const alsa = @import("audio_alsa.zig");
const midi = @import("midi_alsa.zig");
const demo = @import("demo.zig");
const wav = @import("wav.zig");

fn envFlag(a: std.mem.Allocator, name: []const u8) bool {
    if (std.process.getEnvVarOwned(a, name)) |v| {
        a.free(v);
        return true;
    } else |_| return false;
}
fn envFloat(a: std.mem.Allocator, name: []const u8, default: f64) f64 {
    if (std.process.getEnvVarOwned(a, name)) |v| {
        defer a.free(v);
        return std.fmt.parseFloat(f64, v) catch default;
    } else |_| return default;
}

const Scripted = struct { frame: u64, on: bool, note: u8 };

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const srf: f64 = @floatFromInt(sr);
    const block: usize = 256;

    const bpm = envFloat(a, "ZENITH_BPM", 120.0);
    const beats = envFloat(a, "ZENITH_BEATS", 4.0);
    const seconds = envFloat(a, "ZENITH_SECONDS", 8.0);
    const selftest = envFlag(a, "ZENITH_SELFTEST");
    const rec_path = std.process.getEnvVarOwned(a, "ZENITH_REC") catch null;
    defer if (rec_path) |p| a.free(p);

    var s = synth.Synth{ .sample_rate = @floatCast(srf) };
    var tr = Transport.init(srf, bpm, beats);
    var seq = seqmod.Sequence.init(a, tr.length_frames);
    defer seq.deinit();

    const total_frames: u64 = @intFromFloat(seconds * srf);

    // Scripted input (selftest only): one note per beat during loop pass 0.
    const q = tr.length_frames / 4;
    const gate: u64 = if (q > 1500) q - 1500 else q / 2; // note length
    const script = [_]Scripted{
        .{ .frame = 0, .on = true, .note = 60 },         .{ .frame = gate, .on = false, .note = 60 },
        .{ .frame = q, .on = true, .note = 64 },         .{ .frame = q + gate, .on = false, .note = 64 },
        .{ .frame = 2 * q, .on = true, .note = 67 },     .{ .frame = 2 * q + gate, .on = false, .note = 67 },
        .{ .frame = 3 * q, .on = true, .note = 72 },     .{ .frame = 3 * q + gate, .on = false, .note = 72 },
    };
    var script_idx: usize = 0;

    var input: ?midi.MidiInput = null;
    var out: ?alsa.StreamOut = null;
    if (!selftest) {
        input = midi.MidiInput.open("Zenith", "Zenith Looper In") catch |e| {
            std.debug.print("MIDI open failed: {any}\n", .{e});
            return e;
        };
        out = try alsa.StreamOut.open("default", sr, 1, 30_000);
        std.debug.print("Looper: {d:.0} BPM, {d:.0}-beat loop ({d} frames). Connect: aconnect <kbd> {d}:{d}\n", .{ bpm, beats, tr.length_frames, input.?.client, input.?.port });
    } else {
        std.debug.print("Looper self-test: scripted pass-0 notes, {d:.0}s = {d} loops\n", .{ seconds, total_frames / tr.length_frames });
    }
    defer if (input) |*i| i.close();
    defer if (out) |*o| o.close();

    var fbuf: [block]f32 = undefined;
    var ibuf: [block]i16 = undefined;
    var live_ev: [64]midi.MidiEvent = undefined;
    var pb_ev: [256]seqmod.TimedEvent = undefined;

    var rec = std.ArrayList(f32).init(a);
    defer rec.deinit();

    var abs_frame: u64 = 0;
    while (abs_frame < total_frames) {
        const n: usize = @min(block, total_frames - abs_frame);
        const nn: u64 = @intCast(n);
        const loop_pos = tr.pos;
        const loop_idx = abs_frame / tr.length_frames;

        // 1. gather this block's input events
        var in_count: usize = 0;
        if (selftest) {
            if (loop_idx == 0) {
                while (script_idx < script.len and
                    script[script_idx].frame >= loop_pos and
                    script[script_idx].frame < loop_pos + nn) : (script_idx += 1)
                {
                    const sc = script[script_idx];
                    live_ev[in_count] = .{ .kind = if (sc.on) .note_on else .note_off, .note = sc.note, .velocity = 100 };
                    in_count += 1;
                }
            }
        } else {
            in_count = input.?.poll(&live_ev);
        }

        // 2. play live input + record it into the loop (tagged with this pass)
        for (live_ev[0..in_count]) |e| {
            const f = demo.midiToFreq(@floatFromInt(e.note));
            switch (e.kind) {
                .note_on => s.noteOn(f, 1.0),
                .note_off => s.noteOff(f),
            }
            seq.record(loop_pos, loop_idx, e) catch {};
        }

        // 3. replay events recorded in earlier passes
        const pb = seq.collect(loop_pos, nn, loop_idx, &pb_ev);
        for (pb_ev[0..pb]) |te| {
            const f = demo.midiToFreq(@floatFromInt(te.event.note));
            switch (te.event.kind) {
                .note_on => s.noteOn(f, 1.0),
                .note_off => s.noteOff(f),
            }
        }

        // 4. render + output
        s.renderBlock(fbuf[0..n]);
        if (out) |*o| {
            for (fbuf[0..n], 0..) |v, i| ibuf[i] = @intFromFloat(std.math.clamp(v, -1.0, 1.0) * 32767.0);
            try o.writeBlock(ibuf[0..n]);
        }
        if (rec_path != null) try rec.appendSlice(fbuf[0..n]);

        tr.advance(nn);
        abs_frame += nn;
    }

    if (rec_path) |p| {
        try wav.writePcm16(p, rec.items, sr, 1);
        std.debug.print("recorded {d} loops -> {s}\n", .{ total_frames / tr.length_frames, p });
    }
}
