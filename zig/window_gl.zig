//! window_gl.zig — OpenGL (GLX) window that does the glassmorphism blur IN A
//! FRAGMENT SHADER on the GPU. The UI content is uploaded as a texture; the
//! shader samples a blurred neighbourhood for the glass region in real time.
//! Hand-declared GLX + GL + shader entry points (loaded via glXGetProcAddress).

const std = @import("std");

const Display = opaque {};
const XID = c_ulong;
const Window = XID;
const Colormap = XID;
const Atom = c_ulong;
const GLXContext = ?*opaque {};

const InputOutput: c_uint = 1;
const AllocNone: c_int = 0;
const CWBorderPixel: c_ulong = 1 << 3;
const CWColormap: c_ulong = 1 << 13;
const CWEventMask: c_ulong = 1 << 11;
const ExposureMask: c_long = 1 << 15;
const KeyPressMask: c_long = 1 << 0;
const StructureNotifyMask: c_long = 1 << 17;
const KeyPress: c_int = 2;
const ClientMessage: c_int = 33;

const GLX_RGBA: c_int = 4;
const GLX_DOUBLEBUFFER: c_int = 5;
const GLX_RED_SIZE: c_int = 8;
const GLX_GREEN_SIZE: c_int = 9;
const GLX_BLUE_SIZE: c_int = 10;
const GLX_DEPTH_SIZE: c_int = 12;

const GL_COLOR_BUFFER_BIT: c_uint = 0x4000;
const GL_TEXTURE_2D: c_uint = 0x0DE1;
const GL_RGBA: c_uint = 0x1908;
const GL_UNSIGNED_BYTE: c_uint = 0x1401;
const GL_TEXTURE_MIN_FILTER: c_uint = 0x2801;
const GL_TEXTURE_MAG_FILTER: c_uint = 0x2800;
const GL_LINEAR: c_int = 0x2601;
const GL_QUADS: c_uint = 0x0007;
const GL_TEXTURE0: c_uint = 0x84C0;
const GL_VERTEX_SHADER: c_uint = 0x8B31;
const GL_FRAGMENT_SHADER: c_uint = 0x8B30;
const GL_COMPILE_STATUS: c_uint = 0x8B81;
const GL_LINK_STATUS: c_uint = 0x8B82;

const XSetWindowAttributes = extern struct {
    background_pixmap: c_ulong = 0,
    background_pixel: c_ulong = 0,
    border_pixmap: c_ulong = 0,
    border_pixel: c_ulong = 0,
    bit_gravity: c_int = 0,
    win_gravity: c_int = 0,
    backing_store: c_int = 0,
    backing_planes: c_ulong = 0,
    backing_pixel: c_ulong = 0,
    save_under: c_int = 0,
    event_mask: c_long = 0,
    do_not_propagate_mask: c_long = 0,
    override_redirect: c_int = 0,
    colormap: Colormap = 0,
    cursor: c_ulong = 0,
};
const XVisualInfo = extern struct {
    visual: ?*anyopaque,
    visualid: c_ulong,
    screen: c_int,
    depth: c_int,
    class: c_int,
    red_mask: c_ulong,
    green_mask: c_ulong,
    blue_mask: c_ulong,
    colormap_size: c_int,
    bits_per_rgb: c_int,
};
const XGeneric = extern struct { type: c_int, serial: c_ulong, send_event: c_int, display: ?*Display, window: Window };
const XClient = extern struct { type: c_int, serial: c_ulong, send_event: c_int, display: ?*Display, window: Window, message_type: Atom, format: c_int, l0: c_long, l1: c_long, l2: c_long, l3: c_long, l4: c_long };

extern fn XOpenDisplay(name: ?[*:0]const u8) ?*Display;
extern fn XCloseDisplay(d: *Display) c_int;
extern fn XDefaultScreen(d: *Display) c_int;
extern fn XRootWindow(d: *Display, s: c_int) Window;
extern fn XCreateColormap(d: *Display, w: Window, v: *anyopaque, alloc: c_int) Colormap;
extern fn XCreateWindow(d: *Display, parent: Window, x: c_int, y: c_int, w: c_uint, h: c_uint, bw: c_uint, depth: c_int, class: c_uint, visual: *anyopaque, valuemask: c_ulong, attrs: *XSetWindowAttributes) Window;
extern fn XStoreName(d: *Display, w: Window, name: [*:0]const u8) c_int;
extern fn XMapWindow(d: *Display, w: Window) c_int;
extern fn XPending(d: *Display) c_int;
extern fn XNextEvent(d: *Display, ev: *anyopaque) c_int;
extern fn XInternAtom(d: *Display, name: [*:0]const u8, only: c_int) Atom;
extern fn XSetWMProtocols(d: *Display, w: Window, p: *Atom, n: c_int) c_int;

extern fn glXChooseVisual(d: *Display, screen: c_int, attribs: [*]c_int) ?*XVisualInfo;
extern fn glXCreateContext(d: *Display, vis: *XVisualInfo, share: GLXContext, direct: c_int) GLXContext;
extern fn glXMakeCurrent(d: *Display, drawable: Window, ctx: GLXContext) c_int;
extern fn glXSwapBuffers(d: *Display, drawable: Window) void;
extern fn glXDestroyContext(d: *Display, ctx: GLXContext) void;
extern fn glXGetProcAddressARB(name: [*:0]const u8) ?*const anyopaque;

extern fn glViewport(x: c_int, y: c_int, w: c_int, h: c_int) void;
extern fn glClearColor(r: f32, g: f32, b: f32, a: f32) void;
extern fn glClear(mask: c_uint) void;
extern fn glEnable(cap: c_uint) void;
extern fn glGenTextures(n: c_int, tex: *c_uint) void;
extern fn glBindTexture(target: c_uint, tex: c_uint) void;
extern fn glTexImage2D(target: c_uint, level: c_int, internal: c_int, w: c_int, h: c_int, border: c_int, fmt: c_uint, ty: c_uint, data: ?*const anyopaque) void;
extern fn glTexParameteri(target: c_uint, pname: c_uint, param: c_int) void;
extern fn glBegin(mode: c_uint) void;
extern fn glEnd() void;
extern fn glTexCoord2f(s: f32, t: f32) void;
extern fn glVertex2f(x: f32, y: f32) void;

// Modern GL (shaders), loaded via glXGetProcAddress.
const FnUint = *const fn (c_uint) callconv(.c) void;
var glCreateShader: *const fn (c_uint) callconv(.c) c_uint = undefined;
var glShaderSource: *const fn (c_uint, c_int, [*]const [*:0]const u8, ?[*]const c_int) callconv(.c) void = undefined;
var glCompileShader: FnUint = undefined;
var glGetShaderiv: *const fn (c_uint, c_uint, *c_int) callconv(.c) void = undefined;
var glGetShaderInfoLog: *const fn (c_uint, c_int, ?*c_int, [*]u8) callconv(.c) void = undefined;
var glCreateProgram: *const fn () callconv(.c) c_uint = undefined;
var glAttachShader: *const fn (c_uint, c_uint) callconv(.c) void = undefined;
var glLinkProgram: FnUint = undefined;
var glGetProgramiv: *const fn (c_uint, c_uint, *c_int) callconv(.c) void = undefined;
var glUseProgram: FnUint = undefined;
var glGetUniformLocation: *const fn (c_uint, [*:0]const u8) callconv(.c) c_int = undefined;
var glUniform1i: *const fn (c_int, c_int) callconv(.c) void = undefined;
var glUniform2f: *const fn (c_int, f32, f32) callconv(.c) void = undefined;
var glUniform4f: *const fn (c_int, f32, f32, f32, f32) callconv(.c) void = undefined;
var glActiveTexture: FnUint = undefined;

fn loadProc(comptime T: type, name: [*:0]const u8) !T {
    return @ptrCast(glXGetProcAddressARB(name) orelse return error.NoProc);
}

const vert_src: [*:0]const u8 =
    \\#version 120
    \\void main(){ gl_TexCoord[0]=gl_MultiTexCoord0; gl_Position=gl_Vertex; }
;
const frag_src: [*:0]const u8 =
    \\#version 120
    \\uniform sampler2D tex;
    \\uniform vec2 res;
    \\uniform vec4 glass;   // x,y,w,h in pixels (top-left origin)
    \\void main(){
    \\  vec2 uv = gl_TexCoord[0].xy;
    \\  vec3 col = texture2D(tex, uv).rgb;
    \\  vec2 g0 = glass.xy/res;
    \\  vec2 g1 = (glass.xy+glass.zw)/res;
    \\  if (glass.z>1.0 && uv.x>g0.x && uv.x<g1.x && uv.y>g0.y && uv.y<g1.y){
    \\    vec3 acc=vec3(0.0); float n=0.0;
    \\    for(float dx=-5.0;dx<=5.0;dx+=1.0)
    \\    for(float dy=-5.0;dy<=5.0;dy+=1.0){
    \\      acc += texture2D(tex, uv + vec2(dx,dy)*2.0/res).rgb; n+=1.0;
    \\    }
    \\    col = mix(acc/n, vec3(1.0), 0.16);   // frosted tint
    \\  }
    \\  gl_FragColor = vec4(col,1.0);
    \\}
;

fn compile(kind: c_uint, src: [*:0]const u8) !c_uint {
    const sh = glCreateShader(kind);
    const arr = [_][*:0]const u8{src};
    glShaderSource(sh, 1, &arr, null);
    glCompileShader(sh);
    var ok: c_int = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        var log: [1024]u8 = undefined;
        glGetShaderInfoLog(sh, 1024, null, &log);
        std.debug.print("shader compile error: {s}\n", .{@as([*:0]const u8, @ptrCast(&log))});
        return error.ShaderCompile;
    }
    return sh;
}

pub const GlError = error{ NoDisplay, NoVisual, NoContext, NoProc, ShaderCompile, LinkFailed };

pub const GlWindow = struct {
    display: *Display,
    win: Window,
    ctx: GLXContext,
    tex: c_uint,
    program: c_uint,
    u_res: c_int,
    u_glass: c_int,
    width: c_int,
    height: c_int,
    wm_delete: Atom,

    pub fn open(w: usize, h: usize, title: [*:0]const u8) GlError!GlWindow {
        const display = XOpenDisplay(null) orelse return GlError.NoDisplay;
        const screen = XDefaultScreen(display);
        const root = XRootWindow(display, screen);
        var attribs = [_]c_int{ GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, 0 };
        const vi = glXChooseVisual(display, screen, &attribs) orelse return GlError.NoVisual;
        const cmap = XCreateColormap(display, root, vi.visual.?, AllocNone);
        var swa = XSetWindowAttributes{ .colormap = cmap, .border_pixel = 0, .event_mask = ExposureMask | KeyPressMask | StructureNotifyMask };
        const win = XCreateWindow(display, root, 0, 0, @intCast(w), @intCast(h), 0, vi.depth, InputOutput, vi.visual.?, CWBorderPixel | CWColormap | CWEventMask, &swa);
        _ = XStoreName(display, win, title);
        var wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", 0);
        _ = XSetWMProtocols(display, win, &wm_delete, 1);
        _ = XMapWindow(display, win);
        const ctx = glXCreateContext(display, vi, null, 1);
        if (ctx == null) return GlError.NoContext;
        _ = glXMakeCurrent(display, win, ctx);

        // load shader entry points
        glCreateShader = try loadProc(@TypeOf(glCreateShader), "glCreateShader");
        glShaderSource = try loadProc(@TypeOf(glShaderSource), "glShaderSource");
        glCompileShader = try loadProc(FnUint, "glCompileShader");
        glGetShaderiv = try loadProc(@TypeOf(glGetShaderiv), "glGetShaderiv");
        glGetShaderInfoLog = try loadProc(@TypeOf(glGetShaderInfoLog), "glGetShaderInfoLog");
        glCreateProgram = try loadProc(@TypeOf(glCreateProgram), "glCreateProgram");
        glAttachShader = try loadProc(@TypeOf(glAttachShader), "glAttachShader");
        glLinkProgram = try loadProc(FnUint, "glLinkProgram");
        glGetProgramiv = try loadProc(@TypeOf(glGetProgramiv), "glGetProgramiv");
        glUseProgram = try loadProc(FnUint, "glUseProgram");
        glGetUniformLocation = try loadProc(@TypeOf(glGetUniformLocation), "glGetUniformLocation");
        glUniform1i = try loadProc(@TypeOf(glUniform1i), "glUniform1i");
        glUniform2f = try loadProc(@TypeOf(glUniform2f), "glUniform2f");
        glUniform4f = try loadProc(@TypeOf(glUniform4f), "glUniform4f");
        glActiveTexture = try loadProc(FnUint, "glActiveTexture");

        const vs = try compile(GL_VERTEX_SHADER, vert_src);
        const fs = try compile(GL_FRAGMENT_SHADER, frag_src);
        const prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);
        var linked: c_int = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &linked);
        if (linked == 0) return GlError.LinkFailed;

        glUseProgram(prog);
        glUniform1i(glGetUniformLocation(prog, "tex"), 0);
        const u_res = glGetUniformLocation(prog, "res");
        const u_glass = glGetUniformLocation(prog, "glass");

        var tex: c_uint = 0;
        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return .{ .display = display, .win = win, .ctx = ctx, .tex = tex, .program = prog, .u_res = u_res, .u_glass = u_glass, .width = @intCast(w), .height = @intCast(h), .wm_delete = wm_delete };
    }

    /// Present the UI texture with a GPU shader that frosts the given glass rect
    /// (pixels, top-left origin). Pass w=0 to disable the glass blur.
    pub fn presentGlass(self: *GlWindow, rgba: []const u8, gx: f32, gy: f32, gw: f32, gh: f32) void {
        glViewport(0, 0, self.width, self.height);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, self.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, @intCast(GL_RGBA), self.width, self.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.ptr);
        glUseProgram(self.program);
        glUniform2f(self.u_res, @floatFromInt(self.width), @floatFromInt(self.height));
        glUniform4f(self.u_glass, gx, gy, gw, gh);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1, 1);
        glTexCoord2f(1, 0);
        glVertex2f(1, 1);
        glTexCoord2f(1, 1);
        glVertex2f(1, -1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, -1);
        glEnd();
        glXSwapBuffers(self.display, self.win);
    }

    pub fn pump(self: *GlWindow) bool {
        while (XPending(self.display) > 0) {
            var raw: [192]u8 align(8) = undefined;
            _ = XNextEvent(self.display, &raw);
            const t = @as(*const XGeneric, @ptrCast(&raw)).type;
            if (t == KeyPress) return true;
            if (t == ClientMessage) {
                const e: *const XClient = @ptrCast(&raw);
                if (@as(Atom, @bitCast(e.l0)) == self.wm_delete) return true;
            }
        }
        return false;
    }

    pub fn close(self: *GlWindow) void {
        _ = glXMakeCurrent(self.display, 0, null);
        glXDestroyContext(self.display, self.ctx);
        _ = XCloseDisplay(self.display);
    }
};
