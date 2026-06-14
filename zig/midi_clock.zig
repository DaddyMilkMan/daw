//! midi_clock.zig — MIDI beat-clock + transport follower. Feed it the System
//! Real Time messages (clock/start/continue/stop) with a wall-clock timestamp and
//! it tracks transport state and estimates external tempo from the 24-ppqn pulse
//! train. Pure (timestamps are passed in), so it unit-tests without a real clock.

const std = @import("std");
const midi2 = @import("midi2.zig");

pub const Transport = enum { stopped, running };

pub const ClockSync = struct {
    transport: Transport = .stopped,
    bpm: f32 = 120.0,
    locked: bool = false, // true once we have a stable tempo estimate
    last_ns: i128 = 0,
    have_last: bool = false,
    ema_spp: f32 = 0, // smoothed seconds-per-pulse (24 ppqn)

    /// Feed a System message. `now_ns` is a monotonic wall-clock timestamp in
    /// nanoseconds. Returns the new transport state when it CHANGES (so the caller
    /// can start/stop the engine), else null.
    pub fn onSystem(self: *ClockSync, status: u8, now_ns: i128) ?Transport {
        switch (status) {
            midi2.System.timing_clock => {
                self.onClock(now_ns);
                return null;
            },
            midi2.System.start, midi2.System.cont => {
                self.have_last = false; // restart the tempo estimate window
                if (status == midi2.System.start) self.locked = false;
                if (self.transport != .running) {
                    self.transport = .running;
                    return .running;
                }
                return null;
            },
            midi2.System.stop => {
                if (self.transport != .stopped) {
                    self.transport = .stopped;
                    return .stopped;
                }
                return null;
            },
            else => return null,
        }
    }

    /// One 24-ppqn clock pulse at `now_ns`; refine the tempo estimate.
    pub fn onClock(self: *ClockSync, now_ns: i128) void {
        if (self.have_last) {
            const dt_ns: f64 = @floatFromInt(now_ns - self.last_ns);
            if (dt_ns > 0) {
                const spp: f32 = @floatCast(dt_ns / 1_000_000_000.0);
                // EMA smoothing; seed on the first interval
                self.ema_spp = if (self.ema_spp == 0) spp else self.ema_spp * 0.8 + spp * 0.2;
                // 24 pulses per quarter note: bpm = 60 / (24 * seconds_per_pulse)
                const bpm = 60.0 / (24.0 * self.ema_spp);
                if (bpm > 20.0 and bpm < 400.0) {
                    self.bpm = bpm;
                    self.locked = true;
                }
            }
        }
        self.last_ns = now_ns;
        self.have_last = true;
    }
};

test "clock pulses estimate tempo (120 BPM)" {
    var c = ClockSync{};
    _ = c.onSystem(midi2.System.start, 0);
    try std.testing.expectEqual(Transport.running, c.transport);
    // 120 BPM -> 0.5 s/quarter -> 0.5/24 s/pulse ~= 20.8333 ms
    const step_ns: i128 = 20_833_333;
    var t: i128 = 0;
    var i: usize = 0;
    while (i < 64) : (i += 1) {
        t += step_ns;
        c.onClock(t);
    }
    try std.testing.expect(c.locked);
    try std.testing.expect(@abs(c.bpm - 120.0) < 1.0);
}

test "start and stop toggle transport once" {
    var c = ClockSync{};
    try std.testing.expectEqual(@as(?Transport, .running), c.onSystem(midi2.System.start, 0));
    try std.testing.expectEqual(@as(?Transport, null), c.onSystem(midi2.System.start, 1)); // already running
    try std.testing.expectEqual(@as(?Transport, .stopped), c.onSystem(midi2.System.stop, 2));
    try std.testing.expectEqual(@as(?Transport, .running), c.onSystem(midi2.System.cont, 3));
}
