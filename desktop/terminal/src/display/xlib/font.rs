extern crate x11;
use crate::display::xlib::window::Window;
use x11::{xlib, xft};
use std::ptr;
use std::mem;
use std::os::raw::c_char;
use std::collections::HashMap;
use std::ffi::CString;
use std::cell::RefCell;
use std::rc::Rc;

#[derive(Clone)]
pub struct FontMetrics
{
    pub width: i32,
    pub height: i32,
    pub descent: i32,
    pub size: i32,
}

impl FontMetrics
{
    pub fn from(font: *mut xft::XftFont, size: i32) -> Self
    {
        Self
        {
            width: unsafe { (*font).max_advance_width },
            height: unsafe { (*font).height },
            descent: unsafe { (*font).descent },
            size: size,
        }
    }
}

pub struct Font
{
    pub font: *mut xft::XftFont,
    colors: HashMap<u32, xft::XftColor>,
    window: Rc<RefCell<Window>>,
    metrics: FontMetrics,
}

impl Font
{

    fn open_font(display: *mut xlib::Display, size: i32) -> *mut xft::XftFont
    {
        let screen = unsafe { xlib::XDefaultScreen(display) };
        let font_name = format!("DejaVu Sans Mono:size={}:antialias=true", size);
        
        // Create font
        let font = unsafe { xft::XftFontOpenName(display, screen, 
            font_name.as_ptr() as *const c_char) };
        if font == ptr::null_mut()
        {
            println!("Cannot open font {}", font_name);
            assert!(false);
        }

        return font;
    }

    pub fn load(window: Rc<RefCell<Window>>, size: i32) -> Rc<RefCell<Self>>
    {
        let font = Self::open_font(window.borrow().display, size);
        Rc::from(RefCell::from(Self
        {
            font: font,
            colors: HashMap::new(),
            window: window,
            metrics: FontMetrics::from(font, size),
        }))
    }

    pub fn get_metrics(&self) -> FontMetrics
    {
        self.metrics.clone()
    }

    pub fn free(&mut self)
    {
        unsafe
        {
            xft::XftFontClose(self.window.borrow().display, self.font);
        }
    }

    pub fn resize(&mut self, size: i32)
    {
        self.free();
        self.font = Self::open_font(self.window.borrow().display, size);
        self.metrics = FontMetrics::from(self.font, size);
    }

    pub fn color(&mut self, color: &u32) -> xft::XftColor
    {
        if !self.colors.contains_key(&color)
        {
            let window = self.window.borrow();
            unsafe
            {
                // Create color name string
                let mut font_color: xft::XftColor = mem::zeroed();
                let color_string = format!("#{:06x}", (color >> 8) & 0xFFFFFF);

                // Allocate the color
                let color_c_str = CString::new(color_string).expect("Failed to create C string");
                if xft::XftColorAllocName(window.display, window.visual, window.cmap, 
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

    pub fn get_glyph(&self, x: i32, y: i32, code_point: u32) -> xft::XftGlyphFontSpec
    {
        let display = self.window.borrow().display;
        let glyph = unsafe { xft::XftCharIndex(display, self.font, code_point) };
        xft::XftGlyphFontSpec 
        { 
            glyph: glyph,
            x: x as i16,
            y: (y - self.metrics.descent) as i16,
            font: self.font,
        }
    }

}
