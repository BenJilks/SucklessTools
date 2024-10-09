use std::sync::mpsc;

mod buffer;
mod decoder;
mod display;
mod terminal;

#[cfg(feature = "x11")]
use display::xlib::XLibDisplay;

#[cfg(feature = "raylib")]
use display::raylib::start_raylib_display;

fn main() {
    let (event_sender, event_receiver) = mpsc::channel();
    let (draw_action_sender, draw_action_receiver) = mpsc::channel();
    terminal::run(event_receiver, draw_action_sender);

    #[cfg(feature = "x11")]
    {
        let mut display = XLibDisplay::new("Rust Terminal", event_sender, draw_action_receiver);
        display.run_event_loop();
    }

    #[cfg(feature = "raylib")]
    start_raylib_display("Rust Terminal", event_sender, draw_action_receiver);
}
