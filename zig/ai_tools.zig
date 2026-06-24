//! ai_tools.zig — the action surface the AI can take on a Zenith project.
//!
//! This is the crown jewel of the wedge and the one chokepoint that mutates the
//! project model: the model emits tool calls, they land here, and each handler
//! reads or (undo-checkpointed) mutates `project.zig` types. The UI's own edits
//! and the AI share this discipline, so undo/redo and ZNPR persistence come for
//! free.
//!
//! Each tool is a tiny namespace declaring `name`, `description`, a `Params`
//! struct, and `run`. The JSON-Schema advertised to the model is *generated at
//! comptime from `Params`* (`schemaFor`), so the schema and the argument decoder
//! can never drift — the struct is the single source of truth.

const std = @import("std");
const project = @import("project.zig");
const prov = @import("ai_provider.zig");
const aimidi = @import("ai_midi.zig");

/// What a tool handler operates on. The single write-chokepoint.
pub const Context = struct {
    project: *project.Project,
    history: *project.History,
    alloc: std.mem.Allocator,
};

/// A tool's outcome: a small JSON document returned to the model as the `tool`
/// message content. `json` is owned by `ctx.alloc`; the caller frees it.
pub const ToolResult = struct {
    ok: bool,
    json: []const u8,
};

fn okJson(alloc: std.mem.Allocator, comptime fmt: []const u8, args: anytype) !ToolResult {
    return .{ .ok = true, .json = try std.fmt.allocPrint(alloc, fmt, args) };
}

fn trackPtr(ctx: *Context, idx: usize) !*project.Track {
    if (idx >= ctx.project.tracks.items.len) return error.TrackOutOfRange;
    return &ctx.project.tracks.items[idx];
}

fn clipPtr(ctx: *Context, track: usize, clip: usize) !*project.Clip {
    const t = try trackPtr(ctx, track);
    if (clip >= t.clips.items.len) return error.ClipOutOfRange;
    return &t.clips.items[clip];
}

fn setStr(list: *std.ArrayList(u8), s: []const u8) !void {
    list.clearRetainingCapacity();
    try list.appendSlice(s);
}

// ---------------------------------------------------------------------------
// The tools.
// ---------------------------------------------------------------------------

const GetProject = struct {
    pub const name = "get_project";
    pub const description = "Get the project's tempo, sample rate, and track count.";
    pub const Params = struct {};
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        _ = p;
        return okJson(ctx.alloc, "{{\"tempo\":{d},\"sample_rate\":{d},\"track_count\":{d}}}", .{
            ctx.project.tempo, ctx.project.sample_rate, ctx.project.tracks.items.len,
        });
    }
};

const ListTracks = struct {
    pub const name = "list_tracks";
    pub const description = "List every track with its index, name, instrument, volume, pan, mute, and clip count.";
    pub const Params = struct {};
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        _ = p;
        var out = std.ArrayList(u8).init(ctx.alloc);
        errdefer out.deinit();
        var ws = std.json.writeStream(out.writer(), .{});
        defer ws.deinit();
        try ws.beginArray();
        for (ctx.project.tracks.items, 0..) |t, i| {
            try ws.beginObject();
            try ws.objectField("index");
            try ws.write(i);
            try ws.objectField("name");
            try ws.write(t.name.items);
            try ws.objectField("instrument");
            try ws.write(@tagName(t.instrument));
            try ws.objectField("volume");
            try ws.write(t.gain);
            try ws.objectField("pan");
            try ws.write(t.pan);
            try ws.objectField("mute");
            try ws.write(t.mute);
            try ws.objectField("clip_count");
            try ws.write(t.clips.items.len);
            try ws.endObject();
        }
        try ws.endArray();
        return .{ .ok = true, .json = try out.toOwnedSlice() };
    }
};

const CreateTrack = struct {
    pub const name = "create_track";
    pub const description = "Create a new track. instrument is 'synth' or 'sampler'. Returns the new track index.";
    pub const Params = struct {
        name: []const u8 = "Track",
        instrument: project.InstrumentKind = .synth,
    };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        try ctx.history.checkpoint(ctx.project);
        _ = try ctx.project.addTrack(p.name, p.instrument);
        return okJson(ctx.alloc, "{{\"created\":true,\"track\":{d}}}", .{ctx.project.tracks.items.len - 1});
    }
};

const SetTrackVolume = struct {
    pub const name = "set_track_volume";
    pub const description = "Set a track's volume (linear gain, 0.0 = silent, 1.0 = unity).";
    pub const Params = struct { track: u32, volume: f32 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        try ctx.history.checkpoint(ctx.project);
        t.gain = std.math.clamp(p.volume, 0.0, 4.0);
        return okJson(ctx.alloc, "{{\"track\":{d},\"volume\":{d}}}", .{ p.track, t.gain });
    }
};

const SetTrackPan = struct {
    pub const name = "set_track_pan";
    pub const description = "Set a track's pan (-1.0 = hard left, 0.0 = center, 1.0 = hard right).";
    pub const Params = struct { track: u32, pan: f32 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        try ctx.history.checkpoint(ctx.project);
        t.pan = std.math.clamp(p.pan, -1.0, 1.0);
        return okJson(ctx.alloc, "{{\"track\":{d},\"pan\":{d}}}", .{ p.track, t.pan });
    }
};

const SetTrackMute = struct {
    pub const name = "set_track_mute";
    pub const description = "Mute or unmute a track.";
    pub const Params = struct { track: u32, mute: bool };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        try ctx.history.checkpoint(ctx.project);
        t.mute = p.mute;
        return okJson(ctx.alloc, "{{\"track\":{d},\"mute\":{}}}", .{ p.track, t.mute });
    }
};

const AddClip = struct {
    pub const name = "add_clip";
    pub const description = "Add a MIDI clip to a track at a timeline position (in frames). Returns the clip index within that track.";
    pub const Params = struct {
        track: u32,
        name: []const u8 = "Clip",
        start: u64 = 0,
    };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        try ctx.history.checkpoint(ctx.project);
        _ = try t.addClip(p.name, p.start);
        return okJson(ctx.alloc, "{{\"track\":{d},\"clip\":{d}}}", .{ p.track, t.clips.items.len - 1 });
    }
};

const AddNote = struct {
    pub const name = "add_note";
    pub const description = "Add a MIDI note to a clip. pitch is a MIDI note (0-127, 60=middle C); start/length are clip-relative frames; velocity 1-127.";
    pub const Params = struct {
        track: u32,
        clip: u32,
        pitch: u8,
        start: u64,
        length: u64,
        velocity: u8 = 100,
    };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        if (p.clip >= t.clips.items.len) return error.ClipOutOfRange;
        try ctx.history.checkpoint(ctx.project);
        var c = &t.clips.items[p.clip];
        try c.notes.append(.{ .start = p.start, .len = p.length, .pitch = p.pitch, .velocity = p.velocity });
        return okJson(ctx.alloc, "{{\"track\":{d},\"clip\":{d},\"notes\":{d}}}", .{ p.track, p.clip, c.notes.items.len });
    }
};

const SetTempo = struct {
    pub const name = "set_tempo";
    pub const description = "Set the project tempo in beats per minute.";
    pub const Params = struct { bpm: f64 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        try ctx.history.checkpoint(ctx.project);
        ctx.project.tempo = std.math.clamp(p.bpm, 20.0, 999.0);
        return okJson(ctx.alloc, "{{\"tempo\":{d}}}", .{ctx.project.tempo});
    }
};

const DeleteTrack = struct {
    pub const name = "delete_track";
    pub const description = "Delete a track by index (later tracks shift down).";
    pub const Params = struct { track: u32 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        if (p.track >= ctx.project.tracks.items.len) return error.TrackOutOfRange;
        try ctx.history.checkpoint(ctx.project);
        var t = ctx.project.tracks.orderedRemove(p.track);
        t.deinit();
        return okJson(ctx.alloc, "{{\"deleted\":true,\"track_count\":{d}}}", .{ctx.project.tracks.items.len});
    }
};

const RenameTrack = struct {
    pub const name = "rename_track";
    pub const description = "Rename a track.";
    pub const Params = struct { track: u32, name: []const u8 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        try ctx.history.checkpoint(ctx.project);
        try setStr(&t.name, p.name);
        return okJson(ctx.alloc, "{{\"track\":{d},\"name\":\"{s}\"}}", .{ p.track, t.name.items });
    }
};

const SetInstrument = struct {
    pub const name = "set_instrument";
    pub const description = "Set a track's instrument ('synth' or 'sampler').";
    pub const Params = struct { track: u32, instrument: project.InstrumentKind };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        try ctx.history.checkpoint(ctx.project);
        t.instrument = p.instrument;
        return okJson(ctx.alloc, "{{\"track\":{d},\"instrument\":\"{s}\"}}", .{ p.track, @tagName(t.instrument) });
    }
};

const DeleteClip = struct {
    pub const name = "delete_clip";
    pub const description = "Delete a MIDI clip from a track.";
    pub const Params = struct { track: u32, clip: u32 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const t = try trackPtr(ctx, p.track);
        if (p.clip >= t.clips.items.len) return error.ClipOutOfRange;
        try ctx.history.checkpoint(ctx.project);
        var c = t.clips.orderedRemove(p.clip);
        c.deinit();
        return okJson(ctx.alloc, "{{\"deleted\":true,\"clip_count\":{d}}}", .{t.clips.items.len});
    }
};

const MoveClip = struct {
    pub const name = "move_clip";
    pub const description = "Move a clip to a new timeline position (frames).";
    pub const Params = struct { track: u32, clip: u32, start: u64 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const c = try clipPtr(ctx, p.track, p.clip);
        try ctx.history.checkpoint(ctx.project);
        c.start = p.start;
        return okJson(ctx.alloc, "{{\"track\":{d},\"clip\":{d},\"start\":{d}}}", .{ p.track, p.clip, c.start });
    }
};

const ClearClipNotes = struct {
    pub const name = "clear_clip_notes";
    pub const description = "Remove all notes from a clip (keeps the clip).";
    pub const Params = struct { track: u32, clip: u32 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const c = try clipPtr(ctx, p.track, p.clip);
        try ctx.history.checkpoint(ctx.project);
        c.notes.clearRetainingCapacity();
        return okJson(ctx.alloc, "{{\"track\":{d},\"clip\":{d},\"notes\":0}}", .{ p.track, p.clip });
    }
};

const GetClip = struct {
    pub const name = "get_clip";
    pub const description = "Read a clip: its name, position, length, and every note (pitch/start/length/velocity).";
    pub const Params = struct { track: u32, clip: u32 };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const c = try clipPtr(ctx, p.track, p.clip);
        var out = std.ArrayList(u8).init(ctx.alloc);
        errdefer out.deinit();
        var ws = std.json.writeStream(out.writer(), .{});
        defer ws.deinit();
        try ws.beginObject();
        try ws.objectField("name");
        try ws.write(c.name.items);
        try ws.objectField("start");
        try ws.write(c.start);
        try ws.objectField("note_count");
        try ws.write(c.notes.items.len);
        try ws.objectField("notes");
        try ws.beginArray();
        for (c.notes.items) |n| {
            try ws.beginObject();
            try ws.objectField("pitch");
            try ws.write(n.pitch);
            try ws.objectField("start");
            try ws.write(n.start);
            try ws.objectField("length");
            try ws.write(n.len);
            try ws.objectField("velocity");
            try ws.write(n.velocity);
            try ws.endObject();
        }
        try ws.endArray();
        try ws.endObject();
        return .{ .ok = true, .json = try out.toOwnedSlice() };
    }
};

const GeneratePattern = struct {
    pub const name = "generate_pattern";
    pub const description =
        "Algorithmically generate musical MIDI into a clip — no need to place notes one by one. " ++
        "kind: drums|bass|chords|melody|arp. style: trap|house|jazz|lofi|rock|edm|rnb|pop. " ++
        "key like 'C' or 'F#'; scale like 'minor','dorian','pentatonic_minor'. Returns the note count.";
    pub const Params = struct {
        track: u32,
        clip: u32,
        kind: aimidi.Kind = .drums,
        style: aimidi.Style = .trap,
        key: []const u8 = "C",
        scale: []const u8 = "minor",
        bars: u32 = 4,
        complexity: f32 = 0.5,
        swing: f32 = 0.0,
        humanize: f32 = 0.1,
        variation: f32 = 0.5,
        seed: u64 = 0,
    };
    pub fn run(ctx: *Context, p: Params) !ToolResult {
        const c = try clipPtr(ctx, p.track, p.clip);
        const gen = try aimidi.generate(ctx.alloc, .{
            .kind = p.kind,
            .style = p.style,
            .key = p.key,
            .scale = p.scale,
            .bars = p.bars,
            .complexity = p.complexity,
            .swing = p.swing,
            .humanize = p.humanize,
            .variation = p.variation,
            .seed = p.seed,
        });
        defer ctx.alloc.free(gen);
        try ctx.history.checkpoint(ctx.project);

        // beats → clip-relative frames at the project tempo/sample rate.
        const fpb = @as(f64, @floatFromInt(ctx.project.sample_rate)) * 60.0 / ctx.project.tempo;
        var max_end: u64 = 0;
        for (gen) |gn| {
            const start_f: u64 = @intFromFloat(@max(0.0, gn.start_beats * fpb));
            const len_f: u64 = @intFromFloat(@max(1.0, gn.length_beats * fpb));
            const pitch: u8 = @intCast(std.math.clamp(gn.pitch, 0, 127));
            const vel: u8 = @intCast(std.math.clamp(gn.velocity, 1, 127));
            try c.notes.append(.{ .start = start_f, .len = len_f, .pitch = pitch, .velocity = vel });
            if (start_f + len_f > max_end) max_end = start_f + len_f;
        }
        if (max_end > c.length) c.length = max_end;
        return okJson(ctx.alloc, "{{\"track\":{d},\"clip\":{d},\"kind\":\"{s}\",\"generated\":{d},\"clip_notes\":{d}}}", .{ p.track, p.clip, @tagName(p.kind), gen.len, c.notes.items.len });
    }
};

/// The registry. Add a tool here and it's advertised + dispatchable everywhere.
const registry = .{
    GetProject,
    ListTracks,
    CreateTrack,
    DeleteTrack,
    RenameTrack,
    SetInstrument,
    SetTrackVolume,
    SetTrackPan,
    SetTrackMute,
    AddClip,
    DeleteClip,
    MoveClip,
    AddNote,
    ClearClipNotes,
    GetClip,
    GeneratePattern,
    SetTempo,
};

// ---------------------------------------------------------------------------
// Comptime JSON-Schema generation from a `Params` struct.
// ---------------------------------------------------------------------------

fn jsonTypeName(comptime T: type) []const u8 {
    return switch (@typeInfo(T)) {
        .int, .comptime_int => "integer",
        .float, .comptime_float => "number",
        .bool => "boolean",
        .@"enum" => "string",
        .pointer => "string", // []const u8
        else => @compileError("ai_tools: unsupported param type " ++ @typeName(T)),
    };
}

/// Generate a JSON-Schema string describing `T`'s fields. Fields without a
/// default value become `required`. Enums advertise their tag names.
pub fn schemaFor(comptime T: type) []const u8 {
    return comptime blk: {
        const fields = @typeInfo(T).@"struct".fields;
        var props: []const u8 = "";
        var required: []const u8 = "";
        for (fields, 0..) |f, i| {
            var prop: []const u8 = "\"" ++ f.name ++ "\":{\"type\":\"" ++ jsonTypeName(f.type) ++ "\"";
            if (@typeInfo(f.type) == .@"enum") {
                prop = prop ++ ",\"enum\":[";
                const evals = @typeInfo(f.type).@"enum".fields;
                for (evals, 0..) |ev, j| {
                    if (j != 0) prop = prop ++ ",";
                    prop = prop ++ "\"" ++ ev.name ++ "\"";
                }
                prop = prop ++ "]";
            }
            prop = prop ++ "}";
            if (i != 0) props = props ++ ",";
            props = props ++ prop;
            if (f.default_value_ptr == null) {
                if (required.len != 0) required = required ++ ",";
                required = required ++ "\"" ++ f.name ++ "\"";
            }
        }
        break :blk "{\"type\":\"object\",\"properties\":{" ++ props ++ "},\"required\":[" ++ required ++ "]}";
    };
}

/// The tool specs advertised to the model — comptime-built, no allocation.
pub const all_specs = blk: {
    var arr: [registry.len]prov.ToolSpec = undefined;
    for (registry, 0..) |T, i| {
        arr[i] = .{ .name = T.name, .description = T.description, .parameters_schema = schemaFor(T.Params) };
    }
    const final = arr;
    break :blk final;
};

pub fn specs() []const prov.ToolSpec {
    return &all_specs;
}

// ---------------------------------------------------------------------------
// Dispatch.
// ---------------------------------------------------------------------------

/// Look up `tool_name`, decode `args_json` into its Params, and run it.
/// `args_json` may be empty for no-arg tools.
pub fn call(ctx: *Context, tool_name: []const u8, args_json: []const u8) !ToolResult {
    const args_text = if (std.mem.trim(u8, args_json, " \t\r\n").len == 0) "{}" else args_json;

    var args = std.json.parseFromSlice(std.json.Value, ctx.alloc, args_text, .{}) catch {
        return error.BadToolArguments;
    };
    defer args.deinit();

    inline for (registry) |T| {
        if (std.mem.eql(u8, tool_name, T.name)) {
            var p = std.json.parseFromValue(T.Params, ctx.alloc, args.value, .{ .ignore_unknown_fields = true }) catch {
                return error.BadToolArguments;
            };
            defer p.deinit();
            return T.run(ctx, p.value);
        }
    }
    return error.UnknownTool;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

const TestEnv = struct {
    proj: project.Project,
    hist: project.History,
    ctx: Context,

    fn init(a: std.mem.Allocator) *TestEnv {
        const e = std.testing.allocator.create(TestEnv) catch unreachable;
        e.proj = project.Project.init(a);
        e.hist = project.History.init(a);
        e.ctx = .{ .project = &e.proj, .history = &e.hist, .alloc = a };
        return e;
    }
    fn deinit(e: *TestEnv) void {
        e.proj.deinit();
        e.hist.deinit();
        std.testing.allocator.destroy(e);
    }
};

test "schemaFor generates schema with required fields and enums" {
    const s = schemaFor(CreateTrack.Params);
    try std.testing.expect(std.mem.indexOf(u8, s, "\"instrument\":{\"type\":\"string\",\"enum\":[\"synth\",\"sampler\"]}") != null);
    // name has a default → not required; CreateTrack has no required fields
    try std.testing.expect(std.mem.indexOf(u8, s, "\"required\":[]") != null);

    const s2 = schemaFor(AddNote.Params);
    try std.testing.expect(std.mem.indexOf(u8, s2, "\"pitch\":{\"type\":\"integer\"}") != null);
    try std.testing.expect(std.mem.indexOf(u8, s2, "\"track\"") != null);
    // velocity has a default; pitch does not → required list excludes velocity
    try std.testing.expect(std.mem.indexOf(u8, s2, "\"velocity\"") != null);
}

test "all tools are advertised" {
    try std.testing.expectEqual(@as(usize, 17), specs().len);
}

test "generate_pattern lays real notes into a clip" {
    const a = std.testing.allocator;
    var e = TestEnv.init(a);
    defer e.deinit();
    const r0 = try call(&e.ctx, "create_track", "{\"name\":\"Drums\",\"instrument\":\"sampler\"}");
    a.free(r0.json);
    const r1 = try call(&e.ctx, "add_clip", "{\"track\":0,\"start\":0}");
    a.free(r1.json);
    const r2 = try call(&e.ctx, "generate_pattern", "{\"track\":0,\"clip\":0,\"kind\":\"drums\",\"style\":\"house\",\"bars\":2,\"seed\":99}");
    defer a.free(r2.json);

    const clip = &e.proj.tracks.items[0].clips.items[0];
    try std.testing.expect(clip.notes.items.len > 0);
    // clip length must have grown to cover the generated notes
    try std.testing.expect(clip.length > 0);
    for (clip.notes.items) |n| try std.testing.expect(n.pitch <= 127 and n.velocity >= 1);
}

test "delete_track and clear_clip_notes" {
    const a = std.testing.allocator;
    var e = TestEnv.init(a);
    defer e.deinit();
    const r0 = try call(&e.ctx, "create_track", "{}");
    a.free(r0.json);
    const r1 = try call(&e.ctx, "add_clip", "{\"track\":0,\"start\":0}");
    a.free(r1.json);
    const r2 = try call(&e.ctx, "add_note", "{\"track\":0,\"clip\":0,\"pitch\":60,\"start\":0,\"length\":1000}");
    a.free(r2.json);
    const r3 = try call(&e.ctx, "clear_clip_notes", "{\"track\":0,\"clip\":0}");
    a.free(r3.json);
    try std.testing.expectEqual(@as(usize, 0), e.proj.tracks.items[0].clips.items[0].notes.items.len);

    const r4 = try call(&e.ctx, "delete_track", "{\"track\":0}");
    a.free(r4.json);
    try std.testing.expectEqual(@as(usize, 0), e.proj.tracks.items.len);
}

test "create_track mutates the project and is undoable" {
    const a = std.testing.allocator;
    var e = TestEnv.init(a);
    defer e.deinit();

    const r = try call(&e.ctx, "create_track", "{\"name\":\"Drums\",\"instrument\":\"sampler\"}");
    defer a.free(r.json);
    try std.testing.expect(r.ok);
    try std.testing.expectEqual(@as(usize, 1), e.proj.tracks.items.len);
    try std.testing.expectEqualStrings("Drums", e.proj.tracks.items[0].name.items);
    try std.testing.expectEqual(project.InstrumentKind.sampler, e.proj.tracks.items[0].instrument);

    // undo restores the empty project
    _ = try e.hist.undo(&e.proj);
    try std.testing.expectEqual(@as(usize, 0), e.proj.tracks.items.len);
}

test "add_clip then add_note writes a note into the clip" {
    const a = std.testing.allocator;
    var e = TestEnv.init(a);
    defer e.deinit();

    const r0 = try call(&e.ctx, "create_track", "{}");
    a.free(r0.json);
    const r1 = try call(&e.ctx, "add_clip", "{\"track\":0,\"name\":\"verse\",\"start\":0}");
    a.free(r1.json);
    const r2 = try call(&e.ctx, "add_note", "{\"track\":0,\"clip\":0,\"pitch\":60,\"start\":0,\"length\":24000,\"velocity\":110}");
    defer a.free(r2.json);

    const note = e.proj.tracks.items[0].clips.items[0].notes.items[0];
    try std.testing.expectEqual(@as(u8, 60), note.pitch);
    try std.testing.expectEqual(@as(u8, 110), note.velocity);
    try std.testing.expectEqual(@as(u64, 24000), note.len);
}

test "set_tempo and set_track_volume clamp and apply" {
    const a = std.testing.allocator;
    var e = TestEnv.init(a);
    defer e.deinit();

    const r0 = try call(&e.ctx, "create_track", "{}");
    a.free(r0.json);
    const r1 = try call(&e.ctx, "set_tempo", "{\"bpm\":128}");
    a.free(r1.json);
    try std.testing.expectEqual(@as(f64, 128.0), e.proj.tempo);

    const r2 = try call(&e.ctx, "set_track_volume", "{\"track\":0,\"volume\":0.5}");
    a.free(r2.json);
    try std.testing.expectEqual(@as(f32, 0.5), e.proj.tracks.items[0].gain);
}

test "list_tracks returns valid JSON array" {
    const a = std.testing.allocator;
    var e = TestEnv.init(a);
    defer e.deinit();
    const r0 = try call(&e.ctx, "create_track", "{\"name\":\"Bass\"}");
    a.free(r0.json);

    const r = try call(&e.ctx, "list_tracks", "");
    defer a.free(r.json);
    try std.testing.expect(std.mem.indexOf(u8, r.json, "Bass") != null);
    var parsed = try std.json.parseFromSlice(std.json.Value, a, r.json, .{});
    defer parsed.deinit();
    try std.testing.expect(parsed.value == .array);
}

test "out-of-range track and unknown tool error cleanly" {
    const a = std.testing.allocator;
    var e = TestEnv.init(a);
    defer e.deinit();
    try std.testing.expectError(error.TrackOutOfRange, call(&e.ctx, "set_track_pan", "{\"track\":5,\"pan\":0.5}"));
    try std.testing.expectError(error.UnknownTool, call(&e.ctx, "frobnicate", "{}"));
}
