//! clap_test_plugin.zig — a minimal CLAP plugin (built as a .clap shared lib) so
//! the host has something real to load and process. Generates a 440 Hz sine.
//! Single-instance (global state) — enough to exercise the full host path.

const std = @import("std");
const abi = @import("clap_abi.zig");

var g_phase: f64 = 0.0;
var g_sample_rate: f64 = 48000.0;

const features = [_]?[*:0]const u8{ "instrument", null };

const descriptor = abi.PluginDescriptor{
    .clap_version = .{},
    .id = "zenith.test.sine",
    .name = "Zenith Test Sine",
    .vendor = "Zenith",
    .url = "",
    .manual_url = "",
    .support_url = "",
    .version = "1.0.0",
    .description = "440 Hz sine generator (host test)",
    .features = &features,
};

fn plugInit(_: *const abi.Plugin) callconv(.c) bool {
    return true;
}
fn plugDestroy(_: *const abi.Plugin) callconv(.c) void {}
fn plugActivate(_: *const abi.Plugin, sr: f64, _: u32, _: u32) callconv(.c) bool {
    g_sample_rate = sr;
    g_phase = 0;
    return true;
}
fn plugDeactivate(_: *const abi.Plugin) callconv(.c) void {}
fn plugStart(_: *const abi.Plugin) callconv(.c) bool {
    return true;
}
fn plugStop(_: *const abi.Plugin) callconv(.c) void {}
fn plugReset(_: *const abi.Plugin) callconv(.c) void {
    g_phase = 0;
}
fn plugProcess(_: *const abi.Plugin, process: *const abi.Process) callconv(.c) i32 {
    if (process.audio_outputs_count == 0) return abi.PROCESS_CONTINUE;
    const out = process.audio_outputs.?[0];
    const chans = out.data32 orelse return abi.PROCESS_CONTINUE;
    const inc = 2.0 * std.math.pi * 440.0 / g_sample_rate;
    var i: u32 = 0;
    while (i < process.frames_count) : (i += 1) {
        const s: f32 = @floatCast(@sin(g_phase) * 0.3);
        g_phase += inc;
        if (g_phase > 2.0 * std.math.pi) g_phase -= 2.0 * std.math.pi;
        var c: u32 = 0;
        while (c < out.channel_count) : (c += 1) {
            if (chans[c]) |ch| ch[i] = s;
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
    if (std.mem.orderZ(u8, plugin_id, "zenith.test.sine") == .eq) return &plugin_instance;
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

/// The CLAP entry point. dlsym("clap_entry") finds this.
export const clap_entry = abi.PluginEntry{
    .clap_version = .{},
    .init = entryInit,
    .deinit = entryDeinit,
    .get_factory = entryGetFactory,
};
