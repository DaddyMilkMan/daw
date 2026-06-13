//! main_glass.zig — glassmorphism demo: colorful background, then frosted-glass
//! panels (blur the content behind + translucent tint). The effect GPU gives
//! cheaply; here via the software separable blur.

const std = @import("std");
const r2d = @import("render2d.zig");
const bmp = @import("bmp.zig");
const Color = r2d.Color;

const fb = &@import("font_body.zig").font;
const fd = &@import("font_display.zig").font;

fn glass(cv: *r2d.Canvas, x: i32, y: i32, w: i32, h: i32, radius: i32, tintA: u8) void {
    cv.boxBlur(x - 2, y - 2, w + 4, h + 4, 11); // frost whatever is behind
    cv.fillRoundedRect(x, y, w, h, radius, .{ .r = 255, .g = 255, .b = 255, .a = tintA });
    cv.fillRoundedRect(x, y, w, 2, radius, .{ .r = 255, .g = 255, .b = 255, .a = 60 }); // top sheen
    cv.fillRoundedRect(x, y, 2, h, radius, .{ .r = 255, .g = 255, .b = 255, .a = 24 });
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 860;
    const H: usize = 500;

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();
    const Wi: i32 = @intCast(W);
    const Hi: i32 = @intCast(H);

    // colorful background + soft color blobs (so the blur has something to show)
    cv.vGradient(0, 0, Wi, Hi, Color.rgb(46, 32, 78), Color.rgb(18, 26, 54));
    cv.fillRoundedRect(120, 70, 240, 240, 120, .{ .r = 86, .g = 204, .b = 244, .a = 200 });
    cv.fillRoundedRect(520, 240, 280, 240, 130, .{ .r = 244, .g = 110, .b = 196, .a = 190 });
    cv.fillRoundedRect(330, 320, 220, 200, 100, .{ .r = 250, .g = 196, .b = 92, .a = 170 });
    cv.boxBlur(0, 0, Wi, Hi, 6); // soften the blobs overall

    // header text (over the gradient, not glass)
    cv.textAA(28, 24, "Zenith", Color.rgb(255, 255, 255), fd);
    cv.textAA(150, 34, "glassmorphism", Color.rgb(220, 226, 240), fb);

    // frosted glass panels
    glass(&cv, 64, 96, 340, 260, 22, 26);
    cv.textAA(92, 120, "Frosted Panel", Color.rgb(255, 255, 255), fd);
    cv.textAA(92, 158, "real-time blur of the content behind it,", Color.rgb(235, 240, 248), fb);
    cv.textAA(92, 178, "translucent tint, rounded + sheen edges.", Color.rgb(235, 240, 248), fb);
    cv.fillRoundedRect(92, 300, 120, 36, 10, .{ .r = 96, .g = 210, .b = 235, .a = 230 });
    cv.textAA(112, 310, "Action", Color.rgb(16, 20, 26), fb);

    glass(&cv, 456, 150, 320, 280, 22, 22);
    cv.textAA(484, 174, "GPU-ready", Color.rgb(255, 255, 255), fd);
    cv.textAA(484, 212, "this blur is CPU (separable box);", Color.rgb(235, 240, 248), fb);
    cv.textAA(484, 232, "the OpenGL backend (window_gl.zig)", Color.rgb(235, 240, 248), fb);
    cv.textAA(484, 252, "is the path to doing it in a shader,", Color.rgb(235, 240, 248), fb);
    cv.textAA(484, 272, "free, at 60fps + animated.", Color.rgb(235, 240, 248), fb);

    try bmp.write("glass_ui.bmp", cv.pixels, W, H);
    std.debug.print("rendered glassmorphism demo -> glass_ui.bmp ({d}x{d})\n", .{ W, H });
}
