//! main_gpu_daw.zig — the live Zenith DAW, rendered entirely on the GPU toolkit.
//! Borderless custom chrome, resizable, full input — drawing via gpu2d (SDF
//! shapes, analytic shadows, atlas text), no CPU framebuffer.

const std = @import("std");
const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const ui = @import("ui_gpu.zig");

fn edgeDir(x: i32, y: i32, w: i32, h: i32) ?c_long {
    const m: i32 = 6;
    const left = x < m;
    const right = x > w - m;
    const top = y < m;
    const bot = y > h - m;
    if (top and left) return win.RESIZE_TOPLEFT;
    if (top and right) return win.RESIZE_TOPRIGHT;
    if (bot and left) return win.RESIZE_BOTTOMLEFT;
    if (bot and right) return win.RESIZE_BOTTOMRIGHT;
    if (left) return win.RESIZE_LEFT;
    if (right) return win.RESIZE_RIGHT;
    if (top) return win.RESIZE_TOP;
    if (bot) return win.RESIZE_BOTTOM;
    return null;
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    var W: usize = 1180;
    var H: usize = 740;
    const bar: u64 = 96000;

    var window = try win.NativeWindow.open(a, W, H, "Zenith DAW");
    defer window.close();
    window.makeCurrent();

    var g = gpu2d.Gpu.init(a, win.NativeWindow.glProc) catch |e| {
        std.debug.print("gpu2d init failed: {any}\n", .{e});
        return e;
    };
    defer g.deinit();
    var fb = try gpu2d.GpuFont.init(a, &@import("font_body.zig").font);
    defer fb.deinit();
    var fu = try gpu2d.GpuFont.init(a, &@import("font_ui.zig").font);
    defer fu.deinit();
    var fd = try gpu2d.GpuFont.init(a, &@import("font_display.zig").font);
    defer fd.deinit();

    var p = try ui.buildDemoProject(a, bar);
    defer p.deinit();
    var u = ui.UiG.init(&g, &fb, &fu, &fd);
    var state = ui.State{};
    std.debug.print("Zenith DAW on the GPU — borderless, resizable\n", .{});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 8.0;
        } else |_| break :blk 8.0;
    };

    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var lrx: i32 = 0;
    var lry: i32 = 0;
    var elapsed: f64 = 0;

    while (elapsed < secs) {
        while (true) {
            const ev = window.poll();
            switch (ev) {
                .none => break,
                .close => {
                    elapsed = secs;
                    break;
                },
                .resize => |r| {
                    window.resize(r.w, r.h);
                    W = r.w;
                    H = r.h;
                },
                .mouse_move => |m| {
                    mx = m.x;
                    my = m.y;
                },
                .mouse_down => |m| {
                    lrx = m.x_root;
                    lry = m.y_root;
                    if (edgeDir(m.x, m.y, @intCast(W), @intCast(H))) |dir| {
                        window.startMoveResize(dir, m.x_root, m.y_root);
                    } else {
                        mx = m.x;
                        my = m.y;
                        down = true;
                    }
                },
                .mouse_up => down = false,
                .key => |k| if (k == 9) {
                    elapsed = secs;
                },
                .expose => {},
            }
        }

        u.begin(.{ .mx = @floatFromInt(mx), .my = @floatFromInt(my), .mouse_down = down }, 0.016);
        ui.frame(&u, &p, bar, &state, @floatFromInt(W), @floatFromInt(H));
        u.end();

        switch (state.window_action) {
            .none => {},
            .close => elapsed = secs,
            .minimize => window.minimize(),
            .maximize => window.toggleMaximize(),
            .move => window.startMoveResize(win.MOVE, lrx, lry),
        }

        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
