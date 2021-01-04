use std::cmp::min;
use super::display::
{
    self, 
    cursor::*, 
    rune::*,
};

pub struct Buffer<Display>
    where Display: display::Display
{
    cursor: CursorPos,
    attribute: Attribute,
    display: Display,

    rows: i32,
    columns: i32,
    scroll_region_top: i32,
    scroll_region_bottom: i32,
    
    content: Box<[Rune]>,
    scrollback: Vec<Rune>,
    scrollback_rows: i32,
}

#[derive(PartialEq)]
enum DrawMode
{
    Runes,
    Nothing,
}

impl<Display> Buffer<Display>
    where Display: display::Display
{
    
    pub fn new(rows: i32, columns: i32, display: Display) -> Self
    {
        return Self
        {
            cursor: CursorPos::new(0, 0),
            attribute: Attribute::default(),
            display: display,

            rows: rows,
            columns: columns,
            scroll_region_top: 0,
            scroll_region_bottom: rows,

            content: vec![Rune::default(); (rows * columns) as usize].into_boxed_slice(),
            scrollback: Vec::new(),
            scrollback_rows: 0,
        };
    }

    pub fn get_rows(&self) -> i32 { self.rows }
    pub fn get_attribute(&mut self) -> &mut Attribute { &mut self.attribute }
    pub fn get_display(&mut self) -> &mut Display { &mut self.display }

    pub fn redraw(&mut self)
    {
        for row in 0..self.rows
        {
            for column in 0..self.columns
            {
                let index = (row * self.columns + column) as usize;
                let rune = &self.content[index];
                self.display.draw_rune(rune, &CursorPos::new(row, column));
            }
        }
    }

    pub fn resize(&mut self, rows: i32, columns: i32)
    {
        // Noop
        if self.rows == rows && self.columns == columns {
            return;
        }

        // Update main content
        let mut new_content = vec![Rune::default(); (rows * columns) as usize].into_boxed_slice();
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
        
        // Update scrollback
        // TODO: Actually resize this instead of just resetting
        self.scrollback.clear();
        self.scrollback_rows = 0;

        // Update sizes
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

    fn move_row(&mut self, to: i32, from: i32, draw_mode: DrawMode)
    {
        for column in 0..self.columns
        {
            let to_index = (to * self.columns + column) as usize;
            let from_index = (from * self.columns + column) as usize;
            let rune = self.content[from_index].clone();
            if draw_mode == DrawMode::Runes {
                self.display.draw_rune(&rune, &CursorPos::new(to, column));
            }
            self.content[to_index] = rune;
        }
    }

    fn clear_row(&mut self, row: i32, draw_mode: DrawMode)
    {
        for column in 0..self.columns
        {
            let index = (row * self.columns + column) as usize;
            self.content[index] = Rune::default();

            if draw_mode == DrawMode::Runes {
                self.display.draw_rune(&Rune::default(), &CursorPos::new(row, column));
            }
        }
    }

    fn scroll_up(&mut self, mut amount: i32, top: i32, bottom: i32)
    {
        amount = min(amount, bottom - top);
        let start_row = top + amount;
        let end_row = bottom;
        
        for row in (start_row..end_row).rev() {
            self.move_row(row, row - amount, DrawMode::Nothing);
        }
        for row in top..start_row {
            self.clear_row(row, DrawMode::Nothing);
        }
    }

    fn scroll_down(&mut self, mut amount: i32, top: i32, bottom: i32)
    {
        amount = min(amount, bottom - top);
        let start_row = top;
        let end_row = bottom - amount;

        // Copy old rows into scrollback
        for row in start_row..(start_row + amount) 
        {
            for column in 0..self.columns 
            {
                let index = (row * self.columns + column) as usize;
                self.scrollback.push(self.content[index].clone());
            }
            self.scrollback_rows += 1;
        }

        // Move rows up
        for row in start_row..end_row {
            self.move_row(row, row + amount, DrawMode::Nothing);
        }

        // Clear new rows
        for row in end_row..bottom {
            self.clear_row(row, DrawMode::Nothing);
        }
    }

    fn scroll_in_bounds(&mut self, amount: i32, top: i32, bottom: i32)
    {
        if amount < 0 {
            self.scroll_down(-amount, top, bottom);
        } else if amount > 0 {
            self.scroll_up(amount, top, bottom);
        }

        self.display.draw_scroll(amount, 
            top, bottom);
        self.cursor_move(amount, 0);
    }

    pub fn scroll(&mut self, amount: i32)
    {
        let top = self.scroll_region_top;
        let bottom = self.scroll_region_bottom;
        self.scroll_in_bounds(amount, top, bottom);
    }

    fn out_of_bounds(&self, pos: &CursorPos) -> bool
    {
        return pos.get_row() < 0 || pos.get_row() >= self.rows || 
            pos.get_column() < 0 || pos.get_column() >= self.columns;
    }

    fn set_rune_at(&mut self, at: &CursorPos, rune: Rune)
    {
        if self.out_of_bounds(at) {
            return;
        }

        let index = (at.get_row() * self.columns + at.get_column()) as usize;
        self.display.draw_rune(&rune, at);
        self.content[index] = rune;
    }

    fn rune_from_code_point(&self, code_point: u32) -> Rune
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
        let row = self.cursor.get_row();
        for column in (start..end).rev()
        {
            let index = (row * self.columns + column) as usize;
            let rune = self.content[index - count].clone();
            self.display.draw_rune(&rune, &CursorPos::new(row, column));
            self.content[index] = rune;
        }
    }

    pub fn delete(&mut self, count: usize)
    {
        let start = self.cursor.get_column();
        let end = self.columns;
        let row = self.cursor.get_row();
        for column in start..end
        {
            let index = (row * self.columns + column) as usize;
            if column >= start - 1 + count as i32 {
                let rune = self.content[index + count].clone();
                self.display.draw_rune(&rune, &CursorPos::new(row, column));
                self.content[index] = rune;
            }
        }
    }

    pub fn insert_lines(&mut self, count: i32)
    {
        let top = self.cursor.get_row();
        let bottom = self.scroll_region_bottom;
        self.scroll_in_bounds(count, top, bottom);
    }
    
    pub fn delete_lines(&mut self, count: i32)
    {
        let top = self.cursor.get_row();
        let bottom = self.scroll_region_bottom;
        self.scroll_in_bounds(-count, top, bottom);
    }

    pub fn new_line(&mut self)
    {
        self.cursor_move(1, 0);
        self.cursor.carriage_return();
    }

    pub fn carriage_return(&mut self)
    {
        self.cursor.carriage_return();
    }
    
    /* Cursor movement */

    fn cursor_check_move(&mut self)
    {
        let row = self.cursor.get_row();
        if row >= self.scroll_region_bottom {
            self.scroll(self.scroll_region_bottom - row - 1);
        }
        if row < self.scroll_region_top - 1 {
            self.scroll(self.scroll_region_top - row);
        }

        self.cursor.clamp(self.columns, self.rows);
    }

    fn cursor_move(&mut self, row: i32, column: i32)
    {
        self.cursor.move_by(row, column);
        self.cursor_check_move();
    }
    
    pub fn cursor_set(&mut self, row: i32, column: i32) 
    { 
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

    /* Clear lines */
    
    pub fn clear_line_range(&mut self, start: i32, end: i32)
    {
        for column in start..end
        {
            let pos = CursorPos::new(self.cursor.get_row(), column);
            self.set_rune_at(&pos, self.rune_from_code_point(' ' as u32));
        }
    }

    pub fn clear_block_range(&mut self, start: i32, end: i32)
    {
        for row in start..end
        {
            for column in 0..self.columns
            {
                let pos = CursorPos::new(row, column);
                self.set_rune_at(&pos, self.rune_from_code_point(' ' as u32));
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

