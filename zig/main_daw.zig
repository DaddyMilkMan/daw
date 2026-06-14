//! main_daw.zig — the live Zenith DAW. One cohesive app on our own toolkit:
//! flex layout, GPU widgets, SDF/shadow/atlas-text rendering, frosted glass.
//! Borderless, resizable, full input.

const std = @import("std");
const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const daw = @import("daw.zig");
const audio = @import("audio_engine.zig");
const midi = @import("midi_alsa.zig");
const midi2 = @import("midi2.zig");

fn edgeDir(x: i32, y: i32, w: i32, h: i32) ?c_long {
    const m: i32 = 6;
    const left = x < m;
    const right = x > w - m;
    const top = y < m;
    const bot = y > h - m;
    if (top and left) return win.RESIZE_TOPLEFT;
    if (top and right) return win.RESIZE_TOPRIGHT;
    if (bot and left) return win.RESIZE_BOTTOMLEFT;
    if (bot and right) return win.RESIZE_BOTTOMRIGHT;
    if (left) return win.RESIZE_LEFT;
    if (right) return win.RESIZE_RIGHT;
    if (top) return win.RESIZE_TOP;
    if (bot) return win.RESIZE_BOTTOM;
    return null;
}

pub fn main() !void {
    const a = std.heap.page_allocator;
    var W: usize = 1180;
    var H: usize = 760;
    const bar: u64 = 96000;

    var window = try win.NativeWindow.open(a, W, H, "Zenith DAW");
    defer window.close();
    window.makeCurrent();
    var g = gpu2d.Gpu.init(a, win.NativeWindow.glProc) catch |e| {
        std.debug.print("gpu2d init failed: {any}\n", .{e});
        return e;
    };
    defer g.deinit();
    var fc = try gpu2d.GpuFont.init(a, &@import("font_caption.zig").font);
    defer fc.deinit();
    var fb = try gpu2d.GpuFont.init(a, &@import("font_body.zig").font);
    defer fb.deinit();
    var fu = try gpu2d.GpuFont.init(a, &@import("font_ui.zig").font);
    defer fu.deinit();
    var fd = try gpu2d.GpuFont.init(a, &@import("font_display.zig").font);
    defer fd.deinit();

    var p = try daw.buildDemoProject(a, bar);
    defer p.deinit();
    var view = daw.View.init(&g, &fc, &fb, &fu, &fd);
    // synthesize a real 2-bar drum loop; clips show its actual waveform (peak-analyzed)
    view.wave = daw.synthDrumLoop(a, 96000) catch &.{};
    var state = daw.State{};

    // real-time audio: render the loop to the OS device (ALSA -> PipeWire) on its
    // own thread. The UI pushes transport/gain; the engine drives playhead+meters.
    var engine = audio.Engine{ .samples = view.wave, .rate = 48000 };
    engine.start();
    defer engine.stop();
    engine.setPlaying(state.playing);

    // MIDI 2.0 input: open an ALSA-seq port; incoming 1.0 events are up-converted
    // to UMP (MIDI 2.0), decoded, and routed to the engine's synth as notes.
    var midi_in: ?midi.MidiInput = midi.MidiInput.open("Zenith DAW", "Zenith In") catch null;
    defer if (midi_in) |*m| m.close();
    var midi_evs: [64]midi.MidiEvent = undefined;
    if (midi_in != null) std.debug.print("MIDI in: connect a source to 'Zenith DAW:Zenith In' (aconnect)\n", .{});

    std.debug.print("Zenith DAW — flex + glass + GPU toolkit + live audio + MIDI 2.0\n", .{});

    // Run until the user closes the window (or presses Esc). ZENITH_WINDOW_SECONDS
    // caps the runtime (used by automated screenshots); unset = run indefinitely.
    const secs: f64 = blk: {
        if (std.process.getEnvVarOwned(a, "ZENITH_WINDOW_SECONDS")) |v| {
            defer a.free(v);
            break :blk std.fmt.parseFloat(f64, v) catch 1.0e12;
        } else |_| break :blk 1.0e12;
    };
    var mx: i32 = -1;
    var my: i32 = -1;
    var down = false;
    var rclick = false;
    var lrx: i32 = 0;
    var lry: i32 = 0;
    var elapsed: f64 = 0;

    while (elapsed < secs) {
        while (true) {
            const ev = window.poll();
            switch (ev) {
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
                    lrx = m.x_root;
                    lry = m.y_root;
                    mx = m.x;
                    my = m.y;
                    if (m.button == 3) {
                        rclick = true; // right-click → context menu
                    } else if (edgeDir(m.x, m.y, @intCast(W), @intCast(H))) |dir| {
                        window.startMoveResize(dir, m.x_root, m.y_root);
                    } else {
                        down = true;
                    }
                },
                .mouse_up => down = false,
                .key => |k| if (k == 9) {
                    elapsed = secs;
                },
                .expose => {},
            }
        }

        // route MIDI 2.0 in: poll the seq port, up-convert each event to a UMP
        // (the MIDI 2.0 Protocol), decode it, and trigger the engine's synth.
        if (midi_in) |*m| {
            const n = m.poll(&midi_evs);
            for (midi_evs[0..n]) |ev| {
                const msg = midi2.decode(ev.toUmp(0));
                switch (msg) {
                    .note_on => |no| engine.pushNote(true, audio.noteToFreq(no.note)),
                    .note_off => |no| engine.pushNote(false, audio.noteToFreq(no.note)),
                    else => {},
                }
            }
        }

        // pull live audio state into the UI before drawing (playhead follows the
        // actual sample position; meters follow the master output peak AND the
        // live input peak, so the mic moves them too — capture routed into engine)
        state.audio_active = engine.isLive();
        if (state.audio_active) {
            state.playhead = engine.playheadNorm();
            state.audio_level = @max(engine.getPeak(), engine.getInputPeak());
        }

        const action = view.frame(&p, bar, &state, @floatFromInt(W), @floatFromInt(H), @floatFromInt(mx), @floatFromInt(my), down, rclick);
        rclick = false;

        // push UI transport/gain decisions (e.g. the play/pause button) to the engine
        engine.setPlaying(state.playing);
        engine.setGain(state.master_gain);
        switch (action) {
            .none => {},
            .close => elapsed = secs,
            .minimize => window.minimize(),
            .maximize => window.toggleMaximize(),
            .move => window.startMoveResize(win.MOVE, lrx, lry),
        }

        // custom cursors (CSS-`cursor` equivalent): resize arrows on the window
        // edges, grabbing while dragging a control, hand over anything clickable.
        const cshape: win.CursorShape = blk: {
            if (!down) {
                if (edgeDir(mx, my, @intCast(W), @intCast(H))) |dir| break :blk switch (dir) {
                    win.RESIZE_LEFT, win.RESIZE_RIGHT => .resize_h,
                    win.RESIZE_TOP, win.RESIZE_BOTTOM => .resize_v,
                    win.RESIZE_TOPLEFT, win.RESIZE_BOTTOMRIGHT => .resize_nwse,
                    win.RESIZE_TOPRIGHT, win.RESIZE_BOTTOMLEFT => .resize_nesw,
                    else => .default,
                };
            }
            if (view.u.active != 0) break :blk .grabbing;
            if (view.u.hot != 0) break :blk .hand;
            break :blk .default;
        };
        window.setCursor(cshape);

        window.swapBuffers();
        std.time.sleep(16 * std.time.ns_per_ms);
        elapsed += 0.016;
    }
    std.debug.print("closed\n", .{});
}
