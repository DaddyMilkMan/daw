//! window_gl.zig — native window with an OpenGL (GLX) context. Presents the
//! software-rendered framebuffer as a GPU texture on a fullscreen quad. This is
//! the "OpenGL backend": the present path now runs on the GPU (vsync, and the
//! foundation for shader effects like a real-time blur). Hand-declared GLX + GL.

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

// GLX attribs
const GLX_RGBA: c_int = 4;
const GLX_DOUBLEBUFFER: c_int = 5;
const GLX_RED_SIZE: c_int = 8;
const GLX_GREEN_SIZE: c_int = 9;
const GLX_BLUE_SIZE: c_int = 10;
const GLX_DEPTH_SIZE: c_int = 12;

// GL constants
const GL_COLOR_BUFFER_BIT: c_uint = 0x4000;
const GL_TEXTURE_2D: c_uint = 0x0DE1;
const GL_RGBA: c_uint = 0x1908;
const GL_UNSIGNED_BYTE: c_uint = 0x1401;
const GL_TEXTURE_MIN_FILTER: c_uint = 0x2801;
const GL_TEXTURE_MAG_FILTER: c_uint = 0x2800;
const GL_LINEAR: c_int = 0x2601;
const GL_QUADS: c_uint = 0x0007;

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
const XGeneric = extern struct {
    type: c_int,
    serial: c_ulong,
    send_event: c_int,
    display: ?*Display,
    window: Window,
};
const XClient = extern struct {
    type: c_int,
    serial: c_ulong,
    send_event: c_int,
    display: ?*Display,
    window: Window,
    message_type: Atom,
    format: c_int,
    l0: c_long,
    l1: c_long,
    l2: c_long,
    l3: c_long,
    l4: c_long,
};

extern fn XOpenDisplay(name: ?[*:0]const u8) ?*Display;
extern fn XCloseDisplay(d: *Display) c_int;
extern fn XDefaultScreen(d: *Display) c_int;
extern fn XRootWindow(d: *Display, s: c_int) Window;
extern fn XCreateColormap(d: *Display, w: Window, v: *anyopaque, alloc: c_int) Colormap;
extern fn XCreateWindow(d: *Display, parent: Window, x: c_int, y: c_int, w: c_uint, h: c_uint, bw: c_uint, depth: c_int, class: c_uint, visual: *anyopaque, valuemask: c_ulong, attrs: *XSetWindowAttributes) Window;
extern fn XStoreName(d: *Display, w: Window, name: [*:0]const u8) c_int;
extern fn XMapWindow(d: *Display, w: Window) c_int;
extern fn XFlush(d: *Display) c_int;
extern fn XPending(d: *Display) c_int;
extern fn XNextEvent(d: *Display, ev: *anyopaque) c_int;
extern fn XInternAtom(d: *Display, name: [*:0]const u8, only: c_int) Atom;
extern fn XSetWMProtocols(d: *Display, w: Window, p: *Atom, n: c_int) c_int;

extern fn glXChooseVisual(d: *Display, screen: c_int, attribs: [*]c_int) ?*XVisualInfo;
extern fn glXCreateContext(d: *Display, vis: *XVisualInfo, share: GLXContext, direct: c_int) GLXContext;
extern fn glXMakeCurrent(d: *Display, drawable: Window, ctx: GLXContext) c_int;
extern fn glXSwapBuffers(d: *Display, drawable: Window) void;
extern fn glXDestroyContext(d: *Display, ctx: GLXContext) void;

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
extern fn glColor3f(r: f32, g: f32, b: f32) void;
extern fn glTexCoord2f(s: f32, t: f32) void;
extern fn glVertex2f(x: f32, y: f32) void;

pub const GlError = error{ NoDisplay, NoVisual, NoContext };

pub const GlWindow = struct {
    display: *Display,
    win: Window,
    ctx: GLXContext,
    tex: c_uint,
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

        var tex: c_uint = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return .{ .display = display, .win = win, .ctx = ctx, .tex = tex, .width = @intCast(w), .height = @intCast(h), .wm_delete = wm_delete };
    }

    /// Upload the RGBA framebuffer to the GPU and present it on a textured quad.
    pub fn present(self: *GlWindow, rgba: []const u8) void {
        glViewport(0, 0, self.width, self.height);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, self.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, @intCast(GL_RGBA), self.width, self.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.ptr);
        glColor3f(1, 1, 1);
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

    /// Drain events; returns true if the window should close.
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
