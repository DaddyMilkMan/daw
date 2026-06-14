//! vst2_test_plugin.zig — a minimal VST2 plugin (.so): a 440 Hz stereo synth
//! (0 inputs, 2 outputs) so processReplacing() yields verifiable audio. Exports
//! VSTPluginMain. Shares the clean-room ABI in vst2_abi.zig with the host.

const std = @import("std");
const v = @import("vst2_abi.zig");

var g_host: ?v.HostCallback = null;
var g_sr: f64 = 48000.0;
var g_phase: f64 = 0.0;
var g_gain: f32 = 0.5;

fn dispatcher(_: *v.AEffect, opcode: i32, _: i32, _: isize, ptr: ?*anyopaque, opt: f32) callconv(.c) isize {
    switch (opcode) {
        v.effOpen, v.effClose, v.effMainsChanged => return 0,
        v.effSetSampleRate => {
            g_sr = opt;
            g_phase = 0;
            return 0;
        },
        v.effSetBlockSize => return 0,
        v.effGetEffectName, v.effGetProductString => {
            if (ptr) |p| {
                const buf: [*]u8 = @ptrCast(p);
                const name = "Zenith VST2 Test";
                @memcpy(buf[0..name.len], name);
                buf[name.len] = 0;
            }
            return 1;
        },
        v.effGetVendorString => {
            if (ptr) |p| {
                const buf: [*]u8 = @ptrCast(p);
                const s = "Zenith";
                @memcpy(buf[0..s.len], s);
                buf[s.len] = 0;
            }
            return 1;
        },
        v.effGetVstVersion => return 2400,
        else => return 0,
    }
}

fn setParameter(_: *v.AEffect, index: i32, value: f32) callconv(.c) void {
    if (index == 0) g_gain = value;
}
fn getParameter(_: *v.AEffect, index: i32) callconv(.c) f32 {
    return if (index == 0) g_gain else 0;
}

fn processReplacing(effect: *v.AEffect, _: [*]const [*]f32, outputs: [*]const [*]f32, frames: i32) callconv(.c) void {
    const nch: usize = @intCast(effect.numOutputs);
    const n: usize = @intCast(frames);
    const inc = 2.0 * std.math.pi * 440.0 / g_sr;
    var i: usize = 0;
    while (i < n) : (i += 1) {
        const s: f32 = @as(f32, @floatCast(@sin(g_phase))) * g_gain;
        g_phase += inc;
        if (g_phase > 2.0 * std.math.pi) g_phase -= 2.0 * std.math.pi;
        var c: usize = 0;
        while (c < nch) : (c += 1) outputs[c][i] = s;
    }
}

var g_effect = v.AEffect{
    .magic = v.kEffectMagic,
    .dispatcher = dispatcher,
    .process = processReplacing,
    .setParameter = setParameter,
    .getParameter = getParameter,
    .numPrograms = 0,
    .numParams = 1,
    .numInputs = 0,
    .numOutputs = 2,
    .flags = v.effFlagsCanReplacing | v.effFlagsIsSynth,
    .resvd1 = 0,
    .resvd2 = 0,
    .initialDelay = 0,
    .realQualities = 0,
    .offQualities = 0,
    .ioRatio = 1.0,
    .object = null,
    .user = null,
    .uniqueID = 0x5A546B32, // 'ZTk2'
    .version = 1000,
    .processReplacing = processReplacing,
    .processDoubleReplacing = null,
};

export fn VSTPluginMain(host: v.HostCallback) callconv(.c) ?*v.AEffect {
    g_host = host;
    return &g_effect;
}
