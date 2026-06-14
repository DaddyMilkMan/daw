//! main_pianoroll.zig — standalone piano-roll editor (so it's drivable + verifiable
//! via the Talkback harness before it lands in the DAW). Click the grid to add/remove
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
    pr.audition = auditionLog;
    plog.info("piano roll opened with {d} seed notes", .{clip.notes.items.len});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 1.0e12;
        } else |_| break :blk 1.0e12;
    };
    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var prev_down = false;
    var elapsed: f64 = 0;

    while (elapsed < secs) {
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
                    down = true;
                },
                .mouse_up => down = false,
                .key => |k| if (k == 9) {
                    elapsed = secs;
                },
                else => {},
            }
        }
        const pressed = down and !prev_down;
        const released = !down and prev_down;

        const Wf: f32 = @floatFromInt(W);
        const Hf: f32 = @floatFromInt(H);
        g.begin(W, H, Color.rgb(16, 17, 21));
        fd.text(&g, 20, 13, "Piano Roll", Color.rgb(108, 147, 244));
        var nb: [80]u8 = undefined;
        const ns = std.fmt.bufPrint(&nb, "{d} notes  -  click add/remove, drag move, edge resize, lane = velocity", .{clip.notes.items.len}) catch "";
        fb.text(&g, 150, 17, ns, Color.rgb(150, 158, 173));

        pr.update(&g, &fb, .{ 0, 44, Wf, Hf - 44 }, &clip, @floatFromInt(mx), @floatFromInt(my), pressed, down, released);
        switch (pr.last_edit) {
            .added => plog.info("note ADDED -> {d} notes", .{clip.notes.items.len}),
            .deleted => plog.info("note DELETED -> {d} notes", .{clip.notes.items.len}),
            .moved => plog.debug("note MOVED", .{}),
            .resized => plog.debug("note RESIZED", .{}),
            .velocity => plog.debug("velocity edit", .{}),
            .none => {},
        }
        prev_down = down;

        g.flush();
        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    plog.info("piano roll closed with {d} notes", .{clip.notes.items.len});
}

/// Audition hook — logs note previews (in the DAW this drives the engine synth).
fn auditionLog(_: ?*anyopaque, pitch: u8, on: bool) void {
    plog.debug("preview note {d} {s}", .{ pitch, if (on) "ON" else "OFF" });
}
