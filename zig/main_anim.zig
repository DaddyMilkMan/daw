//! main_anim.zig — verify animated micro-interactions: hover the play button and
//! capture frames as the hover highlight eases in.

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

    var fr: usize = 0;
    while (fr < 12) : (fr += 1) {
        // hover the play button (id 1)
        u.begin(.{ .mx = 177, .my = 29, .mouse_down = false }, 0.016);
        ui.frame(&u, &p, bar, &state);
        u.end();
        var name_buf: [32]u8 = undefined;
        const name = try std.fmt.bufPrint(&name_buf, "anim_{d}.bmp", .{fr});
        try bmp.write(name, cv.pixels, W, H);
        std.debug.print("frame {d:2}: play-button hover = {d:.3}\n", .{ fr, u.hoverOf(1) });
    }
    std.debug.print("wrote anim_0..11.bmp (hover fade-in)\n", .{});
}
