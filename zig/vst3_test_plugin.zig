//! vst3_test_plugin.zig — a minimal VST3 plugin (.so) that implements the COM
//! interfaces our host drives: IPluginFactory -> IComponent + IAudioProcessor.
//! It's a 440 Hz stereo tone generator (1 audio output, 0 inputs) so process()
//! produces verifiable non-silent audio. Both host and plugin share vst3_abi.zig.
//!
//! State lives in globals (a single instance is enough to verify the host path);
//! the COM `this` pointers are ignored, which is legal at the ABI level.

const std = @import("std");
const v = @import("vst3_abi.zig");

const class_cid = v.uid(0x5A454E49, 0x54485633, 0x54455354, 0x504C5547); // "ZENI THV3 TEST PLUG"

var g_sr: f64 = 48000.0;
var g_phase: f64 = 0.0;

fn iidEql(p: [*]const u8, target: v.TUID) bool {
    return std.mem.eql(u8, p[0..16], &target);
}
fn putUtf16(dst: []u16, s: []const u8) void {
    @memset(dst, 0);
    var i: usize = 0;
    while (i < s.len and i < dst.len - 1) : (i += 1) dst[i] = s[i];
}
fn putAscii(dst: []u8, s: []const u8) void {
    @memset(dst, 0);
    const n = @min(dst.len - 1, s.len);
    @memcpy(dst[0..n], s[0..n]);
}

// ---------------- IComponent ----------------
fn cQI(_: *anyopaque, iid: [*]const u8, obj: *?*anyopaque) callconv(.c) v.tresult {
    if (iidEql(iid, v.IComponent_iid) or iidEql(iid, v.IPluginBase_iid) or iidEql(iid, v.FUnknown_iid)) {
        obj.* = &g_component;
        return v.kResultOk;
    }
    if (iidEql(iid, v.IAudioProcessor_iid)) {
        obj.* = &g_processor;
        return v.kResultOk;
    }
    obj.* = null;
    return v.kNoInterface;
}
fn refOne(_: *anyopaque) callconv(.c) u32 {
    return 1;
}
fn cInitialize(_: *anyopaque, _: ?*anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cTerminate(_: *anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cGetCtrlId(_: *anyopaque, _: [*]u8) callconv(.c) v.tresult {
    return v.kNotImplemented;
}
fn cSetIoMode(_: *anyopaque, _: i32) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cGetBusCount(_: *anyopaque, media: i32, dir: i32) callconv(.c) i32 {
    if (media == v.kAudio and dir == v.kOutput) return 1; // one stereo output
    return 0;
}
fn cGetBusInfo(_: *anyopaque, media: i32, dir: i32, index: i32, info: *v.BusInfo) callconv(.c) v.tresult {
    if (media != v.kAudio or dir != v.kOutput or index != 0) return v.kInvalidArgument;
    info.* = .{ .media_type = v.kAudio, .direction = v.kOutput, .channel_count = 2, .name = undefined, .bus_type = v.kMain, .flags = v.kDefaultActive };
    putUtf16(&info.name, "Stereo Out");
    return v.kResultOk;
}
fn cGetRouting(_: *anyopaque, _: *anyopaque, _: *anyopaque) callconv(.c) v.tresult {
    return v.kNotImplemented;
}
fn cActivateBus(_: *anyopaque, _: i32, _: i32, _: i32, _: u8) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cSetActive(_: *anyopaque, _: u8) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cSetState(_: *anyopaque, _: ?*anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn cGetState(_: *anyopaque, _: ?*anyopaque) callconv(.c) v.tresult {
    return v.kResultOk;
}

const component_vtbl = v.ComponentVtbl{
    .queryInterface = cQI,
    .addRef = refOne,
    .release = refOne,
    .initialize = cInitialize,
    .terminate = cTerminate,
    .getControllerClassId = cGetCtrlId,
    .setIoMode = cSetIoMode,
    .getBusCount = cGetBusCount,
    .getBusInfo = cGetBusInfo,
    .getRoutingInfo = cGetRouting,
    .activateBus = cActivateBus,
    .setActive = cSetActive,
    .setState = cSetState,
    .getState = cGetState,
};
var g_component = v.Component{ .vtbl = &component_vtbl };

// ---------------- IAudioProcessor ----------------
fn pSetArr(_: *anyopaque, _: ?[*]u64, _: i32, _: ?[*]u64, _: i32) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn pGetArr(_: *anyopaque, _: i32, _: i32, arr: *u64) callconv(.c) v.tresult {
    arr.* = 0x3; // kStereo (L|R)
    return v.kResultOk;
}
fn pCanSize(_: *anyopaque, size: i32) callconv(.c) v.tresult {
    return if (size == v.kSample32) v.kResultOk else v.kResultFalse;
}
fn pLatency(_: *anyopaque) callconv(.c) u32 {
    return 0;
}
fn pSetup(_: *anyopaque, setup: *v.ProcessSetup) callconv(.c) v.tresult {
    g_sr = setup.sample_rate;
    g_phase = 0;
    return v.kResultOk;
}
fn pSetProcessing(_: *anyopaque, _: u8) callconv(.c) v.tresult {
    return v.kResultOk;
}
fn pProcess(_: *anyopaque, data: *v.ProcessData) callconv(.c) v.tresult {
    if (data.symbolic_sample_size != v.kSample32) return v.kResultOk;
    const outs = data.outputs orelse return v.kResultOk;
    if (data.num_outputs < 1) return v.kResultOk;
    const bus = &outs[0];
    const chans = bus.channel_buffers_32 orelse return v.kResultOk;
    const n: usize = @intCast(data.num_samples);
    const inc = 2.0 * std.math.pi * 440.0 / g_sr;
    var i: usize = 0;
    while (i < n) : (i += 1) {
        const s: f32 = @floatCast(@sin(g_phase) * 0.5);
        g_phase += inc;
        if (g_phase > 2.0 * std.math.pi) g_phase -= 2.0 * std.math.pi;
        var c: usize = 0;
        while (c < @as(usize, @intCast(bus.num_channels))) : (c += 1) {
            if (chans[c]) |ch| ch[i] = s;
        }
    }
    bus.silence_flags = 0;
    return v.kResultOk;
}
fn pTail(_: *anyopaque) callconv(.c) u32 {
    return 0;
}

const processor_vtbl = v.ProcessorVtbl{
    .queryInterface = cQI, // same dispatcher: knows IComponent + IAudioProcessor
    .addRef = refOne,
    .release = refOne,
    .setBusArrangements = pSetArr,
    .getBusArrangement = pGetArr,
    .canProcessSampleSize = pCanSize,
    .getLatencySamples = pLatency,
    .setupProcessing = pSetup,
    .setProcessing = pSetProcessing,
    .process = pProcess,
    .getTailSamples = pTail,
};
var g_processor = v.Processor{ .vtbl = &processor_vtbl };

// ---------------- IPluginFactory ----------------
fn fQI(_: *anyopaque, iid: [*]const u8, obj: *?*anyopaque) callconv(.c) v.tresult {
    if (iidEql(iid, v.IPluginFactory_iid) or iidEql(iid, v.FUnknown_iid)) {
        obj.* = &g_factory;
        return v.kResultOk;
    }
    obj.* = null;
    return v.kNoInterface;
}
fn fGetInfo(_: *anyopaque, info: *v.PFactoryInfo) callconv(.c) v.tresult {
    info.* = std.mem.zeroes(v.PFactoryInfo);
    putAscii(&info.vendor, "Zenith");
    putAscii(&info.url, "https://zenith.audio");
    info.flags = 0x10; // kUnicode
    return v.kResultOk;
}
fn fCount(_: *anyopaque) callconv(.c) i32 {
    return 1;
}
fn fGetClass(_: *anyopaque, index: i32, info: *v.PClassInfo) callconv(.c) v.tresult {
    if (index != 0) return v.kInvalidArgument;
    info.* = std.mem.zeroes(v.PClassInfo);
    info.cid = class_cid;
    info.cardinality = v.kManyInstances;
    putAscii(&info.category, "Audio Module Class");
    putAscii(&info.name, "Zenith VST3 Test");
    return v.kResultOk;
}
fn fCreate(_: *anyopaque, cid: [*]const u8, iid: [*]const u8, obj: *?*anyopaque) callconv(.c) v.tresult {
    if (!iidEql(cid, class_cid)) {
        obj.* = null;
        return v.kNoInterface;
    }
    if (iidEql(iid, v.IComponent_iid) or iidEql(iid, v.IPluginBase_iid) or iidEql(iid, v.FUnknown_iid)) {
        obj.* = &g_component;
        return v.kResultOk;
    }
    if (iidEql(iid, v.IAudioProcessor_iid)) {
        obj.* = &g_processor;
        return v.kResultOk;
    }
    obj.* = null;
    return v.kNoInterface;
}

const factory_vtbl = v.FactoryVtbl{
    .queryInterface = fQI,
    .addRef = refOne,
    .release = refOne,
    .getFactoryInfo = fGetInfo,
    .countClasses = fCount,
    .getClassInfo = fGetClass,
    .createInstance = fCreate,
};
var g_factory = v.Factory{ .vtbl = &factory_vtbl };

// ---------------- module entry points ----------------
export fn GetPluginFactory() callconv(.c) ?*v.Factory {
    return &g_factory;
}
export fn ModuleEntry(_: ?*anyopaque) callconv(.c) bool {
    return true;
}
export fn ModuleExit() callconv(.c) bool {
    return true;
}
