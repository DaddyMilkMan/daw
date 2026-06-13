//! gpu2d.zig — a GPU 2D UI renderer, GPUI-style: each primitive type has its own
//! instanced shader, batched into one draw call, composited in LINEAR light via an
//! sRGB framebuffer with premultiplied alpha. Shapes are signed-distance fields
//! (analytic AA, no MSAA); drop shadows are Evan Wallace's closed-form gaussian
//! (erf) — no offscreen blur pass. Modern GL entry points are loaded at runtime
//! via the window's glXGetProcAddress; nothing here is JUCE/Skia-derived.

const std = @import("std");
pub const Color = @import("color.zig").Color;

// ---- GL constants ----------------------------------------------------------
const GL_FLOAT: u32 = 0x1406;
const GL_FALSE: u8 = 0;
const GL_TRIANGLE_STRIP: u32 = 0x0005;
const GL_TRIANGLES: u32 = 0x0004;
const GL_ARRAY_BUFFER: u32 = 0x8892;
const GL_STATIC_DRAW: u32 = 0x88E4;
const GL_DYNAMIC_DRAW: u32 = 0x88E8;
const GL_COLOR_BUFFER_BIT: u32 = 0x4000;
const GL_BLEND: u32 = 0x0BE2;
const GL_ONE: u32 = 1;
const GL_ONE_MINUS_SRC_ALPHA: u32 = 0x0303;
const GL_FUNC_ADD: u32 = 0x8006;
const GL_FRAMEBUFFER_SRGB: u32 = 0x8DB9;
const GL_MULTISAMPLE: u32 = 0x809D;
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
const GL_RGB: u32 = 0x1907;
const GL_RGBA: u32 = 0x1908;
const GL_RGB8: i32 = 0x8051;
const GL_FRAMEBUFFER: u32 = 0x8D40;
const GL_COLOR_ATTACHMENT0: u32 = 0x8CE0;
const GL_FRAMEBUFFER_COMPLETE: u32 = 0x8CD5;

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
extern fn glDrawArrays(mode: c_uint, first: c_int, count: c_int) void;
extern fn glCopyTexImage2D(target: c_uint, level: c_int, internal: c_uint, x: c_int, y: c_int, w: c_int, h: c_int, border: c_int) void;

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
var glUniform1f: *const fn (c_int, f32) callconv(.c) void = undefined;
var glUniform4f: *const fn (c_int, f32, f32, f32, f32) callconv(.c) void = undefined;
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
var glGenFramebuffers: *const fn (c_int, *c_uint) callconv(.c) void = undefined;
var glBindFramebuffer: *const fn (c_uint, c_uint) callconv(.c) void = undefined;
var glFramebufferTexture2D: *const fn (c_uint, c_uint, c_uint, c_uint, c_int) callconv(.c) void = undefined;

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
    glUniform1f = try load(get, @TypeOf(glUniform1f), "glUniform1f");
    glUniform4f = try load(get, @TypeOf(glUniform4f), "glUniform4f");
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
    glGenFramebuffers = try load(get, @TypeOf(glGenFramebuffers), "glGenFramebuffers");
    glBindFramebuffer = try load(get, @TypeOf(glBindFramebuffer), "glBindFramebuffer");
    glFramebufferTexture2D = try load(get, @TypeOf(glFramebufferTexture2D), "glFramebufferTexture2D");
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
    \\layout(location=7) in vec4 iFill2;
    \\layout(location=8) in float iElev;
    \\uniform vec2 uRes;
    \\out vec2 vLocal; out vec2 vHalf; out float vRadius; out float vBorder; out vec4 vFill; out vec4 vBorderCol; out vec4 vFill2; out float vT; out float vElev;
    \\void main(){
    \\  vec2 px = iMin + quad * iSize;
    \\  vHalf = iSize * 0.5;
    \\  vLocal = px - (iMin + vHalf);
    \\  vRadius = iRadius; vBorder = iBorder; vFill = iFill; vBorderCol = iBorderCol; vFill2 = iFill2; vT = quad.y; vElev = iElev;
    \\  vec2 clip = (px / uRes) * 2.0 - 1.0;
    \\  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
    \\}
;
const rect_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec2 vLocal; in vec2 vHalf; in float vRadius; in float vBorder; in vec4 vFill; in vec4 vBorderCol; in vec4 vFill2; in float vT; in float vElev;
    \\out vec4 frag;
    \\float sdRound(vec2 p, vec2 b, float r){ vec2 q = abs(p)-b+r; return min(max(q.x,q.y),0.0)+length(max(q,vec2(0.0)))-r; }
    \\void main(){
    \\  float d = sdRound(vLocal, vHalf, vRadius);
    \\  float aOuter = clamp(0.5 - d, 0.0, 1.0);
    \\  float aInner = clamp(0.5 - (d + max(vBorder,0.0)), 0.0, 1.0);
    \\  float band = clamp(aOuter - aInner, 0.0, 1.0);
    \\  vec4 fillc = mix(vFill, vFill2, vT);
    \\  vec4 col = mix(fillc, vBorderCol, band);
    \\  vec3 rgb = col.rgb;
    \\  // clean lighting: a crisp ~1px rim of light along the top edge (light catching
    \\  // the edge) + a very subtle sheen below it + a soft bottom shade. No smudge.
    \\  float distTop = vLocal.y + vHalf.y;        // 0 at top inner edge, grows down
    \\  float distBot = vHalf.y - vLocal.y;        // 0 at bottom inner edge
    \\  float interior = clamp(aInner, 0.0, 1.0);  // don't light the border ring
    \\  float rim   = (1.0 - smoothstep(0.0, 1.4, distTop)) * 0.14 * vElev * interior;
    \\  float sheen = exp(-distTop / 7.0) * 0.022 * vElev * interior;
    \\  float ish   = exp(-distBot / 9.0) * 0.03  * vElev * interior;
    \\  rgb += rim + sheen - ish;
    \\  // low-amplitude dither to break banding without reading as grain
    \\  float n = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    \\  rgb += (n - 0.5) * (0.7 / 255.0);
    \\  float a = aOuter * col.a;
    \\  frag = vec4(clamp(rgb, 0.0, 1.0) * a, a);
    \\}
;
const tri_vs: [*:0]const u8 =
    \\#version 330 core
    \\layout(location=0) in vec2 pos;
    \\layout(location=1) in vec4 col;
    \\uniform vec2 uRes;
    \\out vec4 vColor;
    \\void main(){ vColor = col; vec2 clip = (pos/uRes)*2.0-1.0; gl_Position = vec4(clip.x,-clip.y,0.0,1.0); }
;
const tri_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec4 vColor; out vec4 frag;
    \\void main(){ frag = vec4(vColor.rgb*vColor.a, vColor.a); }
;
// dual-Kawase backdrop blur (down/up) + frosted-glass composite
const blur_vs: [*:0]const u8 =
    \\#version 330 core
    \\layout(location=0) in vec2 quad;
    \\out vec2 vUv;
    \\void main(){ vUv = quad; gl_Position = vec4(quad*2.0-1.0, 0.0, 1.0); }
;
const down_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec2 vUv; out vec4 frag; uniform sampler2D tex; uniform vec2 hp;
    \\void main(){
    \\  vec4 s = texture(tex, vUv) * 4.0;
    \\  s += texture(tex, vUv - hp);
    \\  s += texture(tex, vUv + hp);
    \\  s += texture(tex, vUv + vec2(hp.x, -hp.y));
    \\  s += texture(tex, vUv - vec2(hp.x, -hp.y));
    \\  frag = s / 8.0;
    \\}
;
const up_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec2 vUv; out vec4 frag; uniform sampler2D tex; uniform vec2 hp;
    \\void main(){
    \\  vec4 s = texture(tex, vUv + vec2(-hp.x*2.0, 0.0));
    \\  s += texture(tex, vUv + vec2(-hp.x, hp.y)) * 2.0;
    \\  s += texture(tex, vUv + vec2(0.0, hp.y*2.0));
    \\  s += texture(tex, vUv + vec2(hp.x, hp.y)) * 2.0;
    \\  s += texture(tex, vUv + vec2(hp.x*2.0, 0.0));
    \\  s += texture(tex, vUv + vec2(hp.x, -hp.y)) * 2.0;
    \\  s += texture(tex, vUv + vec2(0.0, -hp.y*2.0));
    \\  s += texture(tex, vUv + vec2(-hp.x, -hp.y)) * 2.0;
    \\  frag = s / 12.0;
    \\}
;
const glass_vs: [*:0]const u8 =
    \\#version 330 core
    \\layout(location=0) in vec2 quad;
    \\uniform vec2 uRes; uniform vec4 uRect;
    \\out vec2 vScreen;
    \\void main(){
    \\  vec2 px = uRect.xy + quad * uRect.zw;
    \\  vScreen = px;
    \\  vec2 clip = (px / uRes) * 2.0 - 1.0;
    \\  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
    \\}
;
const glass_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec2 vScreen; out vec4 frag;
    \\uniform sampler2D blurTex; uniform vec2 uRes; uniform vec4 uRect; uniform float uRadius;
    \\uniform vec4 uTint; uniform vec4 uBorder;
    \\float sdRound(vec2 p, vec2 b, float r){ vec2 q=abs(p)-b+r; return min(max(q.x,q.y),0.0)+length(max(q,vec2(0.0)))-r; }
    \\vec3 toLin(vec3 c){ return pow(c, vec3(2.2)); }
    \\void main(){
    \\  vec2 center = uRect.xy + uRect.zw*0.5;
    \\  vec2 hs = uRect.zw*0.5;
    \\  float d = sdRound(vScreen - center, hs, uRadius);
    \\  float aOuter = clamp(0.5 - d, 0.0, 1.0);
    \\  if (aOuter <= 0.0) discard;
    \\  vec2 uv = vec2(vScreen.x/uRes.x, 1.0 - vScreen.y/uRes.y);
    \\  vec3 blur = toLin(texture(blurTex, uv).rgb);
    \\  vec3 col = mix(blur, toLin(uTint.rgb), uTint.a);
    \\  // hairline border + top specular for the glass edge
    \\  float aInner = clamp(0.5 - (d + 1.5), 0.0, 1.0);
    \\  float ring = clamp(aOuter - aInner, 0.0, 1.0);
    \\  col = mix(col, toLin(uBorder.rgb), ring * uBorder.a);
    \\  float distTop = vScreen.y - uRect.y;
    \\  col += exp(-distTop/4.0) * 0.06 * aInner;
    \\  frag = vec4(col * aOuter, aOuter);
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

const glyph_vs: [*:0]const u8 =
    \\#version 330 core
    \\layout(location=0) in vec2 quad;
    \\layout(location=1) in vec4 iDst;
    \\layout(location=2) in vec4 iUv;
    \\layout(location=3) in vec4 iColor;
    \\uniform vec2 uRes;
    \\out vec2 vUv; out vec4 vColor;
    \\void main(){
    \\  vec2 px = iDst.xy + quad*iDst.zw;
    \\  vUv = iUv.xy + quad*iUv.zw;
    \\  vColor = iColor;
    \\  vec2 clip = (px/uRes)*2.0-1.0;
    \\  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
    \\}
;
const glyph_fs: [*:0]const u8 =
    \\#version 330 core
    \\in vec2 vUv; in vec4 vColor; out vec4 frag;
    \\uniform sampler2D uAtlas;
    \\void main(){
    \\  float cov = texture(uAtlas, vUv).r;
    \\  // Stem darkening: the coverage was authored for sRGB compositing; blending
    \\  // it in linear light thins the strokes. Fatten with a gamma so light-on-dark
    \\  // text keeps its proper weight (the FreeType/Skia gamma-correct-text fix).
    \\  cov = pow(cov, 0.62);
    \\  float a = cov * vColor.a;
    \\  frag = vec4(vColor.rgb * a, a);
    \\}
;

const RECT_FLOATS = 19; // min2 size2 radius border fill4 border4 fill2_4 elev
const SHADOW_FLOATS = 10; // lower2 upper2 sigma corner color4
const GLYPH_FLOATS = 12; // dst4 uv4 color4
const TRI_FLOATS = 6; // x y r g b a  (per vertex)

const FontT = @import("font.zig").Font;

/// Glyphs are grouped by atlas texture so each font flushes in one draw call.
const GlyphBatch = struct { tex: c_uint, data: std.ArrayList(f32) };

pub const Gpu = struct {
    allocator: std.mem.Allocator,
    rect_prog: c_uint,
    shadow_prog: c_uint,
    glyph_prog: c_uint,
    tri_prog: c_uint,
    rect_u_res: c_int,
    shadow_u_res: c_int,
    glyph_u_res: c_int,
    glyph_u_atlas: c_int,
    tri_u_res: c_int,
    quad_vbo: c_uint,
    rect_vao: c_uint,
    rect_ivbo: c_uint,
    shadow_vao: c_uint,
    shadow_ivbo: c_uint,
    glyph_vao: c_uint,
    glyph_ivbo: c_uint,
    tri_vao: c_uint,
    tri_vbo: c_uint,
    // backdrop blur / glass
    blur_vao: c_uint = 0,
    down_prog: c_uint = 0,
    up_prog: c_uint = 0,
    glass_prog: c_uint = 0,
    tex_full: c_uint = 0,
    half_fbo: c_uint = 0,
    half_tex: c_uint = 0,
    quarter_fbo: c_uint = 0,
    quarter_tex: c_uint = 0,
    gfx_w: usize = 0,
    gfx_h: usize = 0,
    rects: std.ArrayList(f32),
    shadows: std.ArrayList(f32),
    tris: std.ArrayList(f32),
    glyph_batches: std.ArrayList(GlyphBatch),
    width: f32 = 0,
    height: f32 = 0,

    pub fn init(a: std.mem.Allocator, get: GetProc) !Gpu {
        try loadAll(get);
        const rect_prog = try linkProgram(rect_vs, rect_fs);
        const shadow_prog = try linkProgram(shadow_vs, shadow_fs);
        const glyph_prog = try linkProgram(glyph_vs, glyph_fs);
        const tri_prog = try linkProgram(tri_vs, tri_fs);

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
            .glyph_prog = glyph_prog,
            .tri_prog = tri_prog,
            .rect_u_res = glGetUniformLocation(rect_prog, "uRes"),
            .shadow_u_res = glGetUniformLocation(shadow_prog, "uRes"),
            .glyph_u_res = glGetUniformLocation(glyph_prog, "uRes"),
            .glyph_u_atlas = glGetUniformLocation(glyph_prog, "uAtlas"),
            .tri_u_res = glGetUniformLocation(tri_prog, "uRes"),
            .quad_vbo = quad_vbo,
            .rect_vao = 0,
            .rect_ivbo = 0,
            .shadow_vao = 0,
            .shadow_ivbo = 0,
            .glyph_vao = 0,
            .glyph_ivbo = 0,
            .tri_vao = 0,
            .tri_vbo = 0,
            .rects = std.ArrayList(f32).init(a),
            .shadows = std.ArrayList(f32).init(a),
            .tris = std.ArrayList(f32).init(a),
            .glyph_batches = std.ArrayList(GlyphBatch).init(a),
        };
        g.setupRectVao();
        g.setupShadowVao();
        g.setupGlyphVao();
        g.setupTriVao();
        // blur/glass programs + a fullscreen-quad VAO (reuses the unit quad)
        g.down_prog = try linkProgram(blur_vs, down_fs);
        g.up_prog = try linkProgram(blur_vs, up_fs);
        g.glass_prog = try linkProgram(glass_vs, glass_fs);
        glGenVertexArrays(1, &g.blur_vao);
        glBindVertexArray(g.blur_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g.quad_vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * @sizeOf(f32), 0);
        glVertexAttribDivisor(0, 0);
        return g;
    }

    fn newTex(w: usize, h: usize) c_uint {
        var t: c_uint = 0;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, @intCast(w), @intCast(h), 0, GL_RGB, GL_UNSIGNED_BYTE, null);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return t;
    }
    fn ensureGlass(self: *Gpu, w: usize, h: usize) void {
        if (self.gfx_w == w and self.gfx_h == h and self.tex_full != 0) return;
        self.gfx_w = w;
        self.gfx_h = h;
        if (self.tex_full == 0) {
            self.tex_full = newTex(w, h);
            self.half_tex = newTex(@max(w / 2, 1), @max(h / 2, 1));
            self.quarter_tex = newTex(@max(w / 4, 1), @max(h / 4, 1));
            glGenFramebuffers(1, &self.half_fbo);
            glGenFramebuffers(1, &self.quarter_fbo);
        } else {
            glBindTexture(GL_TEXTURE_2D, self.tex_full);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, @intCast(w), @intCast(h), 0, GL_RGB, GL_UNSIGNED_BYTE, null);
            glBindTexture(GL_TEXTURE_2D, self.half_tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, @intCast(@max(w / 2, 1)), @intCast(@max(h / 2, 1)), 0, GL_RGB, GL_UNSIGNED_BYTE, null);
            glBindTexture(GL_TEXTURE_2D, self.quarter_tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, @intCast(@max(w / 4, 1)), @intCast(@max(h / 4, 1)), 0, GL_RGB, GL_UNSIGNED_BYTE, null);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, self.half_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self.half_tex, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, self.quarter_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self.quarter_tex, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    /// Capture the current framebuffer and dual-Kawase blur it into half_tex.
    /// Call after drawing the backdrop (a flush), before drawing glass panels.
    pub fn captureBlur(self: *Gpu, w: usize, h: usize) void {
        self.ensureGlass(w, h);
        const wf: f32 = @floatFromInt(w);
        const hf: f32 = @floatFromInt(h);
        glDisable(GL_BLEND);
        // capture default framebuffer -> tex_full
        glBindTexture(GL_TEXTURE_2D, self.tex_full);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, @intCast(w), @intCast(h), 0);
        glBindVertexArray(self.blur_vao);
        glActiveTexture(GL_TEXTURE0);
        // down: full -> half
        glBindFramebuffer(GL_FRAMEBUFFER, self.half_fbo);
        glViewport(0, 0, @intCast(@max(w / 2, 1)), @intCast(@max(h / 2, 1)));
        glUseProgram(self.down_prog);
        glUniform1i(glGetUniformLocation(self.down_prog, "tex"), 0);
        glUniform2f(glGetUniformLocation(self.down_prog, "hp"), 0.5 / wf, 0.5 / hf);
        glBindTexture(GL_TEXTURE_2D, self.tex_full);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        // down: half -> quarter
        glBindFramebuffer(GL_FRAMEBUFFER, self.quarter_fbo);
        glViewport(0, 0, @intCast(@max(w / 4, 1)), @intCast(@max(h / 4, 1)));
        glUniform2f(glGetUniformLocation(self.down_prog, "hp"), 1.0 / wf, 1.0 / hf);
        glBindTexture(GL_TEXTURE_2D, self.half_tex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        // up: quarter -> half
        glBindFramebuffer(GL_FRAMEBUFFER, self.half_fbo);
        glViewport(0, 0, @intCast(@max(w / 2, 1)), @intCast(@max(h / 2, 1)));
        glUseProgram(self.up_prog);
        glUniform1i(glGetUniformLocation(self.up_prog, "tex"), 0);
        glUniform2f(glGetUniformLocation(self.up_prog, "hp"), 1.0 / wf, 1.0 / hf);
        glBindTexture(GL_TEXTURE_2D, self.quarter_tex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        // restore
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, @intCast(w), @intCast(h));
        glEnable(GL_BLEND);
    }

    /// Draw a frosted-glass rounded panel sampling the blurred backdrop.
    /// tint: panel color + opacity (a = how much tint vs blurred backdrop).
    pub fn glass(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, tint: Color, border: Color) void {
        glUseProgram(self.glass_prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, self.half_tex);
        glUniform1i(glGetUniformLocation(self.glass_prog, "blurTex"), 0);
        glUniform2f(glGetUniformLocation(self.glass_prog, "uRes"), self.width, self.height);
        glUniform4f(glGetUniformLocation(self.glass_prog, "uRect"), x, y, w, h);
        glUniform1f(glGetUniformLocation(self.glass_prog, "uRadius"), radius);
        glUniform4f(glGetUniformLocation(self.glass_prog, "uTint"), @as(f32, @floatFromInt(tint.r)) / 255.0, @as(f32, @floatFromInt(tint.g)) / 255.0, @as(f32, @floatFromInt(tint.b)) / 255.0, @as(f32, @floatFromInt(tint.a)) / 255.0);
        glUniform4f(glGetUniformLocation(self.glass_prog, "uBorder"), @as(f32, @floatFromInt(border.r)) / 255.0, @as(f32, @floatFromInt(border.g)) / 255.0, @as(f32, @floatFromInt(border.b)) / 255.0, @as(f32, @floatFromInt(border.a)) / 255.0);
        glBindVertexArray(self.blur_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
        // loc1 min(2) loc2 size(2) loc3 radius loc4 border loc5 fill(4) loc6 bordercol(4) loc7 fill2(4) loc8 elev(1)
        const specs = [_][3]u32{ .{ 1, 2, 0 }, .{ 2, 2, 2 }, .{ 3, 1, 4 }, .{ 4, 1, 5 }, .{ 5, 4, 6 }, .{ 6, 4, 10 }, .{ 7, 4, 14 }, .{ 8, 1, 18 } };
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
    fn setupGlyphVao(self: *Gpu) void {
        glGenVertexArrays(1, &self.glyph_vao);
        glBindVertexArray(self.glyph_vao);
        self.attribQuad();
        glGenBuffers(1, &self.glyph_ivbo);
        glBindBuffer(GL_ARRAY_BUFFER, self.glyph_ivbo);
        const stride: c_int = GLYPH_FLOATS * @sizeOf(f32);
        const specs = [_][3]u32{ .{ 1, 4, 0 }, .{ 2, 4, 4 }, .{ 3, 4, 8 } };
        inline for (specs) |s| {
            glEnableVertexAttribArray(s[0]);
            glVertexAttribPointer(s[0], @intCast(s[1]), GL_FLOAT, GL_FALSE, stride, s[2] * @sizeOf(f32));
            glVertexAttribDivisor(s[0], 1);
        }
    }
    fn setupTriVao(self: *Gpu) void {
        glGenVertexArrays(1, &self.tri_vao);
        glBindVertexArray(self.tri_vao);
        glGenBuffers(1, &self.tri_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, self.tri_vbo);
        const stride: c_int = TRI_FLOATS * @sizeOf(f32);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, 0);
        glVertexAttribDivisor(0, 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, 2 * @sizeOf(f32));
        glVertexAttribDivisor(1, 0);
    }
    fn glyphBatch(self: *Gpu, tex: c_uint) *GlyphBatch {
        for (self.glyph_batches.items) |*b| if (b.tex == tex) return b;
        self.glyph_batches.append(.{ .tex = tex, .data = std.ArrayList(f32).init(self.allocator) }) catch {};
        return &self.glyph_batches.items[self.glyph_batches.items.len - 1];
    }
    pub fn pushGlyph(self: *Gpu, tex: c_uint, dx: f32, dy: f32, dw: f32, dh: f32, ux: f32, uy: f32, uw: f32, uh: f32, color: Color) void {
        const c = lin(color);
        const b = self.glyphBatch(tex);
        b.data.appendSlice(&.{ dx, dy, dw, dh, ux, uy, uw, uh, c[0], c[1], c[2], c[3] }) catch {};
    }

    pub fn begin(self: *Gpu, w: usize, h: usize, bg: Color) void {
        self.width = @floatFromInt(w);
        self.height = @floatFromInt(h);
        self.rects.clearRetainingCapacity();
        self.shadows.clearRetainingCapacity();
        self.tris.clearRetainingCapacity();
        for (self.glyph_batches.items) |*b| b.data.clearRetainingCapacity();
        glViewport(0, 0, @intCast(w), @intCast(h));
        glEnable(GL_FRAMEBUFFER_SRGB); // shaders output linear; GPU encodes to sRGB
        glEnable(GL_MULTISAMPLE); // MSAA AAs the geometry icons (tri/line)
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
        self.rects.appendSlice(&.{ x, y, w, h, radius, border_w, f[0], f[1], f[2], f[3], b[0], b[1], b[2], b[3], f[0], f[1], f[2], f[3], 0 }) catch {};
    }
    /// Vertically-graded rounded rect (top -> bottom), with optional border. Flat (no material).
    pub fn rectGrad(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, top: Color, bot: Color, border_w: f32, border: Color) void {
        self.gradElev(x, y, w, h, radius, top, bot, border_w, border, 0);
    }
    /// An elevated glass card: gradient fill + border + material depth (specular
    /// top rim, soft inner shadow, dither). `elev` ~1.0 for raised surfaces.
    pub fn card(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, top: Color, bot: Color, border_w: f32, border: Color, elev: f32) void {
        self.gradElev(x, y, w, h, radius, top, bot, border_w, border, elev);
    }
    fn gradElev(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, top: Color, bot: Color, border_w: f32, border: Color, elev: f32) void {
        const t = lin(top);
        const bt = lin(bot);
        const b = lin(border);
        self.rects.appendSlice(&.{ x, y, w, h, radius, border_w, t[0], t[1], t[2], t[3], b[0], b[1], b[2], b[3], bt[0], bt[1], bt[2], bt[3], elev }) catch {};
    }
    /// Hairline rounded-rect border only (transparent interior) — overlays an
    /// existing fill, e.g. a selection ring or rim on top of a card.
    pub fn stroke(self: *Gpu, x: f32, y: f32, w: f32, h: f32, radius: f32, bw: f32, color: Color) void {
        const c = lin(color);
        self.rects.appendSlice(&.{ x, y, w, h, radius, bw, c[0], c[1], c[2], 0, c[0], c[1], c[2], c[3], c[0], c[1], c[2], 0, 0 }) catch {};
    }
    /// Solid triangle (pixel coords). Use for icons.
    pub fn tri(self: *Gpu, x0: f32, y0: f32, x1: f32, y1: f32, x2: f32, y2: f32, color: Color) void {
        const c = lin(color);
        self.tris.appendSlice(&.{
            x0, y0, c[0], c[1], c[2], c[3],
            x1, y1, c[0], c[1], c[2], c[3],
            x2, y2, c[0], c[1], c[2], c[3],
        }) catch {};
    }
    /// Thick line as two triangles (pixel coords).
    pub fn line(self: *Gpu, x0: f32, y0: f32, x1: f32, y1: f32, thick: f32, color: Color) void {
        const dx = x1 - x0;
        const dy = y1 - y0;
        const len = @max(@sqrt(dx * dx + dy * dy), 0.0001);
        const nx = -dy / len * thick * 0.5;
        const ny = dx / len * thick * 0.5;
        self.tri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, color);
        self.tri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, color);
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
        if (self.tris.items.len > 0) {
            glUseProgram(self.tri_prog);
            glUniform2f(self.tri_u_res, self.width, self.height);
            glBindVertexArray(self.tri_vao);
            glBindBuffer(GL_ARRAY_BUFFER, self.tri_vbo);
            glBufferData(GL_ARRAY_BUFFER, @intCast(self.tris.items.len * @sizeOf(f32)), self.tris.items.ptr, GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, @intCast(self.tris.items.len / TRI_FLOATS));
        }
        // glyphs last (on top), one instanced draw per font atlas
        var any_glyphs = false;
        for (self.glyph_batches.items) |*b| if (b.data.items.len > 0) {
            any_glyphs = true;
        };
        if (any_glyphs) {
            glUseProgram(self.glyph_prog);
            glUniform2f(self.glyph_u_res, self.width, self.height);
            glUniform1i(self.glyph_u_atlas, 0);
            glBindVertexArray(self.glyph_vao);
            glActiveTexture(GL_TEXTURE0);
            for (self.glyph_batches.items) |*b| {
                if (b.data.items.len == 0) continue;
                glBindTexture(GL_TEXTURE_2D, b.tex);
                glBindBuffer(GL_ARRAY_BUFFER, self.glyph_ivbo);
                glBufferData(GL_ARRAY_BUFFER, @intCast(b.data.items.len * @sizeOf(f32)), b.data.items.ptr, GL_DYNAMIC_DRAW);
                glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, @intCast(b.data.items.len / GLYPH_FLOATS));
            }
        }
        // clear batches so a subsequent flush() draws only newly-pushed primitives
        // (enables layered rendering, e.g. backdrop -> captureBlur -> glass overlay)
        self.rects.clearRetainingCapacity();
        self.shadows.clearRetainingCapacity();
        self.tris.clearRetainingCapacity();
        for (self.glyph_batches.items) |*b| b.data.clearRetainingCapacity();
    }

    pub fn deinit(self: *Gpu) void {
        self.rects.deinit();
        self.shadows.deinit();
        self.tris.deinit();
        for (self.glyph_batches.items) |*b| b.data.deinit();
        self.glyph_batches.deinit();
    }
};

/// A GPU glyph font: an alpha atlas baked from a CPU `font.Font` (subpixel
/// channels averaged to coverage), plus per-glyph metrics for layout.
pub const GpuFont = struct {
    tex: c_uint,
    cell_h: f32,
    first: u8,
    n: usize,
    advance: []const u8,
    width: []const u8,
    uv: [][4]f32,
    allocator: std.mem.Allocator,

    pub fn init(a: std.mem.Allocator, f: *const FontT) !GpuFont {
        const n = f.advance.len;
        var maxw: usize = 1;
        for (f.width) |w| maxw = @max(maxw, w);
        const cw = maxw + 2;
        const ch = f.cell_h + 2;
        const cols = @max(@as(usize, 1), 1024 / cw);
        const rows = (n + cols - 1) / cols;
        const aw = cols * cw;
        const ah = rows * ch;
        const atlas = try a.alloc(u8, aw * ah);
        defer a.free(atlas);
        @memset(atlas, 0);
        const uv = try a.alloc([4]f32, n);
        const awf: f32 = @floatFromInt(aw);
        const ahf: f32 = @floatFromInt(ah);
        var gi: usize = 0;
        while (gi < n) : (gi += 1) {
            const col = gi % cols;
            const row = gi / cols;
            const ox = col * cw + 1;
            const oy = row * ch + 1;
            const gw = f.width[gi];
            const off = f.offset[gi];
            var yy: usize = 0;
            while (yy < f.cell_h) : (yy += 1) {
                var xx: usize = 0;
                while (xx < gw) : (xx += 1) {
                    const cov: u32 = if (f.subpixel) blk: {
                        const base = off + yy * (gw * 3) + xx * 3;
                        break :blk (@as(u32, f.data[base]) + f.data[base + 1] + f.data[base + 2]) / 3;
                    } else f.data[off + yy * gw + xx];
                    atlas[(oy + yy) * aw + (ox + xx)] = @intCast(cov);
                }
            }
            uv[gi] = .{
                @as(f32, @floatFromInt(ox)) / awf,
                @as(f32, @floatFromInt(oy)) / ahf,
                @as(f32, @floatFromInt(gw)) / awf,
                @as(f32, @floatFromInt(f.cell_h)) / ahf,
            };
        }
        var tex: c_uint = 0;
        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, @intCast(aw), @intCast(ah), 0, GL_RED, GL_UNSIGNED_BYTE, atlas.ptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return .{ .tex = tex, .cell_h = @floatFromInt(f.cell_h), .first = f.first, .n = n, .advance = f.advance, .width = f.width, .uv = uv, .allocator = a };
    }

    pub fn textWidth(self: *const GpuFont, s: []const u8) f32 {
        var w: f32 = 0;
        for (s) |ch| {
            w += if (ch >= self.first and ch < self.first + self.n) @as(f32, @floatFromInt(self.advance[ch - self.first])) else 6;
        }
        return w;
    }
    pub fn text(self: *const GpuFont, g: *Gpu, x: f32, y: f32, s: []const u8, color: Color) void {
        var pen = x;
        const ry = @round(y); // pixel-snap the baseline so atlas sampling stays 1:1 crisp
        for (s) |ch| {
            if (ch < self.first or ch >= self.first + self.n) {
                pen += 6;
                continue;
            }
            const gi = ch - self.first;
            const gw: f32 = @floatFromInt(self.width[gi]);
            const u = self.uv[gi];
            g.pushGlyph(self.tex, @round(pen), ry, gw, self.cell_h, u[0], u[1], u[2], u[3], color);
            pen += @floatFromInt(self.advance[gi]);
        }
    }
    pub fn deinit(self: *GpuFont) void {
        self.allocator.free(self.uv);
    }
};
