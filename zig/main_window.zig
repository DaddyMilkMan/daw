//! main_window.zig — Zenith in a borderless, custom-chrome, resizable window.
//! Drag the bar to move, drag edges to resize, window buttons to min/max/close.
//! Live resize recreates the framebuffer and the UI reflows.

const std = @import("std");
const r2d = @import("render2d.zig");
const uikit = @import("uikit.zig");
const ui = @import("ui.zig");
const win = @import("window_glx.zig");

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

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();
    var p = try ui.buildDemoProject(a, bar);
    defer p.deinit();
    var u = uikit.Ui.init(&cv);
    var state = ui.State{};

    var window = win.NativeWindow.open(a, W, H, "Zenith DAW") catch |e| {
        std.debug.print("window open failed: {any}\n", .{e});
        return e;
    };
    defer window.close();
    std.debug.print("borderless window opened (custom chrome, resizable)\n", .{});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 6.0;
        } else |_| break :blk 6.0;
    };
    const resize_test = (std.process.getEnvVarOwned(a, "ZENITH_RESIZE_TEST") catch null) != null;
    const sizes = [_][2]usize{ .{ 760, 440 }, .{ 1120, 620 }, .{ 420, 320 }, .{ 1000, 300 }, .{ 960, 560 } };

    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var lrx: i32 = 0;
    var lry: i32 = 0;
    var elapsed: f64 = 0;
    var rt_timer: usize = 0;
    var rt_idx: usize = 0;
    var cooldown: usize = 40; // frames left to keep rendering (dirty window)

    while (elapsed < secs) {
        while (true) {
            const ev = window.poll();
            switch (ev) {
                .none => break,
                else => cooldown = 30, // any event -> render a burst (covers animations)
            }
            switch (ev) {
                .none => {},
                .close => {
                    elapsed = secs;
                    break;
                },
                .resize => |r| {
                    cv.deinit();
                    cv = try r2d.Canvas.init(a, r.w, r.h);
                    window.resize(r.w, r.h);
                    W = r.w;
                    H = r.h;
                    std.debug.print("resized -> {d}x{d}\n", .{ r.w, r.h });
                },
                .mouse_move => |m| {
                    mx = m.x;
                    my = m.y;
                },
                .mouse_down => |m| {
                    lrx = m.x_root;
                    lry = m.y_root;
                    if (edgeDir(m.x, m.y, @intCast(W), @intCast(H))) |dir| {
                        window.startMoveResize(dir, m.x_root, m.y_root); // WM-driven edge resize
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

        // Only re-render when something changed or animations are settling.
        if (cooldown > 0) {
            u.begin(.{ .mx = mx, .my = my, .mouse_down = down }, 0.016);
            ui.frame(&u, &p, bar, &state);
            u.end();

            switch (state.window_action) {
                .none => {},
                .close => elapsed = secs,
                .minimize => window.minimize(),
                .maximize => window.toggleMaximize(),
                .move => window.startMoveResize(win.MOVE, lrx, lry),
            }

            window.present(cv.pixels);
            cooldown -= 1;
        }

        if (resize_test) {
            rt_timer += 1;
            if (rt_timer >= 45) {
                rt_timer = 0;
                rt_idx = (rt_idx + 1) % sizes.len;
                window.resizeSelf(sizes[rt_idx][0], sizes[rt_idx][1]);
            }
        }
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
