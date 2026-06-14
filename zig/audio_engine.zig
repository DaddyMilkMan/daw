//! audio_engine.zig — the live DAW's real-time audio engine (output + input).
//!
//! Output thread: opens the OS output (ALSA -> PipeWire) and renders the project
//! loop PLUS a polyphonic synth fed by incoming MIDI, paced by the audio clock
//! (blocking writeBlock, no sleeps). Input thread: opens the default capture and
//! publishes a live input peak (so the engine "hears" the mic). UI <-> audio
//! communicate only through atomics + one lock-free SPSC note queue — no locks on
//! the audio path. The UI pushes playing/gain and MIDI notes; the engine
//! publishes the sample-accurate playhead, master peak, and input peak.

const std = @import("std");
const alsa = @import("audio_alsa.zig");
const synth = @import("synth.zig");

const RING = 512; // note-event queue capacity (power-of-two not required)
const NoteEv = struct { on: bool, freq: f32 };

// Try these output/input devices in order, so audio works on any system — a
// PipeWire/Pulse desktop or bare ALSA hardware of any age. `default` first
// (whatever the user's chosen device is), then explicit servers, then raw HW.
const DEVICE_FALLBACKS = [_][*:0]const u8{ "default", "pipewire", "pulse", "plughw:0,0", "hw:0,0" };

fn openOut(preferred: [*:0]const u8, rate: u32, channels: u16) ?alsa.StreamOut {
    if (alsa.StreamOut.open(preferred, rate, channels, 40_000)) |s| return s else |_| {}
    for (DEVICE_FALLBACKS) |dev| {
        if (alsa.StreamOut.open(dev, rate, channels, 40_000)) |s| return s else |_| {}
    }
    return null;
}
fn openIn(preferred: [*:0]const u8, rate: u32) ?alsa.StreamIn {
    if (alsa.StreamIn.open(preferred, rate, 1, 60_000)) |s| return s else |_| {}
    for (DEVICE_FALLBACKS) |dev| {
        if (alsa.StreamIn.open(dev, rate, 1, 60_000)) |s| return s else |_| {}
    }
    return null;
}

pub const Engine = struct {
    samples: []const f32, // mono source loop (read-only once started)
    rate: u32 = 48000,
    channels: u16 = 2,
    device: [*:0]const u8 = "default",
    capture: bool = true, // also open the input device and meter it

    // --- shared, accessed via atomics from both threads ---
    playing: bool = false,
    quit: bool = false,
    started: bool = false,
    gain_bits: u32 = @bitCast(@as(f32, 0.8)),
    pos: u64 = 0, // current frame index into `samples`
    peak_bits: u32 = 0, // master output peak (bitcast f32)
    in_peak_bits: u32 = 0, // live input peak (bitcast f32)
    // high-res MIDI 2.0 controllers (latest-value-wins, lock-free)
    ctl_cutoff_bits: u32 = @bitCast(@as(f32, 2200.0)), // CC74 -> filter cutoff Hz
    ctl_res_bits: u32 = @bitCast(@as(f32, 0.25)), // CC71 -> resonance 0..1
    ctl_bend_bits: u32 = @bitCast(@as(f32, 1.0)), // pitch bend -> freq multiplier
    ctl_press_bits: u32 = @bitCast(@as(f32, 0.0)), // channel pressure -> brightness

    // --- SPSC note queue: UI/MIDI thread produces, audio thread consumes ---
    events: [RING]NoteEv = undefined,
    ev_head: usize = 0, // producer (next write)
    ev_tail: usize = 0, // consumer (next read)

    synth: synth.Synth = .{},
    out_thread: ?std.Thread = null,
    in_thread: ?std.Thread = null,

    pub fn start(self: *Engine) void {
        self.synth.sample_rate = @floatFromInt(self.rate);
        self.out_thread = std.Thread.spawn(.{}, runOut, .{self}) catch null;
        if (self.capture) self.in_thread = std.Thread.spawn(.{}, runIn, .{self}) catch null;
    }
    pub fn stop(self: *Engine) void {
        @atomicStore(bool, &self.quit, true, .seq_cst);
        if (self.out_thread) |t| t.join();
        if (self.in_thread) |t| t.join();
        self.out_thread = null;
        self.in_thread = null;
    }

    pub fn setPlaying(self: *Engine, p: bool) void {
        @atomicStore(bool, &self.playing, p, .monotonic);
    }
    pub fn setGain(self: *Engine, g: f32) void {
        @atomicStore(u32, &self.gain_bits, @bitCast(g), .monotonic);
    }
    pub fn isLive(self: *Engine) bool {
        return @atomicLoad(bool, &self.started, .monotonic);
    }
    pub fn playheadNorm(self: *Engine) f32 {
        if (self.samples.len == 0) return 0;
        const p = @atomicLoad(u64, &self.pos, .monotonic);
        return @as(f32, @floatFromInt(p % self.samples.len)) / @as(f32, @floatFromInt(self.samples.len));
    }
    pub fn getPeak(self: *Engine) f32 {
        return @bitCast(@atomicLoad(u32, &self.peak_bits, .monotonic));
    }
    pub fn getInputPeak(self: *Engine) f32 {
        return @bitCast(@atomicLoad(u32, &self.in_peak_bits, .monotonic));
    }

    // high-res controller setters (called from the UI/MIDI thread)
    pub fn setCutoff(self: *Engine, hz: f32) void {
        @atomicStore(u32, &self.ctl_cutoff_bits, @bitCast(hz), .monotonic);
    }
    pub fn setResonance(self: *Engine, r: f32) void {
        @atomicStore(u32, &self.ctl_res_bits, @bitCast(r), .monotonic);
    }
    pub fn setBend(self: *Engine, ratio: f32) void {
        @atomicStore(u32, &self.ctl_bend_bits, @bitCast(ratio), .monotonic);
    }
    pub fn setPressure(self: *Engine, v: f32) void {
        @atomicStore(u32, &self.ctl_press_bits, @bitCast(v), .monotonic);
    }

    /// Queue a note on/off (called from the UI/MIDI thread). Lock-free; drops if full.
    pub fn pushNote(self: *Engine, on: bool, freq: f32) void {
        const h = @atomicLoad(usize, &self.ev_head, .monotonic);
        const next = (h + 1) % RING;
        if (next == @atomicLoad(usize, &self.ev_tail, .acquire)) return; // full
        self.events[h] = .{ .on = on, .freq = freq };
        @atomicStore(usize, &self.ev_head, next, .release);
    }
    fn drainNotes(self: *Engine) void {
        while (true) {
            const t = @atomicLoad(usize, &self.ev_tail, .monotonic);
            if (t == @atomicLoad(usize, &self.ev_head, .acquire)) break;
            const ev = self.events[t];
            @atomicStore(usize, &self.ev_tail, (t + 1) % RING, .release);
            if (ev.on) self.synth.noteOn(ev.freq) else self.synth.noteOff(ev.freq);
        }
    }

    fn runOut(self: *Engine) void {
        var out = openOut(self.device, self.rate, self.channels) orelse return;
        defer out.close();
        @atomicStore(bool, &self.started, true, .monotonic);

        const N = 256;
        var buf: [N * 2]i16 = undefined;
        var sb: [N]f32 = undefined;
        var pos: usize = 0;
        const n = self.samples.len;

        while (!@atomicLoad(bool, &self.quit, .monotonic)) {
            self.drainNotes();
            // apply high-res MIDI 2.0 controllers to the synth for this block
            const base_cut: f32 = @bitCast(@atomicLoad(u32, &self.ctl_cutoff_bits, .monotonic));
            const press: f32 = @bitCast(@atomicLoad(u32, &self.ctl_press_bits, .monotonic));
            self.synth.cutoff = std.math.clamp(base_cut * (1.0 + press * 2.0), 100.0, 16000.0);
            self.synth.resonance = @bitCast(@atomicLoad(u32, &self.ctl_res_bits, .monotonic));
            self.synth.bend = @bitCast(@atomicLoad(u32, &self.ctl_bend_bits, .monotonic));
            self.synth.renderBlock(sb[0..N]); // MIDI-driven voices
            const playing = @atomicLoad(bool, &self.playing, .monotonic);
            const gain: f32 = @bitCast(@atomicLoad(u32, &self.gain_bits, .monotonic));
            var pk: f32 = 0;
            var f: usize = 0;
            while (f < N) : (f += 1) {
                var s: f32 = sb[f]; // synth always sounds (it's note-gated)
                if (playing and n > 0) {
                    s += self.samples[pos] * gain;
                    pos += 1;
                    if (pos >= n) pos = 0;
                }
                s = std.math.clamp(s, -1.0, 1.0);
                const a = @abs(s);
                if (a > pk) pk = a;
                const v: i16 = @intFromFloat(s * 32767.0);
                buf[f * 2] = v;
                buf[f * 2 + 1] = v;
            }
            out.writeBlock(&buf) catch {};
            @atomicStore(u64, &self.pos, pos, .monotonic);
            const prev: f32 = @bitCast(@atomicLoad(u32, &self.peak_bits, .monotonic));
            const nx = if (pk > prev) pk else prev * 0.82 + pk * 0.18;
            @atomicStore(u32, &self.peak_bits, @bitCast(nx), .monotonic);
        }
    }

    fn runIn(self: *Engine) void {
        var in = openIn(self.device, self.rate) orelse return;
        defer in.close();
        const N = 512;
        var buf: [N]i16 = undefined;
        while (!@atomicLoad(bool, &self.quit, .monotonic)) {
            const got = in.readBlock(&buf) catch break;
            var pk: f32 = 0;
            for (buf[0..got]) |s| {
                const a = @abs(@as(f32, @floatFromInt(s)) / 32768.0);
                if (a > pk) pk = a;
            }
            const prev: f32 = @bitCast(@atomicLoad(u32, &self.in_peak_bits, .monotonic));
            const nx = if (pk > prev) pk else prev * 0.80 + pk * 0.20;
            @atomicStore(u32, &self.in_peak_bits, @bitCast(nx), .monotonic);
        }
    }
};

/// MIDI note number -> frequency (A4 = 69 = 440 Hz).
pub fn noteToFreq(note: u7) f32 {
    return 440.0 * std.math.pow(f32, 2.0, (@as(f32, @floatFromInt(note)) - 69.0) / 12.0);
}

// ---- MIDI 2.0 high-res controller mapping (32-bit values) -----------------
fn unit32(v: u32) f32 {
    return @as(f32, @floatFromInt(v)) / 4294967295.0; // 0..1
}
/// CC 32-bit value -> filter cutoff Hz (exponential, ~120 Hz .. 12 kHz).
pub fn ccToCutoff(v: u32) f32 {
    return 120.0 * std.math.pow(f32, 12000.0 / 120.0, unit32(v));
}
/// CC 32-bit value -> 0..1 (resonance, etc.).
pub fn ccToUnit(v: u32) f32 {
    return unit32(v);
}
/// 32-bit pitch bend (center 0x8000_0000) -> frequency multiplier (±`semis`).
pub fn bendToRatio(v: u32, semis: f32) f32 {
    const n = (unit32(v) - 0.5) * 2.0; // -1..+1
    return std.math.pow(f32, 2.0, n * semis / 12.0);
}
