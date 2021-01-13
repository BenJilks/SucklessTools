extern crate x11;
use super::{ cursor::*, rune::*, UpdateResult };
use x11::{ xlib, xft, keysym };
use std::{ ptr, mem, collections::HashMap, ffi::CString };
use std::os::raw::
{
    c_ulong,
    c_uint,
    c_char,
};

pub struct XLibDisplay
{
    // Xlib
    display: *mut xlib::Display,
    visual: *mut xlib::Visual,
    gc: xlib::GC,
    window: c_ulong,
    back_buffer: xlib::Pixmap,
    cmap: c_ulong,
    last_foreground_color: u32,
    
    // Window metrics
    fd: i32,
    width: i32,
    height: i32,

    // Xft
    font: *mut xft::XftFont,
    colors: HashMap<u32, xft::XftColor>,
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
        self.width = 1600;
        self.height = 800;

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
                self.display, root_window, 0, 0, self.width as u32, self.height as u32, 0, 
                white_pixel, black_pixel);
            if self.window == 0
            {
                println!("Unable to open x11 window");
                assert!(false);
            }

            // Create back buffer
            self.back_buffer = xlib::XCreatePixmap(self.display, self.window, 
                self.width as u32, self.height as u32, 24);

            // Select inputs
            xlib::XSelectInput(self.display, self.window, 
                xlib::ExposureMask | xlib::KeyPressMask | xlib::StructureNotifyMask | xlib::ButtonPressMask);
            
            // Set title and open window
            let title_c_str = CString::new(title).expect("Failed to create C string");
            xlib::XStoreName(self.display, self.window, title_c_str.as_ptr() as *const u8);
            xlib::XMapWindow(self.display, self.window);

            let screen = xlib::XDefaultScreen(self.display);
            self.gc = xlib::XDefaultGC(self.display, screen);
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
            let font_name = "DejaVu Sans Mono:size=20:antialias=true";
            
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

            // Create draw
            self.draw = xft::XftDrawCreate(self.display, self.back_buffer, self.visual, self.cmap);
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

    fn font_color(&mut self, color: &u32) -> xft::XftColor
    {
        if !self.colors.contains_key(&color)
        {
            unsafe
            {
                // Create color name string
                let mut font_color: xft::XftColor = mem::zeroed();
                let color_string = format!("#{:06x}", (color >> 8) & 0xFFFFFF);

                // Allocate the color
                let color_c_str = CString::new(color_string).expect("Failed to create C string");
                if xft::XftColorAllocName(self.display, self.visual, self.cmap, 
                    color_c_str.as_ptr() as *const c_char, &mut font_color) == 0
                {
                    println!("Could not allocate Xft color");
                    assert!(false);
                }
                self.colors.insert(*color, font_color);
            }
        }

        return *self.colors.get(&color).unwrap();
    }

    pub fn new(title: &str) -> Self
    {
        let mut display = Self
        {
            display: ptr::null_mut(),
            visual: ptr::null_mut(),
            gc: ptr::null_mut(),
            window: 0,
            back_buffer: 0,
            cmap: 0,
            last_foreground_color: 0,

            fd: 0,
            width: 0,
            height: 0,

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
            if self.last_foreground_color != color 
            {
                xlib::XSetForeground(self.display, self.gc, ((color & 0xFFFFFF00) >> 8) as u64);
                self.last_foreground_color = color;
            }
            xlib::XFillRectangle(self.display, self.back_buffer, self.gc, 
                x, y, width as u32, height as u32);
        }
    }
    
    fn draw_rune_impl(&mut self, rune: &Rune, at: &CursorPos)
    {
        // Draw background
        let x = at.get_column() * self.font_width;
        let y = (at.get_row() + 1) * self.font_height;
        self.draw_rect(
            x, y - self.font_height, 
            self.font_width, self.font_height,
            rune.attribute.background);

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
            let color = self.font_color(&rune.attribute.foreground);
            xft::XftDrawGlyphSpec(self.draw, 
                &color, self.font, &mut spec, 1);
        }
    }

    fn draw_scroll_down(&mut self, amount: i32, top: i32, bottom: i32, height: i32)
    {
        unsafe
        {
            xlib::XCopyArea(self.display, self.back_buffer, self.back_buffer, self.gc,
                0, top + amount, 
                self.width as u32, (height - amount) as u32, 
                0, top);

            self.draw_rect(0, bottom - amount, self.width, amount, 
                StandardColor::DefaultBackground.color());
        }
    }

    fn draw_scroll_up(&mut self, amount: i32, top: i32, height: i32)
    {
        unsafe
        {
            xlib::XCopyArea(self.display, self.back_buffer, self.back_buffer, self.gc,
                0, top,
                self.width as u32, (height - self.font_height) as u32,
                0, top + amount);

            self.draw_rect(0, top, self.width, amount, 
                StandardColor::DefaultBackground.color());
        }
    }

    fn draw_scroll_impl(&mut self, amount: i32, top: i32, bottom: i32)
    {
        let amount_pixels = amount * self.font_height;
        let top_pixels = top * self.font_height;
        let bottom_pixels = bottom * self.font_height;
        let height_pixels = bottom_pixels - top_pixels;
        
        if amount < 0 
        {
            self.draw_scroll_down(-amount_pixels, 
                top_pixels, bottom_pixels, height_pixels);
        }
        else
        {
            self.draw_scroll_up(amount_pixels, 
                top_pixels, height_pixels);
        }
    }

    fn on_key_pressed(&self, event: &mut xlib::XEvent) -> UpdateResult
    {
        let keysym = unsafe { xlib::XkbKeycodeToKeysym(self.display, event.key.keycode as u8, 0, 0) };
        match keysym as c_uint
        {
            keysym::XK_Up => return UpdateResult::input_str("\x1b[A"),
            keysym::XK_Down => return UpdateResult::input_str("\x1b[B"),
            keysym::XK_Right => return UpdateResult::input_str("\x1b[C"),
            keysym::XK_Left => return UpdateResult::input_str("\x1b[D"),

            keysym::XK_Home => return UpdateResult::input_str("\x1b[H"),
            keysym::XK_End => return UpdateResult::input_str("\x1b[F"),
            
            keysym::XK_Page_Up => return UpdateResult::input_str("\x1b[5~"),
            keysym::XK_Page_Down => return UpdateResult::input_str("\x1b[6~"),
            keysym::XK_Tab => return UpdateResult::input_str("\t"),
            keysym::XK_Escape => return UpdateResult::input_str("\x1b"),
            _ => {}
        }

        let mut buffer: [u8; 80];
        let len: i32;
        unsafe
        {
            buffer = mem::zeroed();
            len = xlib::XLookupString(&mut event.key, 
                buffer.as_mut_ptr() as *mut u8, buffer.len() as i32, 
                ptr::null_mut(), ptr::null_mut());
        }
         
        return UpdateResult::input(&buffer[..len as usize]);
    }

    fn on_resize(&mut self, event: &mut xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        self.width = unsafe { event.configure.width };
        self.height = unsafe { event.configure.height };
        let rows = self.height / self.font_height;
        let columns = self.width / self.font_width;

        // Resize backbuffer
        unsafe
        {
            xlib::XFreePixmap(self.display, self.back_buffer);
            self.back_buffer = xlib::XCreatePixmap(self.display, self.window, 
                self.width as u32, self.height as u32, 24);

            xft::XftDrawDestroy(self.draw);
            self.draw = xft::XftDrawCreate(self.display, self.back_buffer, self.visual, self.cmap);
        }
        results.push(UpdateResult::resize(rows, columns, self.width, self.height));
    }

    fn on_button(&mut self, event: &xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        let button = unsafe { event.button.button };
        match button
        {
            xlib::Button4 => results.push(UpdateResult::scroll_viewport(1)),
            xlib::Button5 => results.push(UpdateResult::scroll_viewport(-1)),
            _ => {}
        }
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

    fn update(&mut self) -> Vec<UpdateResult>
    {
        let mut results = Vec::<UpdateResult>::new();
        unsafe
        {
            let mut event: xlib::XEvent = mem::zeroed();
            while xlib::XPending(self.display) != 0
            {
                xlib::XNextEvent(self.display, &mut event);
                match event.get_type()
                {
                    xlib::Expose => results.push(UpdateResult::redraw()),
                    xlib::KeyPress => results.push(self.on_key_pressed(&mut event)),
                    xlib::ConfigureNotify => self.on_resize(&mut event, &mut results),
                    xlib::ButtonPress => self.on_button(&event, &mut results),
                    _ => {},
                }
            }
        }

        return results;
    }

    fn draw_rune(&mut self, rune: &Rune, at: &CursorPos)
    {
        self.draw_rune_impl(rune, at);
    }
    
    fn draw_scroll(&mut self, amount: i32, top: i32, bottom: i32)
    {
        self.draw_scroll_impl(amount, top, bottom);
    }
    
    fn draw_clear(&mut self, attribute: &Attribute, 
        row: i32, column: i32, width: i32, height: i32)
    {
        self.draw_rect(
            column * self.font_width, row * self.font_height, 
            width * self.font_width, height * self.font_height, 
            attribute.background);
    }
    
    fn flush(&mut self)
    {
        unsafe 
        {
            xlib::XCopyArea(self.display, self.back_buffer, self.window, self.gc, 
                0, 0, self.width as u32, self.height as u32, 0, 0);
            xlib::XFlush(self.display);
        }
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
