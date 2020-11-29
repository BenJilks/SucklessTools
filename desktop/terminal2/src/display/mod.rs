pub mod xlib_display;
pub mod buffer;
pub mod cursor;
pub mod rune;

pub trait Display
{
    fn update(&mut self, buffer: &buffer::Buffer) -> Option<String>;
    fn should_close(&self) -> bool;

    fn draw_rune(&mut self, buffer: &buffer::Buffer, at: &cursor::CursorPos);
    fn redraw(&mut self, buffer: &buffer::Buffer);
    fn flush(&mut self);
    
    fn get_fd(&self) -> i32;
}

