//! Zenith Zig build.
//!   zig build render   -> writes zenith_hello.wav
//!   zig build play     -> plays the demo phrase via ALSA (links libasound)

const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const render = b.addExecutable(.{
        .name = "zenith_render",
        .root_source_file = b.path("main_render.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(render);

    const play = b.addExecutable(.{
        .name = "zenith_play",
        .root_source_file = b.path("main_play.zig"),
        .target = target,
        .optimize = optimize,
    });
    play.linkSystemLibrary("asound");
    play.linkLibC();
    b.installArtifact(play);

    const run_render = b.addRunArtifact(render);
    const render_step = b.step("render", "Render the demo phrase to zenith_hello.wav");
    render_step.dependOn(&run_render.step);

    const run_play = b.addRunArtifact(play);
    const play_step = b.step("play", "Play the demo phrase via ALSA");
    play_step.dependOn(&run_play.step);

    // Real-time, MIDI-driven engine.
    const live = b.addExecutable(.{
        .name = "zenith_live",
        .root_source_file = b.path("main_live.zig"),
        .target = target,
        .optimize = optimize,
    });
    live.linkSystemLibrary("asound");
    live.linkLibC();
    b.installArtifact(live);

    const run_live = b.addRunArtifact(live);
    const live_step = b.step("live", "Run the real-time MIDI engine (connect a keyboard via aconnect)");
    live_step.dependOn(&run_live.step);

    // Looping recorder (M3).
    const loop = b.addExecutable(.{
        .name = "zenith_loop",
        .root_source_file = b.path("main_loop.zig"),
        .target = target,
        .optimize = optimize,
    });
    loop.linkSystemLibrary("asound");
    loop.linkLibC();
    b.installArtifact(loop);

    const run_loop = b.addRunArtifact(loop);
    const loop_step = b.step("loop", "Run the live looper (connect a keyboard via aconnect)");
    loop_step.dependOn(&run_loop.step);

    // Sampler demo (M4): generate -> write -> read -> play pitched.
    const sampler = b.addExecutable(.{
        .name = "zenith_sampler",
        .root_source_file = b.path("main_sampler.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(sampler);
    const run_sampler = b.addRunArtifact(sampler);
    const sampler_step = b.step("sampler", "Run the sampler demo");
    sampler_step.dependOn(&run_sampler.step);

    // Mixer demo (M6): synth + sampler -> stereo master.
    const mixer = b.addExecutable(.{
        .name = "zenith_mixer",
        .root_source_file = b.path("main_mixer.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(mixer);
    const run_mixer = b.addRunArtifact(mixer);
    const mixer_step = b.step("mixer", "Run the mixer demo");
    mixer_step.dependOn(&run_mixer.step);

    // Project model demo (M9): build -> save -> reload -> play.
    const project = b.addExecutable(.{
        .name = "zenith_project",
        .root_source_file = b.path("main_project.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(project);
    const run_project = b.addRunArtifact(project);
    const project_step = b.step("project", "Run the project save/load demo");
    project_step.dependOn(&run_project.step);

    // Audio recorder (M5).
    const record = b.addExecutable(.{
        .name = "zenith_record",
        .root_source_file = b.path("main_record.zig"),
        .target = target,
        .optimize = optimize,
    });
    record.linkSystemLibrary("asound");
    record.linkLibC();
    b.installArtifact(record);
    const run_record = b.addRunArtifact(record);
    const record_step = b.step("record", "Record audio input to a WAV");
    record_step.dependOn(&run_record.step);

    // Arrangement demo (timeline of clips).
    const arrange_exe = b.addExecutable(.{
        .name = "zenith_arrange",
        .root_source_file = b.path("main_arrange.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(arrange_exe);
    const run_arrange = b.addRunArtifact(arrange_exe);
    const arrange_step = b.step("arrange", "Run the arrangement/timeline demo");
    arrange_step.dependOn(&run_arrange.step);

    // M7 — CLAP plugin hosting: a test .clap plugin + the host.
    const clap_plugin = b.addSharedLibrary(.{
        .name = "zenith_test_clap",
        .root_source_file = b.path("clap_test_plugin.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(clap_plugin);

    const clap_host = b.addExecutable(.{
        .name = "zenith_clap",
        .root_source_file = b.path("main_clap.zig"),
        .target = target,
        .optimize = optimize,
    });
    clap_host.linkLibC();
    b.installArtifact(clap_host);
    const run_clap = b.addRunArtifact(clap_host);
    const clap_step = b.step("clap", "Host a CLAP plugin (defaults to the test plugin)");
    clap_step.dependOn(&run_clap.step);

    // GUI foundation: render a DAW frame to an image.
    const ui = b.addExecutable(.{
        .name = "zenith_ui",
        .root_source_file = b.path("main_ui.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(ui);
    const run_ui = b.addRunArtifact(ui);
    const ui_step = b.step("ui", "Render the Zenith UI frame to a BMP");
    ui_step.dependOn(&run_ui.step);

    // Live native window (X11).
    const window = b.addExecutable(.{
        .name = "zenith_window",
        .root_source_file = b.path("main_window.zig"),
        .target = target,
        .optimize = optimize,
    });
    window.linkSystemLibrary("X11");
    window.linkLibC();
    b.installArtifact(window);
    const run_window = b.addRunArtifact(window);
    const window_step = b.step("window", "Open the live Zenith window (X11)");
    window_step.dependOn(&run_window.step);

    // Effects demo (EQ / delay / reverb).
    const fx = b.addExecutable(.{
        .name = "zenith_fx",
        .root_source_file = b.path("main_fx.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(fx);
    const run_fx = b.addRunArtifact(fx);
    const fx_step = b.step("fx", "Run the effects demo");
    fx_step.dependOn(&run_fx.step);

    // Unit tests.
    const test_step = b.step("test", "Run unit tests");
    for ([_][]const u8{ "midi_alsa.zig", "sequence.zig", "wav.zig", "resample.zig", "mixer.zig", "project.zig", "arrangement.zig", "effects.zig" }) |src| {
        const t = b.addTest(.{ .root_source_file = b.path(src), .target = target, .optimize = optimize });
        t.linkSystemLibrary("asound");
        t.linkLibC();
        test_step.dependOn(&b.addRunArtifact(t).step);
    }
}
