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
    pub input: String,
    pub rows: i32,
    pub columns: i32,
}

impl UpdateResult
{

    pub fn input(input: &str) -> Option<Self>
    {
        return Some(Self
        {
            result_type: UpdateResultType::Input,
            input: input.to_owned(),
            rows: 0,
            columns: 0,
        });
    }

    pub fn resize(rows: i32, columns: i32) -> Option<Self>
    {
        return Some(Self
        {
            result_type: UpdateResultType::Resize,
            input: String::new(),
            rows: rows,
            columns: columns,
        });
    }

}

pub trait Display
{
    fn update(&mut self, buffer: &buffer::Buffer) -> Option<UpdateResult>;
    fn should_close(&self) -> bool;

    fn draw_rune(&mut self, buffer: &buffer::Buffer, at: &cursor::CursorPos);
    fn redraw(&mut self, buffer: &buffer::Buffer);
    fn flush(&mut self);
    
    fn get_fd(&self) -> i32;
}

