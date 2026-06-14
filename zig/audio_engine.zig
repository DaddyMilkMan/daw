//! audio_engine.zig — the live DAW's real-time audio thread.
//!
//! Opens the OS output stream (ALSA -> PipeWire) and continuously renders audio
//! on its own thread, paced by the audio clock (the blocking `writeBlock` is the
//! pacer — no sleeps). It loops the project buffer while transport is playing and
//! feeds silence while stopped (so the device never underruns). The UI thread and
//! audio thread share state through atomics only: the UI pushes `playing`/`gain`,
//! the audio thread publishes the sample-accurate `pos` (playhead) and master
//! `peak` (meters). No locks on the audio path.

const std = @import("std");
const alsa = @import("audio_alsa.zig");

pub const Engine = struct {
    samples: []const f32, // mono source loop (read-only once started)
    rate: u32 = 48000,
    channels: u16 = 2,
    device: [*:0]const u8 = "default",

    // --- shared, accessed via atomics from both threads ---
    playing: bool = false,
    quit: bool = false,
    started: bool = false,
    gain_bits: u32 = @bitCast(@as(f32, 0.8)),
    pos: u64 = 0, // current frame index into `samples`
    peak_bits: u32 = 0, // master peak as bitcast f32

    thread: ?std.Thread = null,

    pub fn start(self: *Engine) void {
        self.thread = std.Thread.spawn(.{}, run, .{self}) catch null;
    }
    pub fn stop(self: *Engine) void {
        @atomicStore(bool, &self.quit, true, .seq_cst);
        if (self.thread) |t| t.join();
        self.thread = null;
    }

    pub fn setPlaying(self: *Engine, p: bool) void {
        @atomicStore(bool, &self.playing, p, .monotonic);
    }
    pub fn setGain(self: *Engine, g: f32) void {
        @atomicStore(u32, &self.gain_bits, @bitCast(g), .monotonic);
    }
    /// True once the output device opened successfully (so the UI knows audio is live).
    pub fn isLive(self: *Engine) bool {
        return @atomicLoad(bool, &self.started, .monotonic);
    }
    /// Playhead position, normalized 0..1 over the loop.
    pub fn playheadNorm(self: *Engine) f32 {
        if (self.samples.len == 0) return 0;
        const p = @atomicLoad(u64, &self.pos, .monotonic);
        return @as(f32, @floatFromInt(p % self.samples.len)) / @as(f32, @floatFromInt(self.samples.len));
    }
    pub fn getPeak(self: *Engine) f32 {
        return @bitCast(@atomicLoad(u32, &self.peak_bits, .monotonic));
    }

    fn run(self: *Engine) void {
        var out = alsa.StreamOut.open(self.device, self.rate, self.channels, 40_000) catch return;
        defer out.close();
        @atomicStore(bool, &self.started, true, .monotonic);

        const N = 256; // frames per block (~5 ms @ 48k)
        var buf: [N * 2]i16 = undefined;
        var pos: usize = 0;
        const n = self.samples.len;

        while (!@atomicLoad(bool, &self.quit, .monotonic)) {
            const playing = @atomicLoad(bool, &self.playing, .monotonic);
            const gain: f32 = @bitCast(@atomicLoad(u32, &self.gain_bits, .monotonic));
            var pk: f32 = 0;
            var f: usize = 0;
            while (f < N) : (f += 1) {
                var s: f32 = 0;
                if (playing and n > 0) {
                    s = self.samples[pos] * gain;
                    pos += 1;
                    if (pos >= n) pos = 0;
                }
                const a = @abs(s);
                if (a > pk) pk = a;
                const v: i16 = @intFromFloat(std.math.clamp(s, -1.0, 1.0) * 32767.0);
                buf[f * 2] = v; // L
                buf[f * 2 + 1] = v; // R
            }
            out.writeBlock(&buf) catch {};
            @atomicStore(u64, &self.pos, pos, .monotonic);
            // publish a lightly-smoothed master peak for the meters (fast attack)
            const prev: f32 = @bitCast(@atomicLoad(u32, &self.peak_bits, .monotonic));
            const next = if (pk > prev) pk else prev * 0.82 + pk * 0.18;
            @atomicStore(u32, &self.peak_bits, @bitCast(next), .monotonic);
        }
    }
};
