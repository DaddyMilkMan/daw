//! main_glwin.zig — GPU shader glassmorphism. The UI content (sharp colorful
//! grid) is uploaded once; a fragment shader frosts a MOVING glass panel in real
//! time. The blur is computed on the GPU per frame — impossible to do cheaply on
//! the CPU, trivial on the GPU.

const std = @import("std");
const r2d = @import("render2d.zig");
const gl = @import("window_gl.zig");
const Color = r2d.Color;

const fb = &@import("font_body.zig").font;
const fd = &@import("font_display.zig").font;

const grid = [_]Color{
    .{ .r = 96, .g = 206, .b = 244 }, .{ .r = 244, .g = 110, .b = 196 },
    .{ .r = 250, .g = 196, .b = 92 },  .{ .r = 122, .g = 211, .b = 140 },
    .{ .r = 178, .g = 140, .b = 248 },
};

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 880;
    const H: usize = 520;
    const Wi: i32 = @intCast(W);
    const Hi: i32 = @intCast(H);

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();

    // sharp, colorful content so the GPU blur is obvious
    cv.vGradient(0, 0, Wi, Hi, Color.rgb(26, 22, 40), Color.rgb(14, 16, 30));
    var gy: i32 = 30;
    var idx: usize = 0;
    while (gy < Hi - 20) : (gy += 64) {
        var gx: i32 = 24;
        while (gx < Wi - 20) : (gx += 64) {
            cv.fillRoundedRect(gx, gy, 48, 48, 10, grid[idx % grid.len]);
            idx += 1;
        }
        idx += 1;
    }
    cv.textAA(26, 18, "GPU shader — live frosted glass", Color.rgb(255, 255, 255), fd);

    var win = gl.GlWindow.open(W, H, "Zenith DAW (GPU shader)") catch |e| {
        std.debug.print("GL window open failed: {any}\n", .{e});
        return e;
    };
    defer win.close();
    std.debug.print("OpenGL + GLSL shader ready — frosting a moving glass panel on the GPU\n", .{});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 5.0;
        } else |_| break :blk 5.0;
    };

    const gw: f32 = 340;
    const gh: f32 = 240;
    const gyf: f32 = 150;
    var t: f64 = 0;
    var frames: usize = 0;
    while (t < secs) {
        if (win.pump()) break;
        const s = 0.5 + 0.5 * std.math.sin(t * 1.4);
        const gx: f32 = 30 + @as(f32, @floatCast(s)) * (@as(f32, @floatFromInt(Wi)) - gw - 60);
        win.presentGlass(cv.pixels, gx, gyf, gw, gh); // shader blurs this rect on the GPU
        frames += 1;
        std.time.sleep(16 * std.time.ns_per_ms);
        t += 0.016;
    }
    std.debug.print("presented {d} GPU-shaded frames (live blur), closing\n", .{frames});
}
