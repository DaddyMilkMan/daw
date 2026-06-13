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

    var fd = try gpu2d.GpuFont.init(a, &@import("font_display.zig").font);
    defer fd.deinit();
    var fu = try gpu2d.GpuFont.init(a, &@import("font_ui.zig").font);
    defer fu.deinit();
    var fb = try gpu2d.GpuFont.init(a, &@import("font_body.zig").font);
    defer fb.deinit();
    std.debug.print("gpu2d ready — SDF shapes + analytic shadows + atlas text\n", .{});

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

        fd.text(&g, 40, 18, "Zenith GPU", accent);
        fu.text(&g, 760, 26, "SDF shapes  -  analytic shadows", Color.rgb(140, 148, 164));

        // a big glass card with a soft elevation shadow + GRADIENT fill + accent bar
        g.shadow(40, 240, 1020, 360, 18, 28, Color.rgba(0, 0, 0, 160));
        g.rectGrad(40, 240, 1020, 360, 18, Color.rgb(37, 40, 51), Color.rgb(27, 29, 38), 1, border);
        g.rect(64, 264, 200, 6, 3, accent);
        // a transport "glass" pill with a play triangle (tri primitive) + a stop square
        g.rectGrad(560, 250, 120, 36, 9, Color.rgb(30, 33, 42), Color.rgb(23, 25, 33), 1, border);
        g.tri(582, 260, 582, 276, 596, 268, accent);
        g.rect(612, 260, 16, 16, 3, Color.rgb(140, 148, 164));

        // nested cards with various accents + labels
        const cols = [_]Color{ Color.rgb(245, 158, 88), Color.rgb(122, 211, 140), accent, Color.rgb(178, 140, 248) };
        const names = [_][]const u8{ "Drums", "Bass", "Lead", "Pad" };
        var j: usize = 0;
        while (j < 4) : (j += 1) {
            const x: f32 = 64 + @as(f32, @floatFromInt(j)) * 250;
            g.shadow(x, 300, 220, 270, 12, 14, Color.rgba(0, 0, 0, 150));
            g.rectBordered(x, 300, 220, 270, 12, Color.rgb(38, 42, 52), 1, border);
            g.rect(x + 16, 320, 188, 5, 2, cols[j]);
            fu.text(&g, x + 16, 334, names[j], Color.rgb(236, 239, 246));
            fb.text(&g, x + 16, 360, "the quick brown fox", Color.rgb(140, 148, 164));
            g.rect(x + 16, 520, 120, 30, 8, cols[j]);
        }

        g.flush();
        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
