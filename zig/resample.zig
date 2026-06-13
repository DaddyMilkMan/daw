//! resample.zig — fractional-position sample reading via 4-point cubic
//! (Catmull-Rom) interpolation. Used by the sampler for pitch-shifted playback.

const std = @import("std");

/// Interpolated value of `buf` at fractional index `pos` (clamped at the edges).
pub fn hermite(buf: []const f32, pos: f64) f32 {
    const n = buf.len;
    if (n == 0) return 0.0;

    const i: usize = @intFromFloat(@floor(pos));
    const frac: f32 = @floatCast(pos - @floor(pos));

    const x0 = buf[if (i > 0) i - 1 else 0];
    const x1 = buf[@min(i, n - 1)];
    const x2 = buf[@min(i + 1, n - 1)];
    const x3 = buf[@min(i + 2, n - 1)];

    // Catmull-Rom
    const c0 = x1;
    const c1 = 0.5 * (x2 - x0);
    const c2 = x0 - 2.5 * x1 + 2.0 * x2 - 0.5 * x3;
    const c3 = 0.5 * (x3 - x0) + 1.5 * (x1 - x2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

test "hermite passes through integer sample points" {
    const buf = [_]f32{ 0.0, 1.0, 0.0, -1.0, 0.0 };
    try std.testing.expectApproxEqAbs(@as(f32, 1.0), hermite(&buf, 1.0), 1e-6);
    try std.testing.expectApproxEqAbs(@as(f32, -1.0), hermite(&buf, 3.0), 1e-6);
    // midpoint is between the two neighbours (no overshoot beyond ~[-1,1] here)
    const mid = hermite(&buf, 1.5);
    try std.testing.expect(mid > 0.0 and mid < 1.2);
}
