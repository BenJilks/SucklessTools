extern crate x11;
mod screen;
mod window;
mod font;
use crate::display::cursor::CursorPos;
use crate::display::rune::{Rune, StandardColor, Attribute};
use crate::display::UpdateResult;
use screen::Screen;
use window::Window;
use font::Font;
use x11::{ xlib, keysym };
use std::{ ptr, mem };
use std::os::raw::c_uint;
use std::time::Instant;
use std::rc::Rc;
use std::cell::RefCell;

pub struct XLibDisplay
{
    // Xlib
    window: Rc<RefCell<Window>>,
    font: Rc<RefCell<Font>>,
    main_screen: Screen,
    
    // Window metrics
    rows: i32,
    columns: i32,
    is_selecting: bool,
    time_since_last_click: Instant,
}

impl XLibDisplay
{

    pub fn new(title: &str) -> Self
    {
        let width = 1600;
        let height = 800;

        let window = Window::open(width, height, title);
        let font = Font::load(window.clone(), 20);
        let main_screen = Screen::new(window.clone(), font.clone());
        Self
        {
            window: window,
            font: font,
            main_screen: main_screen,

            rows: 0,
            columns: 0,
            is_selecting: false,
            time_since_last_click: Instant::now(),
        }
    }

    fn change_font_size(&mut self, font_size: i32) -> UpdateResult
    {
        let (window_width, window_height) = self.window.borrow().get_size();
        self.font.borrow_mut().resize(font_size);
        self.main_screen.resize(window_width, window_height);

        let font = self.font.borrow().get_metrics();
        self.rows = window_height / font.height;
        self.columns = window_width / font.width;
        return UpdateResult::resize(self.rows, self.columns, window_width, window_height);
    }

    fn on_key_pressed(&mut self, event: &mut xlib::XEvent) -> UpdateResult
    {
        let state = unsafe { event.key.state };
        let shift = state & xlib::ShiftMask != 0;
        let ctrl_shift = state & xlib::ControlMask != 0 && shift;
        
        let font = self.font.borrow().get_metrics();
        let keysym = unsafe { xlib::XkbKeycodeToKeysym(self.window.borrow().display, event.key.keycode as u8, 0, 0) };
        match keysym as c_uint
        {
            keysym::XK_Up => return UpdateResult::input_str("\x1b[A"),
            keysym::XK_Down => return UpdateResult::input_str("\x1b[B"),
            keysym::XK_Right => return UpdateResult::input_str("\x1b[C"),
            keysym::XK_Left => return UpdateResult::input_str("\x1b[D"),

            keysym::XK_Home => return UpdateResult::input_str("\x1b[H"),
            keysym::XK_End => return UpdateResult::input_str("\x1b[F"),
            
            keysym::XK_Page_Up => 
            {
                if shift {
                    return UpdateResult::scroll_viewport(self.rows);
                }
                return UpdateResult::input_str("\x1b[5~");
            },
            keysym::XK_Page_Down => 
            {
                if shift {
                    return UpdateResult::scroll_viewport(-self.rows);
                }
                return UpdateResult::input_str("\x1b[6~");
            },
            keysym::XK_Tab => return UpdateResult::input_str("\t"),
            keysym::XK_Escape => return UpdateResult::input_str("\x1b"),

            keysym::XK_equal =>
            {
                if ctrl_shift {
                    return self.change_font_size(font.size + 1);
                }
            },
            keysym::XK_minus =>
            {
                if ctrl_shift {
                    return self.change_font_size(font.size - 1);
                }
            },
            _ => {},
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
        let width = unsafe { event.configure.width };
        let height = unsafe { event.configure.height };
        let font = self.font.borrow().get_metrics();
        {
            let window = &mut self.window.borrow_mut();
            window.width = width;
            window.height = height;
            self.rows = window.height / font.height;
            self.columns = window.width / font.width;
        }

        self.main_screen.resize(width, height);
        results.push(UpdateResult::resize(self.rows, self.columns, width, height));
    }

    fn on_button_pressed(&mut self, event: &xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        let button = unsafe { event.button.button };
        match button
        {
            xlib::Button4 => results.push(UpdateResult::scroll_viewport(1)),
            xlib::Button5 => results.push(UpdateResult::scroll_viewport(-1)),
            xlib::Button1 =>
            {
                let font = self.font.borrow().get_metrics();
                let (x, y) = unsafe { (event.button.x, event.button.y) };
                let row = y / font.height;
                let column = x / font.width;

                if self.time_since_last_click.elapsed().as_millis() < 200 {
                    results.push(UpdateResult::double_click_down(row, column));
                } else {
                    results.push(UpdateResult::mouse_down(row, column));
                }
                self.time_since_last_click = Instant::now();
                self.is_selecting = true;
            },

            _ => {},
        }
    }
    fn on_button_released(&mut self, event: &xlib::XEvent)
    {
        let button = unsafe { event.button.button };
        match button
        {
            xlib::Button1 => self.is_selecting = false,
            _ => {},
        }
    }

    fn on_mouse_move(&mut self, event: &xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        if self.is_selecting
        {
            let font = self.font.borrow().get_metrics();
            let (x, y) = unsafe { (event.motion.x, event.motion.y) };
            let row = y / font.height;
            let column = x / font.width;
            results.push(UpdateResult::mouse_drag(row, column));
        }
    }
    
}

impl Drop for XLibDisplay
{
    fn drop(&mut self)
    {
        self.font.borrow_mut().free();
        self.main_screen.free();
        self.window.borrow_mut().close();
    }
}

impl super::Display for XLibDisplay
{

    fn update(&mut self) -> Vec<UpdateResult>
    {
        let mut results = Vec::<UpdateResult>::new();
        let mut event: xlib::XEvent = unsafe { mem::zeroed() };
        while unsafe { xlib::XPending(self.window.borrow().display) } != 0
        {
            unsafe { xlib::XNextEvent(self.window.borrow().display, &mut event) };

            match event.get_type()
            {
                xlib::Expose => results.push(UpdateResult::redraw()),
                xlib::KeyPress => results.push(self.on_key_pressed(&mut event)),
                xlib::ConfigureNotify => self.on_resize(&mut event, &mut results),
                xlib::ButtonPress => self.on_button_pressed(&event, &mut results),
                xlib::ButtonRelease => self.on_button_released(&event),
                xlib::MotionNotify => self.on_mouse_move(&event, &mut results),
                _ => {},
            }
        }

        return results;
    }

    fn clear_screen(&mut self)
    {
        let (width, height) = self.window.borrow().get_size();
        self.main_screen.draw_rect(
            0, 0, width, height, 
            StandardColor::DefaultBackground.color());
    }

    fn draw_runes(&mut self, runes: &[(Rune, CursorPos)])
    {
        self.main_screen.draw_runes(runes);
    }

    fn draw_scroll(&mut self, amount: i32, top: i32, bottom: i32)
    {
        self.main_screen.draw_scroll(amount, top, bottom);
    }
    
    fn draw_clear(&mut self, attribute: &Attribute, 
        row: i32, column: i32, width: i32, height: i32)
    {
        let font = self.font.borrow().get_metrics();
        self.main_screen.draw_rect(
            column * font.width, row * font.height, 
            width * font.width, height * font.height, 
            attribute.background);
    }
    
    fn flush(&mut self)
    {
        unsafe
        {
            self.main_screen.flush();
            xlib::XFlush(self.window.borrow().display);
        }
    }

    fn should_close(&self) -> bool
    {
        return false;
    }

    fn get_fd(&self) -> i32
    {
        return self.window.borrow().fd;
    }

}
