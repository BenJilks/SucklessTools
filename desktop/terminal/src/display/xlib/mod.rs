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
use std::ptr;
use std::mem;
use std::os::raw::{c_uint, c_char};
use std::time::Instant;
use std::rc::Rc;
use std::cell::RefCell;
use std::collections::HashMap;

pub struct XLibDisplay
{
    // Xlib
    window: Rc<RefCell<Window>>,
    font: Rc<RefCell<Font>>,
    
    // Screens
    main_screen: Screen,
    secondary_screen: Screen,
    current_page: i32,
    
    // Window metrics
    rows: i32,
    columns: i32,
    pixel_width: i32,
    pixel_height: i32,

    // State
    is_selecting: bool,
    time_since_last_click: Instant,
    scroll_offset: i32,
}

impl XLibDisplay
{

    pub fn new(title: &str) -> Self
    {
        let width = 1600;
        let height = 800;

        let window = Window::open(width, height, title);
        let font = Font::load(window.clone(), 20);
        let main_screen = Screen::new(window.clone(), font.clone(), 0);
        let secondary_screen = Screen::new(window.clone(), font.clone(), 1);
        Self
        {
            window: window,
            font: font,

            main_screen: main_screen,
            secondary_screen: secondary_screen,
            current_page: 0,

            rows: 0,
            columns: 0,
            pixel_width: 0,
            pixel_height: 0,
            
            is_selecting: false,
            time_since_last_click: Instant::now(),
            scroll_offset: 0,
        }
    }

    fn recalculate_sizes(&mut self) -> UpdateResult
    {
        let (window_width, window_height) = self.window.borrow().get_size();
        let font = self.font.borrow().get_metrics();
        self.rows = window_height / font.height;
        self.columns = window_width / font.width;

        self.pixel_width = self.columns * font.width;
        self.pixel_height = self.rows * font.height;

        self.main_screen.resize(self.pixel_width, self.pixel_height);
        self.secondary_screen.resize(self.pixel_width, self.pixel_height);
        return UpdateResult::resize(self.rows, self.columns, window_width, window_height);
    }

    fn change_font_size(&mut self, font_size: i32) -> UpdateResult
    {
        self.font.borrow_mut().resize(font_size);
        return self.recalculate_sizes();
    }

    fn handle_type_event(&mut self, event: &mut xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        let mut buffer: [u8; 80];
        let len: i32;
        unsafe
        {
            buffer = mem::zeroed();
            len = xlib::XLookupString(&mut event.key,
                buffer.as_mut_ptr() as *mut c_char, buffer.len() as i32,
                ptr::null_mut(), ptr::null_mut());
        }
         
        results.push(UpdateResult::input(&buffer[..len as usize]));
    }

    fn on_key_pressed(&mut self, event: &mut xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        let state = unsafe { event.key.state };
        let shift = state & xlib::ShiftMask != 0;
        let ctrl_shift = state & xlib::ControlMask != 0 && shift;
        
        let font = self.font.borrow().get_metrics();
        let height = self.window.borrow().height;
        let keysym = unsafe { xlib::XkbKeycodeToKeysym(self.window.borrow().display, event.key.keycode as u8, 0, 0) };
        match keysym as c_uint
        {
            keysym::XK_Up => results.push(UpdateResult::input_str("\x1b[A")),
            keysym::XK_Down => results.push(UpdateResult::input_str("\x1b[B")),
            keysym::XK_Right => results.push(UpdateResult::input_str("\x1b[C")),
            keysym::XK_Left => results.push(UpdateResult::input_str("\x1b[D")),

            keysym::XK_Home => results.push(UpdateResult::input_str("\x1b[H")),
            keysym::XK_End => results.push(UpdateResult::input_str("\x1b[F")),
            
            keysym::XK_Page_Up => 
            {
                if shift {
                    self.scroll_viewport(height + 1, results);
                } else {
                    results.push(UpdateResult::input_str("\x1b[5~"));
                }
            },
            keysym::XK_Page_Down => 
            {
                if shift {
                    self.scroll_viewport(-height - 1, results);
                } else {
                    results.push(UpdateResult::input_str("\x1b[6~"));
                }
            },
            keysym::XK_Tab => results.push(UpdateResult::input_str("\t")),
            keysym::XK_Escape => results.push(UpdateResult::input_str("\x1b")),

            keysym::XK_equal =>
            {
                if ctrl_shift {
                    results.push(self.change_font_size(font.size + 1));
                } else {
                    self.handle_type_event(event, results);
                }
            },
            keysym::XK_minus =>
            {
                if ctrl_shift {
                    results.push(self.change_font_size(font.size - 1));
                } else {
                    self.handle_type_event(event, results);
                }
            },

            _ => self.handle_type_event(event, results),
        }
    }

    fn on_resize(&mut self, event: &mut xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        {
            let window = &mut self.window.borrow_mut();
            window.width = unsafe { event.configure.width };
            window.height = unsafe { event.configure.height };
        }
        results.push(self.recalculate_sizes());
    }

    fn window_to_cursor_position(&self, x: i32, y: i32) -> (i32, i32)
    {
        let font = self.font.borrow().get_metrics();
        let row = (y - self.scroll_offset) / font.height;
        let column = x / font.width;
        return (row, column);
    }

    fn scroll_viewport(&mut self, amount: i32, results: &mut Vec<UpdateResult>)
    {
        use crate::display::Display;
        
        self.scroll_offset += amount;
        if self.scroll_offset >= self.current_page * self.pixel_height
        {
            if self.current_page > 0 
            {
                Screen::swap(&mut self.main_screen, &mut self.secondary_screen);
                self.main_screen.page += 1;
                self.secondary_screen.page += 1;
                self.secondary_screen.clear();
            }

            self.current_page += 1;
            results.push(UpdateResult::request_scrollback(
                -self.current_page * self.rows, self.rows));
        }
        else if self.scroll_offset < (self.current_page - 1) * self.pixel_height
        {
            if self.current_page > 1
            {
                Screen::swap(&mut self.main_screen, &mut self.secondary_screen);
                self.main_screen.page -= 1;
                self.secondary_screen.page -= 1;
                self.main_screen.clear();
            }

            self.current_page -= 1;
            results.push(UpdateResult::request_scrollback(
                -(self.current_page - 1) * self.rows, self.rows));
        }
        self.flush();
    }

    fn on_button_pressed(&mut self, event: &xlib::XEvent, results: &mut Vec<UpdateResult>)
    {
        let button = unsafe { event.button.button };
        match button
        {
            xlib::Button4 => self.scroll_viewport(30, results),
            xlib::Button5 => self.scroll_viewport(-30, results),

            xlib::Button1 =>
            {
                let (x, y) = unsafe { (event.button.x, event.button.y) };
                let (row, column) = self.window_to_cursor_position(x, y);

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
            let (x, y) = unsafe { (event.motion.x, event.motion.y) };
            let (row, column) = self.window_to_cursor_position(x, y);
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
        self.secondary_screen.free();
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
                xlib::KeyPress => self.on_key_pressed(&mut event, &mut results),
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
        self.main_screen.clear();
    }

    fn draw_runes(&mut self, runes: &[(Rune, CursorPos)])
    {
        // Noop
        if self.rows == 0 {
            return;
        }

        // Sort runes into screens
        let mut page_map = HashMap::<i32, Vec<(Rune, CursorPos)>>::new();
        for (rune, pos) in runes
        {
            let mut page = pos.get_row() / self.rows;
            if pos.get_row() < 0 {
                page = -page + 1;
            }

            if !page_map.contains_key(&page) {
                page_map.insert(page, Vec::new());
            }

            let page_runes = page_map.get_mut(&page).unwrap();
            page_runes.push((rune.clone(), pos.clone()));
        }

        for (page, runes) in page_map 
        {
            println!("Drawing page {}, main = {}, secondary = {}", page, self.main_screen.page, self.secondary_screen.page);
            if page == self.main_screen.page {
                self.main_screen.draw_runes(&runes, self.rows);
            }
            if page == self.secondary_screen.page {
                self.secondary_screen.draw_runes(&runes, self.rows);
            }
        }
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
        self.window.borrow_mut().clear();

        let local_offset = self.scroll_offset % self.pixel_height;
        self.main_screen.flush(local_offset);
        self.secondary_screen.flush(local_offset - self.pixel_height);
        unsafe { xlib::XFlush(self.window.borrow().display) };
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
