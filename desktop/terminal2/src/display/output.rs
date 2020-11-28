use super::cursor::*;

pub type rune_t = u32;

pub enum Special
{
    NewLine,
    Return,
}

pub struct Buffer
{
    cursor: CursorPos,
    width: i32,
    height: i32,
    content: Box<[rune_t]>,
}

impl Buffer
{
    
    pub fn new(width: i32, height: i32) -> Self
    {
        return Self
        {
            cursor: CursorPos::new(0, 0),
            width: width,
            height: height,
            content: vec![0; (width * height) as usize].into_boxed_slice(),
        };
    }

    pub fn get_width(&self) -> i32 { self.width }
    pub fn get_height(&self) -> i32 { self.height }

    fn out_of_bounds(&self, pos: &CursorPos) -> bool
    {
        return pos.get_row() >= self.height || pos.get_column() >= self.width;
    }

    pub fn rune_at(&self, at: &CursorPos) -> Option<rune_t>
    {
        if self.out_of_bounds(at) {
            return None;
        }

        let index = at.get_row() * self.width + at.get_column();
        return Some( self.content[index as usize] );
    }

    fn set_rune_at(&mut self, at: &CursorPos, rune: u32)
    {
        if self.out_of_bounds(at) {
            return;
        }

        let index = at.get_row() * self.width + at.get_column();
        self.content[index as usize] = rune;
    }

    pub fn type_rune(&mut self, rune: rune_t)
    {
        let cursor = self.cursor.clone();
        self.set_rune_at(&cursor, rune);

        self.cursor.move_by(0, 1);
        if self.cursor.get_column() >= self.width {
            self.cursor.next_line();
        }
    }

    pub fn type_special(&mut self, special: Special)
    {
        match special
        {
            Special::NewLine => self.cursor.next_line(),
            Special::Return => self.cursor.carriage_return(),
        }
    }

}

