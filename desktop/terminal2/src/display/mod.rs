pub mod xlib_display;
pub mod buffer;
pub mod cursor;
pub mod rune;

pub enum UpdateResultType
{
    Input,
    Resize,
}

pub struct UpdateResult
{
    pub result_type: UpdateResultType,
    pub input: Vec<u8>,
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
            rows: rows,
            columns: columns,
            width: width,
            height: height,
        };
    }

}

pub trait Display
{
    fn update(&mut self, buffer: &buffer::Buffer) -> Vec<UpdateResult>;
    fn should_close(&self) -> bool;

    fn draw_rune(&mut self, buffer: &buffer::Buffer, at: &cursor::CursorPos);
    fn redraw(&mut self, buffer: &buffer::Buffer);
    fn flush(&mut self);
    fn on_input(&mut self, buffer: &buffer::Buffer);
    
    fn get_fd(&self) -> i32;
}

