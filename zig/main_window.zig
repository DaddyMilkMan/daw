//! main_window.zig — Zenith in a live, interactive native window. Drag faders,
//! click play. Runs for ZENITH_WINDOW_SECONDS then auto-closes (Esc/close also).

const std = @import("std");
const r2d = @import("render2d.zig");
const uikit = @import("uikit.zig");
const ui = @import("ui.zig");
const win = @import("window_x11.zig");

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 960;
    const H: usize = 560;
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
    std.debug.print("window opened ({d}x{d})\n", .{ W, H });

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 4.0;
        } else |_| break :blk 4.0;
    };

    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var frames: usize = 0;
    var elapsed: f64 = 0;
    const frame_ms: u64 = 16;

    while (elapsed < secs) {
        while (true) {
            switch (window.poll()) {
                .none => break,
                .close => {
                    elapsed = secs;
                    break;
                },
                .mouse_move => |m| {
                    mx = m.x;
                    my = m.y;
                },
                .mouse_down => |m| {
                    mx = m.x;
                    my = m.y;
                    down = true;
                },
                .mouse_up => {
                    down = false;
                },
                .key => |k| {
                    if (k == 9) elapsed = secs; // Esc
                },
                .expose => {},
            }
        }
        u.begin(.{ .mx = mx, .my = my, .mouse_down = down }, 0.016);
        ui.frame(&u, &p, bar, &state);
        u.end();
        window.present(cv.pixels);
        frames += 1;
        std.time.sleep(frame_ms * std.time.ns_per_ms);
        elapsed += @as(f64, @floatFromInt(frame_ms)) / 1000.0;
    }
    std.debug.print("closed after {d} frames\n", .{frames});
}
