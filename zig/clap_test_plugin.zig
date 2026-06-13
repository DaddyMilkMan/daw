//! clap_test_plugin.zig — a CLAP *instrument* (.clap shared lib): a polyphonic
//! sine synth driven by note events from the host. Exercises the full host path
//! including sample-accurate event delivery.

const std = @import("std");
const abi = @import("clap_abi.zig");

const NVOICES = 8;
const Voice = struct {
    active: bool = false,
    releasing: bool = false,
    key: i16 = 0,
    phase: f64 = 0,
    freq: f64 = 0,
    env: f32 = 0,
};
var g_voices: [NVOICES]Voice = [_]Voice{.{}} ** NVOICES;
var g_sr: f64 = 48000.0;

fn voiceOn(key: i16, vel: f64) void {
    _ = vel;
    const freq = 440.0 * std.math.pow(f64, 2.0, @as(f64, @floatFromInt(key - 69)) / 12.0);
    for (&g_voices) |*v| {
        if (!v.active) {
            v.* = .{ .active = true, .releasing = false, .key = key, .phase = 0, .freq = freq, .env = 0 };
            return;
        }
    }
}
fn voiceOff(key: i16) void {
    for (&g_voices) |*v| {
        if (v.active and v.key == key) v.releasing = true;
    }
}

const features = [_]?[*:0]const u8{ "instrument", null };
const descriptor = abi.PluginDescriptor{
    .clap_version = .{},
    .id = "zenith.test.inst",
    .name = "Zenith Test Instrument",
    .vendor = "Zenith",
    .url = "",
    .manual_url = "",
    .support_url = "",
    .version = "1.0.0",
    .description = "Polyphonic sine instrument (host test)",
    .features = &features,
};

fn plugInit(_: *const abi.Plugin) callconv(.c) bool {
    return true;
}
fn plugDestroy(_: *const abi.Plugin) callconv(.c) void {}
fn plugActivate(_: *const abi.Plugin, sr: f64, _: u32, _: u32) callconv(.c) bool {
    g_sr = sr;
    g_voices = [_]Voice{.{}} ** NVOICES;
    return true;
}
fn plugDeactivate(_: *const abi.Plugin) callconv(.c) void {}
fn plugStart(_: *const abi.Plugin) callconv(.c) bool {
    return true;
}
fn plugStop(_: *const abi.Plugin) callconv(.c) void {}
fn plugReset(_: *const abi.Plugin) callconv(.c) void {
    g_voices = [_]Voice{.{}} ** NVOICES;
}

fn plugProcess(_: *const abi.Plugin, process: *const abi.Process) callconv(.c) i32 {
    if (process.audio_outputs_count == 0) return abi.PROCESS_CONTINUE;
    const out = process.audio_outputs.?[0];
    const chans = out.data32 orelse return abi.PROCESS_CONTINUE;
    const nframes = process.frames_count;

    var ev_count: u32 = 0;
    if (process.in_events) |ie| ev_count = ie.size.?(ie);
    var ev_idx: u32 = 0;

    var i: u32 = 0;
    while (i < nframes) : (i += 1) {
        // apply all events scheduled at or before this sample
        if (process.in_events) |ie| {
            while (ev_idx < ev_count) {
                const h = ie.get.?(ie, ev_idx) orelse break;
                if (h.time > i) break;
                if (h.space_id == abi.CORE_EVENT_SPACE_ID) {
                    if (h.type == abi.EVENT_NOTE_ON) {
                        const ne: *const abi.EventNote = @ptrCast(@alignCast(h));
                        voiceOn(ne.key, ne.velocity);
                    } else if (h.type == abi.EVENT_NOTE_OFF) {
                        const ne: *const abi.EventNote = @ptrCast(@alignCast(h));
                        voiceOff(ne.key);
                    }
                }
                ev_idx += 1;
            }
        }

        var s: f32 = 0;
        for (&g_voices) |*v| {
            if (!v.active) continue;
            if (v.releasing) {
                v.env -= 0.0003;
                if (v.env <= 0) {
                    v.active = false;
                    continue;
                }
            } else if (v.env < 0.6) {
                v.env += 0.002;
            }
            s += @floatCast(@sin(v.phase) * v.env);
            v.phase += 2.0 * std.math.pi * v.freq / g_sr;
            if (v.phase > 2.0 * std.math.pi) v.phase -= 2.0 * std.math.pi;
        }
        s *= 0.4;
        var c: u32 = 0;
        while (c < out.channel_count) : (c += 1) {
            if (chans[c]) |ch| ch[i] = std.math.clamp(s, -1.0, 1.0);
        }
    }
    return abi.PROCESS_CONTINUE;
}

fn plugGetExt(_: *const abi.Plugin, _: [*:0]const u8) callconv(.c) ?*const anyopaque {
    return null;
}
fn plugOnMain(_: *const abi.Plugin) callconv(.c) void {}

const plugin_instance = abi.Plugin{
    .desc = &descriptor,
    .plugin_data = null,
    .init = plugInit,
    .destroy = plugDestroy,
    .activate = plugActivate,
    .deactivate = plugDeactivate,
    .start_processing = plugStart,
    .stop_processing = plugStop,
    .reset = plugReset,
    .process = plugProcess,
    .get_extension = plugGetExt,
    .on_main_thread = plugOnMain,
};

fn facCount(_: *const abi.PluginFactory) callconv(.c) u32 {
    return 1;
}
fn facDesc(_: *const abi.PluginFactory, index: u32) callconv(.c) ?*const abi.PluginDescriptor {
    return if (index == 0) &descriptor else null;
}
fn facCreate(_: *const abi.PluginFactory, _: *const abi.Host, plugin_id: [*:0]const u8) callconv(.c) ?*const abi.Plugin {
    if (std.mem.orderZ(u8, plugin_id, "zenith.test.inst") == .eq) return &plugin_instance;
    return null;
}

const factory = abi.PluginFactory{
    .get_plugin_count = facCount,
    .get_plugin_descriptor = facDesc,
    .create_plugin = facCreate,
};

fn entryInit(_: [*:0]const u8) callconv(.c) bool {
    return true;
}
fn entryDeinit() callconv(.c) void {}
fn entryGetFactory(factory_id: [*:0]const u8) callconv(.c) ?*const anyopaque {
    if (std.mem.orderZ(u8, factory_id, abi.PLUGIN_FACTORY_ID) == .eq) return &factory;
    return null;
}

export const clap_entry = abi.PluginEntry{
    .clap_version = .{},
    .init = entryInit,
    .deinit = entryDeinit,
    .get_factory = entryGetFactory,
};
