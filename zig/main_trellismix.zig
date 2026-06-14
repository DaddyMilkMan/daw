//! main_flexmix.zig — the Zenith mixer, laid out 100% by OUR flexbox engine
//! (trellis.zig). Every position comes from the layout solver (no cy += 22 math);
//! interactive widgets (faders/knobs/sliders/toggles) render into the
//! flex-computed rects via the GPU widget toolkit. Proof that the DAW's most
//! layout-heavy section is fully declarative on our own engine.

const std = @import("std");
const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const trellis = @import("trellis.zig");
const w = @import("ui_gpu.zig");
const Color = gpu2d.Color;
const px = trellis.px;
const grow = trellis.grow;

const card_t = Color.rgb(47, 51, 64);
const card_b = Color.rgb(34, 37, 48);
const panel_t = Color.rgb(37, 40, 51);
const panel_b = Color.rgb(27, 29, 38);
const border = Color.rgba(255, 255, 255, 18);
const txt = Color.rgb(236, 239, 246);
const dim = Color.rgb(140, 148, 164);
const faint = Color.rgb(96, 103, 119);
const amber = Color.rgb(240, 176, 72);
const green = Color.rgb(122, 211, 140);
const red = Color.rgb(238, 96, 96);
const accent = Color.rgb(96, 210, 235);
const palette = [_]Color{
    Color.rgb(245, 158, 88), Color.rgb(122, 211, 140), Color.rgb(96, 206, 240),
    Color.rgb(178, 140, 248), Color.rgb(240, 132, 170),
};

fn miniToggle(u: *w.UiG, g: *gpu2d.Gpu, fb: *const gpu2d.GpuFont, id: u32, r: [4]f32, lbl: []const u8, on: bool, oncol: Color) bool {
    const clicked = u.iconSlot(id, r[0], r[1], r[2], r[3], false);
    if (on) g.rect(r[0], r[1], r[2], r[3], 5, oncol);
    const tw = fb.textWidth(lbl);
    fb.text(g, r[0] + (r[2] - tw) / 2, r[1] + (r[3] - 12) / 2, lbl, if (on) Color.rgb(18, 20, 26) else dim);
    return clicked;
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    var W: usize = 1100;
    var H: usize = 460;
    const bar: u64 = 96000;

    var window = try win.NativeWindow.open(a, W, H, "Zenith Flex Mixer");
    defer window.close();
    window.makeCurrent();
    var g = try gpu2d.Gpu.init(a, win.NativeWindow.glProc);
    defer g.deinit();
    var fb = try gpu2d.GpuFont.init(a, &@import("font_body.zig").font);
    defer fb.deinit();
    var fu = try gpu2d.GpuFont.init(a, &@import("font_ui.zig").font);
    defer fu.deinit();
    var fd = try gpu2d.GpuFont.init(a, &@import("font_display.zig").font);
    defer fd.deinit();

    var p = try w.buildDemoProject(a, bar);
    defer p.deinit();
    var c = trellis.Ctx.init(&g, &fb, &fu, &fd);
    var u = w.UiG.init(&g, &fb, &fu, &fd);
    var state = w.State{};

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 8.0;
        } else |_| break :blk 8.0;
    };
    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var elapsed: f64 = 0;
    const ntr = p.tracks.items.len;

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
                .expose => {},
            }
        }
        const fmx: f32 = @floatFromInt(mx);
        const fmy: f32 = @floatFromInt(my);

        g.begin(W, H, Color.rgb(15, 16, 21));
        g.rectGrad(0, 0, @floatFromInt(W), @floatFromInt(H), 0, Color.rgb(26, 28, 36), Color.rgb(13, 14, 19), 0, border);
        u.begin(.{ .mx = fmx, .my = fmy, .mouse_down = down }, 0.016);

        // ---- LAYOUT: the whole mixer declared structurally -----------------
        const pad: f32 = 16;
        c.beginAt(pad, pad, @as(f32, @floatFromInt(W)) - 2 * pad, @as(f32, @floatFromInt(H)) - 2 * pad, fmx, fmy, down, 0.016);
        c.open(.{ .dir = .row, .w = grow(), .h = grow(), .gap = 10, .pad = 12, .radius = 14, .bg = panel_t, .bg2 = panel_b, .border = border, .elev = 0.6, .shadow = 16 });
        {
            var ti: usize = 0;
            while (ti <= ntr) : (ti += 1) {
                const is_master = ti == ntr;
                const col = if (is_master) accent else palette[ti % palette.len];
                const idx: u64 = @intCast(ti);
                c.open(.{ .dir = .col, .w = grow(), .h = grow(), .pad = 10, .gap = 7, .radius = 11, .bg = card_t, .bg2 = card_b, .border = border, .elev = 1.0 });
                {
                    c.box(.{ .w = grow(), .h = px(4), .radius = 2, .bg = col }); // color bar
                    c.label(if (is_master) "Master" else p.tracks.items[ti].name.items, &fu, txt, .{ .h = px(22) });
                    if (!is_master) {
                        c.open(.{ .dir = .row, .w = grow(), .h = px(16), .gap = 4, .aligni = .center });
                        {
                            c.box(.{ .w = px(22), .h = px(16), .id = 300 + idx }); // M slot
                            c.box(.{ .w = px(22), .h = px(16), .id = 320 + idx }); // S slot
                            c.box(.{ .w = grow() });
                            c.label("PAN", &fb, faint, .{});
                        }
                        c.close();
                        c.box(.{ .w = grow(), .h = px(6), .id = 400 + idx }); // pan slot
                        c.open(.{ .dir = .row, .w = grow(), .h = px(44), .gap = 6, .justify = .center });
                        {
                            c.open(.{ .dir = .col, .w = px(40), .aligni = .center, .gap = 2 });
                            {
                                c.box(.{ .w = px(26), .h = px(26), .id = 600 + idx }); // knob A
                                c.label("A", &fb, faint, .{});
                            }
                            c.close();
                            c.open(.{ .dir = .col, .w = px(40), .aligni = .center, .gap = 2 });
                            {
                                c.box(.{ .w = px(26), .h = px(26), .id = 700 + idx }); // knob B
                                c.label("B", &fb, faint, .{});
                            }
                            c.close();
                        }
                        c.close();
                    }
                    // fader + meter area fills remaining height
                    c.open(.{ .dir = .row, .w = grow(), .h = grow(), .gap = 10, .justify = .center });
                    {
                        c.box(.{ .w = px(10), .h = grow(), .id = 100 + idx }); // fader slot
                        c.box(.{ .w = px(10), .h = grow(), .id = 500 + idx }); // meter slot
                    }
                    c.close();
                    c.box(.{ .w = grow(), .h = px(16) }); // reserves the value row (drawn below)
                }
                c.close();
            }
        }
        c.close();
        c.end(); // draws all chrome + labels; computes every rect

        // ---- WIDGETS: rendered into the flex-computed rects ----------------
        var ti: usize = 0;
        while (ti <= ntr) : (ti += 1) {
            const is_master = ti == ntr;
            const idx: u64 = @intCast(ti);
            const gain = if (is_master) &state.master_gain else &p.tracks.items[ti].gain;
            if (!is_master) {
                if (c.rectOf(300 + idx)) |r| if (miniToggle(&u, &g, &fb, @intCast(300 + idx), r, "M", state.mutes[ti], amber)) {
                    state.mutes[ti] = !state.mutes[ti];
                };
                if (c.rectOf(320 + idx)) |r| if (miniToggle(&u, &g, &fb, @intCast(320 + idx), r, "S", state.solos[ti], green)) {
                    state.solos[ti] = !state.solos[ti];
                };
                if (c.rectOf(400 + idx)) |r| _ = u.hSlider(@intCast(400 + idx), r[0], r[1], r[2], r[3], &p.tracks.items[ti].pan, -1.0, 1.0);
                if (c.rectOf(600 + idx)) |r| _ = u.knob(@intCast(600 + idx), r[0] + r[2] / 2, r[1] + r[3] / 2, 12, &state.sends[ti][0]);
                if (c.rectOf(700 + idx)) |r| _ = u.knob(@intCast(700 + idx), r[0] + r[2] / 2, r[1] + r[3] / 2, 12, &state.sends[ti][1]);
            }
            if (c.rectOf(100 + idx)) |r| _ = u.vFader(@intCast(100 + idx), r[0], r[1], r[2], r[3], gain);
            if (c.rectOf(500 + idx)) |r| {
                g.rect(r[0], r[1], r[2], r[3], 4, Color.rgb(15, 17, 22));
                const lvl = if (!is_master and state.mutes[ti]) 0.0 else gain.* * 0.92;
                const mh = lvl * r[3];
                if (mh > 1) g.rectGrad(r[0], r[1] + r[3] - mh, r[2], mh, 4, red, green, 0, border);
            }
            // value readout over the placeholder
            if (c.rectOf(100 + idx)) |r| {
                var vbuf: [8]u8 = undefined;
                const vs = std.fmt.bufPrint(&vbuf, "{d:.0}", .{gain.* * 100}) catch "";
                fb.text(&g, r[0] - 6, r[1] + r[3] + 6, vs, dim);
            }
        }

        u.end();
        g.flush();
        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("flex mixer closed\n", .{});
}
