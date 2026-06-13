//! audio_alsa.zig — minimal ALSA PCM playback via libasound.
//!
//! This is the first real slice of the platform seam's *device* side: a thin
//! Zig wrapper over the OS audio interface (ALSA), no JUCE. We declare just the
//! handful of libasound symbols we use rather than @cImport the whole header.

const std = @import("std");

const snd_pcm_t = opaque {};

const SND_PCM_STREAM_PLAYBACK: c_int = 0;
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
extern fn snd_pcm_recover(pcm: *snd_pcm_t, err: c_int, silent: c_int) c_int;
extern fn snd_pcm_drain(pcm: *snd_pcm_t) c_int;
extern fn snd_pcm_close(pcm: *snd_pcm_t) c_int;
extern fn snd_strerror(err: c_int) [*:0]const u8;

pub const AlsaError = error{ OpenFailed, SetParamsFailed, WriteFailed };

/// Play interleaved 16-bit PCM through the named device (e.g. "default").
pub fn playInterleavedS16(device: [*:0]const u8, samples: []const i16, rate: u32, channels: u16) AlsaError!void {
    var handle: ?*snd_pcm_t = null;

    var rc = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        std.debug.print("alsa: snd_pcm_open failed: {s}\n", .{snd_strerror(rc)});
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
        std.debug.print("alsa: snd_pcm_set_params failed: {s}\n", .{snd_strerror(rc)});
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
                std.debug.print("alsa: write failed: {s}\n", .{snd_strerror(@intCast(written))});
                return AlsaError.WriteFailed;
            }
            continue;
        }
        offset += @intCast(written);
    }

    _ = snd_pcm_drain(pcm);
}
