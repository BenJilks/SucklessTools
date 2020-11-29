use super::cursor::*;
use super::rune::*;

pub enum Special
{
    NewLine,
    Return,
}

pub enum Command
{
    CursorLeft,
}

pub struct Buffer
{
    cursor: CursorPos,
    attribute: Attribute,

    width: i32,
    height: i32,
    content: Box<[Rune]>,
    changes: Box<[bool]>,
}

impl Buffer
{
    
    pub fn new(width: i32, height: i32) -> Self
    {
        let default_rune = Rune
        {
            code_point: 0,
            attribute: Attribute
            {
                background: Color::DefaultBackground,
                foreground: Color::DefaultForeground,
            },
        };

        return Self
        {
            cursor: CursorPos::new(0, 0),
            attribute: default_rune.attribute.clone(),

            width: width,
            height: height,
            content: vec![default_rune.clone(); (width * height) as usize].into_boxed_slice(),
            changes: vec![false; (width * height) as usize].into_boxed_slice(),
        };
    }

    pub fn get_width(&self) -> i32 { self.width }
    pub fn get_height(&self) -> i32 { self.height }
    pub fn get_changes(&self) -> Box<[bool]> { self.changes.clone() }

    pub fn flush(&mut self)
    {
        for x in self.changes.iter_mut() { 
            *x = false; 
        }
    }

    fn out_of_bounds(&self, pos: &CursorPos) -> bool
    {
        return pos.get_row() >= self.height || pos.get_column() >= self.width;
    }

    pub fn rune_at(&self, at: &CursorPos) -> Option<Rune>
    {
        if self.out_of_bounds(at) {
            return None;
        }

        let index = at.get_row() * self.width + at.get_column();
        let mut rune = self.content[index as usize].clone();
        if *at == self.cursor {
            rune.attribute = self.attribute.inverted();
        }
        return Some( rune );
    }

    fn index_from_cursor(&self, at: &CursorPos) -> usize
    {
        return (at.get_row() * self.width + at.get_column()) as usize;
    }

    fn on_cursor_change(&mut self)
    {
        let index = self.index_from_cursor(&self.cursor);
        self.changes[index] = true;
    }

    fn set_rune_at(&mut self, at: &CursorPos, rune: Rune)
    {
        if self.out_of_bounds(at) {
            return;
        }

        let index = self.index_from_cursor(at);
        self.content[index] = rune;
        self.changes[index] = true;
    }

    pub fn type_rune(&mut self, code_point: u32)
    {
        let cursor = self.cursor.clone();
        let rune = Rune
        {
            code_point: code_point,
            attribute: self.attribute.clone(),
        };
        self.set_rune_at(&cursor, rune);

        self.on_cursor_change();
        self.cursor.move_by(0, 1);
        if self.cursor.get_column() >= self.width {
            self.cursor.next_line();
        }
        self.on_cursor_change();
    }

    pub fn type_special(&mut self, special: Special)
    {
        self.on_cursor_change();
        match special
        {
            Special::NewLine => self.cursor.next_line(),
            Special::Return => self.cursor.carriage_return(),
        }
        self.on_cursor_change();
    }

    pub fn do_command(&mut self, command: Command, arg: i32)
    {
        self.on_cursor_change();
        match command
        {
            Command::CursorLeft => self.cursor.move_by(0, -arg),
        }
        self.on_cursor_change();
    }

}

