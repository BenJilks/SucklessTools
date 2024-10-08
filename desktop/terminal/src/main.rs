use std::sync::mpsc;

mod terminal;
mod display;
mod decoder;
mod buffer;
use display::xlib::XLibDisplay;

fn main() 
{
    let (event_sender, event_receiver) = mpsc::channel();
    let (draw_action_sender, draw_action_receiver) = mpsc::channel();
    terminal::run(event_receiver, draw_action_sender);

    let mut display = XLibDisplay::new("Rust Terminal", event_sender, draw_action_receiver);
    display.run_event_loop();
}
