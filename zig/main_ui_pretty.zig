//! main_ui_pretty.zig — render the polished (antialiased) DAW frame to a BMP.

const std = @import("std");
const r2d = @import("render2d.zig");
const bmp = @import("bmp.zig");
const ui = @import("ui.zig");
const pretty = @import("ui_pretty.zig");

pub fn main() !void {
    const a = std.heap.page_allocator;
    const W: usize = 1040;
    const H: usize = 620;
    const bar: u64 = 96000;

    var cv = try r2d.Canvas.init(a, W, H);
    defer cv.deinit();
    var p = try ui.buildDemoProject(a, bar);
    defer p.deinit();

    pretty.draw(&cv, &p, bar);

    try bmp.write("zenith_pretty.bmp", cv.pixels, W, H);
    std.debug.print("rendered polished UI -> zenith_pretty.bmp ({d}x{d})\n", .{ W, H });
}
