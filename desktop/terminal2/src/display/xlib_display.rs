extern crate x11;
use std::{ptr, mem, ffi::{CString, CStr}};
use x11::{xlib, xft, keysym};
use super::{output::*, cursor::*};
use std::os::raw::
{
    c_ulong,
    c_uint,
    c_char,
};

pub struct XLibDisplay
{
    // xlib
    display: *mut xlib::Display,
    window: c_ulong,
    visual: *mut xlib::Visual,
    cmap: c_ulong,
    fd: i32,

    // xft
    font: *mut xft::XftFont,
    color: xft::XftColor,
    draw: *mut xft::XftDraw,

    // Terminal
    font_width: i32,
    font_height: i32,
    font_descent: i32,
}

impl XLibDisplay
{
   
    fn create_window(&mut self, title: &str)
    {
        unsafe
        {
            self.display = xlib::XOpenDisplay(ptr::null());
            if self.display == ptr::null_mut()
            {
                println!("Could not open x11 display");
                assert!(false);
            }

            let screen = xlib::XDefaultScreen(self.display);
            let root_window = xlib::XDefaultRootWindow(self.display);
        
            let black_pixel = xlib::XBlackPixel(self.display, screen);
            let white_pixel = xlib::XWhitePixel(self.display, screen);
            self.window = xlib::XCreateSimpleWindow(
                self.display, root_window, 0, 0, 100, 100, 0, 
                black_pixel, white_pixel);
            if self.window == 0
            {
                println!("Unable to open x11 window");
                assert!(false);
            }
            
            xlib::XSelectInput(self.display, self.window, 
                xlib::ExposureMask | xlib::KeyReleaseMask);
            
            let title_c_str = CString::new(title).expect("Failed to create C string");
            xlib::XStoreName(self.display, self.window, title_c_str.as_ptr() as *const i8);
            xlib::XMapWindow(self.display, self.window);

            self.fd = xlib::XConnectionNumber(self.display);
        }
    }

    fn load_font(&mut self)
    {
        assert!(self.display != ptr::null_mut());

        unsafe
        {
            let screen = xlib::XDefaultScreen(self.display);
            let root_window = xlib::XDefaultRootWindow(self.display);
            let font_name = "DejaVu Sans Mono:size=16:antialias=true";
            
            // Create font
            self.font = xft::XftFontOpenName(self.display, screen, 
                font_name.as_ptr() as *const c_char);
            if self.font == ptr::null_mut()
            {
                println!("Cannot open font {}", font_name);
                assert!(false);
            }

            // Create visual
            let mut visual_info: xlib::XVisualInfo = mem::zeroed();
            if xlib::XMatchVisualInfo(self.display, screen, 24, xlib::TrueColor, &mut visual_info) == 0
            {
                println!("Could not create X11 visual");
                assert!(false);
            }
            self.visual = visual_info.visual;
            self.cmap = xlib::XCreateColormap(self.display, 
                root_window, self.visual, xlib::AllocNone);

            // Allocate font color
            self.color = mem::zeroed();
            let color = "#FFFFFF";
            let color_c_str = CString::new(color).expect("Failed to create C string");
            if xft::XftColorAllocName(self.display, self.visual, self.cmap, 
                color_c_str.as_ptr() as *const c_char, &mut self.color) == 0
            {
                println!("Could not allocate Xft color");
                assert!(false);
            }

            // Create draw
            self.draw = xft::XftDrawCreate(self.display, self.window, self.visual, self.cmap);
            if self.draw == ptr::null_mut()
            {
                println!("Unable to create Xft draw");
                assert!(false);
            }

            self.font_width = (*self.font).max_advance_width;
            self.font_height = (*self.font).height;
            self.font_descent = (*self.font).descent;
        }
    }

    pub fn new(title: &str) -> Self
    {
        unsafe
        {
            let mut display = Self
            {
                display: ptr::null_mut(),
                window: 0,
                visual: ptr::null_mut(),
                cmap: 0,
                fd: 0,

                font: ptr::null_mut(),
                color: mem::zeroed(),
                draw: ptr::null_mut(),

                font_width: 0,
                font_height: 0,
                font_descent: 0,
            };

            display.create_window(title);
            display.load_font();
            return display;
        }
    }

    fn draw_rect(&mut self, x: i32, y: i32, width: i32, height: i32)
    {
        unsafe
        {
            let screen = xlib::XDefaultScreen(self.display);
            let gc = xlib::XDefaultGC(self.display, screen);
            xlib::XFillRectangle(self.display, self.window, gc, 
                x, y, width as u32, height as u32);
        }
    }
    
    fn draw_rune(&mut self, buffer: &Buffer, at: &CursorPos)
    {
        let x = at.get_column() * self.font_width;
        let y = (at.get_row() + 1) * self.font_height;
        self.draw_rect(x, y - self.font_height, self.font_width, self.font_height);

        let rune_or_none = buffer.rune_at(&at);
        if rune_or_none.is_none() {
            return;
        }

        let rune = rune_or_none.unwrap();
        if rune == 0 {
            return;
        }

        unsafe
        {
            let glyph = xft::XftCharIndex(self.display, self.font, rune);
            let mut spec = xft::XftGlyphSpec 
            { 
                glyph: glyph, 
                x: x as i16, 
                y: (y - self.font_descent) as i16, 
            };

            xft::XftDrawGlyphSpec(self.draw, &mut self.color, self.font, &mut spec, 1);
        }
    }

    fn paint(&mut self, buffer: &Buffer)
    {
        for row in 0..buffer.get_height()
        {
            for column in 0..buffer.get_width()
            {
                let pos = CursorPos::new(row, column);
                self.draw_rune(buffer, &pos);
            }
        }
    }

    fn on_key_pressed(&self, event: &mut xlib::XEvent) -> Option<String>
    {
        let key_str: String;
        unsafe
        {
            let keycode = event.key.keycode;
            let keysym = xlib::XkbKeycodeToKeysym(self.display, keycode as u8, 0, 0);
            match keysym as c_uint
            {
                keysym::XK_space => key_str = " ".to_owned(),
                keysym::XK_Return => key_str = "\n".to_owned(),
                keysym::XK_BackSpace => key_str = "\x08".to_owned(),
                _ => 
                {
                    let c_str = CStr::from_ptr(xlib::XKeysymToString(keysym));
                    key_str = c_str.to_str().expect("Unable to read c string").to_owned();
                },
            }

        }

        return Some( key_str );
    }

}

impl Drop for XLibDisplay
{
    
    fn drop(&mut self)
    {
        unsafe
        {
            xft::XftColorFree(self.display, 
                self.visual, self.cmap, &mut self.color);
            xft::XftDrawDestroy(self.draw);

            xlib::XDestroyWindow(self.display, self.window);
            xlib::XCloseDisplay(self.display);
        }
    }

}

impl super::Display for XLibDisplay
{

    fn update(&mut self, buffer: &Buffer) -> Option<String>
    {
        unsafe
        {
            let mut event: xlib::XEvent = mem::zeroed();
            xlib::XNextEvent(self.display, &mut event);
            match event.get_type()
            {
                xlib::Expose => self.paint(buffer),
                xlib::KeyRelease => return self.on_key_pressed(&mut event),
                _ => {},
            }
        }

        return None;
    }

    fn redraw(&mut self, buffer: &Buffer)
    {
        self.paint(buffer);
    }

    fn should_close(&self) -> bool
    {
        return false;
    }

    fn get_fd(&self) -> i32
    {
        return self.fd;
    }

}

