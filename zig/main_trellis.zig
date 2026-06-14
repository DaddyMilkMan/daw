//! main_trellis.zig — demo of the Trellis layout engine. The whole UI below is
//! declared structurally (rows/cols/gap/padding/grow) — no pixel coordinates —
//! and the engine computes exact positions. Buttons animate on hover.

const std = @import("std");
const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const ui = @import("trellis.zig");
const Color = gpu2d.Color;
const px = ui.px;
const grow = ui.grow;
const pct = ui.pct;

const accent = Color.rgb(96, 210, 235);
const bg0 = Color.rgb(18, 20, 26);
const panel_t = Color.rgb(37, 40, 51);
const panel_b = Color.rgb(27, 29, 38);
const card_t = Color.rgb(47, 51, 64);
const card_b = Color.rgb(34, 37, 48);
const border = Color.rgba(255, 255, 255, 18);
const txt = Color.rgb(236, 239, 246);
const dim = Color.rgb(140, 148, 164);
const faint = Color.rgb(96, 103, 119);

pub fn main() !void {
    const a = std.heap.page_allocator;
    var W: usize = 1100;
    var H: usize = 680;

    var window = try win.NativeWindow.open(a, W, H, "Zenith Flex");
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
    var c = ui.Ctx.init(&g, &fb, &fu, &fd);
    std.debug.print("Trellis layout demo\n", .{});

    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 8.0;
        } else |_| break :blk 8.0;
    };
    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var prev_down = false;
    var elapsed: f64 = 0;
    // dropdown state (open by default so the screenshot shows it)
    var menu_open = true;
    var menu_anim: f32 = 1;
    var menu_hot: i32 = -1;

    const nav = [_][]const u8{ "Sounds", "Drums", "Instruments", "Audio FX", "Samples" };
    const devs = [_]struct { name: []const u8, kind: []const u8, col: Color }{
        .{ .name = "Operator", .kind = "Synthesizer", .col = Color.rgb(178, 140, 248) },
        .{ .name = "Reverb", .kind = "Audio Effect", .col = Color.rgb(122, 211, 140) },
        .{ .name = "EQ Eight", .kind = "Audio Effect", .col = accent },
    };

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

        g.begin(W, H, bg0);
        c.begin(@floatFromInt(W), @floatFromInt(H), @floatFromInt(mx), @floatFromInt(my), down, 0.016);

        // ---- top bar -------------------------------------------------------
        c.open(.{ .dir = .row, .w = grow(), .h = px(56), .pad = 14, .gap = 12, .aligni = .center, .bg = Color.rgb(34, 37, 47), .bg2 = Color.rgb(24, 26, 34), .border = border });
        {
            c.label("Zenith", &fd, accent, .{});
            c.box(.{ .w = grow() }); // spacer pushes the rest right
            _ = c.button("View", 13, .{ .w = px(84), .h = px(34) });
            _ = c.button("New", 10, .{ .w = px(84), .h = px(34) });
            _ = c.button("Open", 11, .{ .w = px(84), .h = px(34) });
            _ = c.button("Export", 12, .{ .w = px(96), .h = px(34), .bg = accent, .bg2 = Color.rgb(64, 168, 196), .hover_bg = Color.rgb(140, 230, 250) });
        }
        c.close();

        // ---- body: sidebar + content --------------------------------------
        c.open(.{ .dir = .row, .w = grow(), .h = grow() });
        {
            // sidebar
            c.open(.{ .dir = .col, .w = px(220), .h = grow(), .pad = 14, .gap = 6, .aligni = .stretch, .bg = panel_t, .bg2 = panel_b, .border = border });
            {
                c.label("LIBRARY", &fb, faint, .{ .h = px(24) });
                for (nav, 0..) |item, i| {
                    _ = c.button(item, 100 + @as(u64, i), .{ .h = px(34) });
                }
            }
            c.close();

            // content
            c.open(.{ .dir = .col, .w = grow(), .h = grow(), .pad = 22, .gap = 16 });
            {
                c.label("Devices", &fu, txt, .{ .h = px(26) });
                c.open(.{ .dir = .row, .w = grow(), .h = px(170), .gap = 16 });
                {
                    for (devs, 0..) |d, i| {
                        c.open(.{ .dir = .col, .w = grow(), .h = grow(), .pad = 16, .gap = 6, .radius = 12, .bg = card_t, .bg2 = card_b, .border = border, .elev = 1, .shadow = 14, .id = 200 + @as(u64, i), .hover_bg = Color.rgb(56, 61, 76) });
                        {
                            c.box(.{ .w = px(40), .h = px(5), .radius = 2, .bg = d.col });
                            c.box(.{ .h = px(6) }); // spacer
                            c.label(d.name, &fu, txt, .{ .h = px(24) });
                            c.label(d.kind, &fb, dim, .{ .h = px(20) });
                            c.box(.{ .h = grow() }); // push the tag to the bottom
                            c.label("Drag to a track", &fb, faint, .{});
                        }
                        c.close();
                    }
                }
                c.close();
                c.label("Declarative Trellis layout — no pixel coordinates.", &fb, faint, .{ .h = px(20) });
            }
            c.close();
        }
        c.close();

        c.end();
        g.flush(); // draw the backdrop (the Trellis chrome) to the framebuffer

        // ---- animated FROSTED-GLASS dropdown overlay -----------------------
        if (c.click == 13) menu_open = !menu_open;
        const target: f32 = if (menu_open) 1.0 else 0.0;
        menu_anim += (target - menu_anim) * 0.28;
        const released = !down and prev_down;
        prev_down = down;
        if (menu_anim > 0.01) {
            if (c.rectOf(13)) |r| {
                const items = [_][]const u8{ "Arrangement", "Mixer", "Piano Roll", "Settings" };
                const iw: f32 = 190;
                const ih: f32 = 38;
                const full_h: f32 = ih * items.len + 12;
                const eased = menu_anim * menu_anim * (3.0 - 2.0 * menu_anim);
                const hh = full_h * eased;
                const mxp = r[0];
                const myp = r[1] + r[3] + 6;
                // shadow stays sharp on the framebuffer around the panel
                g.shadow(mxp, myp, iw, hh, 14, 22, Color.rgba(0, 0, 0, @intFromFloat(170 * eased)));
                g.flush();
                // blur the backdrop behind the panel, then composite frosted glass
                g.captureBlur(W, H);
                g.glass(mxp, myp, iw, hh, 16, Color.rgba(84, 92, 114, 76), Color.rgba(255, 255, 255, 150));
                // foreground items
                menu_hot = -1;
                const fmx: f32 = @floatFromInt(mx);
                const fmy: f32 = @floatFromInt(my);
                for (items, 0..) |it, i| {
                    const iy = myp + 6 + @as(f32, @floatFromInt(i)) * ih;
                    if (iy + ih > myp + hh - 2) continue;
                    const hov = fmx >= mxp + 6 and fmx < mxp + iw - 6 and fmy >= iy and fmy < iy + ih;
                    if (hov) {
                        menu_hot = @intCast(i);
                        g.rect(mxp + 6, iy, iw - 12, ih, 8, Color.rgba(96, 210, 235, 40));
                        g.rect(mxp + 6, iy + 8, 3, ih - 16, 1, accent);
                        if (released) menu_open = false;
                    }
                    fb.text(&g, mxp + 18, iy + (ih - 15) / 2, it, if (hov) txt else dim);
                }
                g.flush();
            }
        }

        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
