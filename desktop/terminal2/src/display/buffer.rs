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
    scroll_region_top: i32,
    scroll_region_bottom: i32,
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
            scroll_region_top: 0,
            scroll_region_bottom: rows,
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
        self.scroll_region_top = 0;
        self.scroll_region_bottom = rows;
        self.rows = rows;
        self.columns = columns;
    }

    pub fn set_scroll_region(&mut self, top: i32, bottom: i32)
    {
        self.scroll_region_top = top;
        self.scroll_region_bottom = bottom;
    }

    fn move_row(&mut self, to: i32, from: i32)
    {
        for column in 0..self.columns 
        {
            let to_index = (to * self.columns + column) as usize;
            let from_index = (from * self.columns + column) as usize;
            self.content[to_index] = self.content[from_index].clone();
            self.changes[to_index] = true;
        }
    }

    fn clear_row(&mut self, row: i32)
    {
        for column in 0..self.columns
        {
            let index = (row * self.columns + column) as usize;
            self.content[index] = Rune::default();
            self.changes[index] = true;
        }
    }

    pub fn scroll(&mut self, amount: i32)
    {
        if amount < 0
        {
            let start_row = self.scroll_region_top;
            let end_row = self.scroll_region_bottom + amount;
            for row in start_row..end_row {
                self.move_row(row, row - amount);
            }
            for row in end_row..self.scroll_region_bottom {
                self.clear_row(row);
            }
        }
        else if amount > 0
        {
            let start_row = self.scroll_region_top + amount;
            let end_row = self.scroll_region_bottom;
            for row in (start_row..end_row).rev() {
                self.move_row(row, row - amount);
            }
            for row in self.scroll_region_top..start_row {
                self.clear_row(row);
            }
        }
        self.cursor_move(amount, 0);
    }

    pub fn flush(&mut self)
    {
        for x in self.changes.iter_mut() { 
            *x = false; 
        }
    }

    fn out_of_bounds(&self, pos: &CursorPos) -> bool
    {
        return pos.get_row() < 0 || pos.get_row() >= self.rows || 
            pos.get_column() < 0 || pos.get_column() >= self.columns;
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

    fn rune_from_code_point(&mut self, code_point: u32) -> Rune
    {
        return Rune
        {
            code_point: code_point,
            attribute: self.attribute.clone(),
        };
    }

    pub fn type_rune(&mut self, code_point: u32)
    {
        let cursor = self.cursor.clone();
        let rune = self.rune_from_code_point(code_point);
        self.set_rune_at(&cursor, rune);
        self.cursor_move(0, 1);
    }

    pub fn fill(&mut self, code_point: u32)
    {
        let mut rune = Rune::default();
        rune.code_point = code_point;

        for row in 0..self.rows
        {
            for column in 0..self.columns
            {
                let pos = CursorPos::new(row, column);
                self.set_rune_at(&pos, rune.clone());
            }
        }
    }

    pub fn insert(&mut self, count: usize)
    {
        let start = self.cursor.get_column();
        let end = self.columns - count as i32;
        for column in (start..end).rev()
        {
            let index = (self.cursor.get_row() * self.columns + column) as usize;
            self.content[index] = self.content[index - count].clone();
            self.changes[index] = true;
        }
    }

    pub fn delete(&mut self, count: usize)
    {
        let start = self.cursor.get_column();
        let end = self.columns;
        for column in start..end
        {
            let index = (self.cursor.get_row() * self.columns + column) as usize;
            if column >= start - 1 + count as i32 {
                self.content[index] = self.content[index + count].clone();
            }
            self.changes[index + count] = true;
        }
    }

    pub fn insert_lines(&mut self, count: i32)
    {
        let start = self.cursor.get_row();
        let end = self.rows - count;
        for row in (start..end).rev()
        {
            for column in 0..self.columns
            {
                let new_index = ((row + count) * self.columns + column) as usize;
                let old_index = (row * self.columns + column) as usize;
                self.content[new_index] = self.content[old_index].clone();
                self.changes[new_index] = true;
            }
        }
    }

    pub fn type_special(&mut self, special: Special)
    {
        self.on_cursor_change();
        match special
        {
            Special::NewLine => self.cursor_move(1, 0),
            Special::Return => self.cursor.carriage_return(),
        }
        self.on_cursor_change();
    }

    fn cursor_check_move(&mut self)
    {
        let row = self.cursor.get_row();
        if row >= self.scroll_region_bottom {
            self.scroll(self.scroll_region_bottom - row - 1);
        }
        if row < self.scroll_region_top {
            self.scroll(self.scroll_region_top - row);
        }

        self.cursor.clamp(self.columns, self.rows);
        self.on_cursor_change();
    }

    fn cursor_move(&mut self, row: i32, column: i32)
    {
        self.on_cursor_change();
        self.cursor.move_by(row, column);
        self.cursor_check_move();
    }
    
    pub fn cursor_set(&mut self, row: i32, column: i32) 
    { 
        self.on_cursor_change();
        self.cursor.move_to(row, column); 
        self.cursor_check_move();
    }

    pub fn cursor_set_row(&mut self, row: i32)
    {
        self.cursor_set(row, self.cursor.get_column());
    }
    
    pub fn cursor_set_column(&mut self, column: i32)
    {
        self.cursor_set(self.cursor.get_row(), column);
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
        self.clear_line_range(self.cursor.get_column(), self.columns);
    }
    pub fn clear_from_cursor_left(&mut self) { 
        self.clear_line_range(0, self.cursor.get_column());
    }
    pub fn clear_whole_line(&mut self) { 
        self.clear_line_range(0, self.columns);
    }
    
    pub fn clear_from_cursor_down(&mut self) {
        self.clear_from_cursor_right();
        self.clear_block_range(self.cursor.get_row() + 1, self.rows);
    }
    pub fn clear_from_cursor_up(&mut self) {
        self.clear_from_cursor_left();
        self.clear_block_range(0, self.cursor.get_row());
    }
    pub fn clear_whole_screen(&mut self) {
        self.clear_block_range(0, self.rows);
    }

}

