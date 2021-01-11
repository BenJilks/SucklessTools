pub mod xlib_display;
pub mod cursor;
pub mod rune;
use rune::Rune;
use cursor::CursorPos;

pub enum UpdateResultType
{
    Input,
    Resize,
    Redraw,
    ScrollViewport,
}

pub struct UpdateResult
{
    pub result_type: UpdateResultType,
    pub input: Vec<u8>,
    pub amount: i32,

    pub rows: i32,
    pub columns: i32,
    pub width: i32,
    pub height: i32,
}

impl UpdateResult
{

    pub fn input(input: &[u8]) -> Self
    {
        return Self
        {
            result_type: UpdateResultType::Input,
            input: input.to_vec(),
            amount: 0,
            rows: 0,
            columns: 0,
            width: 0,
            height: 0,
        };
    }

    pub fn input_str(input: &str) -> Self
    {
        return Self::input(input.as_bytes());
    }

    pub fn resize(rows: i32, columns: i32, width: i32, height: i32) -> Self
    {
        return Self
        {
            result_type: UpdateResultType::Resize,
            input: Vec::new(),
            amount: 0,
            rows: rows,
            columns: columns,
            width: width,
            height: height,
        };
    }

    pub fn redraw() -> Self
    {
        return Self
        {
            result_type: UpdateResultType::Redraw,
            input: Vec::new(),
            amount: 0,
            rows: 0,
            columns: 0,
            width: 0,
            height: 0,
        };
    }

    pub fn scroll_viewport(amount: i32) -> Self
    {
        return Self
        {
            result_type: UpdateResultType::ScrollViewport,
            input: Vec::new(),
            amount: amount,
            rows: 0,
            columns: 0,
            width: 0,
            height: 0,
        };
    }

}

pub trait Display
{
    // Management
    fn update(&mut self) -> Vec<UpdateResult>;
    fn should_close(&self) -> bool;
    fn get_fd(&self) -> i32;

    // Draw routines
    fn draw_rune(&mut self, rune: &Rune, at: &CursorPos);
    fn draw_scroll(&mut self, amount: i32, top: i32, bottom: i32);
    fn draw_clear(&mut self, attribute: &rune::Attribute, 
        row: i32, column: i32, width: i32, height: i32);
    fn flush(&mut self);
    
}

