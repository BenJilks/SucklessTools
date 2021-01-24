extern crate x11;
use crate::display::rune::{Rune, StandardColor};
use crate::display::cursor::CursorPos;
use crate::display::xlib::window::Window;
use crate::display::xlib::font::Font;
use x11::{xlib, xft};
use std::ptr;
use std::mem;
use std::os::raw::c_ulong;
use std::cell::RefCell;
use std::rc::Rc;
use std::collections::HashMap;

pub struct Screen
{
    window: Rc<RefCell<Window>>,
    font: Rc<RefCell<Font>>,
    gc: xlib::GC,

    bitmap: xlib::Pixmap,
    draw: *mut xft::XftDraw,
    
    pub page: i32,
    width: i32,
    height: i32,
}

impl Screen
{
    pub fn new(window_ref: Rc<RefCell<Window>>, font: Rc<RefCell<Font>>, page: i32) -> Self
    {
        let bitmap: c_ulong;
        let gc: xlib::GC;
        let draw: *mut xft::XftDraw;
        let width: i32;
        let height: i32;
        {
            let window = window_ref.borrow();
            width = window.width;
            height = window.height;

            bitmap = unsafe { xlib::XCreatePixmap(window.display, window.window, 
                width as u32, height as u32, 24) };
            
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

        let mut screen = Self
        {
            window: window_ref,
            font: font,
            gc: gc,
            
            bitmap: bitmap,
            draw: draw,
            
            page: page,
            width: width,
            height: height,
        };

        screen.clear();
        return screen;
    }

    pub fn swap(a: &mut Self, b: &mut Self)
    {
        mem::swap(&mut a.bitmap, &mut b.bitmap);
        mem::swap(&mut a.draw, &mut b.draw);
        mem::swap(&mut a.gc, &mut b.gc);
    }

    pub fn resize(&mut self, width: i32, height: i32)
    {
        self.free();
        {
            let window = self.window.borrow();
            self.bitmap = unsafe { xlib::XCreatePixmap(window.display, window.window, 
                width as u32, height as u32, 24) };
            self.draw = unsafe { xft::XftDrawCreate(window.display, self.bitmap, 
                window.visual, window.cmap) };
        }

        // TODO: Better error handling
        assert!(self.bitmap != 0);
        assert!(self.draw != ptr::null_mut());

        self.width = width;
        self.height = height;
        self.clear();
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

    pub fn flush(&mut self, offset: i32)
    {
        unsafe
        {
            let window = self.window.borrow();
            xlib::XCopyArea(window.display, self.bitmap, window.window, self.gc, 
                0, 0, self.width as u32, self.height as u32, 0, offset);
        }
    }

    pub fn draw_rect(&mut self, x: i32, y: i32, width: i32, height: i32, color: u32)
    {
        let window = self.window.borrow();

        unsafe
        {
            xlib::XSetForeground(window.display, self.gc, ((color & 0xFFFFFF00) >> 8) as u64);
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

    pub fn draw_runes(&mut self, runes: &[(Rune, CursorPos)], rows: i32)
    {
        let font = self.font.borrow().get_metrics();
        let offset = self.page * rows;

        let mut draw_calls = HashMap::<u32, Vec<xft::XftGlyphFontSpec>>::new();
        for (rune, pos) in runes
        {
            let x = pos.get_column() * font.width;
            let y = (pos.get_row() + 1 + offset) * font.height;

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
            let spec = self.font.borrow().get_glyph(x, y, rune.code_point);

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

    pub fn clear(&mut self)
    {
        self.draw_rect(
            0, 0, self.width, self.height, 
            StandardColor::DefaultBackground.color());
    }

}
