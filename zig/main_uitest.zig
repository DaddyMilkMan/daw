//! main_uitest.zig — headless verification of interactivity: feed scripted mouse
//! input through the immediate-mode UI and assert that state actually changes
//! (play button toggles; dragging a fader changes that track's gain).

const std = @import("std");
const r2d = @import("render2d.zig");
const bmp = @import("bmp.zig");
const project = @import("project.zig");
const uikit = @import("uikit.zig");
const ui = @import("ui.zig");

fn step(u: *uikit.Ui, p: *project.Project, bar: u64, state: *ui.State, mx: i32, my: i32, down: bool) void {
    u.begin(.{ .mx = mx, .my = my, .mouse_down = down });
    ui.frame(u, p, bar, state);
    u.end();
}

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

    const g0 = p.tracks.items[0].gain;
    const play0 = state.playing;

    step(&u, &p, bar, &state, -1, -1, false); // idle frame
    try bmp.write("uitest_before.bmp", cv.pixels, W, H);

    // click the play button (press then release inside it)
    step(&u, &p, bar, &state, 182, 22, true);
    step(&u, &p, bar, &state, 182, 22, false);
    const play1 = state.playing;

    // grab track-0 fader near the top, drag to the bottom, release
    step(&u, &p, bar, &state, 58, 326, true);
    step(&u, &p, bar, &state, 58, 478, true);
    step(&u, &p, bar, &state, 58, 478, false);
    const g1 = p.tracks.items[0].gain;

    try bmp.write("uitest_after.bmp", cv.pixels, W, H);

    std.debug.print("play button: {} -> {}  (toggled: {})\n", .{ play0, play1, play1 != play0 });
    std.debug.print("track0 gain: {d:.3} -> {d:.3}  (fader drag)\n", .{ g0, g1 });

    const ok = (play1 != play0) and (g0 > 0.5) and (g1 < 0.15);
    std.debug.print("INTERACTIVE UI: {s}\n", .{if (ok) "PASS" else "FAIL"});
    if (!ok) std.process.exit(1);
}
