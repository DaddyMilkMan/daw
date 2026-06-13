//! icons.zig — crisp vector icons drawn with the renderer primitives.

const r2d = @import("render2d.zig");
const Canvas = r2d.Canvas;
const Color = r2d.Color;

fn ff(x: i32) f32 {
    return @floatFromInt(x);
}

pub fn play(cv: *Canvas, cx: i32, cy: i32, s: i32, c: Color) void {
    const h = ff(s);
    cv.fillTriangle(ff(cx) - h * 0.42, ff(cy) - h * 0.55, ff(cx) - h * 0.42, ff(cy) + h * 0.55, ff(cx) + h * 0.58, ff(cy), c);
}

pub fn pause(cv: *Canvas, cx: i32, cy: i32, s: i32, c: Color) void {
    const bw = @divTrunc(s, 3);
    cv.fillRoundedRect(cx - @divTrunc(s, 2), cy - @divTrunc(s, 2), bw, s, 2, c);
    cv.fillRoundedRect(cx + @divTrunc(s, 2) - bw, cy - @divTrunc(s, 2), bw, s, 2, c);
}

pub fn stop(cv: *Canvas, cx: i32, cy: i32, s: i32, c: Color) void {
    cv.fillRoundedRect(cx - @divTrunc(s, 2), cy - @divTrunc(s, 2), s, s, 2, c);
}

pub fn record(cv: *Canvas, cx: i32, cy: i32, s: i32, c: Color) void {
    cv.fillRoundedRect(cx - @divTrunc(s, 2), cy - @divTrunc(s, 2), s, s, @divTrunc(s, 2), c);
}

pub fn minimize(cv: *Canvas, cx: i32, cy: i32, s: i32, c: Color) void {
    cv.line(ff(cx) - ff(s) * 0.5, ff(cy) + ff(s) * 0.4, ff(cx) + ff(s) * 0.5, ff(cy) + ff(s) * 0.4, 1.6, c);
}

pub fn maximize(cv: *Canvas, cx: i32, cy: i32, s: i32, c: Color) void {
    const half = @divTrunc(s, 2);
    const x = cx - half;
    const y = cy - half;
    cv.line(ff(x), ff(y), ff(x + s), ff(y), 1.6, c);
    cv.line(ff(x), ff(y + s), ff(x + s), ff(y + s), 1.6, c);
    cv.line(ff(x), ff(y), ff(x), ff(y + s), 1.6, c);
    cv.line(ff(x + s), ff(y), ff(x + s), ff(y + s), 1.6, c);
}

pub fn close(cv: *Canvas, cx: i32, cy: i32, s: i32, c: Color) void {
    const h = ff(s) * 0.5;
    cv.line(ff(cx) - h, ff(cy) - h, ff(cx) + h, ff(cy) + h, 1.7, c);
    cv.line(ff(cx) - h, ff(cy) + h, ff(cx) + h, ff(cy) - h, 1.7, c);
}
