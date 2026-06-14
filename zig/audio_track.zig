//! audio_track.zig — audio clips + audio tracks + recording-to-timeline.
//!
//! Complements the MIDI/instrument model in project.zig with *audio* material:
//! an AudioClip holds rendered samples placed at a timeline frame; an AudioTrack
//! mixes its clips into a stereo bus (sample-accurate, gain + pan); a Recorder
//! turns captured input frames into a clip on the timeline. Clips round-trip to
//! disk via wav.zig (recordings live as WAV files a project can reference).

const std = @import("std");
const dsp = @import("dsp.zig");
const wav = @import("wav.zig");

/// Audio material placed at `start` (timeline frames). Interleaved f32 samples.
pub const AudioClip = struct {
    samples: []f32, // interleaved by `channels`
    channels: u16,
    start: u64,
    sample_rate: u32,
    allocator: std.mem.Allocator,

    pub fn fromOwned(a: std.mem.Allocator, samples: []f32, channels: u16, start: u64, sr: u32) AudioClip {
        return .{ .samples = samples, .channels = channels, .start = start, .sample_rate = sr, .allocator = a };
    }
    pub fn deinit(self: *AudioClip) void {
        self.allocator.free(self.samples);
    }
    pub fn frames(self: AudioClip) u64 {
        return if (self.channels == 0) 0 else self.samples.len / self.channels;
    }
    pub fn endFrame(self: AudioClip) u64 {
        return self.start + self.frames();
    }
    /// Read frame `f` (clip-relative) as stereo (mono is duplicated).
    pub fn sampleAt(self: AudioClip, f: u64) [2]f32 {
        const i = f * self.channels;
        if (self.channels >= 2) return .{ self.samples[i], self.samples[i + 1] };
        const s = self.samples[i];
        return .{ s, s };
    }

    pub fn saveWav(self: AudioClip, path: []const u8) !void {
        try wav.writeFloat32(path, self.samples, self.sample_rate, self.channels);
    }
    /// Load a WAV file as a clip placed at timeline frame `start`.
    pub fn loadWav(a: std.mem.Allocator, path: []const u8, start: u64) !AudioClip {
        const ad = try wav.readPcm(a, path);
        return .{ .samples = ad.samples, .channels = ad.channels, .start = start, .sample_rate = ad.sample_rate, .allocator = a };
    }
};

pub const AudioTrack = struct {
    name: std.ArrayList(u8),
    clips: std.ArrayList(AudioClip),
    gain: f32 = 0.8,
    pan: f32 = 0.0,
    mute: bool = false,
    solo: bool = false,
    record_arm: bool = false,
    allocator: std.mem.Allocator,

    pub fn init(a: std.mem.Allocator) AudioTrack {
        return .{ .name = std.ArrayList(u8).init(a), .clips = std.ArrayList(AudioClip).init(a), .allocator = a };
    }
    pub fn deinit(self: *AudioTrack) void {
        self.name.deinit();
        for (self.clips.items) |*c| c.deinit();
        self.clips.deinit();
    }
    pub fn addClip(self: *AudioTrack, clip: AudioClip) !void {
        try self.clips.append(clip);
    }
    pub fn lengthFrames(self: AudioTrack) u64 {
        var end: u64 = 0;
        for (self.clips.items) |c| end = @max(end, c.endFrame());
        return end;
    }

    /// Mix this track's clips into the stereo block at timeline `playhead`.
    /// outL/outR are the block buffers (length n); contributions are summed in.
    pub fn render(self: AudioTrack, out_l: []f32, out_r: []f32, playhead: u64) void {
        if (self.mute) return;
        const n = out_l.len;
        const pc = dsp.panConstantPower(self.pan); // constant-power for mono sources
        for (self.clips.items) |clip| {
            const seg_start = @max(playhead, clip.start);
            const seg_end = @min(playhead + n, clip.endFrame());
            if (seg_start >= seg_end) continue;
            var f = seg_start;
            while (f < seg_end) : (f += 1) {
                const oi: usize = @intCast(f - playhead);
                const st = clip.sampleAt(f - clip.start);
                if (clip.channels >= 2) {
                    // stereo source -> balance pan (preserve image, unity at center)
                    const bl: f32 = if (self.pan > 0) 1.0 - self.pan else 1.0;
                    const br: f32 = if (self.pan < 0) 1.0 + self.pan else 1.0;
                    out_l[oi] += st[0] * self.gain * bl;
                    out_r[oi] += st[1] * self.gain * br;
                } else {
                    out_l[oi] += st[0] * self.gain * pc[0];
                    out_r[oi] += st[1] * self.gain * pc[1];
                }
            }
        }
    }
};

/// Mix several audio tracks into a stereo block, honoring mute/solo.
pub fn mixTracks(tracks: []const AudioTrack, out_l: []f32, out_r: []f32, playhead: u64) void {
    @memset(out_l, 0);
    @memset(out_r, 0);
    var any_solo = false;
    for (tracks) |t| {
        if (t.solo) any_solo = true;
    }
    for (tracks) |t| {
        if (any_solo and !t.solo) continue;
        t.render(out_l, out_r, playhead);
    }
}

/// Captures input frames during recording, then yields a clip on the timeline.
pub const Recorder = struct {
    buf: std.ArrayList(f32), // interleaved
    channels: u16,
    sample_rate: u32,
    start: u64, // timeline frame where recording began
    recording: bool = false,

    pub fn init(a: std.mem.Allocator, channels: u16, sr: u32) Recorder {
        return .{ .buf = std.ArrayList(f32).init(a), .channels = channels, .sample_rate = sr, .start = 0 };
    }
    pub fn deinit(self: *Recorder) void {
        self.buf.deinit();
    }
    pub fn start_at(self: *Recorder, frame: u64) void {
        self.start = frame;
        self.recording = true;
        self.buf.clearRetainingCapacity();
    }
    /// Append captured interleaved frames (called per audio block).
    pub fn feed(self: *Recorder, interleaved: []const f32) !void {
        if (!self.recording) return;
        try self.buf.appendSlice(interleaved);
    }
    /// Finalize the take into a clip placed where recording began (transfers buffer).
    pub fn finish(self: *Recorder) !AudioClip {
        self.recording = false;
        const owned = try self.buf.toOwnedSlice();
        return AudioClip.fromOwned(self.buf.allocator, owned, self.channels, self.start, self.sample_rate);
    }
};

// ===========================================================================
// Tests
// ===========================================================================
const expectApproxEqAbs = std.testing.expectApproxEqAbs;
const expect = std.testing.expect;

fn toneClip(a: std.mem.Allocator, freq: f32, frames_n: usize, start: u64, sr: u32) !AudioClip {
    const s = try a.alloc(f32, frames_n);
    for (s, 0..) |*v, i| v.* = @sin(2.0 * std.math.pi * freq * @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(sr)));
    return AudioClip.fromOwned(a, s, 1, start, sr);
}

test "mono clip plays at its timeline offset (sample-accurate)" {
    const a = std.testing.allocator;
    const s = try a.alloc(f32, 4);
    @memset(s, 1.0);
    var clip = AudioClip.fromOwned(a, s, 1, 2, 48000); // 4 frames at frame 2
    var track = AudioTrack.init(a);
    defer track.deinit();
    track.gain = 1.0;
    track.pan = 0.0;
    try track.addClip(clip);

    var l = [_]f32{0} ** 8;
    var r = [_]f32{0} ** 8;
    track.render(&l, &r, 0);
    // frames 0,1 silent; 2..6 = 1.0 * 0.7071 (constant-power center); 6,7 silent
    try expectApproxEqAbs(@as(f32, 0), l[0], 1e-6);
    try expectApproxEqAbs(@as(f32, 0), l[1], 1e-6);
    try expectApproxEqAbs(@as(f32, 0.7071), l[2], 1e-3);
    try expectApproxEqAbs(@as(f32, 0.7071), l[5], 1e-3);
    try expectApproxEqAbs(@as(f32, 0), l[6], 1e-6);
    _ = &clip;
}

test "hard-left pan removes the right channel" {
    const a = std.testing.allocator;
    const s = try a.alloc(f32, 4);
    @memset(s, 1.0);
    var track = AudioTrack.init(a);
    defer track.deinit();
    track.gain = 1.0;
    track.pan = -1.0;
    try track.addClip(AudioClip.fromOwned(a, s, 1, 0, 48000));
    var l = [_]f32{0} ** 4;
    var r = [_]f32{0} ** 4;
    track.render(&l, &r, 0);
    try expect(l[0] > 0.9);
    try expectApproxEqAbs(@as(f32, 0), r[0], 1e-6);
}

test "two tracks mix (sum) and mute silences" {
    const a = std.testing.allocator;
    var t1 = AudioTrack.init(a);
    defer t1.deinit();
    var t2 = AudioTrack.init(a);
    defer t2.deinit();
    const s1 = try a.alloc(f32, 4);
    @memset(s1, 0.5);
    const s2 = try a.alloc(f32, 4);
    @memset(s2, 0.5);
    t1.gain = 1.0;
    t1.pan = 0;
    t2.gain = 1.0;
    t2.pan = 0;
    try t1.addClip(AudioClip.fromOwned(a, s1, 1, 0, 48000));
    try t2.addClip(AudioClip.fromOwned(a, s2, 1, 0, 48000));
    const tracks = [_]AudioTrack{ t1, t2 };
    var l = [_]f32{0} ** 4;
    var r = [_]f32{0} ** 4;
    mixTracks(&tracks, &l, &r, 0);
    try expectApproxEqAbs(@as(f32, 2.0 * 0.5 * 0.7071), l[0], 1e-3); // both summed

    var muted = [_]AudioTrack{ t1, t2 };
    muted[0].mute = true;
    muted[1].mute = true;
    mixTracks(&muted, &l, &r, 0);
    try expectApproxEqAbs(@as(f32, 0), l[0], 1e-6);
}

test "recorder captures frames into a clip at the record position" {
    const a = std.testing.allocator;
    var rec = Recorder.init(a, 1, 48000);
    defer rec.deinit();
    rec.start_at(1000);
    try rec.feed(&[_]f32{ 0.1, 0.2, 0.3 });
    try rec.feed(&[_]f32{ 0.4, 0.5 });
    var clip = try rec.finish();
    defer clip.deinit();
    try std.testing.expectEqual(@as(u64, 1000), clip.start);
    try std.testing.expectEqual(@as(u64, 5), clip.frames());
    try expectApproxEqAbs(@as(f32, 0.3), clip.samples[2], 1e-6);
}

test "clip survives a WAV round-trip (record -> disk -> reload at a new position)" {
    const a = std.testing.allocator;
    var src = try toneClip(a, 440, 480, 0, 48000);
    defer src.deinit();
    var nb: [64]u8 = undefined;
    const path = std.fmt.bufPrint(&nb, "test_clip.{d}.wav", .{std.os.linux.getpid()}) catch "test_clip.wav";
    try src.saveWav(path);
    defer std.fs.cwd().deleteFile(path) catch {};
    var reloaded = try AudioClip.loadWav(a, path, 24000);
    defer reloaded.deinit();
    try std.testing.expectEqual(@as(u64, 24000), reloaded.start);
    try std.testing.expectEqual(src.frames(), reloaded.frames());
    try expectApproxEqAbs(src.samples[100], reloaded.samples[100], 1e-6);
}
