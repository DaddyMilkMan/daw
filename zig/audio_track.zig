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
const project = @import("project.zig");

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

/// A clip that streams its samples from disk on demand instead of holding them
/// in RAM — for long takes/samples. Renders the overlapping span per block by
/// reading just those frames via wav.WavStream.
pub const StreamClip = struct {
    stream: wav.WavStream,
    start: u64,
    gain: f32 = 1.0,
    pan: f32 = 0.0,
    scratch: [2048]f32 = undefined, // reusable decode buffer (no per-block alloc)

    pub fn open(path: []const u8, start: u64) !StreamClip {
        return .{ .stream = try wav.WavStream.open(path), .start = start };
    }
    pub fn close(self: *StreamClip) void {
        self.stream.close();
    }
    pub fn frames(self: StreamClip) u64 {
        return self.stream.frames();
    }
    pub fn endFrame(self: StreamClip) u64 {
        return self.start + self.frames();
    }

    /// Mix this streamed clip into the stereo block at timeline `playhead`.
    pub fn render(self: *StreamClip, out_l: []f32, out_r: []f32, playhead: u64) !void {
        const n = out_l.len;
        const seg_start = @max(playhead, self.start);
        const seg_end = @min(playhead + n, self.endFrame());
        if (seg_start >= seg_end) return;
        const count = seg_end - seg_start; // frames to render
        const ch = self.stream.channels;
        const pc = dsp.panConstantPower(self.pan);
        const max_per_read = self.scratch.len / ch;

        var produced: u64 = 0;
        while (produced < count) {
            const want: usize = @intCast(@min(count - produced, max_per_read));
            const got = try self.stream.readFrames(self.scratch[0 .. want * ch], (seg_start - self.start) + produced);
            if (got == 0) break;
            for (0..got) |f| {
                const oi: usize = @intCast((seg_start - playhead) + produced + f);
                if (ch >= 2) {
                    const bl: f32 = if (self.pan > 0) 1.0 - self.pan else 1.0;
                    const br: f32 = if (self.pan < 0) 1.0 + self.pan else 1.0;
                    out_l[oi] += self.scratch[f * ch] * self.gain * bl;
                    out_r[oi] += self.scratch[f * ch + 1] * self.gain * br;
                } else {
                    const s = self.scratch[f];
                    out_l[oi] += s * self.gain * pc[0];
                    out_r[oi] += s * self.gain * pc[1];
                }
            }
            produced += got;
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

/// Load a project's audio tracks (the file-referenced `AudioClipRef`s) into
/// renderable AudioTracks, honoring each clip's source_offset/length. Caller
/// deinits each track. Tracks with no audio clips are skipped.
pub fn fromProject(a: std.mem.Allocator, p: *const project.Project) !std.ArrayList(AudioTrack) {
    var tracks = std.ArrayList(AudioTrack).init(a);
    errdefer {
        for (tracks.items) |*t| t.deinit();
        tracks.deinit();
    }
    for (p.tracks.items) |pt| {
        if (!pt.isAudio()) continue;
        var track = AudioTrack.init(a);
        errdefer track.deinit();
        try track.name.appendSlice(pt.name.items);
        track.gain = pt.gain;
        track.pan = pt.pan;
        track.mute = pt.mute;
        for (pt.audio_clips.items) |ac| {
            var clip = try AudioClip.loadWav(a, ac.path.items, ac.start);
            // trim to [source_offset, source_offset+length) if requested
            const ch = clip.channels;
            const total = clip.frames();
            const off = @min(ac.source_offset, total);
            const len = if (ac.length == 0) total - off else @min(ac.length, total - off);
            if (off != 0 or len != total) {
                const sr = clip.sample_rate;
                const sub = try a.alloc(f32, @intCast(len * ch));
                @memcpy(sub, clip.samples[@intCast(off * ch)..][0..@intCast(len * ch)]);
                clip.deinit();
                clip = AudioClip.fromOwned(a, sub, ch, ac.start, sr);
            }
            try track.addClip(clip);
        }
        try tracks.append(track);
    }
    return tracks;
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

test "fromProject loads + places a referenced audio clip from the saved model" {
    const a = std.testing.allocator;
    // write a tone file the project will reference
    var nb: [64]u8 = undefined;
    const path = std.fmt.bufPrint(&nb, "test_fp.{d}.wav", .{std.os.linux.getpid()}) catch "test_fp.wav";
    var src = try toneClip(a, 440, 480, 0, 48000);
    defer src.deinit();
    try src.saveWav(path);
    defer std.fs.cwd().deleteFile(path) catch {};

    // a project with one audio track referencing it at frame 1000
    var p = project.Project.init(a);
    defer p.deinit();
    const t = try p.addTrack("Aud", .synth);
    t.gain = 1.0;
    _ = try t.addAudioClip(path, 1000);

    var tracks = try fromProject(a, &p);
    defer {
        for (tracks.items) |*tr| tr.deinit();
        tracks.deinit();
    }
    try std.testing.expectEqual(@as(usize, 1), tracks.items.len);
    try std.testing.expectEqual(@as(u64, 1000), tracks.items[0].clips.items[0].start);

    // render: silent before frame 1000, audible at/after
    var l = [_]f32{0} ** 1100;
    var r = [_]f32{0} ** 1100;
    mixTracks(tracks.items, &l, &r, 0);
    try expectApproxEqAbs(@as(f32, 0), l[500], 1e-6); // before the clip
    var post: f32 = 0;
    for (l[1001..]) |s| post = @max(post, @abs(s));
    try expect(post > 0.1); // clip plays after frame 1000
}

test "streaming clip render matches in-memory render (block by block, no full load)" {
    const a = std.testing.allocator;
    // a 2000-frame mono tone on disk
    var nb: [64]u8 = undefined;
    const path = std.fmt.bufPrint(&nb, "test_stream_clip.{d}.wav", .{std.os.linux.getpid()}) catch "test_stream_clip.wav";
    var src = try toneClip(a, 330, 2000, 0, 48000);
    defer src.deinit();
    try src.saveWav(path);
    defer std.fs.cwd().deleteFile(path) catch {};

    const START: u64 = 300;
    // in-memory reference (track takes ownership of the clip)
    const mem = try AudioClip.loadWav(a, path, START);
    var track = AudioTrack.init(a);
    defer track.deinit();
    track.gain = 0.9;
    track.pan = 0.0;
    try track.addClip(mem);

    const total = 2400;
    const ref_l = try a.alloc(f32, total);
    defer a.free(ref_l);
    const ref_r = try a.alloc(f32, total);
    defer a.free(ref_r);
    @memset(ref_l, 0);
    @memset(ref_r, 0);
    track.render(ref_l, ref_r, 0);

    // streaming render, in small blocks (forces repeated on-demand disk reads)
    var sc = try StreamClip.open(path, START);
    defer sc.close();
    sc.gain = 0.9;
    sc.pan = 0.0;
    const str_l = try a.alloc(f32, total);
    defer a.free(str_l);
    const str_r = try a.alloc(f32, total);
    defer a.free(str_r);
    @memset(str_l, 0);
    @memset(str_r, 0);
    var ph: usize = 0;
    const block = 128;
    while (ph < total) : (ph += block) {
        const nn = @min(block, total - ph);
        try sc.render(str_l[ph .. ph + nn], str_r[ph .. ph + nn], ph);
    }

    for (0..total) |i| {
        try expectApproxEqAbs(ref_l[i], str_l[i], 1e-4);
        try expectApproxEqAbs(ref_r[i], str_r[i], 1e-4);
    }
}

test "full pipeline: record -> WAV -> project -> reload -> timeline render" {
    const a = std.testing.allocator;
    const pid = std.os.linux.getpid();
    var wbuf: [64]u8 = undefined;
    var pbuf: [64]u8 = undefined;
    const wpath = std.fmt.bufPrint(&wbuf, "test_rec.{d}.wav", .{pid}) catch "test_rec.wav";
    const ppath = std.fmt.bufPrint(&pbuf, "test_rec.{d}.zpr", .{pid}) catch "test_rec.zpr";
    defer std.fs.cwd().deleteFile(wpath) catch {};
    defer std.fs.cwd().deleteFile(ppath) catch {};

    // 1) record a take (simulate captured input)
    var rec = Recorder.init(a, 1, 48000);
    defer rec.deinit();
    rec.start_at(2000);
    var blk: [256]f32 = undefined;
    for (0..2) |b| {
        for (&blk, 0..) |*v, i| v.* = if ((b * 256 + i) % 2 == 0) @as(f32, 0.5) else -0.5; // loud signal
        try rec.feed(&blk);
    }
    var clip = try rec.finish();
    try clip.saveWav(wpath); // 2) recorded take lands on disk
    clip.deinit();

    // 3) reference it in a project and persist the project
    var p = project.Project.init(a);
    defer p.deinit();
    const t = try p.addTrack("Rec", .synth);
    t.gain = 1.0;
    _ = try t.addAudioClip(wpath, 2000);
    try p.save(ppath);

    // 4) reload the project and render its audio onto the timeline
    var q = try project.Project.load(a, ppath);
    defer q.deinit();
    var tracks = try fromProject(a, &q);
    defer {
        for (tracks.items) |*tr| tr.deinit();
        tracks.deinit();
    }
    var l = [_]f32{0} ** 2300;
    var r = [_]f32{0} ** 2300;
    mixTracks(tracks.items, &l, &r, 0);
    try expectApproxEqAbs(@as(f32, 0), l[1000], 1e-6); // silent before the take
    var post: f32 = 0;
    for (l[2001..]) |s| post = @max(post, @abs(s));
    try expect(post > 0.1); // the recorded take plays at frame 2000
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
