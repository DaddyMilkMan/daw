//! main_window.zig — Zenith in a live native window. Renders the DAW frame,
//! blits it, and responds to mouse/keyboard. (Headless-test note: runs for
//! ZENITH_WINDOW_SECONDS then auto-closes; Esc/close also exit.)

const std = @import("std");
const r2d = @import("render2d.zig");
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

    var window = win.NativeWindow.open(a, W, H, "Zenith DAW") catch |e| {
        std.debug.print("window open failed: {any}\n", .{e});
        return e;
    };
    defer window.close();
    std.debug.print("window opened ({d}x{d}) on the display\n", .{ W, H });

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 3.0;
        } else |_| break :blk 3.0;
    };

    var mx: i32 = -1;
    var my: i32 = -1;
    var playing = false;
    var frames: usize = 0;
    var input_events: usize = 0;
    var elapsed: f64 = 0;
    const frame_ms: u64 = 16;

    while (elapsed < secs) {
        while (true) {
            switch (window.poll()) {
                .none => break,
                .close => {
                    std.debug.print("received window close\n", .{});
                    elapsed = secs;
                    break;
                },
                .mouse_move => |m| {
                    mx = m.x;
                    my = m.y;
                    input_events += 1;
                },
                .mouse_down => |m| {
                    mx = m.x;
                    my = m.y;
                    input_events += 1;
                    if (m.x >= 170 and m.x < 196 and m.y >= 12 and m.y < 34) playing = !playing; // play button
                },
                .mouse_up => input_events += 1,
                .key => |k| {
                    input_events += 1;
                    if (k == 9) elapsed = secs; // Esc
                    if (k == 65) playing = !playing; // Space
                },
                .expose => {},
            }
        }
        ui.drawFrame(&cv, &p, bar, mx, my, playing);
        window.present(cv.pixels);
        frames += 1;
        std.time.sleep(frame_ms * std.time.ns_per_ms);
        elapsed += @as(f64, @floatFromInt(frame_ms)) / 1000.0;
    }
    std.debug.print("closed after {d} frames, {d} input events\n", .{ frames, input_events });
}
