mod terminal;
mod display;
mod decoder;
use display::xlib_display::XLibDisplay;

fn main() 
{
    println!("Hello, rust terminal!");

    let mut display = XLibDisplay::new("Rust Terminal");
    terminal::run(&mut display);
}

