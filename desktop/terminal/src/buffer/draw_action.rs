use super::{Attribute, CursorPos, Rune};

#[derive(Debug)]
pub enum DrawAction {
    ClearScreen,
    Flush,
    ResetViewport,
    Close,

    Scroll {
        amount: i32,
        top: i32,
        bottom: i32,
    },
    Clear {
        attribute: Attribute,
        row: i32,
        column: i32,
        width: i32,
        height: i32,
    },
    Runes(Vec<(Rune, CursorPos)>),
}
