pub mod xlib_display;
pub mod output;
pub mod cursor;

pub trait Display
{
    fn update(&mut self, buffer: &output::Buffer) -> Option<String>;
    fn redraw(&mut self, buffer: &output::Buffer);
    fn should_close(&self) -> bool;

    fn get_fd(&self) -> i32;
}

