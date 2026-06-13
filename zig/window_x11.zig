//! window_x11.zig — native X11 window with NO OS decorations (borderless, via
//! Motif hints) so the app draws its own title bar, plus robust live resizing
//! (recreate framebuffer on ConfigureNotify) and EWMH move/resize/min/max that
//! works across window managers. The Linux windowing backend.

const std = @import("std");

const Display = opaque {};
const Visual = opaque {};
const XImage = opaque {};
const GC = *opaque {};
const XID = c_ulong;
const Window = XID;
const Atom = c_ulong;

const ExposureMask: c_long = 1 << 15;
const KeyPressMask: c_long = 1 << 0;
const ButtonPressMask: c_long = 1 << 2;
const ButtonReleaseMask: c_long = 1 << 3;
const PointerMotionMask: c_long = 1 << 6;
const StructureNotifyMask: c_long = 1 << 17;
const SubstructureNotifyMask: c_long = 1 << 19;
const SubstructureRedirectMask: c_long = 1 << 20;

const ZPixmap: c_int = 2;
const PropModeReplace: c_int = 0;
const XA_ATOM: Atom = 4;

const KeyPress: c_int = 2;
const ButtonPress: c_int = 4;
const ButtonRelease: c_int = 5;
const MotionNotify: c_int = 6;
const Expose: c_int = 12;
const ConfigureNotify: c_int = 22;
const ClientMessage: c_int = 33;

// _NET_WM_MOVERESIZE directions
pub const RESIZE_TOPLEFT: c_long = 0;
pub const RESIZE_TOP: c_long = 1;
pub const RESIZE_TOPRIGHT: c_long = 2;
pub const RESIZE_RIGHT: c_long = 3;
pub const RESIZE_BOTTOMRIGHT: c_long = 4;
pub const RESIZE_BOTTOM: c_long = 5;
pub const RESIZE_BOTTOMLEFT: c_long = 6;
pub const RESIZE_LEFT: c_long = 7;
pub const MOVE: c_long = 8;

const XGeneric = extern struct { type: c_int, serial: c_ulong, send_event: c_int, display: ?*Display, window: Window };
const XButtonKey = extern struct {
    type: c_int,
    serial: c_ulong,
    send_event: c_int,
    display: ?*Display,
    window: Window,
    root: Window,
    subwindow: Window,
    time: c_ulong,
    x: c_int,
    y: c_int,
    x_root: c_int,
    y_root: c_int,
    state: c_uint,
    keycode_or_button: c_uint,
    same_screen: c_int,
};
const XConfigure = extern struct {
    type: c_int,
    serial: c_ulong,
    send_event: c_int,
    display: ?*Display,
    event: Window,
    window: Window,
    x: c_int,
    y: c_int,
    width: c_int,
    height: c_int,
    border_width: c_int,
    above: Window,
    override_redirect: c_int,
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
extern fn XBlackPixel(d: *Display, s: c_int) c_ulong;
extern fn XCreateSimpleWindow(d: *Display, parent: Window, x: c_int, y: c_int, w: c_uint, h: c_uint, bw: c_uint, border: c_ulong, bg: c_ulong) Window;
extern fn XStoreName(d: *Display, w: Window, name: [*:0]const u8) c_int;
extern fn XSelectInput(d: *Display, w: Window, mask: c_long) c_int;
extern fn XMapWindow(d: *Display, w: Window) c_int;
extern fn XDefaultGC(d: *Display, s: c_int) GC;
extern fn XDefaultVisual(d: *Display, s: c_int) *Visual;
extern fn XDefaultDepth(d: *Display, s: c_int) c_int;
extern fn XCreateImage(d: *Display, v: *Visual, depth: c_uint, format: c_int, offset: c_int, data: [*]u8, w: c_uint, h: c_uint, pad: c_int, bpl: c_int) ?*XImage;
extern fn XPutImage(d: *Display, w: Window, gc: GC, img: *XImage, sx: c_int, sy: c_int, dx: c_int, dy: c_int, w2: c_uint, h2: c_uint) c_int;
extern fn XFlush(d: *Display) c_int;
extern fn XPending(d: *Display) c_int;
extern fn XNextEvent(d: *Display, ev: *anyopaque) c_int;
extern fn XInternAtom(d: *Display, name: [*:0]const u8, only: c_int) Atom;
extern fn XSetWMProtocols(d: *Display, w: Window, protocols: *Atom, count: c_int) c_int;
extern fn XChangeProperty(d: *Display, w: Window, prop: Atom, ty: Atom, format: c_int, mode: c_int, data: [*]const u8, n: c_int) c_int;
extern fn XSendEvent(d: *Display, w: Window, propagate: c_int, mask: c_long, ev: *anyopaque) c_int;
extern fn XUngrabPointer(d: *Display, time: c_ulong) c_int;
extern fn XIconifyWindow(d: *Display, w: Window, screen: c_int) c_int;
extern fn XResizeWindow(d: *Display, w: Window, width: c_uint, height: c_uint) c_int;
extern fn XFree(ptr: ?*anyopaque) c_int;

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

pub const WindowError = error{ NoDisplay, CreateFailed, ImageFailed };

pub const NativeWindow = struct {
    display: *Display,
    screen: c_int,
    root: Window,
    win: Window,
    gc: GC,
    image: *XImage,
    xbuf: []u8,
    width: usize,
    height: usize,
    wm_delete: Atom,
    a_moveresize: Atom,
    a_state: Atom,
    a_max_v: Atom,
    a_max_h: Atom,
    allocator: std.mem.Allocator,

    fn makeImage(self: *NativeWindow) WindowError!void {
        const visual = XDefaultVisual(self.display, self.screen);
        const depth = XDefaultDepth(self.display, self.screen);
        self.image = XCreateImage(self.display, visual, @intCast(depth), ZPixmap, 0, self.xbuf.ptr, @intCast(self.width), @intCast(self.height), 32, 0) orelse return WindowError.ImageFailed;
    }

    pub fn open(a: std.mem.Allocator, w: usize, h: usize, title: [*:0]const u8) WindowError!NativeWindow {
        const display = XOpenDisplay(null) orelse return WindowError.NoDisplay;
        const screen = XDefaultScreen(display);
        const root = XRootWindow(display, screen);
        const win = XCreateSimpleWindow(display, root, 0, 0, @intCast(w), @intCast(h), 0, 0, XBlackPixel(display, screen));
        _ = XStoreName(display, win, title);
        _ = XSelectInput(display, win, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

        // Borderless: remove WM decorations via Motif hints.
        const motif = XInternAtom(display, "_MOTIF_WM_HINTS", 0);
        const hints = [_]c_ulong{ 2, 0, 0, 0, 0 }; // flags=MWM_HINTS_DECORATIONS, decorations=0
        _ = XChangeProperty(display, win, motif, motif, 32, PropModeReplace, @ptrCast(&hints), 5);

        var wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", 0);
        _ = XSetWMProtocols(display, win, &wm_delete, 1);
        _ = XMapWindow(display, win);

        const xbuf = a.alloc(u8, w * h * 4) catch return WindowError.CreateFailed;

        var self = NativeWindow{
            .display = display,
            .screen = screen,
            .root = root,
            .win = win,
            .gc = XDefaultGC(display, screen),
            .image = undefined,
            .xbuf = xbuf,
            .width = w,
            .height = h,
            .wm_delete = wm_delete,
            .a_moveresize = XInternAtom(display, "_NET_WM_MOVERESIZE", 0),
            .a_state = XInternAtom(display, "_NET_WM_STATE", 0),
            .a_max_v = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", 0),
            .a_max_h = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0),
            .allocator = a,
        };
        try self.makeImage();
        return self;
    }

    /// Recreate the framebuffer + XImage at a new size (call on .resize).
    pub fn resize(self: *NativeWindow, w: usize, h: usize) void {
        const nw = @max(w, 1);
        const nh = @max(h, 1);
        if (nw == self.width and nh == self.height) return;
        _ = XFree(@ptrCast(self.image)); // free the XImage struct only (data is ours)
        self.allocator.free(self.xbuf);
        self.width = nw;
        self.height = nh;
        self.xbuf = self.allocator.alloc(u8, nw * nh * 4) catch {
            self.xbuf = self.allocator.alloc(u8, 4) catch unreachable;
            self.width = 1;
            self.height = 1;
            self.makeImage() catch {};
            return;
        };
        self.makeImage() catch {};
    }

    pub fn present(self: *NativeWindow, rgba: []const u8) void {
        const n = @min(self.width * self.height, rgba.len / 4);
        var i: usize = 0;
        while (i < n) : (i += 1) {
            self.xbuf[i * 4 + 0] = rgba[i * 4 + 2];
            self.xbuf[i * 4 + 1] = rgba[i * 4 + 1];
            self.xbuf[i * 4 + 2] = rgba[i * 4 + 0];
            self.xbuf[i * 4 + 3] = 255;
        }
        _ = XPutImage(self.display, self.win, self.gc, self.image, 0, 0, 0, 0, @intCast(self.width), @intCast(self.height));
        _ = XFlush(self.display);
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

    /// Start an interactive move or resize handled by the WM (robust across WMs).
    pub fn startMoveResize(self: *NativeWindow, direction: c_long, x_root: i32, y_root: i32) void {
        _ = XUngrabPointer(self.display, 0);
        self.sendClient(self.a_moveresize, x_root, y_root, direction, 1, 1);
    }
    pub fn minimize(self: *NativeWindow) void {
        _ = XIconifyWindow(self.display, self.win, self.screen);
    }
    pub fn toggleMaximize(self: *NativeWindow) void {
        self.sendClient(self.a_state, 2, @intCast(self.a_max_v), @intCast(self.a_max_h), 1, 0); // 2 = toggle
    }
    /// For testing: ask the WM to resize us (triggers ConfigureNotify).
    pub fn resizeSelf(self: *NativeWindow, w: usize, h: usize) void {
        _ = XResizeWindow(self.display, self.win, @intCast(w), @intCast(h));
        _ = XFlush(self.display);
    }

    pub fn close(self: *NativeWindow) void {
        _ = XFree(@ptrCast(self.image));
        self.allocator.free(self.xbuf);
        _ = XCloseDisplay(self.display);
    }
};
