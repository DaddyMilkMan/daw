//! Zenith Zig build.
//!   zig build render   -> writes zenith_hello.wav
//!   zig build play     -> plays the demo phrase via ALSA (links libasound)

const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const render = b.addExecutable(.{
        .name = "zenith_render",
        .root_source_file = b.path("zig/main_render.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(render);

    const play = b.addExecutable(.{
        .name = "zenith_play",
        .root_source_file = b.path("zig/main_play.zig"),
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
        .root_source_file = b.path("zig/main_live.zig"),
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
        .root_source_file = b.path("zig/main_loop.zig"),
        .target = target,
        .optimize = optimize,
    });
    loop.linkSystemLibrary("asound");
    loop.linkLibC();
    b.installArtifact(loop);

    const run_loop = b.addRunArtifact(loop);
    const loop_step = b.step("loop", "Run the live looper (connect a keyboard via aconnect)");
    loop_step.dependOn(&run_loop.step);

    // Audio input + device/channel enumeration (`zig build audioin`).
    const audioin = b.addExecutable(.{
        .name = "zenith_audioin",
        .root_source_file = b.path("zig/main_audioin.zig"),
        .target = target,
        .optimize = optimize,
    });
    audioin.linkSystemLibrary("asound");
    audioin.linkLibC();
    b.installArtifact(audioin);
    const run_audioin = b.addRunArtifact(audioin);
    const audioin_step = b.step("audioin", "List all audio devices/channels + meter live input");
    audioin_step.dependOn(&run_audioin.step);

    // Sampler demo (M4): generate -> write -> read -> play pitched.
    const sampler = b.addExecutable(.{
        .name = "zenith_sampler",
        .root_source_file = b.path("zig/main_sampler.zig"),
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
        .root_source_file = b.path("zig/main_mixer.zig"),
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
        .root_source_file = b.path("zig/main_project.zig"),
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
        .root_source_file = b.path("zig/main_record.zig"),
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
        .root_source_file = b.path("zig/main_arrange.zig"),
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
        .root_source_file = b.path("zig/clap_test_plugin.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(clap_plugin);

    const clap_host = b.addExecutable(.{
        .name = "zenith_clap",
        .root_source_file = b.path("zig/main_clap.zig"),
        .target = target,
        .optimize = optimize,
    });
    clap_host.linkLibC();
    b.installArtifact(clap_host);
    const run_clap = b.addRunArtifact(clap_host);
    const clap_step = b.step("clap", "Host a CLAP plugin (defaults to the test plugin)");
    clap_step.dependOn(&run_clap.step);

    // CLAP scanner/inspector: reads descriptors + ports + params from a .clap.
    const clapscan = b.addExecutable(.{
        .name = "zenith_clapscan",
        .root_source_file = b.path("zig/main_clapscan.zig"),
        .target = target,
        .optimize = optimize,
    });
    clapscan.linkLibC();
    b.installArtifact(clapscan);
    const run_clapscan = b.addRunArtifact(clapscan);
    run_clapscan.step.dependOn(b.getInstallStep()); // ensure the test .clap is built+installed first
    if (b.args) |a| run_clapscan.addArgs(a);
    const clapscan_step = b.step("clapscan", "Scan + inspect a CLAP plugin (ports, params)");
    clapscan_step.dependOn(&run_clapscan.step);

    // VST3 hosting: a Zig VST3 test plugin (.so) + the COM-ABI host.
    const vst3_plugin = b.addSharedLibrary(.{
        .name = "zenith_vst3_test",
        .root_source_file = b.path("zig/vst3_test_plugin.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(vst3_plugin);

    const vst3 = b.addExecutable(.{
        .name = "zenith_vst3",
        .root_source_file = b.path("zig/main_vst3.zig"),
        .target = target,
        .optimize = optimize,
    });
    vst3.linkLibC();
    b.installArtifact(vst3);
    const run_vst3 = b.addRunArtifact(vst3);
    run_vst3.step.dependOn(b.getInstallStep()); // build the test .so first
    if (b.args) |a| run_vst3.addArgs(a);
    const vst3_step = b.step("vst3", "Host a VST3 plugin end-to-end (defaults to the test plugin)");
    vst3_step.dependOn(&run_vst3.step);

    // VST2 hosting: a Zig VST2 test plugin (.so) + the AEffect host.
    const vst2_plugin = b.addSharedLibrary(.{
        .name = "zenith_vst2_test",
        .root_source_file = b.path("zig/vst2_test_plugin.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(vst2_plugin);

    const vst2 = b.addExecutable(.{
        .name = "zenith_vst2",
        .root_source_file = b.path("zig/main_vst2.zig"),
        .target = target,
        .optimize = optimize,
    });
    vst2.linkLibC();
    b.installArtifact(vst2);
    const run_vst2 = b.addRunArtifact(vst2);
    run_vst2.step.dependOn(b.getInstallStep());
    if (b.args) |a| run_vst2.addArgs(a);
    const vst2_step = b.step("vst2", "Host a VST2 plugin end-to-end (defaults to the test plugin)");
    vst2_step.dependOn(&run_vst2.step);

    // Audio tracks + recording-to-timeline demo/verification.
    const audiotrack = b.addExecutable(.{
        .name = "zenith_audiotrack",
        .root_source_file = b.path("zig/main_audiotrack.zig"),
        .target = target,
        .optimize = optimize,
    });
    audiotrack.linkLibC();
    b.installArtifact(audiotrack);
    const run_audiotrack = b.addRunArtifact(audiotrack);
    const audiotrack_step = b.step("audiotrack", "Render an audio-clip timeline + recording round-trip");
    audiotrack_step.dependOn(&run_audiotrack.step);

    // GUI foundation: render a DAW frame to an image.
    const ui = b.addExecutable(.{
        .name = "zenith_ui",
        .root_source_file = b.path("zig/main_ui.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(ui);
    const run_ui = b.addRunArtifact(ui);
    const ui_step = b.step("ui", "Render the Zenith UI frame to a BMP");
    ui_step.dependOn(&run_ui.step);

    // Polished (antialiased) UI frame.
    const ui2 = b.addExecutable(.{
        .name = "zenith_ui_pretty",
        .root_source_file = b.path("zig/main_ui_pretty.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(ui2);
    const run_ui2 = b.addRunArtifact(ui2);
    const ui2_step = b.step("ui2", "Render the polished antialiased UI frame");
    ui2_step.dependOn(&run_ui2.step);

    // Widget + layout demo (knobs/toggles via the layout engine).
    const kit = b.addExecutable(.{
        .name = "zenith_kit",
        .root_source_file = b.path("zig/main_kit.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(kit);
    const run_kit = b.addRunArtifact(kit);
    const kit_step = b.step("kit", "Render the widget/layout demo");
    kit_step.dependOn(&run_kit.step);

    // Glassmorphism demo (software blur).
    const glass = b.addExecutable(.{
        .name = "zenith_glass",
        .root_source_file = b.path("zig/main_glass.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(glass);
    const run_glass = b.addRunArtifact(glass);
    const glass_step = b.step("glass", "Render the glassmorphism demo");
    glass_step.dependOn(&run_glass.step);

    // OpenGL (GLX) window — GPU present backend.
    const glwin = b.addExecutable(.{
        .name = "zenith_glwin",
        .root_source_file = b.path("zig/main_glwin.zig"),
        .target = target,
        .optimize = optimize,
    });
    glwin.linkSystemLibrary("GL");
    glwin.linkSystemLibrary("X11");
    glwin.linkLibC();
    b.installArtifact(glwin);
    const run_glwin = b.addRunArtifact(glwin);
    const glwin_step = b.step("glwin", "Open the OpenGL (GPU) window");
    glwin_step.dependOn(&run_glwin.step);

    // Live native window (X11).
    const window = b.addExecutable(.{
        .name = "zenith_window",
        .root_source_file = b.path("zig/main_window.zig"),
        .target = target,
        .optimize = optimize,
    });
    window.linkSystemLibrary("GL");
    window.linkSystemLibrary("X11");
    window.linkLibC();
    b.installArtifact(window);
    const run_window = b.addRunArtifact(window);
    const window_step = b.step("window", "Open the live Zenith window (GPU/GLX)");
    window_step.dependOn(&run_window.step);

    // GPU 2D renderer smoke test (SDF shapes + analytic shadows).
    const gpu = b.addExecutable(.{
        .name = "zenith_gpu",
        .root_source_file = b.path("zig/main_gpu.zig"),
        .target = target,
        .optimize = optimize,
    });
    gpu.linkSystemLibrary("GL");
    gpu.linkSystemLibrary("X11");
    gpu.linkLibC();
    b.installArtifact(gpu);
    const run_gpu = b.addRunArtifact(gpu);
    const gpu_step = b.step("gpu", "Run the GPU 2D renderer smoke test");
    gpu_step.dependOn(&run_gpu.step);

    // The consolidated live Zenith DAW (flex + glass + GPU toolkit).
    const dawexe = b.addExecutable(.{
        .name = "zenith",
        .root_source_file = b.path("zig/main_daw.zig"),
        .target = target,
        .optimize = optimize,
    });
    dawexe.linkSystemLibrary("GL");
    dawexe.linkSystemLibrary("X11");
    dawexe.linkSystemLibrary("asound"); // real-time audio output (audio_engine)
    dawexe.linkLibC();
    b.installArtifact(dawexe);
    const run_daw = b.addRunArtifact(dawexe);
    const daw_step = b.step("daw", "Run the live Zenith DAW (consolidated)");
    daw_step.dependOn(&run_daw.step);

    // UI toolkit showcase (gradients, glass, shadows, all controls, icons).
    const showcase = b.addExecutable(.{
        .name = "zenith_showcase",
        .root_source_file = b.path("zig/main_showcase.zig"),
        .target = target,
        .optimize = optimize,
    });
    showcase.linkSystemLibrary("GL");
    showcase.linkSystemLibrary("X11");
    showcase.linkLibC();
    b.installArtifact(showcase);
    const run_showcase = b.addRunArtifact(showcase);
    const showcase_step = b.step("showcase", "Run the UI toolkit showcase (all effects)");
    showcase_step.dependOn(&run_showcase.step);

    // Mixer laid out by the flex engine + GPU widgets.
    const flexmix = b.addExecutable(.{
        .name = "zenith_flexmix",
        .root_source_file = b.path("zig/main_flexmix.zig"),
        .target = target,
        .optimize = optimize,
    });
    flexmix.linkSystemLibrary("GL");
    flexmix.linkSystemLibrary("X11");
    flexmix.linkLibC();
    b.installArtifact(flexmix);
    const run_flexmix = b.addRunArtifact(flexmix);
    const flexmix_step = b.step("flexmix", "Run the flex-laid-out mixer");
    flexmix_step.dependOn(&run_flexmix.step);

    // Flexbox layout engine demo.
    const flex = b.addExecutable(.{
        .name = "zenith_flex",
        .root_source_file = b.path("zig/main_flex.zig"),
        .target = target,
        .optimize = optimize,
    });
    flex.linkSystemLibrary("GL");
    flex.linkSystemLibrary("X11");
    flex.linkLibC();
    b.installArtifact(flex);
    const run_flex = b.addRunArtifact(flex);
    const flex_step = b.step("flex", "Run the flexbox layout engine demo");
    flex_step.dependOn(&run_flex.step);

    // The live DAW, rendered entirely on the GPU toolkit.
    const gpudaw = b.addExecutable(.{
        .name = "zenith_gpu_daw",
        .root_source_file = b.path("zig/main_gpu_daw.zig"),
        .target = target,
        .optimize = optimize,
    });
    gpudaw.linkSystemLibrary("GL");
    gpudaw.linkSystemLibrary("X11");
    gpudaw.linkLibC();
    b.installArtifact(gpudaw);
    const run_gpudaw = b.addRunArtifact(gpudaw);
    const gpudaw_step = b.step("gpudaw", "Run the live Zenith DAW on the GPU toolkit");
    gpudaw_step.dependOn(&run_gpudaw.step);

    // Effects demo (EQ / delay / reverb).
    const fx = b.addExecutable(.{
        .name = "zenith_fx",
        .root_source_file = b.path("zig/main_fx.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(fx);
    const run_fx = b.addRunArtifact(fx);
    const fx_step = b.step("fx", "Run the effects demo");
    fx_step.dependOn(&run_fx.step);

    // Headless interactive-UI test.
    const uitest = b.addExecutable(.{
        .name = "zenith_uitest",
        .root_source_file = b.path("zig/main_uitest.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(uitest);
    const run_uitest = b.addRunArtifact(uitest);
    const uitest_step = b.step("uitest", "Run the headless interactive-UI test");
    uitest_step.dependOn(&run_uitest.step);

    // Animation/micro-interaction capture.
    const anim = b.addExecutable(.{
        .name = "zenith_anim",
        .root_source_file = b.path("zig/main_anim.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(anim);
    const run_anim = b.addRunArtifact(anim);
    const anim_step = b.step("anim", "Capture hover-animation frames");
    anim_step.dependOn(&run_anim.step);

    // Unit tests.
    const test_step = b.step("test", "Run unit tests");
    for ([_][]const u8{ "midi_alsa.zig", "midi2.zig", "audio_devices.zig", "ttf.zig", "image.zig", "svg.zig", "sequence.zig", "wav.zig", "resample.zig", "mixer.zig", "project.zig", "arrangement.zig", "effects.zig", "dsp.zig", "aiff.zig", "vst3_abi.zig", "vst2_abi.zig", "audio_track.zig" }) |src| {
        const t = b.addTest(.{ .root_source_file = b.path(b.fmt("zig/{s}", .{src})), .target = target, .optimize = optimize });
        t.linkSystemLibrary("asound");
        t.linkLibC();
        test_step.dependOn(&b.addRunArtifact(t).step);
    }
}
