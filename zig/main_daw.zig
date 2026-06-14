//! main_daw.zig — the live Zenith DAW. One cohesive app on our own toolkit:
//! Trellis layout, GPU widgets, SDF/shadow/atlas-text rendering, frosted glass.
//! Borderless, resizable, full input.

const std = @import("std");

/// Route every std.log line to the app console (file + stderr). See log.zig.
pub const std_options: std.Options = .{ .log_level = .debug, .logFn = @import("log.zig").logFn };
const elog = std.log.scoped(.daw);

const win = @import("window_glx.zig");
const gpu2d = @import("gpu2d.zig");
const daw = @import("daw.zig");
const audio = @import("audio_engine.zig");
const ainspect = @import("audio_inspect.zig");
const effects = @import("effects.zig");
const midi = @import("midi_alsa.zig");
const midi2 = @import("midi2.zig");
const midi2_alsa = @import("midi2_alsa.zig");
const midi_clock = @import("midi_clock.zig");

/// Route a decoded MIDI 2.0 message into the engine: notes -> synth voices, and
/// the high-resolution (32-bit) controllers -> synth parameters. CC74 (the de
/// facto brightness/MPE timbre) drives the filter cutoff, CC71 resonance, the
/// 32-bit pitch bend bends pitch (±2 semitones), channel pressure opens the
/// filter. 16-bit velocity 0 on note-on is a note-off (the MIDI convention).
fn routeMidi(engine: *audio.Engine, msg: midi2.Message, clock: *midi_clock.ClockSync) ?midi_clock.Transport {
    switch (msg) {
        .note_on => |no| {
            const on = no.velocity > 0;
            engine.pushNote(on, audio.noteToFreq(no.note), audio.vel16(no.velocity));
            elog.debug("midi note {s} {d} vel {d}", .{ if (on) "ON" else "OFF", no.note, no.velocity });
        },
        .note_off => |no| {
            engine.pushNote(false, audio.noteToFreq(no.note), 0);
            elog.debug("midi note OFF {d}", .{no.note});
        },
        .control_change => |cc| switch (cc.index) {
            1 => engine.setMod(audio.ccToUnit(cc.value)), // mod wheel -> vibrato
            7, 11 => engine.setExpression(audio.ccToUnit(cc.value)), // volume / expression
            64 => engine.setSustain(cc.value >= 0x4000_0000), // sustain pedal (>=64)
            71 => engine.setResonance(audio.ccToUnit(cc.value)),
            74 => engine.setCutoff(audio.ccToCutoff(cc.value)),
            else => elog.debug("midi cc {d} = {d}", .{ cc.index, cc.value }),
        },
        .pitch_bend => |pb| engine.setBend(audio.bendToRatio(pb.value, 2.0)),
        .channel_pressure => |cp| engine.setPressure(audio.ccToUnit(cp.value)),
        .program_change => |pc| elog.debug("midi program change -> {d}", .{pc.program}),
        .system => |sys| {
            const tr = clock.onSystem(sys.status, std.time.nanoTimestamp());
            if (tr) |t| elog.info("midi transport: {s} (ext tempo {d:.1} BPM)", .{ @tagName(t), clock.bpm });
            return tr;
        },
        else => {},
    }
    return null;
}

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
        elog.err("gpu2d init failed: {any}", .{e});
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

    // real-time audio: per-track mixer STEMS rendered to the OS device on its own
    // thread. The UI pushes transport + per-track gain/pan/mute/solo; the engine
    // mixes the stems and publishes per-track + master levels back to the meters.
    const loop_n: usize = if (view.wave.len > 0) view.wave.len else 96000;
    const ntr = @min(p.tracks.items.len, audio.MAXTRACKS);
    const stems = daw.synthStems(a, 48000, loop_n, ntr) catch &[_][]f32{};
    var engine = audio.Engine{ .samples = view.wave, .rate = 48000 };
    engine.stems = stems;
    engine.ntracks = if (stems.len > 0) ntr else 0;
    // analyze each stem once (dominant frequency) for the "what's playing" monitor
    var stem_hz: [audio.MAXTRACKS]f32 = [_]f32{0} ** audio.MAXTRACKS;
    for (0..ntr) |ti| stem_hz[ti] = ainspect.analyze(a, stems[ti], 48000).dominant_hz;
    // shared aux reverb fed by each channel's send knob (per-track FX chains).
    // Robust: if init fails the engine just runs without the send return (null-safe).
    var reverb_opt: ?effects.Reverb = effects.Reverb.init(a, 0.62, 0.4) catch null;
    if (reverb_opt) |*rv| {
        rv.mix = 1.0; // 100%-wet send return; the dry path bypasses it
        engine.reverb = rv;
    }
    defer if (reverb_opt) |*rv| rv.deinit();
    engine.start();
    defer engine.stop();
    engine.setPlaying(state.playing);
    // the piano-roll previews notes through the engine synth
    view.pr.audition = auditionSynth;
    view.pr.audition_ctx = &engine;

    // MIDI 2.0 input. Prefer a NATIVE UMP MIDI 2.0 client (the kernel delivers
    // genuine Universal MIDI Packets and translates legacy senders to MIDI 2.0
    // for us). Fall back to a legacy MIDI 1.0 client + our own up-conversion if
    // the kernel/alsa-lib lacks UMP support.
    // native MIDI 2.0 UMP OUTPUT — Zenith as a MIDI 2.0 source other apps can read
    var midi_out: ?midi2_alsa.Midi2Output = midi2_alsa.Midi2Output.open("Zenith DAW", "Zenith Out") catch null;
    defer if (midi_out) |*m| m.close();
    const out_client: c_int = if (midi_out) |o| o.client else -1; // input excludes this (no loop)
    var midi2_in: ?midi2_alsa.Midi2Input = midi2_alsa.Midi2Input.open("Zenith DAW", "Zenith In", out_client) catch null;
    defer if (midi2_in) |*m| m.close();
    var midi_in: ?midi.MidiInput = if (midi2_in == null) (midi.MidiInput.open("Zenith DAW", "Zenith In") catch null) else null;
    defer if (midi_in) |*m| m.close();
    var ump_buf: [64]midi2.Ump = undefined;
    var midi_evs: [64]midi.MidiEvent = undefined;
    var clock = midi_clock.ClockSync{}; // external MIDI beat-clock + transport follower
    if (midi2_in) |*m| {
        const n = m.connectAllSources();
        elog.info("MIDI 2.0 (native UMP): auto-connected {d} source(s); hot-plug on", .{n});
    } else if (midi_in) |*m| {
        const n = m.connectAllSources();
        elog.info("MIDI (legacy 1.0 -> UMP): auto-connected {d} source(s); hot-plug on", .{n});
    }

    elog.info("Zenith DAW started — Trellis + glass + GPU + live audio + MIDI 2.0", .{});

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
    var prev_playing = state.playing;
    var ft = std.time.Timer.start() catch null;
    var frames: u64 = 0;
    var render_ns: u64 = 0;

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
                .key => |k| {
                    if (k == 9) { // Esc: close the editor if open, else quit
                        if (state.editing) state.editing = false else elapsed = secs;
                    }
                    if (k == 65) state.playing = !state.playing; // Space: transport
                    if (k == 26) { // 'E': toggle the piano-roll on the selected clip
                        state.editing = !state.editing;
                        if (state.editing) {
                            state.edit_track = @intCast(@max(state.sel_track, 0));
                            state.edit_clip = 0;
                        }
                        const nc: usize = if (state.edit_track < p.tracks.items.len) p.tracks.items[state.edit_track].clips.items.len else 0;
                        elog.info("editor {s} -> track {d} ({d} clips)", .{ if (state.editing) "OPEN" else "CLOSE", state.edit_track, nc });
                    }
                },
                .expose => {},
            }
        }

        // route MIDI in -> engine synth (notes + high-res controllers), and echo
        // every packet OUT as native MIDI 2.0 UMP (Zenith as a MIDI 2.0 source).
        // Native path: the kernel hands us real UMP. Legacy path: up-convert here.
        if (midi2_in) |*m| {
            const n = m.poll(&ump_buf);
            for (ump_buf[0..n]) |ump| {
                if (routeMidi(&engine, midi2.decode(ump), &clock)) |t| state.playing = (t == .running);
                if (midi_out) |*o| o.send(ump);
            }
        } else if (midi_in) |*m| {
            const n = m.poll(&midi_evs);
            for (midi_evs[0..n]) |ev| {
                const ump = ev.toUmp(0);
                if (routeMidi(&engine, midi2.decode(ump), &clock)) |t| state.playing = (t == .running);
                if (midi_out) |*o| o.send(ump);
            }
        }

        // pull live audio state into the UI before drawing (playhead follows the
        // actual sample position; meters follow the master output peak AND the
        // live input peak, so the mic moves them too — capture routed into engine)
        state.audio_active = engine.isLive();
        if (state.audio_active) {
            state.playhead = engine.playheadNorm();
            state.audio_level = @max(engine.getPeak(), engine.getInputPeak());
            for (0..ntr) |ti| state.track_levels[ti] = engine.getTrackLevel(ti);
        }

        const action = view.frame(&p, bar, &state, @floatFromInt(W), @floatFromInt(H), @floatFromInt(mx), @floatFromInt(my), down, rclick);

        // push the live mixer state (per-track gain/pan/mute/solo) to the engine
        for (0..ntr) |ti| {
            engine.setTrackGain(ti, p.tracks.items[ti].gain);
            engine.setTrackPan(ti, p.tracks.items[ti].pan);
            engine.setTrackMute(ti, state.mutes[ti]);
            engine.setTrackSolo(ti, state.solos[ti]);
            engine.setTrackSend(ti, state.sends[ti][0]); // send-A knob -> reverb bus
        }
        // play the edited clip's notes through the synth (live, RT-safe double buffer)
        if (state.editing and state.edit_track < p.tracks.items.len and state.edit_clip < p.tracks.items[state.edit_track].clips.items.len) {
            engine.setSequence(p.tracks.items[state.edit_track].clips.items[state.edit_clip].notes.items, bar * 4);
        } else {
            engine.setSequence(&.{}, 0);
        }
        rclick = false;

        // push UI transport/gain decisions (e.g. the play/pause button) to the engine
        if (state.playing != prev_playing) {
            elog.info("transport: {s} (playhead {d:.2})", .{ if (state.playing) "PLAY" else "STOP", state.playhead });
            prev_playing = state.playing;
        }
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

        g.grain(W, H, 0.014); // subtle film grain — modern premium finish, kills banding
        window.swapBuffers();

        // frame timing: log average render cost (excludes the pacing sleep) every ~120 frames
        if (ft) |*t| render_ns += t.lap();
        frames += 1;
        if (frames % 120 == 0) {
            elog.info("perf: {d} frames, render avg {d:.2}ms (~{d:.0} fps headroom)", .{ frames, @as(f64, @floatFromInt(render_ns / 120)) / 1e6, 1e9 / @as(f64, @floatFromInt(@max(render_ns / 120, 1))) });
            render_ns = 0;

            // audio monitor: what's playing, where from, and the numbers
            var any_solo = false;
            for (0..ntr) |ti| {
                if (state.solos[ti]) any_solo = true;
            }
            const mp = engine.getPeak();
            const rl = engine.getReverbLevel();
            elog.info("audio: device '{s}' @ {d}Hz {d}ch | transport {s} | master {d:.0}% ({d:.1}dB) peak {d:.3} | synth {d:.3} | limiter GR {d:.1}dB | reverb-bus {d:.3} ({d:.1}dB)", .{ engine.device_opened, engine.rate, engine.channels, if (state.playing) "PLAYING" else "STOPPED", state.master_gain * 100, ainspect.dbFromLinear(state.master_gain), mp, engine.getSynthLevel(), engine.getLimiterGrDb(), rl, ainspect.dbFromLinear(rl) });
            for (0..ntr) |ti| {
                const gn = p.tracks.items[ti].gain;
                const lvl = state.track_levels[ti];
                const active = !state.mutes[ti] and (!any_solo or state.solos[ti]);
                const status = if (active) "PLAYING" else if (state.mutes[ti]) "muted" else "(silenced by solo)";
                elog.info("  src[{d}] {s}: vol {d:.2} ({d:.1}dB) pan {d:.2} send {d:.2} | FX[hp+comp] live {d:.3} ({d:.1}dB) | ~{d:.0}Hz | {s}", .{ ti, p.tracks.items[ti].name.items, gn, ainspect.dbFromLinear(gn), p.tracks.items[ti].pan, state.sends[ti][0], lvl, ainspect.dbFromLinear(lvl), stem_hz[ti], status });
            }
        }
        std.time.sleep(16 * std.time.ns_per_ms);
        if (ft) |*t| _ = t.lap(); // discard the sleep interval
        elapsed += 0.016;
    }
    elog.info("Zenith DAW closed", .{});
}

/// The piano-roll's audition hook: preview an edited note on the engine synth.
fn auditionSynth(ctx: ?*anyopaque, pitch: u8, on: bool) void {
    const eng: *audio.Engine = @ptrCast(@alignCast(ctx orelse return));
    eng.pushNote(on, audio.noteToFreq(@intCast(pitch)), 0.85); // audition at a firm velocity
}
