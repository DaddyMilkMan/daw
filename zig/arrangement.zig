//! arrangement.zig — linear (song) timeline playback over the project model.
//! Schedules each clip's notes at their absolute timeline positions, across all
//! tracks. Also a record-to-clip helper that turns captured input into a clip.

const std = @import("std");
const project = @import("project.zig");
const midi = @import("midi_alsa.zig");

pub const ScheduledEvent = struct {
    track: usize,
    on: bool,
    pitch: u8,
    velocity: u8,
};

/// Collect note on/off events from all clips/tracks that fall in [start, start+n).
/// Linear timeline (no looping). Returns the count written to `out`.
pub fn collectEvents(p: *const project.Project, start: u64, n: u64, out: []ScheduledEvent) usize {
    var c: usize = 0;
    for (p.tracks.items, 0..) |t, ti| {
        if (t.mute) continue;
        for (t.clips.items) |clip| {
            for (clip.notes.items) |note| {
                const on_at = clip.start + note.start;
                const off_at = on_at + note.len;
                if (on_at >= start and on_at < start + n and c < out.len) {
                    out[c] = .{ .track = ti, .on = true, .pitch = note.pitch, .velocity = note.velocity };
                    c += 1;
                }
                if (off_at >= start and off_at < start + n and c < out.len) {
                    out[c] = .{ .track = ti, .on = false, .pitch = note.pitch, .velocity = note.velocity };
                    c += 1;
                }
            }
        }
    }
    return c;
}

/// Total timeline length in frames (end of the last clip's last note).
pub fn lengthFrames(p: *const project.Project) u64 {
    var end: u64 = 0;
    for (p.tracks.items) |t| {
        for (t.clips.items) |clip| {
            for (clip.notes.items) |note| {
                const e = clip.start + note.start + note.len;
                if (e > end) end = e;
            }
        }
    }
    return end;
}

/// Helper for record-to-clip: append a captured note into a clip, clip-relative.
pub fn recordNote(clip: *project.Clip, clip_start: u64, abs_on: u64, abs_off: u64, pitch: u8, velocity: u8) !void {
    const start = if (abs_on > clip_start) abs_on - clip_start else 0;
    const len = if (abs_off > abs_on) abs_off - abs_on else 0;
    try clip.notes.append(.{ .start = start, .len = len, .pitch = pitch, .velocity = velocity });
}

test "collectEvents schedules clips at their timeline positions" {
    const a = std.testing.allocator;
    var p = project.Project.init(a);
    defer p.deinit();
    const t = try p.addTrack("T", .synth);
    const c = try t.addClip("c", 1000); // clip starts at frame 1000
    try c.notes.append(.{ .start = 0, .len = 500, .pitch = 60, .velocity = 100 }); // abs on=1000 off=1500

    var buf: [8]ScheduledEvent = undefined;
    // block [0,512): nothing
    try std.testing.expectEqual(@as(usize, 0), collectEvents(&p, 0, 512, &buf));
    // block [768,1280): the note-on at 1000
    const n1 = collectEvents(&p, 768, 512, &buf);
    try std.testing.expectEqual(@as(usize, 1), n1);
    try std.testing.expect(buf[0].on and buf[0].pitch == 60);
    // block [1280,1792): the note-off at 1500
    const n2 = collectEvents(&p, 1280, 512, &buf);
    try std.testing.expectEqual(@as(usize, 1), n2);
    try std.testing.expect(!buf[0].on);
    try std.testing.expectEqual(@as(u64, 1500), lengthFrames(&p));
}
