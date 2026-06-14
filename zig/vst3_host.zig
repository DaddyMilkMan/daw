//! vst3_host.zig — load a VST3 module, reach its factory, and instantiate the
//! IComponent / IAudioProcessor. Resolves Linux `.vst3` bundles or a bare `.so`.
//! Hand-declared ABI (vst3_abi.zig); links libc for dlopen.

const std = @import("std");
const v = @import("vst3_abi.zig");

pub const Module = struct {
    lib: std.DynLib,
    factory: *v.Factory,
    has_exit: bool,

    /// Inside a `Name.vst3` bundle the binary is Contents/x86_64-linux/Name.so.
    fn resolve(a: std.mem.Allocator, path: []const u8) ![]u8 {
        if (!std.mem.endsWith(u8, path, ".vst3")) return a.dupe(u8, path);
        const stat = std.fs.cwd().statFile(path) catch return a.dupe(u8, path);
        if (stat.kind != .directory) return a.dupe(u8, path);
        const base = std.fs.path.basename(path);
        const name = base[0 .. base.len - ".vst3".len];
        return std.fmt.allocPrint(a, "{s}/Contents/x86_64-linux/{s}.so", .{ path, name });
    }

    pub fn open(a: std.mem.Allocator, path: []const u8) !Module {
        const real = try resolve(a, path);
        defer a.free(real);
        var lib = try std.DynLib.open(real);
        errdefer lib.close();

        if (lib.lookup(*const fn (?*anyopaque) callconv(.c) bool, v.SYM_MODULE_ENTRY)) |entry| {
            if (!entry(null)) return error.ModuleEntryFailed;
        }
        const get = lib.lookup(*const fn () callconv(.c) ?*v.Factory, v.SYM_GET_FACTORY) orelse return error.NoFactorySymbol;
        const factory = get() orelse return error.NoFactory;
        return .{ .lib = lib, .factory = factory, .has_exit = lib.lookup(*const anyopaque, v.SYM_MODULE_EXIT) != null };
    }

    pub fn close(self: *Module) void {
        if (self.lib.lookup(*const fn () callconv(.c) bool, v.SYM_MODULE_EXIT)) |exit| _ = exit();
        self.lib.close();
    }

    pub fn factoryInfo(self: *Module) v.PFactoryInfo {
        var info: v.PFactoryInfo = std.mem.zeroes(v.PFactoryInfo);
        _ = self.factory.vtbl.getFactoryInfo(self.factory, &info);
        return info;
    }
    pub fn classCount(self: *Module) i32 {
        return self.factory.vtbl.countClasses(self.factory);
    }
    pub fn classInfo(self: *Module, index: i32) ?v.PClassInfo {
        var info: v.PClassInfo = std.mem.zeroes(v.PClassInfo);
        if (self.factory.vtbl.getClassInfo(self.factory, index, &info) != v.kResultOk) return null;
        return info;
    }

    /// Create the IComponent for an "Audio Module Class". Returns null if none.
    pub fn createComponent(self: *Module) !*v.Component {
        const n = self.classCount();
        var i: i32 = 0;
        while (i < n) : (i += 1) {
            const ci = self.classInfo(i) orelse continue;
            if (!std.mem.startsWith(u8, sliceZ(&ci.category), "Audio Module Class")) continue;
            var obj: ?*anyopaque = null;
            const r = self.factory.vtbl.createInstance(self.factory, &ci.cid, &v.IComponent_iid, &obj);
            if (r == v.kResultOk and obj != null) return @ptrCast(@alignCast(obj.?));
        }
        return error.NoAudioClass;
    }
};

/// Query the IAudioProcessor sub-interface from a component.
pub fn getProcessor(component: *v.Component) ?*v.Processor {
    var obj: ?*anyopaque = null;
    if (component.vtbl.queryInterface(component, &v.IAudioProcessor_iid, &obj) != v.kResultOk) return null;
    return @ptrCast(@alignCast(obj orelse return null));
}

pub fn sliceZ(buf: []const u8) []const u8 {
    const end = std.mem.indexOfScalar(u8, buf, 0) orelse buf.len;
    return buf[0..end];
}
