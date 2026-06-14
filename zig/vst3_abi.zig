//! vst3_abi.zig — VST3 (Steinberg) COM-style ABI, hand-declared in Zig.
//!
//! Clean-room: the interface vtables, struct layouts, and IIDs are transcribed
//! from the VST3 SDK *headers* (pluginterfaces/) — read to understand the ABI,
//! never copied. Enough of the interface to load a module, instantiate the
//! IComponent + IAudioProcessor, and pull audio through process().
//!
//! Linux x86-64: COM_COMPATIBLE=0, so a TUID is the big-endian bytes of the four
//! u32 words, and PLUGIN_API is the plain System V C calling convention.

const std = @import("std");

pub const tresult = i32;
pub const kResultOk: tresult = 0;
pub const kResultTrue: tresult = 0;
pub const kResultFalse: tresult = 1;
pub const kNoInterface: tresult = @bitCast(@as(u32, 0x80004002));
pub const kInvalidArgument: tresult = @bitCast(@as(u32, 0x80070057));
pub const kNotImplemented: tresult = @bitCast(@as(u32, 0x80004001));

pub const TUID = [16]u8;

/// INLINE_UID for non-COM platforms (#else branch): big-endian bytes of l1..l4.
pub fn uid(l1: u32, l2: u32, l3: u32, l4: u32) TUID {
    return .{
        @truncate(l1 >> 24), @truncate(l1 >> 16), @truncate(l1 >> 8), @truncate(l1),
        @truncate(l2 >> 24), @truncate(l2 >> 16), @truncate(l2 >> 8), @truncate(l2),
        @truncate(l3 >> 24), @truncate(l3 >> 16), @truncate(l3 >> 8), @truncate(l3),
        @truncate(l4 >> 24), @truncate(l4 >> 16), @truncate(l4 >> 8), @truncate(l4),
    };
}

pub const FUnknown_iid = uid(0x00000000, 0x00000000, 0xC0000000, 0x00000046);
pub const IPluginBase_iid = uid(0x22888DDB, 0x156E45AE, 0x8358B348, 0x08190625);
pub const IPluginFactory_iid = uid(0x7A4D811C, 0x52114A1F, 0xAED9D2EE, 0x0B43BF9F);
pub const IComponent_iid = uid(0xE831FF31, 0xF2D54301, 0x928EBBEE, 0x25697802);
pub const IAudioProcessor_iid = uid(0x42043F99, 0xB7DA453C, 0xA569E79D, 0x9AAEC33D);
pub const IEventList_iid = uid(0x3A2C4214, 0x346349FE, 0xB2C4F397, 0xB9695A44);

// Enums (all int32 in the ABI).
pub const kAudio: i32 = 0;
pub const kEvent: i32 = 1;
pub const kInput: i32 = 0;
pub const kOutput: i32 = 1;
pub const kMain: i32 = 0;
pub const kSample32: i32 = 0;
pub const kSample64: i32 = 1;
pub const kRealtime: i32 = 0;
pub const kDefaultActive: u32 = 1 << 0;

// --- structs (exact SDK layouts) ---
pub const PFactoryInfo = extern struct {
    vendor: [64]u8,
    url: [256]u8,
    email: [128]u8,
    flags: i32,
};

pub const PClassInfo = extern struct {
    cid: TUID,
    cardinality: i32,
    category: [32]u8,
    name: [64]u8,
};
pub const kManyInstances: i32 = 0x7FFFFFFF;

pub const BusInfo = extern struct {
    media_type: i32,
    direction: i32,
    channel_count: i32,
    name: [128]u16, // String128 (UTF-16)
    bus_type: i32,
    flags: u32,
};

pub const ProcessSetup = extern struct {
    process_mode: i32,
    symbolic_sample_size: i32,
    max_samples_per_block: i32,
    sample_rate: f64,
};

pub const AudioBusBuffers = extern struct {
    num_channels: i32,
    silence_flags: u64,
    channel_buffers_32: ?[*]const ?[*]f32,
};

pub const ProcessData = extern struct {
    process_mode: i32,
    symbolic_sample_size: i32,
    num_samples: i32,
    num_inputs: i32,
    num_outputs: i32,
    inputs: ?[*]AudioBusBuffers,
    outputs: ?[*]AudioBusBuffers,
    input_param_changes: ?*anyopaque,
    output_param_changes: ?*anyopaque,
    input_events: ?*anyopaque,
    output_events: ?*anyopaque,
    process_context: ?*anyopaque,
};

// --- events (Event is 48 bytes: union is 8-aligned at offset 24, sized 24 by
//     the largest member NoteExpressionTextEvent — match the SDK exactly). ---
pub const kNoteOnEvent: u16 = 0;
pub const kNoteOffEvent: u16 = 1;

pub const NoteOnEvent = extern struct {
    channel: i16,
    pitch: i16,
    tuning: f32,
    velocity: f32,
    length: i32,
    note_id: i32,
};
pub const NoteOffEvent = extern struct {
    channel: i16,
    pitch: i16,
    velocity: f32,
    note_id: i32,
    tuning: f32,
};
pub const EventData = extern union {
    note_on: NoteOnEvent,
    note_off: NoteOffEvent,
    _align: u64, // force 8-byte alignment (matches pointer/double members)
    _size: [24]u8, // force >= 24 bytes (NoteExpressionTextEvent)
};
pub const Event = extern struct {
    bus_index: i32,
    sample_offset: i32,
    ppq_position: f64,
    flags: u16,
    type: u16,
    data: EventData, // lands at offset 24 thanks to EventData's 8-align
};

pub const EventListVtbl = extern struct {
    queryInterface: *const fn (*anyopaque, [*]const u8, *?*anyopaque) callconv(.c) tresult,
    addRef: *const fn (*anyopaque) callconv(.c) u32,
    release: *const fn (*anyopaque) callconv(.c) u32,
    getEventCount: *const fn (*anyopaque) callconv(.c) i32,
    getEvent: *const fn (*anyopaque, i32, *Event) callconv(.c) tresult,
    addEvent: *const fn (*anyopaque, *Event) callconv(.c) tresult,
};
pub const EventList = extern struct { vtbl: *const EventListVtbl };

comptime {
    std.debug.assert(@sizeOf(Event) == 48);
    std.debug.assert(@offsetOf(Event, "data") == 24);
}

// --- interface vtables ---
// `this` is typed *anyopaque: the host passes the real interface pointer; a C
// COM object recovers its state from it (our test plugin uses globals).
pub const FactoryVtbl = extern struct {
    queryInterface: *const fn (*anyopaque, [*]const u8, *?*anyopaque) callconv(.c) tresult,
    addRef: *const fn (*anyopaque) callconv(.c) u32,
    release: *const fn (*anyopaque) callconv(.c) u32,
    getFactoryInfo: *const fn (*anyopaque, *PFactoryInfo) callconv(.c) tresult,
    countClasses: *const fn (*anyopaque) callconv(.c) i32,
    getClassInfo: *const fn (*anyopaque, i32, *PClassInfo) callconv(.c) tresult,
    createInstance: *const fn (*anyopaque, [*]const u8, [*]const u8, *?*anyopaque) callconv(.c) tresult,
};
pub const Factory = extern struct { vtbl: *const FactoryVtbl };

pub const ComponentVtbl = extern struct {
    queryInterface: *const fn (*anyopaque, [*]const u8, *?*anyopaque) callconv(.c) tresult,
    addRef: *const fn (*anyopaque) callconv(.c) u32,
    release: *const fn (*anyopaque) callconv(.c) u32,
    initialize: *const fn (*anyopaque, ?*anyopaque) callconv(.c) tresult,
    terminate: *const fn (*anyopaque) callconv(.c) tresult,
    getControllerClassId: *const fn (*anyopaque, [*]u8) callconv(.c) tresult,
    setIoMode: *const fn (*anyopaque, i32) callconv(.c) tresult,
    getBusCount: *const fn (*anyopaque, i32, i32) callconv(.c) i32,
    getBusInfo: *const fn (*anyopaque, i32, i32, i32, *BusInfo) callconv(.c) tresult,
    getRoutingInfo: *const fn (*anyopaque, *anyopaque, *anyopaque) callconv(.c) tresult,
    activateBus: *const fn (*anyopaque, i32, i32, i32, u8) callconv(.c) tresult,
    setActive: *const fn (*anyopaque, u8) callconv(.c) tresult,
    setState: *const fn (*anyopaque, ?*anyopaque) callconv(.c) tresult,
    getState: *const fn (*anyopaque, ?*anyopaque) callconv(.c) tresult,
};
pub const Component = extern struct { vtbl: *const ComponentVtbl };

pub const ProcessorVtbl = extern struct {
    queryInterface: *const fn (*anyopaque, [*]const u8, *?*anyopaque) callconv(.c) tresult,
    addRef: *const fn (*anyopaque) callconv(.c) u32,
    release: *const fn (*anyopaque) callconv(.c) u32,
    setBusArrangements: *const fn (*anyopaque, ?[*]u64, i32, ?[*]u64, i32) callconv(.c) tresult,
    getBusArrangement: *const fn (*anyopaque, i32, i32, *u64) callconv(.c) tresult,
    canProcessSampleSize: *const fn (*anyopaque, i32) callconv(.c) tresult,
    getLatencySamples: *const fn (*anyopaque) callconv(.c) u32,
    setupProcessing: *const fn (*anyopaque, *ProcessSetup) callconv(.c) tresult,
    setProcessing: *const fn (*anyopaque, u8) callconv(.c) tresult,
    process: *const fn (*anyopaque, *ProcessData) callconv(.c) tresult,
    getTailSamples: *const fn (*anyopaque) callconv(.c) u32,
};
pub const Processor = extern struct { vtbl: *const ProcessorVtbl };

/// Module entry/exit + factory getter symbol names (Linux).
pub const SYM_GET_FACTORY = "GetPluginFactory";
pub const SYM_MODULE_ENTRY = "ModuleEntry";
pub const SYM_MODULE_EXIT = "ModuleExit";

test "uid encodes FUnknown iid big-endian" {
    // FUnknown = INLINE_UID(0,0,0xC0000000,0x00000046) -> bytes ...C0 00 00 00 / 00 00 00 46
    const u = FUnknown_iid;
    try std.testing.expectEqual(@as(u8, 0xC0), u[8]);
    try std.testing.expectEqual(@as(u8, 0x46), u[15]);
    try std.testing.expectEqual(@as(u8, 0x00), u[0]);
}
