//! window_glx.zig — full-featured OpenGL window: borderless/custom-chrome,
//! resizable, full input, EWMH move/resize/min/max — but presents the
//! framebuffer through the GPU (GLX texture quad) instead of XPutImage. Same API
//! as window_x11.NativeWindow so the app swaps with a one-line import change.

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
const ButtonPressMask: c_long = 1 << 2;
const ButtonReleaseMask: c_long = 1 << 3;
const PointerMotionMask: c_long = 1 << 6;
const StructureNotifyMask: c_long = 1 << 17;
const SubstructureNotifyMask: c_long = 1 << 19;
const SubstructureRedirectMask: c_long = 1 << 20;
const PropModeReplace: c_int = 0;

const KeyPress: c_int = 2;
const ButtonPress: c_int = 4;
const ButtonRelease: c_int = 5;
const MotionNotify: c_int = 6;
const Expose: c_int = 12;
const ConfigureNotify: c_int = 22;
const ClientMessage: c_int = 33;

pub const RESIZE_TOPLEFT: c_long = 0;
pub const RESIZE_TOPRIGHT: c_long = 2;
pub const RESIZE_RIGHT: c_long = 3;
pub const RESIZE_BOTTOMRIGHT: c_long = 4;
pub const RESIZE_BOTTOM: c_long = 5;
pub const RESIZE_BOTTOMLEFT: c_long = 6;
pub const RESIZE_LEFT: c_long = 7;
pub const RESIZE_TOP: c_long = 1;
pub const MOVE: c_long = 8;

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
const XVisualInfo = extern struct { visual: ?*anyopaque, visualid: c_ulong, screen: c_int, depth: c_int, class: c_int, red_mask: c_ulong, green_mask: c_ulong, blue_mask: c_ulong, colormap_size: c_int, bits_per_rgb: c_int };
const XGeneric = extern struct { type: c_int, serial: c_ulong, send_event: c_int, display: ?*Display, window: Window };
const XButtonKey = extern struct { type: c_int, serial: c_ulong, send_event: c_int, display: ?*Display, window: Window, root: Window, subwindow: Window, time: c_ulong, x: c_int, y: c_int, x_root: c_int, y_root: c_int, state: c_uint, keycode_or_button: c_uint, same_screen: c_int };
const XConfigure = extern struct { type: c_int, serial: c_ulong, send_event: c_int, display: ?*Display, event: Window, window: Window, x: c_int, y: c_int, width: c_int, height: c_int, border_width: c_int, above: Window, override_redirect: c_int };
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
extern fn XFlush(d: *Display) c_int;
extern fn XInternAtom(d: *Display, name: [*:0]const u8, only: c_int) Atom;
extern fn XSetWMProtocols(d: *Display, w: Window, p: *Atom, n: c_int) c_int;
extern fn XChangeProperty(d: *Display, w: Window, prop: Atom, ty: Atom, format: c_int, mode: c_int, data: [*]const u8, n: c_int) c_int;
extern fn XSendEvent(d: *Display, w: Window, propagate: c_int, mask: c_long, ev: *anyopaque) c_int;
extern fn XUngrabPointer(d: *Display, time: c_ulong) c_int;
extern fn XIconifyWindow(d: *Display, w: Window, screen: c_int) c_int;
extern fn XResizeWindow(d: *Display, w: Window, width: c_uint, height: c_uint) c_int;

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

pub const Event = union(enum) {
    none,
    close,
    key: u32,
    mouse_move: struct { x: i32, y: i32 },
    mouse_down: struct { x: i32, y: i32, x_root: i32, y_root: i32, button: u32 },
    mouse_up: struct { x: i32, y: i32 },
    resize: struct { w: usize, h: usize },
    expose,
};

pub const WindowError = error{ NoDisplay, NoVisual, NoContext };

pub const NativeWindow = struct {
    display: *Display,
    screen: c_int,
    root: Window,
    win: Window,
    ctx: GLXContext,
    tex: c_uint,
    width: usize,
    height: usize,
    wm_delete: Atom,
    a_moveresize: Atom,
    a_state: Atom,
    a_max_v: Atom,
    a_max_h: Atom,
    allocator: std.mem.Allocator,

    pub fn open(a: std.mem.Allocator, w: usize, h: usize, title: [*:0]const u8) WindowError!NativeWindow {
        const display = XOpenDisplay(null) orelse return WindowError.NoDisplay;
        const screen = XDefaultScreen(display);
        const root = XRootWindow(display, screen);
        var attribs = [_]c_int{ GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, 0 };
        const vi = glXChooseVisual(display, screen, &attribs) orelse return WindowError.NoVisual;
        const cmap = XCreateColormap(display, root, vi.visual.?, AllocNone);
        var swa = XSetWindowAttributes{ .colormap = cmap, .border_pixel = 0, .event_mask = ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask };
        const win = XCreateWindow(display, root, 0, 0, @intCast(w), @intCast(h), 0, vi.depth, InputOutput, vi.visual.?, CWBorderPixel | CWColormap | CWEventMask, &swa);
        _ = XStoreName(display, win, title);

        const motif = XInternAtom(display, "_MOTIF_WM_HINTS", 0);
        const hints = [_]c_ulong{ 2, 0, 0, 0, 0 };
        _ = XChangeProperty(display, win, motif, motif, 32, PropModeReplace, @ptrCast(&hints), 5);

        var wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", 0);
        _ = XSetWMProtocols(display, win, &wm_delete, 1);
        _ = XMapWindow(display, win);

        const ctx = glXCreateContext(display, vi, null, 1);
        if (ctx == null) return WindowError.NoContext;
        _ = glXMakeCurrent(display, win, ctx);
        var tex: c_uint = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return .{ .display = display, .screen = screen, .root = root, .win = win, .ctx = ctx, .tex = tex, .width = w, .height = h, .wm_delete = wm_delete, .a_moveresize = XInternAtom(display, "_NET_WM_MOVERESIZE", 0), .a_state = XInternAtom(display, "_NET_WM_STATE", 0), .a_max_v = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", 0), .a_max_h = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0), .allocator = a };
    }

    pub fn resize(self: *NativeWindow, w: usize, h: usize) void {
        self.width = @max(w, 1);
        self.height = @max(h, 1);
    }

    /// Make this window's GL context current (for callers issuing their own GL).
    pub fn makeCurrent(self: *NativeWindow) void {
        _ = glXMakeCurrent(self.display, self.win, self.ctx);
    }
    /// Swap the back buffer to screen (for the GPU primitive renderer path).
    pub fn swapBuffers(self: *NativeWindow) void {
        glXSwapBuffers(self.display, self.win);
    }
    pub fn glProc(name: [*:0]const u8) ?*const anyopaque {
        return glXGetProcAddressARB(name);
    }

    /// Upload the RGBA framebuffer to the GPU and present it on a textured quad.
    pub fn present(self: *NativeWindow, rgba: []const u8) void {
        glViewport(0, 0, @intCast(self.width), @intCast(self.height));
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, self.tex);
        glTexImage2D(GL_TEXTURE_2D, 0, @intCast(GL_RGBA), @intCast(self.width), @intCast(self.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.ptr);
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

    pub fn poll(self: *NativeWindow) Event {
        if (XPending(self.display) == 0) return .none;
        var raw: [192]u8 align(8) = undefined;
        _ = XNextEvent(self.display, &raw);
        switch (@as(*const XGeneric, @ptrCast(&raw)).type) {
            Expose => return .expose,
            KeyPress => return .{ .key = @as(*const XButtonKey, @ptrCast(&raw)).keycode_or_button },
            ButtonPress => {
                const e: *const XButtonKey = @ptrCast(&raw);
                return .{ .mouse_down = .{ .x = e.x, .y = e.y, .x_root = e.x_root, .y_root = e.y_root, .button = e.keycode_or_button } };
            },
            ButtonRelease => {
                const e: *const XButtonKey = @ptrCast(&raw);
                return .{ .mouse_up = .{ .x = e.x, .y = e.y } };
            },
            MotionNotify => {
                const e: *const XButtonKey = @ptrCast(&raw);
                return .{ .mouse_move = .{ .x = e.x, .y = e.y } };
            },
            ConfigureNotify => {
                const e: *const XConfigure = @ptrCast(&raw);
                const nw: usize = @intCast(@max(e.width, 1));
                const nh: usize = @intCast(@max(e.height, 1));
                if (nw != self.width or nh != self.height) return .{ .resize = .{ .w = nw, .h = nh } };
                return .none;
            },
            ClientMessage => {
                const e: *const XClient = @ptrCast(&raw);
                if (@as(Atom, @bitCast(e.l0)) == self.wm_delete) return .close;
                return .none;
            },
            else => return .none,
        }
    }

    fn sendClient(self: *NativeWindow, msg: Atom, l0: c_long, l1: c_long, l2: c_long, l3: c_long, l4: c_long) void {
        var raw: [192]u8 align(8) = [_]u8{0} ** 192;
        const e: *XClient = @ptrCast(&raw);
        e.type = ClientMessage;
        e.send_event = 1;
        e.display = self.display;
        e.window = self.win;
        e.message_type = msg;
        e.format = 32;
        e.l0 = l0;
        e.l1 = l1;
        e.l2 = l2;
        e.l3 = l3;
        e.l4 = l4;
        _ = XSendEvent(self.display, self.root, 0, SubstructureRedirectMask | SubstructureNotifyMask, &raw);
        _ = XFlush(self.display);
    }
    pub fn startMoveResize(self: *NativeWindow, direction: c_long, x_root: i32, y_root: i32) void {
        _ = XUngrabPointer(self.display, 0);
        self.sendClient(self.a_moveresize, x_root, y_root, direction, 1, 1);
    }
    pub fn minimize(self: *NativeWindow) void {
        _ = XIconifyWindow(self.display, self.win, self.screen);
    }
    pub fn toggleMaximize(self: *NativeWindow) void {
        self.sendClient(self.a_state, 2, @intCast(self.a_max_v), @intCast(self.a_max_h), 1, 0);
    }
    pub fn resizeSelf(self: *NativeWindow, w: usize, h: usize) void {
        _ = XResizeWindow(self.display, self.win, @intCast(w), @intCast(h));
        _ = XFlush(self.display);
    }
    pub fn close(self: *NativeWindow) void {
        _ = glXMakeCurrent(self.display, 0, null);
        glXDestroyContext(self.display, self.ctx);
        _ = XCloseDisplay(self.display);
    }
};
