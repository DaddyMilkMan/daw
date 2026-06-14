//! uireg.zig — a global registry of interactive widget rects, keyed by id.
//!
//! trellis.zig and widgets.zig publish every interactive node's (id, rect) here each
//! frame (upsert). It's the "accessibility tree" the Talkback harness resolves
//! selectors against: Talkback (window_glx) looks up `center(id)` to
//! drive a widget by id instead of by pixel, and `dump()` writes the whole map so
//! a script author can discover ids. Process-global + lock-free-ish (single UI
//! thread); intentionally tiny.

const std = @import("std");

const MAX = 2048;
const Entry = struct { id: u64, x: f32, y: f32, w: f32, h: f32 };
var g: [MAX]Entry = undefined;
var n: usize = 0;

/// Upsert a widget's rect. Stable ids overwrite in place each frame.
pub fn put(id: u64, x: f32, y: f32, w: f32, h: f32) void {
    if (id == 0) return;
    var i: usize = 0;
    while (i < n) : (i += 1) {
        if (g[i].id == id) {
            g[i] = .{ .id = id, .x = x, .y = y, .w = w, .h = h };
            return;
        }
    }
    if (n < MAX) {
        g[n] = .{ .id = id, .x = x, .y = y, .w = w, .h = h };
        n += 1;
    }
}

pub fn find(id: u64) ?[4]f32 {
    var i: usize = 0;
    while (i < n) : (i += 1) {
        if (g[i].id == id) return .{ g[i].x, g[i].y, g[i].w, g[i].h };
    }
    return null;
}

/// Center point of a widget's rect (for click/drag targeting).
pub fn center(id: u64) ?[2]f32 {
    if (find(id)) |r| return .{ r[0] + r[2] / 2, r[1] + r[3] / 2 };
    return null;
}

pub fn count() usize {
    return n;
}

/// Dump the registry (id \t x \t y \t w \t h) — the discoverable element map.
pub fn dump(path: []const u8) void {
    const f = std.fs.cwd().createFile(path, .{}) catch return;
    defer f.close();
    var bw = std.io.bufferedWriter(f.writer());
    const w = bw.writer();
    w.print("# id\tx\ty\tw\th  ({d} interactive widgets)\n", .{n}) catch {};
    var i: usize = 0;
    while (i < n) : (i += 1) {
        w.print("{d}\t{d:.0}\t{d:.0}\t{d:.0}\t{d:.0}\n", .{ g[i].id, g[i].x, g[i].y, g[i].w, g[i].h }) catch {};
    }
    bw.flush() catch {};
}

test "registry upserts and centers" {
    n = 0;
    put(7, 100, 200, 40, 20);
    put(7, 110, 210, 40, 20); // upsert (same id)
    try std.testing.expectEqual(@as(usize, 1), count());
    const c = center(7).?;
    try std.testing.expectEqual(@as(f32, 130), c[0]);
    try std.testing.expectEqual(@as(f32, 220), c[1]);
    try std.testing.expect(find(999) == null);
}
