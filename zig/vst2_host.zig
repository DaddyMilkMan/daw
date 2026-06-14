//! vst2_host.zig — load a VST2 plugin, call VSTPluginMain, and drive its
//! AEffect: open -> setSampleRate/setBlockSize -> resume -> processReplacing.
//! Clean-room ABI (vst2_abi.zig); links libc for dlopen.

const std = @import("std");
const v = @import("vst2_abi.zig");

/// Minimal host callback: answer version queries, ignore the rest.
pub fn hostCallback(_: ?*v.AEffect, opcode: i32, _: i32, _: isize, _: ?*anyopaque, _: f32) callconv(.c) isize {
    return switch (opcode) {
        v.audioMasterVersion => 2400,
        else => 0,
    };
}

pub const Plugin = struct {
    lib: std.DynLib,
    effect: *v.AEffect,

    pub fn open(path: []const u8) !Plugin {
        var lib = try std.DynLib.open(path);
        errdefer lib.close();
        const main = lib.lookup(*const fn (v.HostCallback) callconv(.c) ?*v.AEffect, v.SYM_MAIN) orelse
            lib.lookup(*const fn (v.HostCallback) callconv(.c) ?*v.AEffect, v.SYM_MAIN_LEGACY) orelse
            return error.NoVstMain;
        const effect = main(hostCallback) orelse return error.NoEffect;
        if (effect.magic != v.kEffectMagic) return error.BadMagic;
        return .{ .lib = lib, .effect = effect };
    }

    fn dispatch(self: *Plugin, opcode: i32, index: i32, value: isize, ptr: ?*anyopaque, opt: f32) isize {
        return self.effect.dispatcher.?(self.effect, opcode, index, value, ptr, opt);
    }

    pub fn start(self: *Plugin, sample_rate: f64, block: i32) void {
        _ = self.dispatch(v.effOpen, 0, 0, null, 0);
        _ = self.dispatch(v.effSetSampleRate, 0, 0, null, @floatCast(sample_rate));
        _ = self.dispatch(v.effSetBlockSize, 0, block, null, 0);
        _ = self.dispatch(v.effMainsChanged, 0, 1, null, 0); // resume
    }
    pub fn stop(self: *Plugin) void {
        _ = self.dispatch(v.effMainsChanged, 0, 0, null, 0); // suspend
        _ = self.dispatch(v.effClose, 0, 0, null, 0);
    }
    pub fn effectName(self: *Plugin, buf: []u8) []const u8 {
        @memset(buf, 0);
        _ = self.dispatch(v.effGetEffectName, 0, 0, buf.ptr, 0);
        const end = std.mem.indexOfScalar(u8, buf, 0) orelse buf.len;
        return buf[0..end];
    }
    pub fn sendEvents(self: *Plugin, events: *v.VstEvents) void {
        _ = self.dispatch(v.effProcessEvents, 0, 0, @ptrCast(events), 0);
    }
    pub fn process(self: *Plugin, inputs: [*]const [*]f32, outputs: [*]const [*]f32, frames: i32) void {
        self.effect.processReplacing.?(self.effect, inputs, outputs, frames);
    }
    pub fn close(self: *Plugin) void {
        self.lib.close();
    }
};
