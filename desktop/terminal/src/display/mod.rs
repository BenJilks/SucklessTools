pub mod xlib;
pub mod cursor;
pub mod rune;
use rune::Rune;
use cursor::CursorPos;

pub enum UpdateResultType
{
    Input,
    Resize,
    RedrawRange,
    MouseDown,
    MouseDrag,
    DoubleClickDown,
}

pub struct UpdateResult
{
    pub result_type: UpdateResultType,
    pub input: Vec<u8>,
    pub amount: i32,
    pub start: i32,

    pub rows: i32,
    pub columns: i32,
    pub width: i32,
    pub height: i32,
}

impl Default for UpdateResult
{

    fn default() -> Self
    {
        Self
        {
            result_type: UpdateResultType::Input,
            input: Vec::new(),
            amount: 0,
            start: 0,

            rows: 0,
            columns: 0,
            width: 0,
            height: 0,
        }
    }

}

impl UpdateResult
{

    pub fn input(input: &[u8]) -> Self
    {
        return Self
        {
            result_type: UpdateResultType::Input,
            input: input.to_vec(),
            ..Self::default()
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
            rows: rows,
            columns: columns,
            width: width,
            height: height,
            ..Self::default()
        };
    }

    pub fn redraw_range(start: i32, height: i32) -> Self
    {
        Self
        {
            result_type: UpdateResultType::RedrawRange,
            start: start,
            height: height,
            ..Self::default()
        }
    }

    pub fn mouse_down(row: i32, column: i32) -> Self
    {
        return Self
        {
            result_type: UpdateResultType::MouseDown,
            rows: row,
            columns: column,
            ..Self::default()
        };
    }
    pub fn mouse_drag(row: i32, column: i32) -> Self
    {
        return Self
        {
            result_type: UpdateResultType::MouseDrag,
            rows: row,
            columns: column,
            ..Self::default()
        };
    }
    pub fn double_click_down(row: i32, column: i32) -> Self
    {
        return Self
        {
            result_type: UpdateResultType::DoubleClickDown,
            rows: row,
            columns: column,
            ..Self::default()
        };
    }

}

pub trait Display
{
    // Management
    fn update(&mut self) -> Vec<UpdateResult>;
    fn should_close(&self) -> bool;
    fn get_fd(&self) -> i32;
    fn reset_viewport(&mut self);

    // Draw routines
    fn clear_screen(&mut self);
    fn draw_runes(&mut self, runes: &[(Rune, CursorPos)]);
    fn draw_scroll(&mut self, amount: i32, top: i32, bottom: i32);
    fn draw_clear(&mut self, attribute: &rune::Attribute, 
        row: i32, column: i32, width: i32, height: i32);
    fn flush(&mut self);
    
}
