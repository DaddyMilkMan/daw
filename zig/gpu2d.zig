//! gpu2d.zig — a GPU 2D UI renderer, GPUI-style: each primitive type has its own
//! instanced shader, batched into one draw call, composited in LINEAR light via an
//! sRGB framebuffer with premultiplied alpha. Shapes are signed-distance fields
//! (analytic AA, no MSAA); drop shadows are Evan Wallace's closed-form gaussian
//! (erf) — no offscreen blur pass. Modern GL entry points are loaded at runtime
//! via the window's glXGetProcAddress; nothing here is JUCE/Skia-derived.

const std = @import("std");
const r2d = @import("render2d.zig");
pub const Color = r2d.Color;

// ---- GL constants ----------------------------------------------------------
const GL_FLOAT: u32 = 0x1406;
const GL_FALSE: u8 = 0;
const GL_TRIANGLE_STRIP: u32 = 0x0005;
const GL_ARRAY_BUFFER: u32 = 0x8892;
const GL_STATIC_DRAW: u32 = 0x88E4;
const GL_DYNAMIC_DRAW: u32 = 0x88E8;
const GL_COLOR_BUFFER_BIT: u32 = 0x4000;
const GL_BLEND: u32 = 0x0BE2;
const GL_ONE: u32 = 1;
const GL_ONE_MINUS_SRC_ALPHA: u32 = 0x0303;
const GL_FUNC_ADD: u32 = 0x8006;
const GL_FRAMEBUFFER_SRGB: u32 = 0x8DB9;
const GL_VERTEX_SHADER: u32 = 0x8B31;
const GL_FRAGMENT_SHADER: u32 = 0x8B30;
const GL_COMPILE_STATUS: u32 = 0x8B81;
const GL_LINK_STATUS: u32 = 0x8B82;
const GL_TEXTURE_2D: u32 = 0x0DE1;
const GL_TEXTURE0: u32 = 0x84C0;
const GL_RED: u32 = 0x1903;
const GL_R8: i32 = 0x8229;
const GL_UNSIGNED_BYTE: u32 = 0x1401;
const GL_LINEAR: i32 = 0x2601;
const GL_TEXTURE_MIN_FILTER: u32 = 0x2801;
const GL_TEXTURE_MAG_FILTER: u32 = 0x2800;
const GL_TEXTURE_WRAP_S: u32 = 0x2802;
const GL_TEXTURE_WRAP_T: u32 = 0x2803;
const GL_CLAMP_TO_EDGE: i32 = 0x812F;
const GL_UNPACK_ALIGNMENT: u32 = 0x0CF5;

// ---- core GL (linked) ------------------------------------------------------
extern fn glViewport(x: c_int, y: c_int, w: c_int, h: c_int) void;
extern fn glClear(mask: c_uint) void;
extern fn glClearColor(r: f32, g: f32, b: f32, a: f32) void;
extern fn glEnable(cap: c_uint) void;
extern fn glDisable(cap: c_uint) void;
extern fn glBlendFunc(s: c_uint, d: c_uint) void;
extern fn glGenTextures(n: c_int, t: *c_uint) void;
extern fn glBindTexture(target: c_uint, t: c_uint) void;
extern fn glTexImage2D(target: c_uint, level: c_int, internal: c_int, w: c_int, h: c_int, border: c_int, fmt: c_uint, ty: c_uint, data: ?*const anyopaque) void;
extern fn glTexParameteri(target: c_uint, pname: c_uint, param: c_int) void;
extern fn glPixelStorei(pname: c_uint, param: c_int) void;

// ---- modern GL (loaded via glXGetProcAddress) ------------------------------
var glCreateShader: *const fn (c_uint) callconv(.c) c_uint = undefined;
var glShaderSource: *const fn (c_uint, c_int, [*]const [*:0]const u8, ?[*]const c_int) callconv(.c) void = undefined;
var glCompileShader: *const fn (c_uint) callconv(.c) void = undefined;
var glGetShaderiv: *const fn (c_uint, c_uint, *c_int) callconv(.c) void = undefined;
var glGetShaderInfoLog: *const fn (c_uint, c_int, ?*c_int, [*]u8) callconv(.c) void = undefined;
var glCreateProgram: *const fn () callconv(.c) c_uint = undefined;
var glAttachShader: *const fn (c_uint, c_uint) callconv(.c) void = undefined;
var glLinkProgram: *const fn (c_uint) callconv(.c) void = undefined;
var glGetProgramiv: *const fn (c_uint, c_uint, *c_int) callconv(.c) void = undefined;
var glUseProgram: *const fn (c_uint) callconv(.c) void = undefined;
var glGetUniformLocation: *const fn (c_uint, [*:0]const u8) callconv(.c) c_int = undefined;
var glUniform2f: *const fn (c_int, f32, f32) callconv(.c) void = undefined;
var glUniform1i: *const fn (c_int, c_int) callconv(.c) void = undefined;
var glActiveTexture: *const fn (c_uint) callconv(.c) void = undefined;
var glGenVertexArrays: *const fn (c_int, *c_uint) callconv(.c) void = undefined;
var glBindVertexArray: *const fn (c_uint) callconv(.c) void = undefined;
var glGenBuffers: *const fn (c_int, *c_uint) callconv(.c) void = undefined;
var glBindBuffer: *const fn (c_uint, c_uint) callconv(.c) void = undefined;
var glBufferData: *const fn (c_uint, isize, ?*const anyopaque, c_uint) callconv(.c) void = undefined;
var glEnableVertexAttribArray: *const fn (c_uint) callconv(.c) void = undefined;
var glVertexAttribPointer: *const fn (c_uint, c_int, c_uint, u8, c_int, usize) callconv(.c) void = undefined;
var glVertexAttribDivisor: *const fn (c_uint, c_uint) callconv(.c) void = undefined;
var glDrawArraysInstanced: *const fn (c_uint, c_int, c_int, c_int) callconv(.c) void = undefined;

const GetProc = *const fn (name: [*:0]const u8) ?*const anyopaque;

fn load(get: GetProc, comptime T: type, name: [*:0]const u8) !T {
    return @ptrCast(get(name) orelse {
        std.debug.print("gpu2d: missing GL proc {s}\n", .{name});
        return error.NoProc;
    });
}

fn loadAll(get: GetProc) !void {
    glCreateShader = try load(get, @TypeOf(glCreateShader), "glCreateShader");
    glShaderSource = try load(get, @TypeOf(glShaderSource), "glShaderSource");
    glCompileShader = try load(get, @TypeOf(glCompileShader), "glCompileShader");
    glGetShaderiv = try load(get, @TypeOf(glGetShaderiv), "glGetShaderiv");
    glGetShaderInfoLog = try load(get, @TypeOf(glGetShaderInfoLog), "glGetShaderInfoLog");
    glCreateProgram = try load(get, @TypeOf(glCreateProgram), "glCreateProgram");
    glAttachShader = try load(get, @TypeOf(glAttachShader), "glAttachShader");
    glLinkProgram = try load(get, @TypeOf(glLinkProgram), "glLinkProgram");
    glGetProgramiv = try load(get, @TypeOf(glGetProgramiv), "glGetProgramiv");
    glUseProgram = try load(get, @TypeOf(glUseProgram), "glUseProgram");
    glGetUniformLocation = try load(get, @TypeOf(glGetUniformLocation), "glGetUniformLocation");
    glUniform2f = try load(get, @TypeOf(glUniform2f), "glUniform2f");
    glUniform1i = try load(get, @TypeOf(glUniform1i), "glUniform1i");
    glActiveTexture = try load(get, @TypeOf(glActiveTexture), "glActiveTexture");
    glGenVertexArrays = try load(get, @TypeOf(glGenVertexArrays), "glGenVertexArrays");
    glBindVertexArray = try load(get, @TypeOf(glBindVertexArray), "glBindVertexArray");
    glGenBuffers = try load(get, @TypeOf(glGenBuffers), "glGenBuffers");
    glBindBuffer = try load(get, @TypeOf(glBindBuffer), "glBindBuffer");
    glBufferData = try load(get, @TypeOf(glBufferData), "glBufferData");
    glEnableVertexAttribArray = try load(get, @TypeOf(glEnableVertexAttribArray), "glEnableVertexAttribArray");
    glVertexAttribPointer = try load(get, @TypeOf(glVertexAttribPointer), "glVertexAttribPointer");
    glVertexAttribDivisor = try load(get, @TypeOf(glVertexAttribDivisor), "glVertexAttribDivisor");
    glDrawArraysInstanced = try load(get, @TypeOf(glDrawArraysInstanced), "glDrawArraysInstanced");
}

fn compile(kind: u32, src: [*:0]const u8) !c_uint {
    const sh = glCreateShader(kind);
    const arr = [_][*:0]const u8{src};
    glShaderSource(sh, 1, &arr, null);
    glCompileShader(sh);
    var ok: c_int = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        var log: [2048]u8 = undefined;
        glGetShaderInfoLog(sh, 2048, null, &log);
        std.debug.print("gpu2d shader compile error:\n{s}\n", .{@as([*:0]const u8, @ptrCast(&log))});
        return error.ShaderCompile;
    }
    return sh;
}
fn linkProgram(vs_src: [*:0]const u8, fs_src: [*:0]const u8) !c_uint {
    const vs = try compile(GL_VERTEX_SHADER, vs_src);
    const fs = try compile(GL_FRAGMENT_SHADER, fs_src);
    const p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    var ok: c_int = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (ok == 0) return error.LinkFailed;
    return p;
}

// sRGB->linear for passing linear colors to shaders (framebuffer re-encodes).
fn srgbToLin(c: u8) f32 {
    const x = @as(f32, @floatFromInt(c)) / 255.0;
    return if (x <= 0.04045) x / 12.92 else std.math.pow(f32, (x + 0.055) / 1.055, 2.4);
}
fn lin(c: Color) [4]f32 {
    return .{ srgbToLin(c.r), srgbToLin(c.g), srgbToLin(c.b), @as(f32, @floatFromInt(c.a)) / 255.0 };
}

// ---- shaders ---------------------------------------------------------------
const rect_vs: [*:0]const u8 =
    \\#version 330 core
    \\layout(location=0) in vec2 quad;
    \\layout(location=1) in vec2 iMin;
    \\layout(location=2) in vec2 iSize;
    \\layout(location=3) in float iRadius;
    \\layout(location=4) in float iBorder;
    \\layout(location=5) in vec4 iFill;
    \\layout(location=6) in vec4 iBorderCol;
    \\uniform vec2 uRes;
    \\out vec2 vLocal; out vec2 vHalf; out float vRadius; out float vBorder; out vec4 vFill; out vec4 vBorderCol;
    \\void main(){
    \\  vec2 px = iMin + quad * iSize;
    \\  vHalf = iSize * 0.5;
    \\  vLocal = px - (iMin + vHalf);
    \\  vRadius = iRadius; vBorder = iBorder; vFill = iFill; vBorderCol = iBorderCol;
    \\  vec2 clip = (px / uRes) * 2.0 - 1.0;
    \\  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
    \\}
;
const rect_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec2 vLocal; in vec2 vHalf; in float vRadius; in float vBorder; in vec4 vFill; in vec4 vBorderCol;
    \\out vec4 frag;
    \\float sdRound(vec2 p, vec2 b, float r){ vec2 q = abs(p)-b+r; return min(max(q.x,q.y),0.0)+length(max(q,vec2(0.0)))-r; }
    \\void main(){
    \\  float d = sdRound(vLocal, vHalf, vRadius);
    \\  float aOuter = clamp(0.5 - d, 0.0, 1.0);
    \\  float aInner = clamp(0.5 - (d + max(vBorder,0.0)), 0.0, 1.0);
    \\  float band = clamp(aOuter - aInner, 0.0, 1.0);
    \\  vec4 col = mix(vFill, vBorderCol, band);
    \\  float a = aOuter * col.a;
    \\  frag = vec4(col.rgb * a, a);
    \\}
;
const shadow_vs: [*:0]const u8 =
    \\#version 330 core
    \\layout(location=0) in vec2 quad;
    \\layout(location=1) in vec2 iLower;
    \\layout(location=2) in vec2 iUpper;
    \\layout(location=3) in float iSigma;
    \\layout(location=4) in float iCorner;
    \\layout(location=5) in vec4 iColor;
    \\uniform vec2 uRes;
    \\out vec2 vPoint; out vec2 vLower; out vec2 vUpper; out float vSigma; out float vCorner; out vec4 vColor;
    \\void main(){
    \\  float pad = 3.0*iSigma + iCorner + 2.0;
    \\  vec2 lo = iLower - pad; vec2 hi = iUpper + pad;
    \\  vec2 px = mix(lo, hi, quad);
    \\  vPoint = px; vLower = iLower; vUpper = iUpper; vSigma = max(iSigma,0.5); vCorner = iCorner; vColor = iColor;
    \\  vec2 clip = (px / uRes) * 2.0 - 1.0;
    \\  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
    \\}
;
const shadow_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec2 vPoint; in vec2 vLower; in vec2 vUpper; in float vSigma; in float vCorner; in vec4 vColor;
    \\out vec4 frag;
    \\vec2 erf(vec2 x){ vec2 s=sign(x), a=abs(x); x=1.0+(0.278393+(0.230389+0.078108*(a*a))*a)*a; x*=x; return s - s/(x*x); }
    \\float gaussian(float x,float sigma){ return exp(-(x*x)/(2.0*sigma*sigma))/(2.5066282746*sigma); }
    \\float rbsX(float x,float y,float sigma,float corner,vec2 hs){
    \\  float delta = min(hs.y - corner - abs(y), 0.0);
    \\  float curved = hs.x - corner + sqrt(max(0.0, corner*corner - delta*delta));
    \\  vec2 integral = 0.5 + 0.5*erf((x+vec2(-curved,curved))*(0.7071067812/sigma));
    \\  return integral.y - integral.x;
    \\}
    \\float rbShadow(vec2 lo, vec2 hi, vec2 pt, float sigma, float corner){
    \\  vec2 center=(lo+hi)*0.5; vec2 hs=(hi-lo)*0.5; vec2 p=pt-center;
    \\  float low=p.y-hs.y, high=p.y+hs.y;
    \\  float start=clamp(-3.0*sigma, low, high);
    \\  float end=clamp(3.0*sigma, low, high);
    \\  float step=(end-start)/4.0; float y=start+step*0.5; float val=0.0;
    \\  for(int i=0;i<4;i++){ val += rbsX(p.x, p.y-y, sigma, corner, hs)*gaussian(y,sigma)*step; y+=step; }
    \\  return val;
    \\}
    \\void main(){
    \\  float a = clamp(rbShadow(vLower, vUpper, vPoint, vSigma, vCorner), 0.0, 1.0) * vColor.a;
    \\  frag = vec4(vColor.rgb * a, a);
    \\}
;

const RECT_FLOATS = 14; // min2 size2 radius border fill4 border4
const SHADOW_FLOATS = 10; // lower2 upper2 sigma corner color4

pub const Gpu = struct {
    allocator: std.mem.Allocator,
    rect_prog: c_uint,
    shadow_prog: c_uint,
    rect_u_res: c_int,
    shadow_u_res: c_int,
    quad_vbo: c_uint,
    rect_vao: c_uint,
    rect_ivbo: c_uint,
    shadow_vao: c_uint,
    shadow_ivbo: c_uint,
    rects: std.ArrayList(f32),
    shadows: std.ArrayList(f32),
    width: f32 = 0,
    height: f32 = 0,

    pub fn init(a: std.mem.Allocator, get: GetProc) !Gpu {
        try loadAll(get);
        const rect_prog = try linkProgram(rect_vs, rect_fs);
        const shadow_prog = try linkProgram(shadow_vs, shadow_fs);

        // unit quad (triangle strip): (0,0)(1,0)(0,1)(1,1)
        const quad = [_]f32{ 0, 0, 1, 0, 0, 1, 1, 1 };
        var quad_vbo: c_uint = 0;
        glGenBuffers(1, &quad_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, @sizeOf(@TypeOf(quad)), &quad, GL_STATIC_DRAW);

        var g = Gpu{
            .allocator = a,
            .rect_prog = rect_prog,
            .shadow_prog = shadow_prog,
            .rect_u_res = glGetUniformLocation(rect_prog, "uRes"),
            .shadow_u_res = glGetUniformLocation(shadow_prog, "uRes"),
            .quad_vbo = quad_vbo,
            .rect_vao = 0,
            .rect_ivbo = 0,
            .shadow_vao = 0,
            .shadow_ivbo = 0,
            .rects = std.ArrayList(f32).init(a),
            .shadows = std.ArrayList(f32).init(a),
        };
        g.setupRectVao();
        g.setupShadowVao();
        return g;
    }

    fn attribQuad(self: *Gpu) void {
        glBindBuffer(GL_ARRAY_BUFFER, self.quad_vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * @sizeOf(f32), 0);
        glVertexAttribDivisor(0, 0);
    }
    fn setupRectVao(self: *Gpu) void {
        glGenVertexArrays(1, &self.rect_vao);
        glBindVertexArray(self.rect_vao);
        self.attribQuad();
        glGenBuffers(1, &self.rect_ivbo);
        glBindBuffer(GL_ARRAY_BUFFER, self.rect_ivbo);
        const stride: c_int = RECT_FLOATS * @sizeOf(f32);
        // loc1 min(2) loc2 size(2) loc3 radius(1) loc4 border(1) loc5 fill(4) loc6 bordercol(4)
        const specs = [_][3]u32{ .{ 1, 2, 0 }, .{ 2, 2, 2 }, .{ 3, 1, 4 }, .{ 4, 1, 5 }, .{ 5, 4, 6 }, .{ 6, 4, 10 } };
        inline for (specs) |s| {
            glEnableVertexAttribArray(s[0]);
            glVertexAttribPointer(s[0], @intCast(s[1]), GL_FLOAT, GL_FALSE, stride, s[2] * @sizeOf(f32));
            glVertexAttribDivisor(s[0], 1);
        }
    }
    fn setupShadowVao(self: *Gpu) void {
        glGenVertexArrays(1, &self.shadow_vao);
        glBindVertexArray(self.shadow_vao);
        self.attribQuad();
        glGenBuffers(1, &self.shadow_ivbo);
        glBindBuffer(GL_ARRAY_BUFFER, self.shadow_ivbo);
        const stride: c_int = SHADOW_FLOATS * @sizeOf(f32);
        const specs = [_][3]u32{ .{ 1, 2, 0 }, .{ 2, 2, 2 }, .{ 3, 1, 4 }, .{ 4, 1, 5 }, .{ 5, 4, 6 } };
        inline for (specs) |s| {
            glEnableVertexAttribArray(s[0]);
            glVertexAttribPointer(s[0], @intCast(s[1]), GL_FLOAT, GL_FALSE, stride, s[2] * @sizeOf(f32));
            glVertexAttribDivisor(s[0], 1);
        }
    }

    pub fn begin(self: *Gpu, w: usize, h: usize, bg: Color) void {
        self.width = @floatFromInt(w);
        self.height = @floatFromInt(h);
        self.rects.clearRetainingCapacity();
        self.shadows.clearRetainingCapacity();
        glViewport(0, 0, @intCast(w), @intCast(h));
        glEnable(GL_FRAMEBUFFER_SRGB); // shaders output linear; GPU encodes to sRGB
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // premultiplied over
        const bl = lin(bg);
        glClearColor(bl[0], bl[1], bl[2], 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    /// Filled rounded rect with optional 1px+ border. Pixel coords, top-left origin.
    pub fn rect(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, fill: Color) void {
        self.rectBordered(x, y, w, h, radius, fill, 0, fill);
    }
    pub fn rectBordered(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, fill: Color, border_w: f32, border: Color) void {
        const f = lin(fill);
        const b = lin(border);
        self.rects.appendSlice(&.{ x, y, w, h, radius, border_w, f[0], f[1], f[2], f[3], b[0], b[1], b[2], b[3] }) catch {};
    }
    /// Analytic gaussian drop shadow for a rounded rect. sigma = blur std-dev.
    pub fn shadow(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, sigma: f32, color: Color) void {
        const c = lin(color);
        self.shadows.appendSlice(&.{ x, y, x + w, y + h, sigma, radius, c[0], c[1], c[2], c[3] }) catch {};
    }

    pub fn flush(self: *Gpu) void {
        // shadows first (drawn behind), then rects
        if (self.shadows.items.len > 0) {
            glUseProgram(self.shadow_prog);
            glUniform2f(self.shadow_u_res, self.width, self.height);
            glBindVertexArray(self.shadow_vao);
            glBindBuffer(GL_ARRAY_BUFFER, self.shadow_ivbo);
            glBufferData(GL_ARRAY_BUFFER, @intCast(self.shadows.items.len * @sizeOf(f32)), self.shadows.items.ptr, GL_DYNAMIC_DRAW);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, @intCast(self.shadows.items.len / SHADOW_FLOATS));
        }
        if (self.rects.items.len > 0) {
            glUseProgram(self.rect_prog);
            glUniform2f(self.rect_u_res, self.width, self.height);
            glBindVertexArray(self.rect_vao);
            glBindBuffer(GL_ARRAY_BUFFER, self.rect_ivbo);
            glBufferData(GL_ARRAY_BUFFER, @intCast(self.rects.items.len * @sizeOf(f32)), self.rects.items.ptr, GL_DYNAMIC_DRAW);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, @intCast(self.rects.items.len / RECT_FLOATS));
        }
    }

    pub fn deinit(self: *Gpu) void {
        self.rects.deinit();
        self.shadows.deinit();
    }
};
