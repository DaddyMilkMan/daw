//! clap_host.zig — reusable CLAP host: directory scanning + plugin inspection.
//! Walks the standard CLAP search paths, reads each plugin's descriptors, and
//! queries the audio-ports / note-ports / params extensions on a live instance.
//! Hand-declared ABI (clap_abi.zig) — no SDK link. Pure Zig + libc (dlopen).

const std = @import("std");
const abi = @import("clap_abi.zig");

fn hostGetExt(_: *const abi.Host, _: [*:0]const u8) callconv(.c) ?*const anyopaque {
    return null;
}
fn hostReq(_: *const abi.Host) callconv(.c) void {}

/// A host descriptor to hand to `create_plugin`.
pub fn makeHost() abi.Host {
    return .{
        .clap_version = .{},
        .host_data = null,
        .name = "Zenith",
        .vendor = "Zenith",
        .url = "",
        .version = "0.1",
        .get_extension = hostGetExt,
        .request_restart = hostReq,
        .request_process = hostReq,
        .request_callback = hostReq,
    };
}

pub const PluginInfo = struct {
    id: []u8,
    name: []u8,
    vendor: []u8,
    is_instrument: bool,

    pub fn deinit(self: *PluginInfo, a: std.mem.Allocator) void {
        a.free(self.id);
        a.free(self.name);
        a.free(self.vendor);
    }
};

fn dupZ(a: std.mem.Allocator, p: ?[*:0]const u8) ![]u8 {
    const s = if (p) |q| std.mem.span(q) else "";
    return a.dupe(u8, s);
}

fn hasFeature(desc: *const abi.PluginDescriptor, want: []const u8) bool {
    const feats = desc.features orelse return false;
    var i: usize = 0;
    while (feats[i]) |f| : (i += 1) {
        if (std.mem.eql(u8, std.mem.span(f), want)) return true;
    }
    return false;
}

/// Open one .clap (a shared object), read every plugin descriptor it exposes,
/// copy the metadata out (owned), and close the library. Caller frees each info.
pub fn scanFile(a: std.mem.Allocator, path: []const u8, out: *std.ArrayList(PluginInfo)) !void {
    var lib = try std.DynLib.open(path);
    defer lib.close();
    const entry = lib.lookup(*const abi.PluginEntry, "clap_entry") orelse return error.NoEntry;
    if (entry.init) |ini| {
        if (!ini(@ptrCast(path.ptr))) return error.EntryInit;
    }
    defer if (entry.deinit) |de| de();

    const raw = entry.get_factory.?(abi.PLUGIN_FACTORY_ID) orelse return error.NoFactory;
    const fac: *const abi.PluginFactory = @ptrCast(@alignCast(raw));
    const n = fac.get_plugin_count.?(fac);
    var i: u32 = 0;
    while (i < n) : (i += 1) {
        const desc = fac.get_plugin_descriptor.?(fac, i) orelse continue;
        try out.append(.{
            .id = try dupZ(a, desc.id),
            .name = try dupZ(a, desc.name),
            .vendor = try dupZ(a, desc.vendor),
            .is_instrument = hasFeature(desc, "instrument"),
        });
    }
}

/// The standard Linux CLAP search paths (+ $CLAP_PATH). Caller frees each path.
pub fn searchPaths(a: std.mem.Allocator) ![][]u8 {
    var list = std.ArrayList([]u8).init(a);
    errdefer {
        for (list.items) |p| a.free(p);
        list.deinit();
    }
    if (std.posix.getenv("HOME")) |home| {
        try list.append(try std.fs.path.join(a, &.{ home, ".clap" }));
    }
    try list.append(try a.dupe(u8, "/usr/lib/clap"));
    try list.append(try a.dupe(u8, "/usr/local/lib/clap"));
    if (std.posix.getenv("CLAP_PATH")) |cp| {
        var it = std.mem.tokenizeScalar(u8, cp, ':');
        while (it.next()) |seg| try list.append(try a.dupe(u8, seg));
    }
    return list.toOwnedSlice();
}

/// Recursively find every `*.clap` under the standard search paths. Owned paths.
pub fn findClapFiles(a: std.mem.Allocator) ![][]u8 {
    var files = std.ArrayList([]u8).init(a);
    errdefer {
        for (files.items) |p| a.free(p);
        files.deinit();
    }
    const roots = try searchPaths(a);
    defer {
        for (roots) |p| a.free(p);
        a.free(roots);
    }
    for (roots) |root| {
        var dir = std.fs.openDirAbsolute(root, .{ .iterate = true }) catch continue;
        defer dir.close();
        var walker = dir.walk(a) catch continue;
        defer walker.deinit();
        while (walker.next() catch null) |entry| {
            if (entry.kind == .file and std.mem.endsWith(u8, entry.path, ".clap")) {
                try files.append(try std.fs.path.join(a, &.{ root, entry.path }));
            }
        }
    }
    return files.toOwnedSlice();
}

// --- live-instance inspection ---
pub const Ports = struct {
    audio_in: u32 = 0,
    audio_out: u32 = 0,
    note_in: u32 = 0,
    note_out: u32 = 0,
};

pub fn queryPorts(plugin: *const abi.Plugin) Ports {
    var p = Ports{};
    if (plugin.get_extension.?(plugin, abi.EXT_AUDIO_PORTS)) |raw| {
        const ap: *const abi.PluginAudioPorts = @ptrCast(@alignCast(raw));
        p.audio_in = ap.count.?(plugin, true);
        p.audio_out = ap.count.?(plugin, false);
    }
    if (plugin.get_extension.?(plugin, abi.EXT_NOTE_PORTS)) |raw| {
        const np: *const abi.PluginNotePorts = @ptrCast(@alignCast(raw));
        p.note_in = np.count.?(plugin, true);
        p.note_out = np.count.?(plugin, false);
    }
    return p;
}

pub fn paramsExt(plugin: *const abi.Plugin) ?*const abi.PluginParams {
    const raw = plugin.get_extension.?(plugin, abi.EXT_PARAMS) orelse return null;
    return @ptrCast(@alignCast(raw));
}
