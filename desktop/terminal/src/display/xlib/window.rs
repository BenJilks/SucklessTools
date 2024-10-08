extern crate x11;
use crate::buffer::rune::StandardColor;
use x11::xlib;
use std::ptr;
use std::mem;
use std::os::raw::{c_ulong, c_char};
use std::ffi::CString;
use std::cell::RefCell;
use std::rc::Rc;

#[derive(Clone)]
pub struct Window
{
    pub display: *mut xlib::Display,
    pub visual: *mut xlib::Visual,
    pub window: c_ulong,
    pub cmap: c_ulong,

    pub width: i32,
    pub height: i32,
}

impl Window
{
    pub fn open(width: i32, height: i32, title: &str) -> Rc<RefCell<Self>>
    {
        // Open display
        let display = unsafe { xlib::XOpenDisplay(ptr::null()) };
        if display == ptr::null_mut()
        {
            println!("Could not open x11 display");
            assert!(false);
        }

        // Grab default info
        let screen = unsafe { xlib::XDefaultScreen(display) };
        let root_window = unsafe { xlib::XDefaultRootWindow(display) };
    
        // Create window
        let black_pixel = unsafe { xlib::XBlackPixel(display, screen) };
        let white_pixel = unsafe { xlib::XWhitePixel(display, screen) };
        let window = unsafe { xlib::XCreateSimpleWindow(
            display, root_window, 0, 0, width as u32, height as u32, 0, 
            white_pixel, black_pixel) };
        if window == 0
        {
            println!("Unable to open x11 window");
            assert!(false);
        }

        // Select inputs
        unsafe
        {
            xlib::XSelectInput(display, window, 
                xlib::ExposureMask | xlib::KeyPressMask | 
                xlib::StructureNotifyMask | xlib::ButtonPressMask | 
                xlib::PointerMotionMask | xlib::ButtonReleaseMask);
        }
        
        // Set title and open window
        let title_c_str = CString::new(title).expect("Failed to create C string");
        unsafe
        {
            xlib::XStoreName(display, window, title_c_str.as_ptr() as *const c_char);
            xlib::XMapWindow(display, window);
        }

        // Create visual
        let mut visual_info: xlib::XVisualInfo = unsafe { mem::zeroed() };
        if unsafe { xlib::XMatchVisualInfo(display, screen, 24, xlib::TrueColor, &mut visual_info) } == 0
        {
            println!("Could not create X11 visual");
            assert!(false);
        }
        let visual = visual_info.visual;
        let cmap = unsafe { xlib::XCreateColormap(display, 
            root_window, visual, xlib::AllocNone) };

        Rc::from(RefCell::from(Self
        {
            display,
            visual,
            window,
            cmap,

            width,
            height,
        }))
    }

    pub fn clear(&mut self)
    {
        let color = (StandardColor::DefaultBackground.color() & 0xFFFFFF00) >> 8;
        unsafe 
        { 
            let screen = xlib::XDefaultScreen(self.display);
            let gc = xlib::XDefaultGC(self.display, screen);
            xlib::XSetForeground(self.display, gc, color as u64);
            xlib::XFillRectangle(self.display, self.window, gc, 
                0, 0, self.width as u32, self.height as u32);
        }
    }

    pub fn get_size(&self) -> (i32, i32)
    {
        (self.width, self.height)
    }

    pub fn close(&mut self)
    {
        unsafe
        {
            xlib::XDestroyWindow(self.display, self.window);
            xlib::XCloseDisplay(self.display);
        }
    }
}
