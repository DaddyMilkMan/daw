//! sequence.zig — a recorded MIDI loop. Stores timed events and, on playback,
//! emits the ones recorded in earlier loop passes that fall in the current block.

const std = @import("std");
const midi = @import("midi_alsa.zig");

pub const TimedEvent = struct {
    frame: u64, // loop-relative position
    loop: u64, // which loop pass it was recorded in
    event: midi.MidiEvent,
};

/// Is `frame` inside the half-open window [start, start+n) modulo `length`?
fn inWindow(frame: u64, start: u64, n: u64, length: u64) bool {
    const end = start + n;
    if (end <= length) return frame >= start and frame < end;
    return frame >= start or frame < (end - length); // window wraps the loop point
}

pub const Sequence = struct {
    events: std.ArrayList(TimedEvent),
    length_frames: u64,

    pub fn init(allocator: std.mem.Allocator, length_frames: u64) Sequence {
        return .{ .events = std.ArrayList(TimedEvent).init(allocator), .length_frames = length_frames };
    }
    pub fn deinit(self: *Sequence) void {
        self.events.deinit();
    }

    pub fn record(self: *Sequence, frame: u64, loop: u64, event: midi.MidiEvent) !void {
        try self.events.append(.{ .frame = frame % self.length_frames, .loop = loop, .event = event });
    }

    /// Emit events recorded in loops STRICTLY BEFORE `current_loop` whose frame
    /// falls in this block's window. Returns the number written to `out`.
    pub fn collect(self: *Sequence, start: u64, n: u64, current_loop: u64, out: []TimedEvent) usize {
        var c: usize = 0;
        for (self.events.items) |te| {
            if (c >= out.len) break;
            if (te.loop < current_loop and inWindow(te.frame, start, n, self.length_frames)) {
                out[c] = te;
                c += 1;
            }
        }
        return c;
    }
};

test "inWindow handles wrap" {
    try std.testing.expect(inWindow(5, 0, 10, 100));
    try std.testing.expect(!inWindow(15, 0, 10, 100));
    try std.testing.expect(inWindow(98, 95, 10, 100)); // before wrap
    try std.testing.expect(inWindow(2, 95, 10, 100)); // after wrap
    try std.testing.expect(!inWindow(50, 95, 10, 100));
}
