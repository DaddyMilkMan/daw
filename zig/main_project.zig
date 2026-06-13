//! main_project.zig — M9 demo: build a project, save it, reload it, and play the
//! reloaded notes through the synth. Proves persistence end to end.

const std = @import("std");
const proj = @import("project.zig");
const Synth = @import("synth.zig").Synth;
const demo = @import("demo.zig");
const wav = @import("wav.zig");

pub fn main() !void {
    const a = std.heap.page_allocator;
    const sr: u32 = 48000;
    const srf: f64 = @floatFromInt(sr);

    // 1. Build a project: one synth track with a 6-note melody.
    var p = proj.Project.init(a);
    defer p.deinit();
    p.tempo = 120.0;
    p.sample_rate = sr;
    const t = try p.addTrack("Lead Synth", .synth);
    const c = try t.addClip("melody", 0);
    const beat: u64 = @intFromFloat(0.4 * srf);
    const melody = [_]u8{ 60, 64, 67, 72, 67, 64 }; // C E G C5 G E
    for (melody, 0..) |pitch, i| {
        try c.notes.append(.{ .start = @as(u64, i) * beat, .len = beat - 2000, .pitch = pitch, .velocity = 100 });
    }
    std.debug.print("built project: {d} track(s), {d} notes, {d:.0} BPM\n", .{ p.tracks.items.len, c.notes.items.len, p.tempo });

    // 2. Save.
    try p.save("demo_project.zpr");
    std.debug.print("saved -> demo_project.zpr\n", .{});

    // 3. Reload (fresh Project from disk).
    var q = try proj.Project.load(a, "demo_project.zpr");
    defer q.deinit();
    std.debug.print("reloaded: {d} track(s), {d:.0} BPM, track[0]='{s}'\n", .{ q.tracks.items.len, q.tempo, q.tracks.items[0].name.items });

    // 4. Play the RELOADED project's notes through the synth -> WAV.
    const total: usize = @intFromFloat(3.0 * srf);
    const buf = try a.alloc(f32, total);
    defer a.free(buf);
    @memset(buf, 0.0);

    var syn = Synth{ .sample_rate = @floatCast(srf) };
    const notes = q.tracks.items[0].clips.items[0].notes.items;
    const block: usize = 256;
    var i: usize = 0;
    while (i < total) {
        const n = @min(block, total - i);
        for (notes) |nt| {
            if (nt.start >= i and nt.start < i + n)
                syn.noteOn(demo.midiToFreq(@floatFromInt(nt.pitch)));
            const off = nt.start + nt.len;
            if (off >= i and off < i + n)
                syn.noteOff(demo.midiToFreq(@floatFromInt(nt.pitch)));
        }
        syn.renderBlock(buf[i .. i + n]);
        i += n;
    }
    try wav.writePcm16("project_demo.wav", buf, sr, 1);
    std.debug.print("played reloaded project -> project_demo.wav\n", .{});
}
