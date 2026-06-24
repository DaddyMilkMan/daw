//! mix_graph.zig — a real mixing graph: channels (gain/pan/mute/solo) route to
//! the master or a bus, with optional post-fader aux sends; buses sum and fold
//! into the master. Peak/RMS metering at every node. Builds on mixer.zig's
//! single-stage pan law and extends it to buses/sends/metering.
//!
//! Two-stage: channels -> (master | bus), sends -> bus; then buses -> master.

const std = @import("std");

pub const Meter = struct { peak: f32 = 0, rms: f32 = 0 };

/// A post-fader aux send to a bus.
pub const Send = struct { bus: u32, gain: f32 };

pub const Channel = struct {
    gain: f32 = 1.0,
    pan: f32 = 0.0, // -1 L .. +1 R (constant power)
    mute: bool = false,
    solo: bool = false,
    dest: i32 = -1, // -1 = master; >=0 = bus index (main routing)
    send: ?Send = null, // optional post-fader aux send
};

pub const Bus = struct {
    gain: f32 = 1.0,
    pan: f32 = 0.0, // balance
    mute: bool = false,
};

fn panGains(pan: f32, gain: f32) [2]f32 {
    const angle = (pan * 0.5 + 0.5) * (std.math.pi / 2.0);
    return .{ @cos(angle) * gain, @sin(angle) * gain };
}

fn computeMeter(buf: []const f32) Meter {
    var peak: f32 = 0;
    var sum: f64 = 0;
    for (buf) |s| {
        const a = @abs(s);
        if (a > peak) peak = a;
        sum += @as(f64, s) * @as(f64, s);
    }
    const rms: f32 = if (buf.len == 0) 0 else @floatCast(@sqrt(sum / @as(f64, @floatFromInt(buf.len))));
    return .{ .peak = peak, .rms = rms };
}

pub const MixGraph = struct {
    master_gain: f32 = 1.0,

    /// Mix mono channel buffers through the routing graph into `out_master`
    /// (interleaved stereo). `bus_bufs[j]` is a stereo scratch buffer per bus
    /// (each `out_master.len` long). Meter arrays are optional (`null` to skip).
    pub fn mix(
        self: MixGraph,
        channels: []const Channel,
        mono_bufs: []const []const f32,
        buses: []const Bus,
        bus_bufs: []const []f32,
        out_master: []f32,
        ch_meters: ?[]Meter,
        bus_meters: ?[]Meter,
        master_meter: ?*Meter,
    ) void {
        const frames = out_master.len / 2;
        @memset(out_master, 0);
        for (bus_bufs) |bb| @memset(bb, 0);

        var any_solo = false;
        for (channels) |c| {
            if (c.solo) any_solo = true;
        }

        for (channels, mono_bufs, 0..) |c, buf, i| {
            const active = !c.mute and !(any_solo and !c.solo);
            if (ch_meters) |m| {
                if (i < m.len) {
                    var mt = computeMeter(buf[0..@min(buf.len, frames)]);
                    mt.peak *= c.gain;
                    mt.rms *= c.gain;
                    m[i] = if (active) mt else Meter{};
                }
            }
            if (!active) continue;

            const g = panGains(c.pan, c.gain);
            const main: []f32 = if (c.dest >= 0 and @as(usize, @intCast(c.dest)) < bus_bufs.len)
                bus_bufs[@intCast(c.dest)]
            else
                out_master;

            var k: usize = 0;
            while (k < frames and k < buf.len) : (k += 1) {
                const l = buf[k] * g[0];
                const r = buf[k] * g[1];
                main[2 * k] += l;
                main[2 * k + 1] += r;
                if (c.send) |s| {
                    if (s.bus < bus_bufs.len) {
                        bus_bufs[s.bus][2 * k] += l * s.gain;
                        bus_bufs[s.bus][2 * k + 1] += r * s.gain;
                    }
                }
            }
        }

        // fold buses into the master
        for (buses, 0..) |b, j| {
            if (j >= bus_bufs.len) break;
            if (bus_meters) |m| {
                if (j < m.len) m[j] = if (b.mute) Meter{} else computeMeter(bus_bufs[j]);
            }
            if (b.mute) continue;
            const bl: f32 = (if (b.pan > 0) 1.0 - b.pan else 1.0) * b.gain;
            const br: f32 = (if (b.pan < 0) 1.0 + b.pan else 1.0) * b.gain;
            var k: usize = 0;
            while (k < frames) : (k += 1) {
                out_master[2 * k] += bus_bufs[j][2 * k] * bl;
                out_master[2 * k + 1] += bus_bufs[j][2 * k + 1] * br;
            }
        }

        for (out_master) |*v| v.* *= self.master_gain;
        if (master_meter) |mm| mm.* = computeMeter(out_master);
    }
};

// ===========================================================================
// Tests
// ===========================================================================
const expectApproxEqAbs = std.testing.expectApproxEqAbs;
const C = std.math.sqrt1_2; // 0.7071 — centered constant-power pan gain

test "channel routes to master with constant-power pan" {
    var g = MixGraph{};
    const mono = [_]f32{ 1.0, 1.0 };
    const bufs = [_][]const f32{&mono};
    var out: [4]f32 = undefined;
    g.mix(&[_]Channel{.{}}, &bufs, &.{}, &.{}, &out, null, null, null);
    try expectApproxEqAbs(@as(f32, C), out[0], 1e-4);
    try expectApproxEqAbs(@as(f32, C), out[1], 1e-4);
}

test "channel routed through a bus is scaled by the bus gain" {
    var g = MixGraph{};
    const mono = [_]f32{1.0};
    const bufs = [_][]const f32{&mono};
    var bus0: [2]f32 = undefined;
    const bus_bufs = [_][]f32{&bus0};
    var out: [2]f32 = undefined;
    // route channel to bus 0; bus 0 at half gain
    g.mix(&[_]Channel{.{ .dest = 0 }}, &bufs, &[_]Bus{.{ .gain = 0.5 }}, &bus_bufs, &out, null, null, null);
    try expectApproxEqAbs(@as(f32, C * 0.5), out[0], 1e-4); // pan-center * bus 0.5
}

test "post-fader aux send adds to a bus on top of the direct signal" {
    var g = MixGraph{};
    const mono = [_]f32{1.0};
    const bufs = [_][]const f32{&mono};
    var bus0: [2]f32 = undefined;
    const bus_bufs = [_][]f32{&bus0};
    var out: [2]f32 = undefined;
    // channel -> master directly, AND a 0.5 send to bus 0 (bus gain 1)
    g.mix(&[_]Channel{.{ .dest = -1, .send = .{ .bus = 0, .gain = 0.5 } }}, &bufs, &[_]Bus{.{}}, &bus_bufs, &out, null, null, null);
    // master = direct (C) + bus (C*0.5) = C*1.5
    try expectApproxEqAbs(@as(f32, C * 1.5), out[0], 1e-4);
}

test "solo isolates, mute silences" {
    var g = MixGraph{};
    const a = [_]f32{1.0};
    const b = [_]f32{1.0};
    const bufs = [_][]const f32{ &a, &b };
    var out: [2]f32 = undefined;

    // ch0 soloed -> only ch0 contributes
    g.mix(&[_]Channel{ .{ .solo = true }, .{} }, &bufs, &.{}, &.{}, &out, null, null, null);
    try expectApproxEqAbs(@as(f32, C), out[0], 1e-4); // one channel only

    // both active -> sum of two
    g.mix(&[_]Channel{ .{}, .{} }, &bufs, &.{}, &.{}, &out, null, null, null);
    try expectApproxEqAbs(@as(f32, 2 * C), out[0], 1e-4);

    // ch0 muted -> only ch1
    g.mix(&[_]Channel{ .{ .mute = true }, .{} }, &bufs, &.{}, &.{}, &out, null, null, null);
    try expectApproxEqAbs(@as(f32, C), out[0], 1e-4);
}

test "metering reports peak + rms at channel and master" {
    var g = MixGraph{};
    // a full-scale square-ish signal: peak 1, rms 1
    const mono = [_]f32{ 1.0, -1.0, 1.0, -1.0 };
    const bufs = [_][]const f32{&mono};
    var out: [8]f32 = undefined;
    var ch_meters: [1]Meter = undefined;
    var master_meter: Meter = undefined;
    g.mix(&[_]Channel{.{ .gain = 0.5, .pan = 0 }}, &bufs, &.{}, &.{}, &out, &ch_meters, null, &master_meter);
    // channel post-fader (gain 0.5): peak 0.5, rms 0.5
    try expectApproxEqAbs(@as(f32, 0.5), ch_meters[0].peak, 1e-4);
    try expectApproxEqAbs(@as(f32, 0.5), ch_meters[0].rms, 1e-4);
    // master (center pan, both channels equal): peak ~ 0.5*C
    try expectApproxEqAbs(@as(f32, 0.5 * C), master_meter.peak, 1e-4);
}
