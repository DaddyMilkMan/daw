//! ai_provider.zig — the LLM provider seam for Zenith's AI wedge.
//!
//! This is the *only* place the AI talks to an outside model. Everything above
//! it (the agent loop, the tool registry over the project model) is provider-
//! agnostic. The wire format is OpenAI-compatible chat/completions with
//! function "tools" + `tool_choice:auto`, so xAI Grok today is a config swap
//! away from OpenAI / Anthropic-compatible endpoints tomorrow.
//!
//! Two implementations ship here:
//!   * `GrokProvider` — real HTTPS via `std.http.Client`. The request builder
//!     and response parser are pure functions (`buildRequestBody` /
//!     `parseResponse`) so they unit-test without a network or an API key.
//!   * `MockProvider` — replays a scripted sequence of responses, so the agent
//!     loop + tool dispatch verify fully offline under `zig build test`.

const std = @import("std");

pub const Role = enum { system, user, assistant, tool };

/// One function/tool invocation the model wants performed. `arguments` is a raw
/// JSON string (OpenAI-style) decoded by the tool registry, not here.
pub const ToolCall = struct {
    id: []const u8,
    name: []const u8,
    arguments: []const u8,
};

/// A turn in the conversation. For `assistant` turns that call tools, `content`
/// may be empty and `tool_calls` populated. For `tool` turns, `tool_call_id`
/// and `name` identify which call this is the result of.
pub const Message = struct {
    role: Role,
    content: []const u8 = "",
    tool_calls: []const ToolCall = &.{},
    tool_call_id: ?[]const u8 = null,
    name: ?[]const u8 = null,
};

/// A tool advertised to the model: name + description + a JSON-Schema string for
/// its parameters (generated at comptime by `ai_tools.schemaFor`).
pub const ToolSpec = struct {
    name: []const u8,
    description: []const u8,
    parameters_schema: []const u8,
};

/// The model's reply. Owns its strings via an arena; call `deinit` when done.
pub const ChatResponse = struct {
    content: []const u8,
    tool_calls: []const ToolCall,
    arena: *std.heap.ArenaAllocator,

    pub fn deinit(self: *ChatResponse) void {
        const a = self.arena;
        const child = a.child_allocator;
        a.deinit();
        child.destroy(a);
    }
};

/// The provider interface — a fat pointer (impl + chat fn). Implementations
/// build their own `Provider` via `.provider()`.
pub const Provider = struct {
    ptr: *anyopaque,
    chatFn: *const fn (ptr: *anyopaque, alloc: std.mem.Allocator, messages: []const Message, tools: []const ToolSpec) anyerror!ChatResponse,

    pub fn chat(self: Provider, alloc: std.mem.Allocator, messages: []const Message, tools: []const ToolSpec) anyerror!ChatResponse {
        return self.chatFn(self.ptr, alloc, messages, tools);
    }
};

/// Build a `ChatResponse` that owns deep copies of `content` + `calls` in a
/// fresh arena. Both providers funnel through this so ownership is uniform.
pub fn makeResponse(child: std.mem.Allocator, content: []const u8, calls: []const ToolCall) !ChatResponse {
    const arena = try child.create(std.heap.ArenaAllocator);
    arena.* = std.heap.ArenaAllocator.init(child);
    errdefer {
        arena.deinit();
        child.destroy(arena);
    }
    const a = arena.allocator();
    const owned_calls = try a.alloc(ToolCall, calls.len);
    for (calls, 0..) |c, i| {
        owned_calls[i] = .{
            .id = try a.dupe(u8, c.id),
            .name = try a.dupe(u8, c.name),
            .arguments = try a.dupe(u8, c.arguments),
        };
    }
    return .{
        .content = try a.dupe(u8, content),
        .tool_calls = owned_calls,
        .arena = arena,
    };
}

// ---------------------------------------------------------------------------
// Grok (xAI) — real HTTPS, OpenAI-compatible chat/completions.
// ---------------------------------------------------------------------------

pub const GrokProvider = struct {
    api_key: []const u8,
    base_url: []const u8 = "https://api.x.ai/v1",
    model: []const u8 = "grok-4.1-fast",
    temperature: f64 = 0.3,

    pub fn provider(self: *GrokProvider) Provider {
        return .{ .ptr = self, .chatFn = chatThunk };
    }

    fn chatThunk(ptr: *anyopaque, alloc: std.mem.Allocator, messages: []const Message, tools: []const ToolSpec) anyerror!ChatResponse {
        const self: *GrokProvider = @ptrCast(@alignCast(ptr));
        return self.chat(alloc, messages, tools);
    }

    pub fn chat(self: *GrokProvider, alloc: std.mem.Allocator, messages: []const Message, tools: []const ToolSpec) !ChatResponse {
        const body = try buildRequestBody(alloc, self.model, self.temperature, messages, tools);
        defer alloc.free(body);

        const url = try std.fmt.allocPrint(alloc, "{s}/chat/completions", .{self.base_url});
        defer alloc.free(url);
        const auth = try std.fmt.allocPrint(alloc, "Bearer {s}", .{self.api_key});
        defer alloc.free(auth);

        var client = std.http.Client{ .allocator = alloc };
        defer client.deinit();

        var response = std.ArrayList(u8).init(alloc);
        defer response.deinit();

        const res = try client.fetch(.{
            .location = .{ .url = url },
            .method = .POST,
            .payload = body,
            .headers = .{ .content_type = .{ .override = "application/json" } },
            .extra_headers = &.{.{ .name = "Authorization", .value = auth }},
            .response_storage = .{ .dynamic = &response },
            .max_append_size = 8 * 1024 * 1024,
        });
        if (res.status != .ok) {
            std.log.err("Grok HTTP {d}: {s}", .{ @intFromEnum(res.status), response.items });
            return error.ProviderHttpError;
        }
        return parseResponse(alloc, response.items) catch |e| {
            std.log.err("Grok response ({s}): {s}", .{ @errorName(e), response.items });
            return e;
        };
    }
};

/// Serialize an OpenAI-style chat request. Pure (no I/O) so it unit-tests.
pub fn buildRequestBody(
    alloc: std.mem.Allocator,
    model: []const u8,
    temperature: f64,
    messages: []const Message,
    tools: []const ToolSpec,
) ![]u8 {
    var out = std.ArrayList(u8).init(alloc);
    errdefer out.deinit();
    var ws = std.json.writeStream(out.writer(), .{});
    defer ws.deinit();

    try ws.beginObject();
    try ws.objectField("model");
    try ws.write(model);
    try ws.objectField("temperature");
    try ws.write(temperature);

    try ws.objectField("messages");
    try ws.beginArray();
    for (messages) |m| {
        try ws.beginObject();
        try ws.objectField("role");
        try ws.write(@tagName(m.role));
        try ws.objectField("content");
        try ws.write(m.content);
        if (m.tool_call_id) |id| {
            try ws.objectField("tool_call_id");
            try ws.write(id);
        }
        if (m.name) |n| {
            try ws.objectField("name");
            try ws.write(n);
        }
        if (m.tool_calls.len > 0) {
            try ws.objectField("tool_calls");
            try ws.beginArray();
            for (m.tool_calls) |tc| {
                try ws.beginObject();
                try ws.objectField("id");
                try ws.write(tc.id);
                try ws.objectField("type");
                try ws.write("function");
                try ws.objectField("function");
                try ws.beginObject();
                try ws.objectField("name");
                try ws.write(tc.name);
                try ws.objectField("arguments");
                try ws.write(tc.arguments); // arguments is a JSON *string* per the spec
                try ws.endObject();
                try ws.endObject();
            }
            try ws.endArray();
        }
        try ws.endObject();
    }
    try ws.endArray();

    if (tools.len > 0) {
        try ws.objectField("tools");
        try ws.beginArray();
        for (tools) |t| {
            try ws.beginObject();
            try ws.objectField("type");
            try ws.write("function");
            try ws.objectField("function");
            try ws.beginObject();
            try ws.objectField("name");
            try ws.write(t.name);
            try ws.objectField("description");
            try ws.write(t.description);
            try ws.objectField("parameters");
            // parameters_schema is already JSON — embed it raw.
            try ws.beginWriteRaw();
            try ws.stream.writeAll(t.parameters_schema);
            ws.endWriteRaw();
            try ws.endObject();
            try ws.endObject();
        }
        try ws.endArray();
        try ws.objectField("tool_choice");
        try ws.write("auto");
    }
    try ws.endObject();

    return out.toOwnedSlice();
}

/// Parse an OpenAI-style chat response into a `ChatResponse`. Pure; unit-tested.
pub fn parseResponse(alloc: std.mem.Allocator, body: []const u8) !ChatResponse {
    const Wire = struct {
        choices: []const struct {
            message: struct {
                content: ?[]const u8 = null,
                tool_calls: ?[]const struct {
                    id: []const u8,
                    function: struct {
                        name: []const u8,
                        arguments: []const u8,
                    },
                } = null,
            },
        } = &.{},
        @"error": ?struct { message: []const u8 } = null,
    };

    var parsed = std.json.parseFromSlice(Wire, alloc, body, .{ .ignore_unknown_fields = true }) catch {
        return error.ProviderBadResponse;
    };
    defer parsed.deinit();
    const w = parsed.value;

    if (w.@"error") |_| return error.ProviderApiError;
    if (w.choices.len == 0) return makeResponse(alloc, "", &.{});

    const msg = w.choices[0].message;
    const content = msg.content orelse "";
    if (msg.tool_calls) |tcs| {
        var calls = try alloc.alloc(ToolCall, tcs.len);
        defer alloc.free(calls);
        for (tcs, 0..) |tc, i| {
            calls[i] = .{ .id = tc.id, .name = tc.function.name, .arguments = tc.function.arguments };
        }
        return makeResponse(alloc, content, calls);
    }
    return makeResponse(alloc, content, &.{});
}

// ---------------------------------------------------------------------------
// Mock provider — replays a scripted sequence (offline tests + demos).
// ---------------------------------------------------------------------------

pub const MockProvider = struct {
    /// One canned reply per chat() call, consumed in order.
    pub const Canned = struct {
        content: []const u8 = "",
        tool_calls: []const ToolCall = &.{},
    };

    script: []const Canned,
    idx: usize = 0,

    pub fn provider(self: *MockProvider) Provider {
        return .{ .ptr = self, .chatFn = chatThunk };
    }

    fn chatThunk(ptr: *anyopaque, alloc: std.mem.Allocator, messages: []const Message, tools: []const ToolSpec) anyerror!ChatResponse {
        _ = messages;
        _ = tools;
        const self: *MockProvider = @ptrCast(@alignCast(ptr));
        if (self.idx >= self.script.len) {
            // Past the script: behave like a model that just says it's done.
            return makeResponse(alloc, "Done.", &.{});
        }
        const c = self.script[self.idx];
        self.idx += 1;
        return makeResponse(alloc, c.content, c.tool_calls);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

test "buildRequestBody emits model, messages, and tools" {
    const a = std.testing.allocator;
    const msgs = [_]Message{
        .{ .role = .system, .content = "you are a DAW assistant" },
        .{ .role = .user, .content = "make a track" },
    };
    const tools = [_]ToolSpec{
        .{ .name = "create_track", .description = "make a track", .parameters_schema = "{\"type\":\"object\",\"properties\":{}}" },
    };
    const body = try buildRequestBody(a, "grok-4.1-fast", 0.3, &msgs, &tools);
    defer a.free(body);

    try std.testing.expect(std.mem.indexOf(u8, body, "\"model\":\"grok-4.1-fast\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, body, "\"role\":\"user\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, body, "\"tool_choice\":\"auto\"") != null);
    try std.testing.expect(std.mem.indexOf(u8, body, "\"name\":\"create_track\"") != null);
    // the raw schema must be embedded as an object, not a quoted string
    try std.testing.expect(std.mem.indexOf(u8, body, "\"parameters\":{\"type\":\"object\"") != null);

    // result must be valid JSON
    var parsed = try std.json.parseFromSlice(std.json.Value, a, body, .{});
    defer parsed.deinit();
    try std.testing.expect(parsed.value == .object);
}

test "parseResponse extracts content and tool calls" {
    const a = std.testing.allocator;
    const text_body =
        \\{"choices":[{"message":{"role":"assistant","content":"hello there"}}]}
    ;
    var r1 = try parseResponse(a, text_body);
    defer r1.deinit();
    try std.testing.expectEqualStrings("hello there", r1.content);
    try std.testing.expectEqual(@as(usize, 0), r1.tool_calls.len);

    const tool_body =
        \\{"choices":[{"message":{"content":null,"tool_calls":[
        \\{"id":"call_1","type":"function","function":{"name":"set_tempo","arguments":"{\"bpm\":128}"}}
        \\]}}]}
    ;
    var r2 = try parseResponse(a, tool_body);
    defer r2.deinit();
    try std.testing.expectEqual(@as(usize, 1), r2.tool_calls.len);
    try std.testing.expectEqualStrings("set_tempo", r2.tool_calls[0].name);
    try std.testing.expectEqualStrings("{\"bpm\":128}", r2.tool_calls[0].arguments);
}

test "parseResponse surfaces API errors" {
    const a = std.testing.allocator;
    const body =
        \\{"error":{"message":"invalid api key"}}
    ;
    try std.testing.expectError(error.ProviderApiError, parseResponse(a, body));
}

test "MockProvider replays its script" {
    const a = std.testing.allocator;
    var mock = MockProvider{ .script = &.{
        .{ .tool_calls = &.{.{ .id = "1", .name = "set_tempo", .arguments = "{\"bpm\":120}" }} },
        .{ .content = "all set" },
    } };
    const p = mock.provider();

    var r1 = try p.chat(a, &.{}, &.{});
    defer r1.deinit();
    try std.testing.expectEqual(@as(usize, 1), r1.tool_calls.len);

    var r2 = try p.chat(a, &.{}, &.{});
    defer r2.deinit();
    try std.testing.expectEqualStrings("all set", r2.content);
}
