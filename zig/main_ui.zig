//! main_ui.zig — render one Zenith UI frame to a BMP (no input).

const std = @import("std");
const r2d = @import("render2d.zig");
const bmp = @import("bmp.zig");
const uikit = @import("uikit.zig");
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

    var u = uikit.Ui.init(&cv);
    var state = ui.State{};
    u.begin(.{});
    ui.frame(&u, &p, bar, &state);
    u.end();

    try bmp.write("zenith_ui.bmp", cv.pixels, W, H);
    std.debug.print("rendered Zenith UI frame -> zenith_ui.bmp ({d}x{d})\n", .{ W, H });
}
