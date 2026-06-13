//! main_arrange.zig — arrangement demo: a multi-clip, multi-track song timeline,
//! saved, reloaded, and played back linearly through per-track synths into a
//! stereo mix. Shows clips at timeline positions + record-to-clip.

const std = @import("std");
const project = @import("project.zig");
const arrange = @import("arrangement.zig");
const Synth = @import("synth.zig").Synth;
const mix = @import("mixer.zig");
const demo = @import("demo.zig");
const wav = @import("wav.zig");

fn buildSong(p: *project.Project, beat: u64) !void {
    // Track 0 — chords: two clips back to back on the timeline.
    const chords = try p.addTrack("Chords", .synth);
    chords.pan = -0.25;
    {
        const c1 = try chords.addClip("Cmaj", 0);
        for ([_]u8{ 60, 64, 67 }) |nn| try c1.notes.append(.{ .start = 0, .len = 2 * beat - 2000, .pitch = nn, .velocity = 85 });
        const c2 = try chords.addClip("Fmaj", 2 * beat);
        for ([_]u8{ 65, 69, 72 }) |nn| try c2.notes.append(.{ .start = 0, .len = 2 * beat - 2000, .pitch = nn, .velocity = 85 });
    }

    // Track 1 — lead: a clip whose notes are added via the record-to-clip path.
    const lead = try p.addTrack("Lead", .synth);
    lead.pan = 0.3;
    {
        const cs = beat; // clip starts on beat 2
        const clip = try lead.addClip("riff", cs);
        // simulate captured performance (absolute frame times -> clip-relative)
        try arrange.recordNote(clip, cs, beat + 0, beat + beat / 2, 72, 100);
        try arrange.recordNote(clip, cs, beat + beat, beat + beat + beat / 2, 71, 100);
        try arrange.recordNote(clip, cs, beat + 2 * beat, beat + 2 * beat + beat / 2, 67, 100);
    }
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const srf: f64 = @floatFromInt(sr);
    const beat: u64 = @intFromFloat(0.5 * srf);

    // Build -> save -> reload (play the persisted arrangement).
    var p = project.Project.init(a);
    defer p.deinit();
    p.sample_rate = sr;
    try buildSong(&p, beat);
    try p.save("song.zpr");

    var q = try project.Project.load(a, "song.zpr");
    defer q.deinit();
    const ntracks = q.tracks.items.len;
    std.debug.print("arrangement: {d} tracks, length {d} frames ({d:.1}s)\n", .{ ntracks, arrange.lengthFrames(&q), @as(f64, @floatFromInt(arrange.lengthFrames(&q))) / srf });

    const tail: u64 = @intFromFloat(0.6 * srf);
    const total: usize = @intCast(arrange.lengthFrames(&q) + tail);

    var synths: [8]Synth = undefined;
    var bufs: [8][]f32 = undefined;
    for (0..ntracks) |i| {
        synths[i] = Synth{ .sample_rate = @floatCast(srf) };
        bufs[i] = try a.alloc(f32, total);
        @memset(bufs[i], 0.0);
    }
    defer for (0..ntracks) |i| a.free(bufs[i]);

    const block: usize = 256;
    var evbuf: [64]arrange.ScheduledEvent = undefined;
    var pos: usize = 0;
    while (pos < total) {
        const n = @min(block, total - pos);
        const cnt = arrange.collectEvents(&q, @intCast(pos), @intCast(n), &evbuf);
        for (evbuf[0..cnt]) |ev| {
            const f = demo.midiToFreq(@floatFromInt(ev.pitch));
            if (ev.on) synths[ev.track].noteOn(f) else synths[ev.track].noteOff(f);
        }
        for (0..ntracks) |i| synths[i].renderBlock(bufs[i][pos .. pos + n]);
        pos += n;
    }

    // Mix tracks -> stereo (using each track's saved gain/pan).
    var mixer = mix.Mixer{ .master_gain = 0.85 };
    var tracks: [8]mix.Track = undefined;
    var monos: [8][]const f32 = undefined;
    for (0..ntracks) |i| {
        tracks[i] = .{ .gain = q.tracks.items[i].gain, .pan = q.tracks.items[i].pan };
        monos[i] = bufs[i];
    }
    const out = try a.alloc(f32, total * 2);
    defer a.free(out);
    mixer.mixToStereo(tracks[0..ntracks], monos[0..ntracks], out);

    try wav.writePcm16("arrangement_demo.wav", out, sr, 2);
    std.debug.print("rendered arrangement_demo.wav (stereo, {d:.1}s)\n", .{@as(f64, @floatFromInt(total)) / srf});
}
