//! log.zig — the app's "console": a std.log backend that tees every log line to
//! a file (ZENITH_LOG, default zenith.log) AND stderr, with a monotonic timestamp,
//! level, and scope. Thread-safe (audio/input/UI threads all log here). Install in
//! the root file: `pub const std_options: std.Options = .{ .log_level = .debug,
//! .logFn = @import("log.zig").logFn };` then use std.log.info(...) anywhere.

const std = @import("std");

var g_file: ?std.fs.File = null;
var g_timer: ?std.time.Timer = null;
var g_open = false;
var g_mutex: std.Thread.Mutex = .{};

fn ensureOpen() void {
    if (g_open) return;
    g_open = true;
    g_timer = std.time.Timer.start() catch null;
    const owned = std.process.getEnvVarOwned(std.heap.page_allocator, "ZENITH_LOG") catch null;
    defer if (owned) |p| std.heap.page_allocator.free(p);
    const path = owned orelse "zenith.log";
    g_file = std.fs.cwd().createFile(path, .{}) catch null;
}

pub fn logFn(
    comptime level: std.log.Level,
    comptime scope: @Type(.enum_literal),
    comptime format: []const u8,
    args: anytype,
) void {
    g_mutex.lock();
    defer g_mutex.unlock();
    ensureOpen();
    const ms: u64 = if (g_timer) |*t| t.read() / std.time.ns_per_ms else 0;
    const scope_txt = comptime if (scope == .default) "" else "(" ++ @tagName(scope) ++ ")";
    const line = "[{d:>8}ms] " ++ comptime level.asText() ++ scope_txt ++ ": " ++ format ++ "\n";
    const full_args = .{ms} ++ args;

    if (g_file) |f| f.writer().print(line, full_args) catch {};
    std.io.getStdErr().writer().print(line, full_args) catch {};
}
