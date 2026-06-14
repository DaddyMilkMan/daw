//! vst2_abi.zig — VST 2.4 ABI, clean-room in Zig.
//!
//! The VST2 SDK was withdrawn by Steinberg, but the binary interface (the
//! `AEffect` struct, the dispatcher opcode numbers, the host callback) is public
//! knowledge and trivially small. We declare it ourselves — no SDK, no headers.
//! Enough to load a .so/.dll-style plugin, open it, and pull audio through
//! processReplacing(). Host and plugin share these definitions.

const std = @import("std");

pub const kEffectMagic: i32 = 0x56737450; // 'VstP'

pub const HostCallback = *const fn (?*AEffect, i32, i32, isize, ?*anyopaque, f32) callconv(.c) isize;
pub const DispatcherProc = *const fn (*AEffect, i32, i32, isize, ?*anyopaque, f32) callconv(.c) isize;
pub const ProcessProc = *const fn (*AEffect, [*]const [*]f32, [*]const [*]f32, i32) callconv(.c) void;
pub const ProcessDoubleProc = *const fn (*AEffect, [*]const [*]f64, [*]const [*]f64, i32) callconv(.c) void;
pub const SetParamProc = *const fn (*AEffect, i32, f32) callconv(.c) void;
pub const GetParamProc = *const fn (*AEffect, i32) callconv(.c) f32;

/// The canonical VST 2.4 AEffect layout (natural C alignment on x86-64).
pub const AEffect = extern struct {
    magic: i32,
    dispatcher: ?DispatcherProc,
    process: ?ProcessProc, // deprecated (accumulating)
    setParameter: ?SetParamProc,
    getParameter: ?GetParamProc,
    numPrograms: i32,
    numParams: i32,
    numInputs: i32,
    numOutputs: i32,
    flags: i32,
    resvd1: isize,
    resvd2: isize,
    initialDelay: i32,
    realQualities: i32, // deprecated
    offQualities: i32, // deprecated
    ioRatio: f32, // deprecated
    object: ?*anyopaque,
    user: ?*anyopaque,
    uniqueID: i32,
    version: i32,
    processReplacing: ?ProcessProc,
    processDoubleReplacing: ?ProcessDoubleProc,
    future: [56]u8 = [_]u8{0} ** 56,
};

// AEffect flags
pub const effFlagsHasEditor: i32 = 1 << 0;
pub const effFlagsCanReplacing: i32 = 1 << 4;
pub const effFlagsProgramChunks: i32 = 1 << 5;
pub const effFlagsIsSynth: i32 = 1 << 8;

// MIDI / events
pub const kVstMidiType: i32 = 1;
pub const effProcessEvents: i32 = 25;
pub const audioMasterProcessEvents: i32 = 8;

pub const VstEvent = extern struct {
    type: i32,
    byte_size: i32,
    delta_frames: i32,
    flags: i32,
    data: [16]u8,
};
pub const VstMidiEvent = extern struct {
    type: i32,
    byte_size: i32,
    delta_frames: i32,
    flags: i32,
    note_length: i32,
    note_offset: i32,
    midi_data: [4]u8,
    detune: i8,
    note_off_velocity: i8,
    reserved1: i8,
    reserved2: i8,
};
pub const VstEvents = extern struct {
    num_events: i32,
    reserved: isize,
    events: [2]?*VstEvent, // variable-length in spirit; 2 is enough here
};

// Plugin dispatcher opcodes (host -> plugin)
pub const effOpen: i32 = 0;
pub const effClose: i32 = 1;
pub const effSetProgram: i32 = 2;
pub const effGetProgram: i32 = 3;
pub const effGetParamName: i32 = 8;
pub const effSetSampleRate: i32 = 10;
pub const effSetBlockSize: i32 = 11;
pub const effMainsChanged: i32 = 12;
pub const effGetEffectName: i32 = 45;
pub const effGetVendorString: i32 = 47;
pub const effGetProductString: i32 = 48;
pub const effGetVstVersion: i32 = 58;

// Host callback opcodes (plugin -> host)
pub const audioMasterAutomate: i32 = 0;
pub const audioMasterVersion: i32 = 1;
pub const audioMasterCurrentId: i32 = 2;
pub const audioMasterIdle: i32 = 3;

pub const SYM_MAIN = "VSTPluginMain";
pub const SYM_MAIN_LEGACY = "main";

test "AEffect magic is 'VstP'" {
    const m = "VstP";
    const want: i32 = (@as(i32, m[0]) << 24) | (@as(i32, m[1]) << 16) | (@as(i32, m[2]) << 8) | @as(i32, m[3]);
    try std.testing.expectEqual(want, kEffectMagic);
}
