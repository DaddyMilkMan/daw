//! automation.zig — parameter automation lanes.
//!
//! A Lane is a time-ordered list of breakpoints (frame, value). `valueAt` reads
//! the curve at any frame (binary search + hold/linear interpolation), so the
//! engine can drive a track gain/pan or a plugin param sample-accurately. Pure,
//! allocation-light, fully testable offline.

const std = @import("std");

pub const Interp = enum { hold, linear };

pub const Point = struct { frame: u64, value: f32 };

pub const Lane = struct {
    points: std.ArrayList(Point),
    interp: Interp = .linear,
    default: f32 = 0.0, // value when the lane is empty

    pub fn init(a: std.mem.Allocator) Lane {
        return .{ .points = std.ArrayList(Point).init(a) };
    }
    pub fn deinit(self: *Lane) void {
        self.points.deinit();
    }
    pub fn isEmpty(self: Lane) bool {
        return self.points.items.len == 0;
    }

    /// Insert a breakpoint, keeping the lane sorted by frame (replaces an exact
    /// frame match so editing a point in place is idempotent).
    pub fn add(self: *Lane, frame: u64, value: f32) !void {
        var i: usize = 0;
        while (i < self.points.items.len and self.points.items[i].frame < frame) : (i += 1) {}
        if (i < self.points.items.len and self.points.items[i].frame == frame) {
            self.points.items[i].value = value;
            return;
        }
        try self.points.insert(i, .{ .frame = frame, .value = value });
    }

    /// Read the automated value at `frame`. Clamps to the first/last point
    /// outside the lane's range.
    pub fn valueAt(self: Lane, frame: u64) f32 {
        const pts = self.points.items;
        if (pts.len == 0) return self.default;
        if (frame <= pts[0].frame) return pts[0].value;
        if (frame >= pts[pts.len - 1].frame) return pts[pts.len - 1].value;

        // binary search for the last point with frame <= target
        var lo: usize = 0;
        var hi: usize = pts.len - 1;
        while (lo + 1 < hi) {
            const mid = (lo + hi) / 2;
            if (pts[mid].frame <= frame) lo = mid else hi = mid;
        }
        const a = pts[lo];
        const b = pts[lo + 1];
        if (self.interp == .hold) return a.value;
        const span = b.frame - a.frame;
        if (span == 0) return a.value;
        const t = @as(f32, @floatFromInt(frame - a.frame)) / @as(f32, @floatFromInt(span));
        return a.value + (b.value - a.value) * t;
    }

    /// Fill `out` with the curve over [start_frame, start_frame+out.len) — the
    /// sample-accurate way to apply automation across a render block.
    pub fn render(self: Lane, out: []f32, start_frame: u64) void {
        for (out, 0..) |*v, i| v.* = self.valueAt(start_frame + i);
    }
};

// ===========================================================================
// Tests
// ===========================================================================
const expectApproxEqAbs = std.testing.expectApproxEqAbs;

test "linear interpolation between breakpoints, clamped at the ends" {
    const a = std.testing.allocator;
    var lane = Lane.init(a);
    defer lane.deinit();
    try lane.add(0, 0.0);
    try lane.add(100, 1.0);
    try expectApproxEqAbs(@as(f32, 0.0), lane.valueAt(0), 1e-6);
    try expectApproxEqAbs(@as(f32, 0.5), lane.valueAt(50), 1e-6);
    try expectApproxEqAbs(@as(f32, 0.25), lane.valueAt(25), 1e-6);
    try expectApproxEqAbs(@as(f32, 1.0), lane.valueAt(100), 1e-6);
    try expectApproxEqAbs(@as(f32, 1.0), lane.valueAt(500), 1e-6); // past the end
}

test "hold mode steps at each breakpoint" {
    const a = std.testing.allocator;
    var lane = Lane.init(a);
    defer lane.deinit();
    lane.interp = .hold;
    try lane.add(0, 0.2);
    try lane.add(100, 0.8);
    try expectApproxEqAbs(@as(f32, 0.2), lane.valueAt(50), 1e-6);
    try expectApproxEqAbs(@as(f32, 0.2), lane.valueAt(99), 1e-6);
    try expectApproxEqAbs(@as(f32, 0.8), lane.valueAt(100), 1e-6);
}

test "out-of-order inserts stay sorted; exact-frame replaces" {
    const a = std.testing.allocator;
    var lane = Lane.init(a);
    defer lane.deinit();
    try lane.add(100, 1.0);
    try lane.add(0, 0.0);
    try lane.add(50, 0.5);
    try lane.add(50, 0.6); // replace
    try std.testing.expectEqual(@as(usize, 3), lane.points.items.len);
    try std.testing.expectEqual(@as(u64, 0), lane.points.items[0].frame);
    try std.testing.expectEqual(@as(u64, 50), lane.points.items[1].frame);
    try expectApproxEqAbs(@as(f32, 0.6), lane.points.items[1].value, 1e-6);
}

test "empty lane returns its default" {
    const a = std.testing.allocator;
    var lane = Lane.init(a);
    defer lane.deinit();
    lane.default = 0.8;
    try expectApproxEqAbs(@as(f32, 0.8), lane.valueAt(1234), 1e-6);
}

test "render applies a gain ramp sample-accurately" {
    const a = std.testing.allocator;
    var lane = Lane.init(a);
    defer lane.deinit();
    try lane.add(0, 0.0);
    try lane.add(1000, 1.0); // fade in over 1000 frames

    var sig = [_]f32{1.0} ** 1000; // constant signal
    var env: [1000]f32 = undefined;
    lane.render(&env, 0);
    for (&sig, env) |*s, g| s.* *= g; // apply automation

    try expectApproxEqAbs(@as(f32, 0.0), sig[0], 1e-6);
    try expectApproxEqAbs(@as(f32, 0.5), sig[500], 1e-3);
    try expectApproxEqAbs(@as(f32, 0.999), sig[999], 1e-3);
}
