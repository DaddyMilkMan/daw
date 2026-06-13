//! main_glwin.zig — present a frame through the OpenGL backend (GPU). Renders a
//! glass scene to the software canvas, then blits it via a GL texture quad.

const std = @import("std");
const r2d = @import("render2d.zig");
const gl = @import("window_gl.zig");
const Color = r2d.Color;

const fb = &@import("font_body.zig").font;
const fd = &@import("font_display.zig").font;

fn glass(cv: *r2d.Canvas, x: i32, y: i32, w: i32, h: i32, radius: i32, ta: u8) void {
    cv.boxBlur(x - 2, y - 2, w + 4, h + 4, 11);
    cv.fillRoundedRect(x, y, w, h, radius, .{ .r = 255, .g = 255, .b = 255, .a = ta });
    cv.fillRoundedRect(x, y, w, 2, radius, .{ .r = 255, .g = 255, .b = 255, .a = 60 });
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 860;
    const H: usize = 500;
    const Wi: i32 = @intCast(W);
    const Hi: i32 = @intCast(H);

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();

    cv.vGradient(0, 0, Wi, Hi, Color.rgb(46, 32, 78), Color.rgb(18, 26, 54));
    cv.fillRoundedRect(120, 70, 240, 240, 120, .{ .r = 86, .g = 204, .b = 244, .a = 200 });
    cv.fillRoundedRect(520, 240, 280, 240, 130, .{ .r = 244, .g = 110, .b = 196, .a = 190 });
    cv.boxBlur(0, 0, Wi, Hi, 6);
    glass(&cv, 70, 110, 360, 270, 22, 26);
    cv.textAA(98, 134, "OpenGL Backend", Color.rgb(255, 255, 255), fd);
    cv.textAA(98, 174, "this frame is uploaded to the GPU and", Color.rgb(235, 240, 248), fb);
    cv.textAA(98, 194, "presented on a textured quad (GLX).", Color.rgb(235, 240, 248), fb);
    cv.textAA(98, 222, "frosted glass + vector text, on GPU.", Color.rgb(235, 240, 248), fb);

    var win = gl.GlWindow.open(W, H, "Zenith DAW (OpenGL)") catch |e| {
        std.debug.print("GL window open failed: {any}\n", .{e});
        return e;
    };
    defer win.close();
    std.debug.print("OpenGL window opened — presenting via GLX texture quad\n", .{});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 4.0;
        } else |_| break :blk 4.0;
    };
    var t: f64 = 0;
    var frames: usize = 0;
    while (t < secs) {
        if (win.pump()) break;
        win.present(cv.pixels);
        frames += 1;
        std.time.sleep(16 * std.time.ns_per_ms);
        t += 0.016;
    }
    std.debug.print("presented {d} frames via OpenGL, closing\n", .{frames});
}
