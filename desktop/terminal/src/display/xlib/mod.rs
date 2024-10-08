extern crate x11;
use super::{Event, ResizeEvent};
use crate::buffer::cursor::CursorPos;
use crate::buffer::rune::{Attribute, Rune};
use crate::buffer::DrawAction;
use font::Font;
use screen::Screen;
use std::cell::RefCell;
use std::collections::HashMap;
use std::mem;
use std::os::raw::{c_char, c_uint};
use std::ptr;
use std::rc::Rc;
use std::sync::mpsc;
use std::time::Instant;
use window::Window;
use x11::keysym;
use x11::xlib;

mod font;
mod screen;
mod window;

pub struct XLibDisplay {
    event_sender: mpsc::Sender<Event>,
    draw_action_receiver: mpsc::Receiver<DrawAction>,

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

impl XLibDisplay {
    pub fn new(
        title: &str,
        event_sender: mpsc::Sender<Event>,
        draw_action_receiver: mpsc::Receiver<DrawAction>,
    ) -> Self {
        let width = 1600;
        let height = 800;

        let window = Window::open(width, height, title);
        let font = Font::load(window.clone(), 20);
        let main_screen = Screen::new(window.clone(), font.clone(), 0);
        let secondary_screen = Screen::new(window.clone(), font.clone(), 1);

        Self {
            event_sender,
            draw_action_receiver,

            window,
            font,

            main_screen,
            secondary_screen,
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

    pub fn run_event_loop(&mut self) {
        while !self.should_close() {
            let mut event: xlib::XEvent = unsafe { mem::zeroed() };
            while unsafe { xlib::XPending(self.window.borrow().display) } != 0 {
                unsafe { xlib::XNextEvent(self.window.borrow().display, &mut event) };

                match event.get_type() {
                    xlib::Expose => self.redraw_rows_in_viewport(),
                    xlib::KeyPress => self.on_key_pressed(&mut event),
                    xlib::ConfigureNotify => self.on_resize(&mut event),
                    xlib::ButtonPress => self.on_button_pressed(&event),
                    xlib::ButtonRelease => self.on_button_released(&event),
                    xlib::MotionNotify => self.on_mouse_move(&event),
                    _ => {}
                }
            }

            while let Ok(draw_action) = self.draw_action_receiver.try_recv() {
                match draw_action {
                    DrawAction::ClearScreen => self.clear_screen(),
                    DrawAction::Flush => self.flush(),
                    DrawAction::ResetViewport => self.reset_viewport(),
                    DrawAction::Runes(runes) => self.draw_runes(&runes),
                    DrawAction::Close => return,

                    DrawAction::Scroll {
                        amount,
                        top,
                        bottom,
                    } => self.draw_scroll(amount, top, bottom),

                    DrawAction::Clear {
                        attribute,
                        row,
                        column,
                        width,
                        height,
                    } => self.draw_clear(&attribute, row, column, width, height),
                }
            }
        }
    }

    fn send_event(&self, event: Event) {
        let _ = self.event_sender.send(event);
    }

    fn recalculate_sizes(&mut self) {
        let (window_width, window_height) = self.window.borrow().get_size();
        let font = self.font.borrow().get_metrics();
        self.rows = window_height / font.height;
        self.columns = window_width / font.width;

        self.pixel_width = self.columns * font.width;
        self.pixel_height = self.rows * font.height;

        self.main_screen.resize(self.pixel_width, self.pixel_height);
        self.secondary_screen
            .resize(self.pixel_width, self.pixel_height);

        self.send_event(Event::Resize(ResizeEvent {
            rows: self.rows,
            columns: self.columns,
            width: window_width,
            height: window_height,
        }));
        self.redraw_rows_in_viewport();
    }

    fn change_font_size(&mut self, font_size: i32) {
        self.font.borrow_mut().resize(font_size);
        self.recalculate_sizes();
    }

    fn handle_type_event(&mut self, event: &mut xlib::XEvent) {
        let mut buffer: [u8; 80];
        let len: i32;
        unsafe {
            buffer = mem::zeroed();
            len = xlib::XLookupString(
                &mut event.key,
                buffer.as_mut_ptr() as *mut c_char,
                buffer.len() as i32,
                ptr::null_mut(),
                ptr::null_mut(),
            );
        }

        let input = buffer[..len as usize].iter().map(|x| *x).collect();
        self.send_event(Event::Input(input));
    }

    fn on_key_pressed(&mut self, event: &mut xlib::XEvent) {
        let state = unsafe { event.key.state };
        let shift = state & xlib::ShiftMask != 0;
        let ctrl_shift = state & xlib::ControlMask != 0 && shift;

        let font = self.font.borrow().get_metrics();
        let height = self.window.borrow().height;
        let keysym = unsafe {
            xlib::XkbKeycodeToKeysym(self.window.borrow().display, event.key.keycode as u8, 0, 0)
        };
        match keysym as c_uint {
            keysym::XK_Up => self.send_event(Event::input_str("\x1b[A")),
            keysym::XK_Down => self.send_event(Event::input_str("\x1b[B")),
            keysym::XK_Right => self.send_event(Event::input_str("\x1b[C")),
            keysym::XK_Left => self.send_event(Event::input_str("\x1b[D")),

            keysym::XK_Home => self.send_event(Event::input_str("\x1b[H")),
            keysym::XK_End => self.send_event(Event::input_str("\x1b[F")),

            keysym::XK_Page_Up => {
                if shift {
                    self.scroll_viewport(height + 1);
                } else {
                    self.send_event(Event::input_str("\x1b[5~"));
                }
            }
            keysym::XK_Page_Down => {
                if shift {
                    self.scroll_viewport(-height - 1);
                } else {
                    self.send_event(Event::input_str("\x1b[6~"));
                }
            }
            keysym::XK_Tab => self.send_event(Event::input_str("\t")),
            keysym::XK_Escape => self.send_event(Event::input_str("\x1b")),

            keysym::XK_equal => {
                if ctrl_shift {
                    self.change_font_size(font.size + 1);
                } else {
                    self.handle_type_event(event);
                }
            }
            keysym::XK_minus => {
                if ctrl_shift {
                    self.change_font_size(font.size - 1);
                } else {
                    self.handle_type_event(event);
                }
            }

            _ => self.handle_type_event(event),
        }
    }

    fn on_resize(&mut self, event: &mut xlib::XEvent) {
        {
            let window = &mut self.window.borrow_mut();
            window.width = unsafe { event.configure.width };
            window.height = unsafe { event.configure.height };
        }
        self.recalculate_sizes();
    }

    fn window_to_cursor_position(&self, x: i32, y: i32) -> (i32, i32) {
        let font = self.font.borrow().get_metrics();
        let mut absoulute_y = y - self.scroll_offset;
        if absoulute_y < 0 {
            absoulute_y -= font.height;
        }

        let row = absoulute_y / font.height;
        let column = x / font.width;
        return (row, column);
    }

    fn scroll_viewport(&mut self, amount: i32) {
        self.scroll_offset += amount;
        if self.scroll_offset >= self.current_page * self.pixel_height {
            if self.current_page > 0 {
                Screen::swap(&mut self.main_screen, &mut self.secondary_screen);
                self.main_screen.page += 1;
                self.secondary_screen.page += 1;
                self.secondary_screen.clear();
            }

            self.current_page += 1;
            self.send_event(Event::RedrawRange {
                start: -self.current_page * self.rows,
                height: self.rows,
            });
        } else if self.scroll_offset < (self.current_page - 1) * self.pixel_height {
            if self.current_page > 1 {
                Screen::swap(&mut self.main_screen, &mut self.secondary_screen);
                self.main_screen.page -= 1;
                self.secondary_screen.page -= 1;
                self.main_screen.clear();
            }

            self.current_page -= 1;
            self.send_event(Event::RedrawRange {
                start: -(self.current_page - 1) * self.rows,
                height: self.rows,
            });
        }
        self.flush();
    }

    fn redraw_rows_in_viewport(&mut self) {
        let start = -self.current_page * self.rows;
        self.send_event(Event::RedrawRange {
            start,
            height: self.rows * 2,
        });
    }

    fn on_button_pressed(&mut self, event: &xlib::XEvent) {
        let button = unsafe { event.button.button };
        match button {
            xlib::Button4 => self.scroll_viewport(30),
            xlib::Button5 => self.scroll_viewport(-30),

            xlib::Button1 => {
                let (x, y) = unsafe { (event.button.x, event.button.y) };
                let (row, column) = self.window_to_cursor_position(x, y);

                if self.time_since_last_click.elapsed().as_millis() < 200 {
                    self.send_event(Event::DoubleClick { row, column });
                } else {
                    self.send_event(Event::MouseDown { row, column });
                }
                self.time_since_last_click = Instant::now();
                self.is_selecting = true;
            }

            _ => {}
        }
    }
    fn on_button_released(&mut self, event: &xlib::XEvent) {
        let button = unsafe { event.button.button };
        match button {
            xlib::Button1 => self.is_selecting = false,
            _ => {}
        }
    }

    fn on_mouse_move(&mut self, event: &xlib::XEvent) {
        if self.is_selecting {
            let (x, y) = unsafe { (event.motion.x, event.motion.y) };
            let (row, column) = self.window_to_cursor_position(x, y);
            self.send_event(Event::MouseDrag { row, column });
        }
    }

    fn clear_screen(&mut self) {
        self.main_screen.clear();
    }

    fn draw_runes(&mut self, runes: &[(Rune, CursorPos)]) {
        // Noop
        if self.rows == 0 {
            return;
        }

        // Sort runes into screens
        let mut page_map = HashMap::<i32, Vec<(Rune, CursorPos)>>::new();
        for (rune, pos) in runes {
            let row = pos.get_row();
            let page = if row >= 0 {
                0
            } else if row == -1 {
                1
            } else {
                (-row - 1) / self.rows + 1
            };

            if !page_map.contains_key(&page) {
                page_map.insert(page, Vec::new());
            }

            let page_runes = page_map.get_mut(&page).unwrap();
            page_runes.push((rune.clone(), pos.clone()));
        }

        for (page, runes) in page_map {
            if page == self.main_screen.page {
                self.main_screen.draw_runes(&runes, self.rows);
            }
            if page == self.secondary_screen.page {
                self.secondary_screen.draw_runes(&runes, self.rows);
            }
        }
    }

    fn draw_scroll(&mut self, amount: i32, top: i32, bottom: i32) {
        self.main_screen.draw_scroll(amount, top, bottom);
    }

    fn draw_clear(
        &mut self,
        attribute: &Attribute,
        row: i32,
        column: i32,
        width: i32,
        height: i32,
    ) {
        let font = self.font.borrow().get_metrics();
        self.main_screen.draw_rect(
            column * font.width,
            row * font.height,
            width * font.width,
            height * font.height,
            attribute.background,
        );
    }

    fn flush(&mut self) {
        let local_offset = match self.pixel_height {
            0 => 0,
            _ => self.scroll_offset % self.pixel_height,
        };

        self.window.borrow_mut().clear();
        self.main_screen.flush(local_offset);
        self.secondary_screen
            .flush(local_offset - self.pixel_height);
        unsafe { xlib::XFlush(self.window.borrow().display) };
    }

    fn should_close(&self) -> bool {
        return false;
    }

    fn reset_viewport(&mut self) {
        self.scroll_offset = 0;
        self.current_page = 0;
        self.main_screen.page = 0;
        self.secondary_screen.page = 1;
    }
}

impl Drop for XLibDisplay {
    fn drop(&mut self) {
        self.font.borrow_mut().free();
        self.main_screen.free();
        self.secondary_screen.free();
        self.window.borrow_mut().close();
    }
}
