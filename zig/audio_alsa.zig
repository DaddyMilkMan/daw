//! audio_alsa.zig — minimal ALSA PCM playback via libasound.
//!
//! This is the first real slice of the platform seam's *device* side: a thin
//! Zig wrapper over the OS audio interface (ALSA), no JUCE. We declare just the
//! handful of libasound symbols we use rather than @cImport the whole header.

const std = @import("std");

const snd_pcm_t = opaque {};

const SND_PCM_STREAM_PLAYBACK: c_int = 0;
const SND_PCM_STREAM_CAPTURE: c_int = 1;
const SND_PCM_FORMAT_S16_LE: c_int = 2;
const SND_PCM_ACCESS_RW_INTERLEAVED: c_uint = 3;

extern fn snd_pcm_open(pcm: *?*snd_pcm_t, name: [*:0]const u8, stream: c_int, mode: c_int) c_int;
extern fn snd_pcm_set_params(
    pcm: *snd_pcm_t,
    format: c_int,
    access: c_uint,
    channels: c_uint,
    rate: c_uint,
    soft_resample: c_int,
    latency_us: c_uint,
) c_int;
extern fn snd_pcm_writei(pcm: *snd_pcm_t, buffer: *const anyopaque, size: c_ulong) c_long;
extern fn snd_pcm_readi(pcm: *snd_pcm_t, buffer: *anyopaque, size: c_ulong) c_long;
extern fn snd_pcm_recover(pcm: *snd_pcm_t, err: c_int, silent: c_int) c_int;
extern fn snd_pcm_drain(pcm: *snd_pcm_t) c_int;
extern fn snd_pcm_close(pcm: *snd_pcm_t) c_int;
extern fn snd_strerror(err: c_int) [*:0]const u8;

pub const AlsaError = error{ OpenFailed, SetParamsFailed, WriteFailed, ReadFailed };

/// Play interleaved 16-bit PCM through the named device (e.g. "default").
pub fn playInterleavedS16(device: [*:0]const u8, samples: []const i16, rate: u32, channels: u16) AlsaError!void {
    var handle: ?*snd_pcm_t = null;

    var rc = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        std.log.scoped(.alsa).err("snd_pcm_open failed: {s}", .{snd_strerror(rc)});
        return AlsaError.OpenFailed;
    }
    const pcm = handle.?;
    defer _ = snd_pcm_close(pcm);

    rc = snd_pcm_set_params(
        pcm,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        @intCast(channels),
        rate,
        1, // allow soft resampling
        200_000, // ~200 ms latency
    );
    if (rc < 0) {
        std.log.scoped(.alsa).err("snd_pcm_set_params failed: {s}", .{snd_strerror(rc)});
        return AlsaError.SetParamsFailed;
    }

    const frames_total: usize = samples.len / channels;
    var offset: usize = 0;
    while (offset < frames_total) {
        const remaining = frames_total - offset;
        const written = snd_pcm_writei(pcm, &samples[offset * channels], @intCast(remaining));
        if (written < 0) {
            // underrun / suspend — try to recover and continue
            const recovered = snd_pcm_recover(pcm, @intCast(written), 1);
            if (recovered < 0) {
                std.log.scoped(.alsa).warn("write failed: {s}", .{snd_strerror(@intCast(written))});
                return AlsaError.WriteFailed;
            }
            continue;
        }
        offset += @intCast(written);
    }

    _ = snd_pcm_drain(pcm);
}

/// Streaming output: open once, write small blocks continuously. This is the
/// real-time path the live engine uses.
pub const StreamOut = struct {
    pcm: *snd_pcm_t,
    channels: u16,

    pub fn open(device: [*:0]const u8, rate: u32, channels: u16, latency_us: u32) AlsaError!StreamOut {
        var handle: ?*snd_pcm_t = null;
        if (snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0) < 0)
            return AlsaError.OpenFailed;
        const pcm = handle.?;
        const rc = snd_pcm_set_params(
            pcm,
            SND_PCM_FORMAT_S16_LE,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            @intCast(channels),
            rate,
            1,
            latency_us,
        );
        if (rc < 0) {
            _ = snd_pcm_close(pcm);
            return AlsaError.SetParamsFailed;
        }
        return .{ .pcm = pcm, .channels = channels };
    }

    /// Write one interleaved block; recovers from underruns.
    pub fn writeBlock(self: *StreamOut, samples: []const i16) AlsaError!void {
        const frames_total: usize = samples.len / self.channels;
        var offset: usize = 0;
        while (offset < frames_total) {
            const remaining = frames_total - offset;
            const written = snd_pcm_writei(self.pcm, &samples[offset * self.channels], @intCast(remaining));
            if (written < 0) {
                if (snd_pcm_recover(self.pcm, @intCast(written), 1) < 0) return AlsaError.WriteFailed;
                continue;
            }
            offset += @intCast(written);
        }
    }

    pub fn close(self: *StreamOut) void {
        _ = snd_pcm_drain(self.pcm);
        _ = snd_pcm_close(self.pcm);
    }
};

/// Streaming capture (audio input). The Linux/ALSA implementation of the
/// platform "audio input" interface; CoreAudio/WASAPI backends slot in later.
pub const StreamIn = struct {
    pcm: *snd_pcm_t,
    channels: u16,

    pub fn open(device: [*:0]const u8, rate: u32, channels: u16, latency_us: u32) AlsaError!StreamIn {
        var handle: ?*snd_pcm_t = null;
        if (snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0) < 0)
            return AlsaError.OpenFailed;
        const pcm = handle.?;
        const rc = snd_pcm_set_params(
            pcm,
            SND_PCM_FORMAT_S16_LE,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            @intCast(channels),
            rate,
            1,
            latency_us,
        );
        if (rc < 0) {
            _ = snd_pcm_close(pcm);
            return AlsaError.SetParamsFailed;
        }
        return .{ .pcm = pcm, .channels = channels };
    }

    /// Fill `samples` (interleaved) with captured audio; returns frames read.
    pub fn readBlock(self: *StreamIn, samples: []i16) AlsaError!usize {
        const want: usize = samples.len / self.channels;
        var got: usize = 0;
        while (got < want) {
            const r = snd_pcm_readi(self.pcm, &samples[got * self.channels], @intCast(want - got));
            if (r < 0) {
                if (snd_pcm_recover(self.pcm, @intCast(r), 1) < 0) return AlsaError.ReadFailed;
                continue;
            }
            got += @intCast(r);
        }
        return got;
    }

    pub fn close(self: *StreamIn) void {
        _ = snd_pcm_close(self.pcm);
    }
};
