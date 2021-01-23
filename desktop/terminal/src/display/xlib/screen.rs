extern crate x11;
use crate::display::rune::{Rune, StandardColor};
use crate::display::cursor::CursorPos;
use crate::display::xlib::window::Window;
use crate::display::xlib::font::Font;
use x11::{xlib, xft};
use std::ptr;
use std::os::raw::c_ulong;
use std::cell::RefCell;
use std::rc::Rc;
use std::collections::HashMap;

pub struct Screen
{
    window: Rc<RefCell<Window>>,
    font: Rc<RefCell<Font>>,
    gc: xlib::GC,
    last_foreground_color: u32,

    bitmap: xlib::Pixmap,
    draw: *mut xft::XftDraw,
}

impl Screen
{
    pub fn new(window_ref: Rc<RefCell<Window>>, font: Rc<RefCell<Font>>) -> Self
    {
        let bitmap: c_ulong;
        let gc: xlib::GC;
        let draw: *mut xft::XftDraw;
        {
            let window = window_ref.borrow();

            bitmap = unsafe { xlib::XCreatePixmap(window.display, window.window, 
                window.width as u32, window.height as u32, 24) };
            
            draw = unsafe { xft::XftDrawCreate(window.display, bitmap, 
                window.visual, window.cmap) };
            if draw == ptr::null_mut()
            {
                println!("Unable to create Xft draw");
                panic!(); // TODO: Don't just crash here
            }

            let screen = unsafe { xlib::XDefaultScreen(window.display) };
            gc = unsafe { xlib::XDefaultGC(window.display, screen) };
        }

        Self
        {
            window: window_ref,
            font: font,
            gc: gc,
            last_foreground_color: 0,

            bitmap: bitmap,
            draw: draw,
        }
    }

    pub fn resize(&mut self, width: i32, height: i32)
    {
        self.free();

        let window = self.window.borrow();
        self.bitmap = unsafe { xlib::XCreatePixmap(window.display, window.window, 
            width as u32, height as u32, 24) };
        self.draw = unsafe { xft::XftDrawCreate(window.display, self.bitmap, 
            window.visual, window.cmap) };

        // TODO: Better error handling
        assert!(self.bitmap != 0);
        assert!(self.draw != ptr::null_mut());
    }

    pub fn free(&mut self)
    {
        unsafe
        {
            xlib::XFreePixmap(self.window.borrow().display, self.bitmap);
            xft::XftDrawDestroy(self.draw);
        }
    }

    /* Drawing functions */

    pub fn flush(&mut self)
    {
        let window = self.window.borrow();
        unsafe
        {
            xlib::XCopyArea(window.display, self.bitmap, window.window, self.gc, 
                0, 0, window.width as u32, window.height as u32, 0, 0);
        }
    }

    pub fn draw_rect(&mut self, x: i32, y: i32, width: i32, height: i32, color: u32)
    {
        let window = self.window.borrow();

        unsafe
        {
            if self.last_foreground_color != color 
            {
                xlib::XSetForeground(window.display, self.gc, ((color & 0xFFFFFF00) >> 8) as u64);
                self.last_foreground_color = color;
            }
            xlib::XFillRectangle(window.display, self.bitmap, self.gc, 
                x, y, width as u32, height as u32);
        }
    }

    fn draw_scroll_down(&mut self, amount: i32, top: i32, bottom: i32, height: i32)
    {
        let (window_width, _) = self.window.borrow().get_size();
        unsafe
        {
            xlib::XCopyArea(self.window.borrow().display, self.bitmap, self.bitmap, self.gc,
                0, top + amount,
                window_width as u32, (height - amount) as u32,
                0, top);
        }

        self.draw_rect(0, bottom - amount, window_width, amount, 
            StandardColor::DefaultBackground.color());
    }

    fn draw_scroll_up(&mut self, amount: i32, top: i32, height: i32)
    {
        let font = self.font.borrow().get_metrics();
        let (window_width, _) = self.window.borrow().get_size();
        unsafe
        {
            xlib::XCopyArea(self.window.borrow().display, self.bitmap, self.bitmap, self.gc,
                0, top,
                window_width as u32, (height - font.height) as u32,
                0, top + amount);
        }
    
        self.draw_rect(0, top, window_width, amount, 
            StandardColor::DefaultBackground.color());
    }

    pub fn draw_scroll(&mut self, amount: i32, top: i32, bottom: i32)
    {
        let font = self.font.borrow().get_metrics();
        let amount_pixels = amount * font.height;
        let top_pixels = top * font.height;
        let bottom_pixels = bottom * font.height;
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

    pub fn draw_runes(&mut self, runes: &[(Rune, CursorPos)])
    {
        let font = self.font.borrow().get_metrics();
        let font_ptr = self.font.borrow().font;
        let display = self.window.borrow().display;

        let mut draw_calls = HashMap::<u32, Vec<xft::XftGlyphFontSpec>>::new();
        for (rune, pos) in runes
        {
            let x = pos.get_column() * font.width;
            let y = (pos.get_row() + 1) * font.height;

            // Draw background
            self.draw_rect(
                x, y - font.height,
                font.width, font.height,
                rune.attribute.background);

            // If it's a 0, don't draw it
            if rune.code_point == 0 {
                continue;
            }

            // Fetch char data
            let glyph = unsafe { xft::XftCharIndex(display, font_ptr, rune.code_point) };
            let spec = xft::XftGlyphFontSpec 
            { 
                glyph: glyph,
                x: x as i16,
                y: (y - font.descent) as i16,
                font: font_ptr,
            };

            // Add to draw calls
            let color = rune.attribute.foreground;
            if !draw_calls.contains_key(&color) {
                draw_calls.insert(color, Vec::new());
            }
            draw_calls.get_mut(&color).unwrap().push(spec);
        }

        if draw_calls.is_empty() {
            return;
        }

        let mut font = self.font.borrow_mut();
        for (color, specs) in draw_calls.clone()
        {
            let mut xft_color = font.color(&color);
            unsafe
            {
                xft::XftDrawGlyphFontSpec(self.draw, 
                    &mut xft_color, specs.as_ptr(), specs.len() as i32);
            }
        }
    }

}
