//! mixer.zig — a minimal mixing stage: mono track buffers → per-track gain +
//! constant-power pan → summed interleaved stereo master. The seed of the
//! routing graph (buses/sends come later).

const std = @import("std");

pub const Track = struct {
    gain: f32 = 1.0,
    pan: f32 = 0.0, // -1 = hard left, 0 = center, +1 = hard right
    mute: bool = false,
};

pub const Mixer = struct {
    master_gain: f32 = 1.0,

    /// Mix N mono track buffers into one interleaved stereo buffer.
    /// `out.len` must be `2 * frames`; each `mono_bufs[i]` has `frames` samples.
    pub fn mixToStereo(self: *Mixer, tracks: []const Track, mono_bufs: []const []const f32, out: []f32) void {
        const frames = out.len / 2;
        @memset(out, 0.0);

        for (tracks, mono_bufs) |t, buf| {
            if (t.mute) continue;
            // constant-power pan: -1→(1,0), 0→(.707,.707), +1→(0,1)
            const angle: f32 = (t.pan * 0.5 + 0.5) * (std.math.pi / 2.0);
            const lg = @cos(angle) * t.gain;
            const rg = @sin(angle) * t.gain;
            var i: usize = 0;
            while (i < frames and i < buf.len) : (i += 1) {
                const s = buf[i];
                out[2 * i] += s * lg;
                out[2 * i + 1] += s * rg;
            }
        }

        for (out) |*v| v.* = std.math.clamp(v.* * self.master_gain, -1.0, 1.0);
    }
};

test "pan law: hard left/right and centered power" {
    var m = Mixer{};
    const mono = [_]f32{ 1.0, 1.0 };
    const bufs = [_][]const f32{&mono};
    var out: [4]f32 = undefined;

    m.mixToStereo(&[_]Track{.{ .pan = -1.0 }}, &bufs, &out);
    try std.testing.expectApproxEqAbs(@as(f32, 1.0), out[0], 1e-5); // L
    try std.testing.expectApproxEqAbs(@as(f32, 0.0), out[1], 1e-5); // R

    m.mixToStereo(&[_]Track{.{ .pan = 1.0 }}, &bufs, &out);
    try std.testing.expectApproxEqAbs(@as(f32, 0.0), out[0], 1e-5);
    try std.testing.expectApproxEqAbs(@as(f32, 1.0), out[1], 1e-5);
}
