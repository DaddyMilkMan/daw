//! transport.zig — the clock/playhead. Owns tempo and a looping playhead.

pub const Transport = struct {
    sample_rate: f64,
    bpm: f64,
    length_frames: u64, // loop length in samples
    pos: u64 = 0, // loop-relative playhead

    pub fn init(sample_rate: f64, bpm: f64, beats: f64) Transport {
        const secs_per_beat = 60.0 / bpm;
        const length: u64 = @intFromFloat(beats * secs_per_beat * sample_rate);
        return .{ .sample_rate = sample_rate, .bpm = bpm, .length_frames = length };
    }

    /// Advance the playhead by n frames, wrapping at the loop boundary.
    pub fn advance(self: *Transport, n: u64) void {
        self.pos = (self.pos + n) % self.length_frames;
    }
};
