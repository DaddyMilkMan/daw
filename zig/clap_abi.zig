//! clap_abi.zig — minimal CLAP plugin ABI, hand-declared in Zig (no @cImport).
//! Mirrors the stable CLAP C headers (clap.h) so a real third-party .clap loads
//! the same way our test plugin does. Both host and plugin share these defs.

pub const Version = extern struct {
    major: u32 = 1,
    minor: u32 = 2,
    revision: u32 = 0,
};

pub const PluginDescriptor = extern struct {
    clap_version: Version,
    id: ?[*:0]const u8,
    name: ?[*:0]const u8,
    vendor: ?[*:0]const u8,
    url: ?[*:0]const u8,
    manual_url: ?[*:0]const u8,
    support_url: ?[*:0]const u8,
    version: ?[*:0]const u8,
    description: ?[*:0]const u8,
    features: ?[*]const ?[*:0]const u8,
};

pub const Host = extern struct {
    clap_version: Version,
    host_data: ?*anyopaque,
    name: ?[*:0]const u8,
    vendor: ?[*:0]const u8,
    url: ?[*:0]const u8,
    version: ?[*:0]const u8,
    get_extension: ?*const fn (*const Host, [*:0]const u8) callconv(.c) ?*const anyopaque,
    request_restart: ?*const fn (*const Host) callconv(.c) void,
    request_process: ?*const fn (*const Host) callconv(.c) void,
    request_callback: ?*const fn (*const Host) callconv(.c) void,
};

pub const AudioBuffer = extern struct {
    data32: ?[*]const ?[*]f32,
    data64: ?[*]const ?[*]f64,
    channel_count: u32,
    latency: u32,
    constant_mask: u64,
};

pub const EventHeader = extern struct {
    size: u32,
    time: u32,
    space_id: u16,
    type: u16,
    flags: u32,
};

pub const InputEvents = extern struct {
    ctx: ?*anyopaque,
    size: ?*const fn (*const InputEvents) callconv(.c) u32,
    get: ?*const fn (*const InputEvents, u32) callconv(.c) ?*const EventHeader,
};

pub const OutputEvents = extern struct {
    ctx: ?*anyopaque,
    try_push: ?*const fn (*const OutputEvents, *const EventHeader) callconv(.c) bool,
};

pub const Process = extern struct {
    steady_time: i64,
    frames_count: u32,
    transport: ?*const anyopaque,
    audio_inputs: ?[*]const AudioBuffer,
    audio_outputs: ?[*]AudioBuffer,
    audio_inputs_count: u32,
    audio_outputs_count: u32,
    in_events: ?*const InputEvents,
    out_events: ?*const OutputEvents,
};

pub const Plugin = extern struct {
    desc: ?*const PluginDescriptor,
    plugin_data: ?*anyopaque,
    init: ?*const fn (*const Plugin) callconv(.c) bool,
    destroy: ?*const fn (*const Plugin) callconv(.c) void,
    activate: ?*const fn (*const Plugin, f64, u32, u32) callconv(.c) bool,
    deactivate: ?*const fn (*const Plugin) callconv(.c) void,
    start_processing: ?*const fn (*const Plugin) callconv(.c) bool,
    stop_processing: ?*const fn (*const Plugin) callconv(.c) void,
    reset: ?*const fn (*const Plugin) callconv(.c) void,
    process: ?*const fn (*const Plugin, *const Process) callconv(.c) i32,
    get_extension: ?*const fn (*const Plugin, [*:0]const u8) callconv(.c) ?*const anyopaque,
    on_main_thread: ?*const fn (*const Plugin) callconv(.c) void,
};

pub const PluginFactory = extern struct {
    get_plugin_count: ?*const fn (*const PluginFactory) callconv(.c) u32,
    get_plugin_descriptor: ?*const fn (*const PluginFactory, u32) callconv(.c) ?*const PluginDescriptor,
    create_plugin: ?*const fn (*const PluginFactory, *const Host, [*:0]const u8) callconv(.c) ?*const Plugin,
};

/// The exported symbol every .clap file provides: `clap_entry`.
pub const PluginEntry = extern struct {
    clap_version: Version,
    init: ?*const fn ([*:0]const u8) callconv(.c) bool,
    deinit: ?*const fn () callconv(.c) void,
    get_factory: ?*const fn ([*:0]const u8) callconv(.c) ?*const anyopaque,
};

pub const PLUGIN_FACTORY_ID: [*:0]const u8 = "clap.plugin-factory";
pub const PROCESS_CONTINUE: i32 = 1;

// Note events (clap_event_note) — how the host drives an instrument plugin.
pub const CORE_EVENT_SPACE_ID: u16 = 0;
pub const EVENT_NOTE_ON: u16 = 0;
pub const EVENT_NOTE_OFF: u16 = 1;

pub const EventNote = extern struct {
    header: EventHeader,
    note_id: i32,
    port_index: i16,
    channel: i16,
    key: i16,
    velocity: f64,
};
