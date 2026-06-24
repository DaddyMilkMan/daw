//! ai_agent.zig — the produce-with-you loop.
//!
//! Ties the provider seam (`ai_provider`) to the project-model tool registry
//! (`ai_tools`). One `run(user_msg)`:
//!   1. append the user turn,
//!   2. ask the model (advertising every tool),
//!   3. if it returned tool calls, execute each against the project and feed the
//!      results back,
//!   4. repeat until the model answers in prose (or `max_steps` is hit).
//!
//! All conversation strings are owned by the agent's arena, so the message
//! history handed to the provider each turn stays valid for the agent's life.

const std = @import("std");
const prov = @import("ai_provider.zig");
const tools = @import("ai_tools.zig");

pub const SYSTEM_PROMPT =
    \\You are Zenith's in-DAW production assistant. You help the user build music by
    \\directly editing their project through the provided tools — do not just describe
    \\what to do, call the tools to do it.
    \\
    \\Model facts:
    \\- Time is measured in sample frames. One second = sample_rate frames.
    \\- One beat = sample_rate * 60 / tempo frames; a 16th note = that / 4.
    \\- MIDI pitch 60 is middle C. Velocity is 1-127.
    \\- Notes live inside clips, which live on tracks. Create a track, then a clip on
    \\  it, then add notes to the clip.
    \\
    \\Call get_project first if you need the current tempo or sample rate. Prefer many
    \\small precise tool calls over guessing. When the requested edits are done, reply
    \\with a short plain-text summary of what you changed.
;

pub const Agent = struct {
    alloc: std.mem.Allocator,
    provider: prov.Provider,
    ctx: *tools.Context,
    arena: std.heap.ArenaAllocator,
    messages: std.ArrayList(prov.Message),
    max_steps: usize = 12,
    /// When true, narrate tool calls + results to stderr (for the CLI driver).
    trace: bool = false,

    pub fn init(alloc: std.mem.Allocator, provider: prov.Provider, ctx: *tools.Context) !Agent {
        var self = Agent{
            .alloc = alloc,
            .provider = provider,
            .ctx = ctx,
            .arena = std.heap.ArenaAllocator.init(alloc),
            .messages = std.ArrayList(prov.Message).init(alloc),
        };
        try self.messages.append(.{ .role = .system, .content = SYSTEM_PROMPT });
        return self;
    }

    pub fn deinit(self: *Agent) void {
        self.messages.deinit();
        self.arena.deinit();
    }

    fn dupe(self: *Agent, s: []const u8) ![]const u8 {
        return self.arena.allocator().dupe(u8, s);
    }

    /// Process one user request. Returns the model's final prose reply, owned by
    /// `self.alloc` (caller frees).
    pub fn run(self: *Agent, user_msg: []const u8) ![]u8 {
        try self.messages.append(.{ .role = .user, .content = try self.dupe(user_msg) });

        var step: usize = 0;
        while (step < self.max_steps) : (step += 1) {
            var resp = try self.provider.chat(self.alloc, self.messages.items, tools.specs());
            defer resp.deinit();

            if (resp.tool_calls.len == 0) {
                try self.messages.append(.{ .role = .assistant, .content = try self.dupe(resp.content) });
                return self.alloc.dupe(u8, resp.content);
            }

            // Record the assistant's tool-call turn (deep-copied into the arena).
            const owned_calls = try self.arena.allocator().alloc(prov.ToolCall, resp.tool_calls.len);
            for (resp.tool_calls, 0..) |tc, i| {
                owned_calls[i] = .{
                    .id = try self.dupe(tc.id),
                    .name = try self.dupe(tc.name),
                    .arguments = try self.dupe(tc.arguments),
                };
            }
            try self.messages.append(.{
                .role = .assistant,
                .content = try self.dupe(resp.content),
                .tool_calls = owned_calls,
            });

            // Execute each call and feed its result back as a `tool` message.
            for (owned_calls) |tc| {
                if (self.trace) std.debug.print("  ▸ {s}({s})\n", .{ tc.name, tc.arguments });
                const result_json = self.dispatch(tc) catch |e| blk: {
                    break :blk try std.fmt.allocPrint(self.alloc, "{{\"error\":\"{s}\"}}", .{@errorName(e)});
                };
                defer self.alloc.free(result_json);
                if (self.trace) std.debug.print("    └ {s}\n", .{result_json});
                try self.messages.append(.{
                    .role = .tool,
                    .content = try self.dupe(result_json),
                    .tool_call_id = try self.dupe(tc.id),
                    .name = try self.dupe(tc.name),
                });
            }
        }
        return self.alloc.dupe(u8, "(stopped: reached the maximum number of tool steps)");
    }

    /// Run one tool call against the project; returns the JSON result string
    /// (owned by `self.alloc`). Errors propagate so `run` can report them back
    /// to the model rather than aborting the conversation.
    fn dispatch(self: *Agent, tc: prov.ToolCall) ![]const u8 {
        const r = try tools.call(self.ctx, tc.name, tc.arguments);
        return r.json;
    }
};

// ---------------------------------------------------------------------------
// Tests — the full spine, end-to-end, offline via MockProvider.
// ---------------------------------------------------------------------------

const project = @import("project.zig");

test "agent executes a scripted tool plan against the project" {
    const a = std.testing.allocator;
    var proj = project.Project.init(a);
    defer proj.deinit();
    var hist = project.History.init(a);
    defer hist.deinit();
    var ctx = tools.Context{ .project = &proj, .history = &hist, .alloc = a };

    // The model: set tempo, make two tracks, add a clip + a note, then summarize.
    var mock = prov.MockProvider{ .script = &.{
        .{ .tool_calls = &.{.{ .id = "c1", .name = "set_tempo", .arguments = "{\"bpm\":128}" }} },
        .{ .tool_calls = &.{
            .{ .id = "c2", .name = "create_track", .arguments = "{\"name\":\"Drums\",\"instrument\":\"sampler\"}" },
            .{ .id = "c3", .name = "create_track", .arguments = "{\"name\":\"Bass\"}" },
        } },
        .{ .tool_calls = &.{.{ .id = "c4", .name = "add_clip", .arguments = "{\"track\":1,\"name\":\"bassline\",\"start\":0}" }} },
        .{ .tool_calls = &.{.{ .id = "c5", .name = "add_note", .arguments = "{\"track\":1,\"clip\":0,\"pitch\":36,\"start\":0,\"length\":12000,\"velocity\":120}" }} },
        .{ .content = "Made a 128 BPM sketch: a sampler Drums track and a Bass track with one note." },
    } };

    var agent = try Agent.init(a, mock.provider(), &ctx);
    defer agent.deinit();

    const reply = try agent.run("make me a quick 128 bpm techno sketch");
    defer a.free(reply);

    try std.testing.expect(std.mem.indexOf(u8, reply, "128") != null);
    try std.testing.expectEqual(@as(f64, 128.0), proj.tempo);
    try std.testing.expectEqual(@as(usize, 2), proj.tracks.items.len);
    try std.testing.expectEqualStrings("Drums", proj.tracks.items[0].name.items);
    try std.testing.expectEqual(project.InstrumentKind.sampler, proj.tracks.items[0].instrument);
    const note = proj.tracks.items[1].clips.items[0].notes.items[0];
    try std.testing.expectEqual(@as(u8, 36), note.pitch);
}

test "agent reports a tool error back into the conversation without crashing" {
    const a = std.testing.allocator;
    var proj = project.Project.init(a);
    defer proj.deinit();
    var hist = project.History.init(a);
    defer hist.deinit();
    var ctx = tools.Context{ .project = &proj, .history = &hist, .alloc = a };

    // Calls an out-of-range track; the loop should feed the error back, then the
    // model (next script entry) recovers with prose.
    var mock = prov.MockProvider{ .script = &.{
        .{ .tool_calls = &.{.{ .id = "x", .name = "set_track_volume", .arguments = "{\"track\":9,\"volume\":1.0}" }} },
        .{ .content = "There are no tracks yet, so I couldn't set a volume." },
    } };

    var agent = try Agent.init(a, mock.provider(), &ctx);
    defer agent.deinit();
    const reply = try agent.run("turn up track 9");
    defer a.free(reply);

    // The error must have been recorded as a tool message for the model to see.
    var saw_error_tool_msg = false;
    for (agent.messages.items) |m| {
        if (m.role == .tool and std.mem.indexOf(u8, m.content, "TrackOutOfRange") != null) saw_error_tool_msg = true;
    }
    try std.testing.expect(saw_error_tool_msg);
    try std.testing.expect(std.mem.indexOf(u8, reply, "no tracks") != null);
}
