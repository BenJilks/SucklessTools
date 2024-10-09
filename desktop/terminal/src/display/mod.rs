#[cfg(feature = "x11")]
pub mod xlib;

#[cfg(feature = "raylib")]
pub mod raylib;

#[derive(Debug)]
pub enum Event {
    Input(Vec<u8>),
    Resize(ResizeEvent),
    RedrawRange { start: i32, height: i32 },
    MouseDown { row: i32, column: i32 },
    MouseDrag { row: i32, column: i32 },
    DoubleClick { row: i32, column: i32 },
}

#[derive(Debug)]
pub struct ResizeEvent {
    pub rows: i32,
    pub columns: i32,
    pub width: i32,
    pub height: i32,
}

impl Event {
    pub fn input_str(input: &str) -> Self {
        Self::Input(input.bytes().into_iter().collect())
    }
}
