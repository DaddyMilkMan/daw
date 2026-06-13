//! main_gpu.zig — Milestone A smoke test for the GPU 2D renderer: open a GL
//! window and draw SDF rounded rects (with borders) and analytic gaussian
//! shadows entirely on the GPU. No CPU framebuffer, no texture upload.

const std = @import("std");
const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const Color = gpu2d.Color;

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 1100;
    const H: usize = 680;

    var window = try win.NativeWindow.open(a, W, H, "Zenith GPU");
    defer window.close();
    window.makeCurrent();

    var g = gpu2d.Gpu.init(a, win.NativeWindow.glProc) catch |e| {
        std.debug.print("gpu2d init failed: {any}\n", .{e});
        return e;
    };
    defer g.deinit();
    std.debug.print("gpu2d ready — drawing SDF shapes + analytic shadows\n", .{});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 6.0;
        } else |_| break :blk 6.0;
    };

    const accent = Color.rgb(96, 210, 235);
    const card_top = Color.rgb(46, 50, 63);
    const border = Color.rgba(255, 255, 255, 38);

    var elapsed: f64 = 0;
    while (elapsed < secs) {
        while (true) {
            switch (window.poll()) {
                .none => break,
                .close => {
                    elapsed = secs;
                    break;
                },
                .key => |k| if (k == 9) {
                    elapsed = secs;
                },
                else => {},
            }
        }

        g.begin(W, H, Color.rgb(18, 20, 26));

        // a row of swatches at increasing corner radius
        var i: usize = 0;
        while (i < 5) : (i += 1) {
            const x: f32 = 40 + @as(f32, @floatFromInt(i)) * 150;
            const rad: f32 = @floatFromInt(2 + i * 8);
            g.shadow(x, 60, 120, 120, rad, 16, Color.rgba(0, 0, 0, 180));
            g.rectBordered(x, 60, 120, 120, rad, card_top, 1, border);
        }

        // a big glass card with a soft elevation shadow + accent bar
        g.shadow(40, 240, 1020, 360, 18, 28, Color.rgba(0, 0, 0, 160));
        g.rectBordered(40, 240, 1020, 360, 18, Color.rgb(31, 34, 43), 1, border);
        g.rect(64, 264, 200, 6, 3, accent);

        // nested cards with various accents
        const cols = [_]Color{ Color.rgb(245, 158, 88), Color.rgb(122, 211, 140), accent, Color.rgb(178, 140, 248) };
        var j: usize = 0;
        while (j < 4) : (j += 1) {
            const x: f32 = 64 + @as(f32, @floatFromInt(j)) * 250;
            g.shadow(x, 300, 220, 270, 12, 14, Color.rgba(0, 0, 0, 150));
            g.rectBordered(x, 300, 220, 270, 12, Color.rgb(38, 42, 52), 1, border);
            g.rect(x + 16, 320, 188, 5, 2, cols[j]);
            g.rect(x + 16, 520, 120, 30, 8, cols[j]);
        }

        g.flush();
        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
