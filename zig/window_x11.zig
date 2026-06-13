//! window_x11.zig — a minimal native window over Xlib (the Linux impl of the
//! platform windowing interface; Win32/Cocoa/Wayland slot in later). Creates a
//! window, blits an RGBA framebuffer, and returns input events. No JUCE.

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

const ZPixmap: c_int = 2;

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
extern fn XInternAtom(d: *Display, name: [*:0]const u8, only_if_exists: c_int) Atom;
extern fn XSetWMProtocols(d: *Display, w: Window, protocols: *Atom, count: c_int) c_int;

const KeyPress: c_int = 2;
const ButtonPress: c_int = 4;
const ButtonRelease: c_int = 5;
const MotionNotify: c_int = 6;
const Expose: c_int = 12;
const ClientMessage: c_int = 33;

// Just enough of the event layout to read what we use (C ABI offsets).
const XGenericEvent = extern struct {
    type: c_int,
    serial: c_ulong,
    send_event: c_int,
    display: ?*Display,
    window: Window,
};
const XKeyOrButtonEvent = extern struct {
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
    keycode_or_button: c_uint, // keycode (KeyEvent) or button (ButtonEvent)
    same_screen: c_int,
};
const XClientMessageEvent = extern struct {
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

pub const Event = union(enum) {
    none,
    close,
    key: u32, // keycode
    mouse_move: struct { x: i32, y: i32 },
    mouse_down: struct { x: i32, y: i32, button: u32 },
    mouse_up: struct { x: i32, y: i32, button: u32 },
    expose,
};

pub const WindowError = error{ NoDisplay, CreateFailed, ImageFailed };

pub const NativeWindow = struct {
    display: *Display,
    screen: c_int,
    win: Window,
    gc: GC,
    image: *XImage,
    xbuf: []u8, // BGRX server-format pixels
    width: usize,
    height: usize,
    wm_delete: Atom,
    allocator: std.mem.Allocator,

    pub fn open(a: std.mem.Allocator, w: usize, h: usize, title: [*:0]const u8) WindowError!NativeWindow {
        const display = XOpenDisplay(null) orelse return WindowError.NoDisplay;
        const screen = XDefaultScreen(display);
        const root = XRootWindow(display, screen);
        const win = XCreateSimpleWindow(display, root, 0, 0, @intCast(w), @intCast(h), 0, 0, XBlackPixel(display, screen));
        _ = XStoreName(display, win, title);
        _ = XSelectInput(display, win, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

        var wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", 0);
        _ = XSetWMProtocols(display, win, &wm_delete, 1);
        _ = XMapWindow(display, win);

        const xbuf = a.alloc(u8, w * h * 4) catch return WindowError.CreateFailed;
        const visual = XDefaultVisual(display, screen);
        const depth = XDefaultDepth(display, screen);
        const image = XCreateImage(display, visual, @intCast(depth), ZPixmap, 0, xbuf.ptr, @intCast(w), @intCast(h), 32, 0) orelse return WindowError.ImageFailed;

        return .{
            .display = display,
            .screen = screen,
            .win = win,
            .gc = XDefaultGC(display, screen),
            .image = image,
            .xbuf = xbuf,
            .width = w,
            .height = h,
            .wm_delete = wm_delete,
            .allocator = a,
        };
    }

    /// Blit an RGBA framebuffer to the window.
    pub fn present(self: *NativeWindow, rgba: []const u8) void {
        var i: usize = 0;
        const n = self.width * self.height;
        while (i < n) : (i += 1) {
            self.xbuf[i * 4 + 0] = rgba[i * 4 + 2]; // B
            self.xbuf[i * 4 + 1] = rgba[i * 4 + 1]; // G
            self.xbuf[i * 4 + 2] = rgba[i * 4 + 0]; // R
            self.xbuf[i * 4 + 3] = 255;
        }
        _ = XPutImage(self.display, self.win, self.gc, self.image, 0, 0, 0, 0, @intCast(self.width), @intCast(self.height));
        _ = XFlush(self.display);
    }

    /// Non-blocking: returns the next pending event, or .none.
    pub fn poll(self: *NativeWindow) Event {
        if (XPending(self.display) == 0) return .none;
        var raw: [192]u8 align(8) = undefined;
        _ = XNextEvent(self.display, &raw);
        const t = @as(*const XGenericEvent, @ptrCast(&raw)).type;
        switch (t) {
            Expose => return .expose,
            KeyPress => {
                const e: *const XKeyOrButtonEvent = @ptrCast(&raw);
                return .{ .key = e.keycode_or_button };
            },
            ButtonPress => {
                const e: *const XKeyOrButtonEvent = @ptrCast(&raw);
                return .{ .mouse_down = .{ .x = e.x, .y = e.y, .button = e.keycode_or_button } };
            },
            ButtonRelease => {
                const e: *const XKeyOrButtonEvent = @ptrCast(&raw);
                return .{ .mouse_up = .{ .x = e.x, .y = e.y, .button = e.keycode_or_button } };
            },
            MotionNotify => {
                const e: *const XKeyOrButtonEvent = @ptrCast(&raw);
                return .{ .mouse_move = .{ .x = e.x, .y = e.y } };
            },
            ClientMessage => {
                const e: *const XClientMessageEvent = @ptrCast(&raw);
                if (@as(Atom, @bitCast(e.l0)) == self.wm_delete) return .close;
                return .none;
            },
            else => return .none,
        }
    }

    pub fn close(self: *NativeWindow) void {
        self.allocator.free(self.xbuf);
        _ = XCloseDisplay(self.display);
    }
};
