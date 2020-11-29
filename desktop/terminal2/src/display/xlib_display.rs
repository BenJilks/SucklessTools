extern crate x11;
use std::{ptr, mem, collections::HashMap, ffi::CString};
use x11::{xlib, xft, keysym};
use super::{buffer::*, cursor::*, rune, UpdateResult};
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
    colors: HashMap<rune::Color, xft::XftColor>,
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
            // Open display
            self.display = xlib::XOpenDisplay(ptr::null());
            if self.display == ptr::null_mut()
            {
                println!("Could not open x11 display");
                assert!(false);
            }

            // Grab default info
            let screen = xlib::XDefaultScreen(self.display);
            let root_window = xlib::XDefaultRootWindow(self.display);
        
            // Create window
            let black_pixel = xlib::XBlackPixel(self.display, screen);
            let white_pixel = xlib::XWhitePixel(self.display, screen);
            self.window = xlib::XCreateSimpleWindow(
                self.display, root_window, 0, 0, 1600, 800, 0, 
                white_pixel, black_pixel);
            if self.window == 0
            {
                println!("Unable to open x11 window");
                assert!(false);
            }
            
            // Select inputs
            xlib::XSelectInput(self.display, self.window, 
                xlib::ExposureMask | xlib::KeyPressMask | xlib::StructureNotifyMask);
            
            // Set title and open window
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
            let mut allocate_color = |color_name|
            {
                // Create color name string
                let mut color: xft::XftColor = mem::zeroed();
                let color_string = format!("#{}", 
                    rune::string_from_color(&color_name)
                        .chars()
                        .into_iter()
                        .take(6)
                        .collect::<String>());

                // Allocate the color
                let color_c_str = CString::new(color_string).expect("Failed to create C string");
                if xft::XftColorAllocName(self.display, self.visual, self.cmap, 
                    color_c_str.as_ptr() as *const c_char, &mut color) == 0
                {
                    println!("Could not allocate Xft color");
                    assert!(false);
                }
                self.colors.insert(color_name, color);
            };
            allocate_color(rune::Color::Black);
            allocate_color(rune::Color::Red);
            allocate_color(rune::Color::Green);
            allocate_color(rune::Color::Yellow);
            allocate_color(rune::Color::Blue);
            allocate_color(rune::Color::Magenta);
            allocate_color(rune::Color::Cyan);
            allocate_color(rune::Color::White);
            allocate_color(rune::Color::DefaultBackground);
            allocate_color(rune::Color::DefaultForeground);

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
        let mut display = Self
        {
            display: ptr::null_mut(),
            window: 0,
            visual: ptr::null_mut(),
            cmap: 0,
            fd: 0,

            font: ptr::null_mut(),
            colors: HashMap::new(),
            draw: ptr::null_mut(),

            font_width: 0,
            font_height: 0,
            font_descent: 0,
        };

        display.create_window(title);
        display.load_font();
        return display;
    }

    fn draw_rect(&mut self, x: i32, y: i32, width: i32, height: i32, color: u32)
    {
        unsafe
        {
            let screen = xlib::XDefaultScreen(self.display);
            let gc = xlib::XDefaultGC(self.display, screen);
            xlib::XSetForeground(self.display, gc, ((color & 0xFFFFFF00) >> 8) as u64);
            xlib::XFillRectangle(self.display, self.window, gc, 
                x, y, width as u32, height as u32);
        }
    }
    
    fn draw_rune_internal(&mut self, buffer: &Buffer, at: &CursorPos)
    {
        // Fetch rune at position
        let rune_or_none = buffer.rune_at(&at);
        if rune_or_none.is_none() {
            return;
        }

        // Draw background
        let rune = rune_or_none.unwrap();
        let x = at.get_column() * self.font_width;
        let y = (at.get_row() + 1) * self.font_height;
        self.draw_rect(
            x, y - self.font_height, 
            self.font_width, self.font_height,
            rune::int_from_color(&rune.attribute.background));

        // If it's a 0, don't draw it
        if rune.code_point == 0 {
            return;
        }

        unsafe
        {
            // Fetch char data
            let glyph = xft::XftCharIndex(self.display, self.font, rune.code_point);
            let mut spec = xft::XftGlyphSpec 
            { 
                glyph: glyph, 
                x: x as i16, 
                y: (y - self.font_descent) as i16, 
            };

            // Draw to screen
            let color = self.colors.get(&rune.attribute.foreground).unwrap();
            xft::XftDrawGlyphSpec(self.draw, 
                color, self.font, &mut spec, 1);
        }
    }

    fn paint(&mut self, buffer: &Buffer)
    {
        for row in 0..buffer.get_rows()
        {
            for column in 0..buffer.get_columns()
            {
                let pos = CursorPos::new(row, column);
                self.draw_rune_internal(buffer, &pos);
            }
        }
    }

    fn on_key_pressed(&self, event: &mut xlib::XEvent) -> Option<UpdateResult>
    {
        let keysym = unsafe { xlib::XkbKeycodeToKeysym(self.display, event.key.keycode as u8, 0, 0) };
        match keysym as c_uint
        {
            keysym::XK_Up => return UpdateResult::input("\x1b[A"),
            keysym::XK_Down => return UpdateResult::input("\x1b[B"),
            keysym::XK_Right => return UpdateResult::input("\x1b[C"),
            keysym::XK_Left => return UpdateResult::input("\x1b[D"),

            keysym::XK_Home => return UpdateResult::input("\x1b[H"),
            keysym::XK_End => return UpdateResult::input("\x1b[F"),
            
            keysym::XK_Page_Up => return UpdateResult::input("\x1b[5~"),
            keysym::XK_Page_Down => return UpdateResult::input("\x1b[6~"),
            _ => {}
        }

        let mut buffer: [u8; 80];
        unsafe
        {
            buffer = mem::zeroed();
            xlib::XLookupString(&mut event.key, 
                buffer.as_mut_ptr() as *mut i8, buffer.len() as i32, 
                ptr::null_mut(), ptr::null_mut());
        }
         
        let str_or_error = std::str::from_utf8(&buffer);
        if str_or_error.is_err() {
            return None;
        }

        return UpdateResult::input(str_or_error.unwrap());
    }

    fn on_resize(&mut self, event: &mut xlib::XEvent) -> Option<UpdateResult>
    {
        let width: i32;
        let height: i32;
        unsafe
        {
            width = event.configure.width;
            height = event.configure.height;
        }
        
        let rows = height / self.font_height;
        let columns = width / self.font_width;
        return UpdateResult::resize(rows, columns);
    }

}

impl Drop for XLibDisplay
{
    
    fn drop(&mut self)
    {
        unsafe
        {
            for color in &mut self.colors
            {
                xft::XftColorFree(self.display, 
                    self.visual, self.cmap, color.1);
            }
            xft::XftDrawDestroy(self.draw);

            xlib::XDestroyWindow(self.display, self.window);
            xlib::XCloseDisplay(self.display);
        }
    }

}

impl super::Display for XLibDisplay
{

    fn update(&mut self, buffer: &Buffer) -> Option<UpdateResult>
    {
        unsafe
        {
            let mut event: xlib::XEvent = mem::zeroed();
            xlib::XNextEvent(self.display, &mut event);
            match event.get_type()
            {
                xlib::Expose => self.paint(buffer),
                xlib::KeyPress => return self.on_key_pressed(&mut event),
                xlib::ConfigureNotify => return self.on_resize(&mut event),
                _ => {},
            }
        }

        return None;
    }

    fn redraw(&mut self, buffer: &Buffer)
    {
        self.paint(buffer);
        self.flush();
    }

    fn flush(&mut self)
    {
        unsafe
        {
            xlib::XFlush(self.display);
        }
    }
    
    fn draw_rune(&mut self, buffer: &Buffer, at: &CursorPos)
    {
        self.draw_rune_internal(buffer, at);
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

