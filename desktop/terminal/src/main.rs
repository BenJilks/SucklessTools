mod terminal;
mod display;
mod decoder;
mod buffer;
use display::xlib_display::XLibDisplay;

fn main() 
{
    let display = XLibDisplay::new("Rust Terminal");
    terminal::run(display);
}

