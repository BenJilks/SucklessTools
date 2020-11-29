use super::cursor::*;
use super::rune::*;
use std::cmp::min;

pub enum Special
{
    NewLine,
    Return,
}

pub struct Buffer
{
    cursor: CursorPos,
    attribute: Attribute,

    rows: i32,
    columns: i32,
    content: Box<[Rune]>,
    changes: Box<[bool]>,
}

impl Buffer
{
    
    pub fn new(rows: i32, columns: i32) -> Self
    {
        return Self
        {
            cursor: CursorPos::new(0, 0),
            attribute: Attribute::default(),

            rows: rows,
            columns: columns,
            content: vec![Rune::default(); (rows * columns) as usize].into_boxed_slice(),
            changes: vec![false; (rows * columns) as usize].into_boxed_slice(),
        };
    }

    pub fn get_rows(&self) -> i32 { self.rows }
    pub fn get_columns(&self) -> i32 { self.columns }
    pub fn get_attribute(&mut self) -> &mut Attribute { &mut self.attribute }
    pub fn get_changes(&self) -> Box<[bool]> { self.changes.clone() }

    pub fn resize(&mut self, rows: i32, columns: i32)
    {
        let mut new_content = vec![Rune::default(); (rows * columns) as usize].into_boxed_slice();
        let new_changes = vec![true; (rows * columns) as usize].into_boxed_slice();
        
        for row in 0..min(rows, self.rows)
        {
            for column in 0..min(columns, self.columns)
            {
                let old_index = (row * self.columns + column) as usize;
                let new_index = (row * columns + column) as usize;
                new_content[new_index] = self.content[old_index].clone();
            }
        }

        self.content = new_content;
        self.changes = new_changes;
        self.rows = rows;
        self.columns = columns;
    }

    pub fn flush(&mut self)
    {
        for x in self.changes.iter_mut() { 
            *x = false; 
        }
    }

    fn out_of_bounds(&self, pos: &CursorPos) -> bool
    {
        return pos.get_row() >= self.rows || pos.get_column() >= self.columns;
    }

    pub fn rune_at(&self, at: &CursorPos) -> Option<Rune>
    {
        if self.out_of_bounds(at) {
            return None;
        }

        let index = at.get_row() * self.columns + at.get_column();
        let mut rune = self.content[index as usize].clone();
        if *at == self.cursor {
            rune.attribute = self.attribute.inverted();
        }
        return Some( rune );
    }

    fn index_from_cursor(&self, at: &CursorPos) -> usize
    {
        return (at.get_row() * self.columns + at.get_column()) as usize;
    }

    fn on_cursor_change(&mut self)
    {
        if self.out_of_bounds(&self.cursor) {
            return;
        }

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
        if self.cursor.get_column() >= self.columns {
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

    fn cursor_move(&mut self, row: i32, column: i32)
    {
        self.on_cursor_change();
        self.cursor.move_by(row, column);
        self.cursor.clamp(self.columns, self.rows);
        self.on_cursor_change();
    }
    
    pub fn cursor_set(&mut self, row: i32, column: i32) 
    { 
        self.on_cursor_change();
        self.cursor.move_to(row, column); 
        self.cursor.clamp(self.columns, self.rows);
        self.on_cursor_change();
    }

    pub fn cursor_left(&mut self, amount: i32) { self.cursor_move(0, -amount) }
    pub fn cursor_right(&mut self, amount: i32) { self.cursor_move(0, amount) }
    pub fn cursor_up(&mut self, amount: i32) { self.cursor_move(-amount, 0) }
    pub fn cursor_down(&mut self, amount: i32) { self.cursor_move(amount, 0) }

    pub fn clear_line_range(&mut self, start: i32, end: i32)
    {
        for column in start..end
        {
            let pos = CursorPos::new(self.cursor.get_row(), column);
            self.set_rune_at(&pos, Rune::default());
        }
    }

    pub fn clear_block_range(&mut self, start: i32, end: i32)
    {
        for row in start..end
        {
            for column in 0..self.columns
            {
                let pos = CursorPos::new(row, column);
                self.set_rune_at(&pos, Rune::default());
            }
        }
    }

    pub fn clear_from_cursor_right(&mut self) {
        self.clear_line_range(self.cursor.get_column(), self.columns) 
    }
    pub fn clear_from_cursor_left(&mut self) { 
        self.clear_line_range(0, self.cursor.get_column()) 
    }
    pub fn clear_whole_line(&mut self) { 
        self.clear_line_range(0, self.columns) 
    }
    
    pub fn clear_from_cursor_down(&mut self) {
        self.clear_block_range(self.cursor.get_row(), self.rows)
    }
    pub fn clear_from_cursor_up(&mut self) {
        self.clear_block_range(0, self.cursor.get_row()) 
    }
    pub fn clear_whole_screen(&mut self) {
        self.clear_block_range(0, self.rows) 
    }

}

