//! main_pianoroll.zig — standalone piano-roll editor (so it's drivable + verifiable
//! via the control harness before it lands in the DAW). Click the grid to add/remove
//! notes on a seed clip.

const std = @import("std");
pub const std_options: std.Options = .{ .log_level = .debug, .logFn = @import("log.zig").logFn };
const plog = std.log.scoped(.pianoroll);

const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const project = @import("project.zig");
const PianoRoll = @import("pianoroll.zig").PianoRoll;

const Color = gpu2d.Color;

pub fn main() !void {
    const a = std.heap.page_allocator;
    var W: usize = 1100;
    var H: usize = 680;
    var window = try win.NativeWindow.open(a, W, H, "Zenith Piano Roll");
    defer window.close();
    window.makeCurrent();
    var g = try gpu2d.Gpu.init(a, win.NativeWindow.glProc);
    defer g.deinit();
    var fb = try gpu2d.GpuFont.init(a, &@import("font_body.zig").font);
    defer fb.deinit();
    var fd = try gpu2d.GpuFont.init(a, &@import("font_display.zig").font);
    defer fd.deinit();

    // a seed clip (a little arpeggio) to edit
    var clip = project.Clip.init(a);
    defer clip.deinit();
    try clip.name.appendSlice("riff");
    const cf: u64 = 6000; // an eighth note at this grid
    try clip.notes.append(.{ .start = 0 * cf, .len = cf, .pitch = 60, .velocity = 100 });
    try clip.notes.append(.{ .start = 2 * cf, .len = cf, .pitch = 64, .velocity = 100 });
    try clip.notes.append(.{ .start = 4 * cf, .len = cf, .pitch = 67, .velocity = 100 });
    try clip.notes.append(.{ .start = 6 * cf, .len = 3 * cf, .pitch = 72, .velocity = 100 });

    var pr = PianoRoll{ .bar_frames = 48000 };
    plog.info("piano roll opened with {d} seed notes", .{clip.notes.items.len});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 1.0e12;
        } else |_| break :blk 1.0e12;
    };
    var mx: i32 = -1;
    var my: i32 = -1;
    var elapsed: f64 = 0;

    while (elapsed < secs) {
        var clicked = false;
        while (true) {
            switch (window.poll()) {
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
                    mx = m.x;
                    my = m.y;
                    clicked = true;
                },
                .key => |k| if (k == 9) {
                    elapsed = secs;
                },
                else => {},
            }
        }

        const Wf: f32 = @floatFromInt(W);
        const Hf: f32 = @floatFromInt(H);
        g.begin(W, H, Color.rgb(16, 17, 21));
        fd.text(&g, 20, 13, "Piano Roll", Color.rgb(108, 147, 244));
        var nb: [48]u8 = undefined;
        const ns = std.fmt.bufPrint(&nb, "{d} notes  -  click a cell to add / remove", .{clip.notes.items.len}) catch "";
        fb.text(&g, 150, 17, ns, Color.rgb(150, 158, 173));

        pr.render(&g, &fb, .{ 0, 44, Wf, Hf - 44 }, &clip, @floatFromInt(mx), @floatFromInt(my), clicked);
        if (pr.last_edit == .added) plog.info("note ADDED -> {d} notes", .{clip.notes.items.len});
        if (pr.last_edit == .deleted) plog.info("note DELETED -> {d} notes", .{clip.notes.items.len});

        g.flush();
        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    plog.info("piano roll closed with {d} notes", .{clip.notes.items.len});
}
