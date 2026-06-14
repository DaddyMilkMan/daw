//! audio_devices.zig — enumerate every audio device/channel the OS exposes,
//! via ALSA's device-name hints (which, on a PipeWire/Pulse system, include the
//! server endpoints as well as the raw hardware). For each device we report its
//! direction (input/output/both) and its channel + sample-rate range by opening
//! it and reading the hardware parameter space. Pure Zig over libasound.

const std = @import("std");

const snd_pcm_t = opaque {};
const snd_pcm_hw_params_t = opaque {};

const STREAM_PLAYBACK: c_int = 0;
const STREAM_CAPTURE: c_int = 1;
const NONBLOCK: c_int = 1;

extern fn snd_device_name_hint(card: c_int, iface: [*:0]const u8, hints: *?[*]?*anyopaque) c_int;
extern fn snd_device_name_get_hint(hint: ?*anyopaque, id: [*:0]const u8) ?[*:0]u8;
extern fn snd_device_name_free_hint(hints: ?[*]?*anyopaque) c_int;
extern fn snd_pcm_open(pcm: *?*snd_pcm_t, name: [*:0]const u8, stream: c_int, mode: c_int) c_int;
extern fn snd_pcm_close(pcm: *snd_pcm_t) c_int;
extern fn snd_pcm_hw_params_malloc(ptr: *?*snd_pcm_hw_params_t) c_int;
extern fn snd_pcm_hw_params_free(p: *snd_pcm_hw_params_t) void;
extern fn snd_pcm_hw_params_any(pcm: *snd_pcm_t, params: *snd_pcm_hw_params_t) c_int;
extern fn snd_pcm_hw_params_get_channels_min(params: *snd_pcm_hw_params_t, val: *c_uint) c_int;
extern fn snd_pcm_hw_params_get_channels_max(params: *snd_pcm_hw_params_t, val: *c_uint) c_int;
extern fn snd_pcm_hw_params_get_rate_min(params: *snd_pcm_hw_params_t, val: *c_uint, dir: *c_int) c_int;
extern fn snd_pcm_hw_params_get_rate_max(params: *snd_pcm_hw_params_t, val: *c_uint, dir: *c_int) c_int;
extern fn free(ptr: ?*anyopaque) void;

pub const Direction = enum { input, output, duplex };

pub const Device = struct {
    name: []const u8, // ALSA PCM name (open this)
    desc: []const u8, // human description
    dir: Direction,
    ch_min: u32 = 0,
    ch_max: u32 = 0,
    rate_min: u32 = 0,
    rate_max: u32 = 0,

    pub fn deinit(self: *Device, a: std.mem.Allocator) void {
        a.free(self.name);
        a.free(self.desc);
    }
};

fn queryParams(name: [*:0]const u8, stream: c_int, dev: *Device) void {
    var pcm: ?*snd_pcm_t = null;
    if (snd_pcm_open(&pcm, name, stream, NONBLOCK) < 0) return;
    defer _ = snd_pcm_close(pcm.?);
    var params: ?*snd_pcm_hw_params_t = null;
    if (snd_pcm_hw_params_malloc(&params) < 0) return;
    defer snd_pcm_hw_params_free(params.?);
    if (snd_pcm_hw_params_any(pcm.?, params.?) < 0) return;
    var lo: c_uint = 0;
    var hi: c_uint = 0;
    var d: c_int = 0;
    if (snd_pcm_hw_params_get_channels_min(params.?, &lo) >= 0) dev.ch_min = lo;
    if (snd_pcm_hw_params_get_channels_max(params.?, &hi) >= 0) dev.ch_max = hi;
    if (snd_pcm_hw_params_get_rate_min(params.?, &lo, &d) >= 0) dev.rate_min = lo;
    if (snd_pcm_hw_params_get_rate_max(params.?, &hi, &d) >= 0) dev.rate_max = hi;
}

/// Enumerate all PCM devices the system advertises. Caller frees each Device and
/// the returned slice.
pub fn list(a: std.mem.Allocator) ![]Device {
    var hints: ?[*]?*anyopaque = null;
    if (snd_device_name_hint(-1, "pcm", &hints) < 0) return &.{};
    defer _ = snd_device_name_free_hint(hints);

    var out = std.ArrayList(Device).init(a);
    errdefer {
        for (out.items) |*d| d.deinit(a);
        out.deinit();
    }
    var i: usize = 0;
    while (hints.?[i] != null) : (i += 1) {
        const h = hints.?[i];
        const cname = snd_device_name_get_hint(h, "NAME") orelse continue;
        defer free(cname);
        const cdesc = snd_device_name_get_hint(h, "DESC");
        defer if (cdesc) |p| free(p);
        const cioid = snd_device_name_get_hint(h, "IOID");
        defer if (cioid) |p| free(p);

        const name = std.mem.span(cname);
        // collapse multi-line descriptions to a single line
        var desc_raw: []const u8 = if (cdesc) |p| std.mem.span(p) else name;
        if (std.mem.indexOfScalar(u8, desc_raw, '\n')) |nl| desc_raw = desc_raw[0..nl];

        const dir: Direction = blk: {
            if (cioid) |p| {
                const s = std.mem.span(p);
                if (std.mem.eql(u8, s, "Input")) break :blk .input;
                if (std.mem.eql(u8, s, "Output")) break :blk .output;
            }
            break :blk .duplex; // null IOID = both directions
        };

        var dev = Device{
            .name = try a.dupe(u8, name),
            .desc = try a.dupe(u8, desc_raw),
            .dir = dir,
        };
        // probe channel/rate ranges from whichever direction(s) apply
        if (dir != .input) queryParams(cname, STREAM_PLAYBACK, &dev);
        if (dir == .input or (dir == .duplex and dev.ch_max == 0)) queryParams(cname, STREAM_CAPTURE, &dev);
        try out.append(dev);
    }
    return out.toOwnedSlice();
}

pub fn freeList(a: std.mem.Allocator, devs: []Device) void {
    for (devs) |*d| d.deinit(a);
    a.free(devs);
}
