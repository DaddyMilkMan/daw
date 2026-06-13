//! main_ui.zig — render the Zenith UI frame to a BMP (headless-friendly).

const std = @import("std");
const r2d = @import("render2d.zig");
const bmp = @import("bmp.zig");
const ui = @import("ui.zig");

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 960;
    const H: usize = 560;
    const bar: u64 = 96000;

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();
    var p = try ui.buildDemoProject(a, bar);
    defer p.deinit();

    ui.drawFrame(&cv, &p, bar, -1, -1, false);

    try bmp.write("zenith_ui.bmp", cv.pixels, W, H);
    std.debug.print("rendered Zenith UI frame -> zenith_ui.bmp ({d}x{d})\n", .{ W, H });
}
