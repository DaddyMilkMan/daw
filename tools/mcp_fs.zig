//! mcp_fs.zig — MCP JSON-RPC stdio filesystem server.
//! Replaces tools/mcp_fs_stdio_server.py.
//!
//! Reads newline-delimited JSON-RPC 2.0 messages from stdin, writes responses
//! to stdout. Exposes 4 tools: list_directory, read_file, write_file, delete_path.
//!
//! Usage:
//!   zenith_mcp_fs --root /path/to/project
//!   MCP_FS_ROOT=/path zenith_mcp_fs

const std = @import("std");

const PROTOCOL_VERSION = "2024-11-05";
const SERVER_NAME = "zenith-fs-stdio";
const SERVER_VERSION = "1.0.0";

const TOOLS_JSON =
    \\[{"name":"list_directory","description":"List directory entries relative to root.","inputSchema":{"type":"object","properties":{"path":{"type":"string"}}}},{"name":"read_file","description":"Read a text file relative to root.","inputSchema":{"type":"object","properties":{"path":{"type":"string"}},"required":["path"]}},{"name":"write_file","description":"Write a text file relative to root.","inputSchema":{"type":"object","properties":{"path":{"type":"string"},"content":{"type":"string"}},"required":["path","content"]}},{"name":"delete_path","description":"Delete a file or directory relative to root.","inputSchema":{"type":"object","properties":{"path":{"type":"string"}},"required":["path"]}}]
;

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const alloc = gpa.allocator();

    const args = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, args);

    var root_arg: []const u8 = std.posix.getenv("MCP_FS_ROOT") orelse "";
    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        if (std.mem.eql(u8, args[i], "--root") and i + 1 < args.len) {
            i += 1;
            root_arg = args[i];
        }
    }
    if (root_arg.len == 0) {
        try std.io.getStdErr().writer().writeAll("Missing --root or MCP_FS_ROOT\n");
        std.process.exit(2);
    }

    var root_buf: [std.fs.max_path_bytes]u8 = undefined;
    const root = std.fs.realpath(root_arg, &root_buf) catch |e| {
        try std.io.getStdErr().writer().print("Cannot resolve root {s}: {}\n", .{ root_arg, e });
        std.process.exit(2);
    };

    const stdin_file = std.io.getStdIn();
    var buf_reader = std.io.bufferedReader(stdin_file.reader());
    const reader = buf_reader.reader();
    const stdout = std.io.getStdOut().writer();

    var line_buf = std.ArrayList(u8).init(alloc);
    defer line_buf.deinit();

    var initialized = false;

    outer: while (true) {
        line_buf.clearRetainingCapacity();
        reader.readUntilDelimiterArrayList(&line_buf, '\n', 4 * 1024 * 1024) catch |e| switch (e) {
            error.EndOfStream => {
                if (line_buf.items.len == 0) break;
            },
            else => return e,
        };
        const line = std.mem.trim(u8, line_buf.items, &std.ascii.whitespace);
        if (line.len == 0) continue;

        const doc = std.json.parseFromSlice(std.json.Value, alloc, line, .{}) catch {
            try sendError(stdout, alloc, null, -32700, "Parse error");
            continue;
        };
        defer doc.deinit();

        const obj = switch (doc.value) {
            .object => |o| o,
            else => {
                try sendError(stdout, alloc, null, -32600, "Not an object");
                continue;
            },
        };

        const id = obj.get("id");
        const method = getString(obj, "method") orelse {
            try sendError(stdout, alloc, id, -32600, "Missing method");
            continue;
        };

        if (std.mem.eql(u8, method, "initialize")) {
            try sendResult(stdout, alloc, id,
                \\{"protocolVersion":"2024-11-05","capabilities":{"tools":{},"resources":{}},"serverInfo":{"name":"zenith-fs-stdio","version":"1.0.0"}}
            );
            continue;
        }
        if (std.mem.eql(u8, method, "notifications/initialized")) {
            initialized = true;
            continue;
        }
        if (std.mem.eql(u8, method, "ping")) {
            try sendResult(stdout, alloc, id, "{}");
            continue;
        }
        if (!initialized) {
            try sendError(stdout, alloc, id, -32002, "Not initialized");
            continue;
        }
        if (std.mem.eql(u8, method, "tools/list")) {
            try sendResult(stdout, alloc, id, "{\"tools\":" ++ TOOLS_JSON ++ "}");
            continue;
        }
        if (std.mem.eql(u8, method, "tools/call")) {
            const p = getObject(obj, "params") orelse {
                try sendError(stdout, alloc, id, -32600, "Missing params");
                continue;
            };
            const tool_name = getString(p, "name") orelse {
                try sendError(stdout, alloc, id, -32600, "Missing tool name");
                continue;
            };
            const tool_args = getObject(p, "arguments");

            if (std.mem.eql(u8, tool_name, "list_directory")) {
                const sub = if (tool_args) |a| getString(a, "path") else null;
                const text = listDir(alloc, root, sub) catch |e| {
                    const msg = try std.fmt.allocPrint(alloc, "Tool failed: {}", .{e});
                    defer alloc.free(msg);
                    try sendError(stdout, alloc, id, -32003, msg);
                    continue :outer;
                };
                defer alloc.free(text);
                try sendTextContent(stdout, alloc, id, text);
            } else if (std.mem.eql(u8, tool_name, "read_file")) {
                const sub = (if (tool_args) |a| getString(a, "path") else null) orelse {
                    try sendError(stdout, alloc, id, -32600, "Missing path");
                    continue;
                };
                const text = readFile(alloc, root, sub) catch |e| {
                    const msg = try std.fmt.allocPrint(alloc, "Tool failed: {}", .{e});
                    defer alloc.free(msg);
                    try sendError(stdout, alloc, id, -32003, msg);
                    continue :outer;
                };
                defer alloc.free(text);
                try sendTextContent(stdout, alloc, id, text);
            } else if (std.mem.eql(u8, tool_name, "write_file")) {
                const sub = (if (tool_args) |a| getString(a, "path") else null) orelse {
                    try sendError(stdout, alloc, id, -32600, "Missing path");
                    continue;
                };
                const content = (if (tool_args) |a| getString(a, "content") else null) orelse {
                    try sendError(stdout, alloc, id, -32600, "Missing content");
                    continue;
                };
                writeFile(alloc, root, sub, content) catch |e| {
                    const msg = try std.fmt.allocPrint(alloc, "Tool failed: {}", .{e});
                    defer alloc.free(msg);
                    try sendError(stdout, alloc, id, -32003, msg);
                    continue :outer;
                };
                try sendTextContent(stdout, alloc, id, "ok");
            } else if (std.mem.eql(u8, tool_name, "delete_path")) {
                const sub = (if (tool_args) |a| getString(a, "path") else null) orelse {
                    try sendError(stdout, alloc, id, -32600, "Missing path");
                    continue;
                };
                deletePath(alloc, root, sub) catch |e| {
                    const msg = try std.fmt.allocPrint(alloc, "Tool failed: {}", .{e});
                    defer alloc.free(msg);
                    try sendError(stdout, alloc, id, -32003, msg);
                    continue :outer;
                };
                try sendTextContent(stdout, alloc, id, "ok");
            } else {
                const msg = try std.fmt.allocPrint(alloc, "Tool not found: {s}", .{tool_name});
                defer alloc.free(msg);
                try sendError(stdout, alloc, id, -32002, msg);
            }
            continue;
        }

        const msg = try std.fmt.allocPrint(alloc, "Method not found: {s}", .{method});
        defer alloc.free(msg);
        try sendError(stdout, alloc, id, -32601, msg);
    }
}

// ---- filesystem operations -------------------------------------------------

fn resolveSafe(alloc: std.mem.Allocator, root: []const u8, sub: ?[]const u8) ![]const u8 {
    const joined = if (sub) |s|
        try std.fs.path.join(alloc, &.{ root, s })
    else
        try alloc.dupe(u8, root);
    defer alloc.free(joined);

    var real_buf: [std.fs.max_path_bytes]u8 = undefined;
    const real = std.fs.realpath(joined, &real_buf) catch |e| switch (e) {
        error.FileNotFound => {
            // For write, parent may exist but file not yet — check parent is safe
            const parent = std.fs.path.dirname(joined) orelse root;
            var pb: [std.fs.max_path_bytes]u8 = undefined;
            const rp = try std.fs.realpath(parent, &pb);
            if (!std.mem.startsWith(u8, rp, root)) return error.PermissionDenied;
            return alloc.dupe(u8, joined);
        },
        else => return e,
    };
    if (!std.mem.startsWith(u8, real, root)) return error.PermissionDenied;
    return alloc.dupe(u8, real);
}

fn listDir(alloc: std.mem.Allocator, root: []const u8, sub: ?[]const u8) ![]const u8 {
    const path = try resolveSafe(alloc, root, sub);
    defer alloc.free(path);

    var dir = try std.fs.openDirAbsolute(path, .{ .iterate = true });
    defer dir.close();

    var entries = std.ArrayList([]const u8).init(alloc);
    defer {
        for (entries.items) |e| alloc.free(e);
        entries.deinit();
    }

    var it = dir.iterate();
    while (try it.next()) |entry| {
        const suffix = if (entry.kind == .directory) "/" else "";
        try entries.append(try std.fmt.allocPrint(alloc, "{s}{s}", .{ entry.name, suffix }));
    }

    std.mem.sort([]const u8, entries.items, {}, struct {
        fn lt(_: void, a: []const u8, b: []const u8) bool {
            return std.mem.lessThan(u8, a, b);
        }
    }.lt);

    return std.mem.join(alloc, "\n", entries.items);
}

fn readFile(alloc: std.mem.Allocator, root: []const u8, sub: []const u8) ![]const u8 {
    const path = try resolveSafe(alloc, root, sub);
    defer alloc.free(path);
    return std.fs.cwd().readFileAlloc(alloc, path, 64 * 1024 * 1024);
}

fn writeFile(alloc: std.mem.Allocator, root: []const u8, sub: []const u8, content: []const u8) !void {
    const path = try resolveSafe(alloc, root, sub);
    defer alloc.free(path);
    const parent = std.fs.path.dirname(path) orelse "/";
    try std.fs.makeDirAbsolute(parent);
    try std.fs.cwd().writeFile(.{ .sub_path = path, .data = content });
}

fn deletePath(alloc: std.mem.Allocator, root: []const u8, sub: []const u8) !void {
    const path = try resolveSafe(alloc, root, sub);
    defer alloc.free(path);
    const stat = try std.fs.cwd().statFile(path);
    if (stat.kind == .directory) {
        try std.fs.deleteTreeAbsolute(path);
    } else {
        try std.fs.deleteFileAbsolute(path);
    }
}

// ---- JSON-RPC helpers ------------------------------------------------------

fn getString(obj: std.json.ObjectMap, key: []const u8) ?[]const u8 {
    const v = obj.get(key) orelse return null;
    return switch (v) {
        .string => |s| s,
        else => null,
    };
}

fn getObject(obj: std.json.ObjectMap, key: []const u8) ?std.json.ObjectMap {
    const v = obj.get(key) orelse return null;
    return switch (v) {
        .object => |o| o,
        else => null,
    };
}

fn idJson(alloc: std.mem.Allocator, id: ?std.json.Value) ![]const u8 {
    if (id) |v| {
        var buf = std.ArrayList(u8).init(alloc);
        try std.json.stringify(v, .{}, buf.writer());
        return buf.toOwnedSlice();
    }
    return alloc.dupe(u8, "null");
}

fn sendResult(writer: anytype, alloc: std.mem.Allocator, id: ?std.json.Value, result_json: []const u8) !void {
    const id_str = try idJson(alloc, id);
    defer alloc.free(id_str);
    try writer.print(
        "{{\"jsonrpc\":\"2.0\",\"id\":{s},\"result\":{s}}}\n",
        .{ id_str, result_json },
    );
}

fn sendError(writer: anytype, alloc: std.mem.Allocator, id: ?std.json.Value, code: i32, message: []const u8) !void {
    const id_str = try idJson(alloc, id);
    defer alloc.free(id_str);
    var msg_buf = std.ArrayList(u8).init(alloc);
    defer msg_buf.deinit();
    try std.json.stringify(message, .{}, msg_buf.writer());
    try writer.print(
        "{{\"jsonrpc\":\"2.0\",\"id\":{s},\"error\":{{\"code\":{d},\"message\":{s}}}}}\n",
        .{ id_str, code, msg_buf.items },
    );
}

fn sendTextContent(writer: anytype, alloc: std.mem.Allocator, id: ?std.json.Value, text: []const u8) !void {
    var text_json = std.ArrayList(u8).init(alloc);
    defer text_json.deinit();
    try std.json.stringify(text, .{}, text_json.writer());
    const result = try std.fmt.allocPrint(
        alloc,
        "{{\"content\":[{{\"type\":\"text\",\"text\":{s}}}]}}",
        .{text_json.items},
    );
    defer alloc.free(result);
    try sendResult(writer, alloc, id, result);
}
