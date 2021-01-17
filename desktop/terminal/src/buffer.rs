use std::cmp::min;
use std::rc::Rc;
use std::cell::RefCell;
use super::display::
{
    self, 
    cursor::*,
    rune::*,
};

struct Line
{
    content: Vec<Rune>,
    changes: Vec<bool>,
    columns: usize,
    dirty: bool,
}
type LineRef = Rc<RefCell<Line>>;

impl Line
{
    pub fn new(columns: i32) -> LineRef
    {
        return Rc::from(RefCell::new(Self
        {
            content: vec![Rune::default(); columns as usize],
            changes: vec![true; columns as usize],
            columns: columns as usize,
            dirty: true,
        }));
    }

    pub fn resize(&mut self, columns: i32)
    {
        if columns == self.content.len() as i32 {
            return;
        }

        self.content.resize(columns as usize, Rune::default());
        self.changes.resize(columns as usize, true);
        self.columns = columns as usize;
    }

    pub fn clear(&mut self)
    {
        for i in 0..self.columns {
            self.content[i] = Rune::default();
            self.changes[i] = true;
        }
        self.dirty = true;
    }

    pub fn set(&mut self, column: i32, rune: Rune)
    {
        self.content[column as usize] = rune;
        self.changes[column as usize] = true;
        self.dirty = true;
    }

    pub fn get(&self, column: i32) -> &Rune
    {
        &self.content[column as usize]
    }

    pub fn draw(&mut self, row: i32) -> Vec<(Rune, CursorPos)>
    {
        if !self.dirty {
            return Vec::new();
        }

        let mut runes = Vec::<(Rune, CursorPos)>::new();
        runes.reserve(self.columns);
        for i in 0..self.columns 
        {
            if self.changes[i]
            {
                let pos = CursorPos::new(row, i as i32);
                runes.push((self.content[i].clone(), pos));
                self.changes[i] = false;
            }
        }
        self.dirty = false;
        return runes;
    }

    pub fn invalidate(&mut self)
    {
        for it in &mut self.changes {
            *it = true;
        }
        self.dirty = true;
    }
}

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
    scroll_buffer: i32,
    
    content: Vec<LineRef>,
    scrollback: Vec<LineRef>,
    viewport_offset: i32,
}

impl<Display> Buffer<Display>
    where Display: display::Display
{
    
    pub fn new(rows: i32, columns: i32, display: Display) -> Self
    {
        let mut buffer = Self
        {
            cursor: CursorPos::new(0, 0),
            attribute: Attribute::default(),
            display: display,

            rows: rows,
            columns: columns,
            scroll_region_top: 0,
            scroll_region_bottom: rows,
            scroll_buffer: 0,

            content: vec![Line::new(columns); (rows * columns) as usize],
            scrollback: Vec::new(),
            viewport_offset: 0,
        };

        for row in 0..rows {
            buffer.content[row as usize] = Line::new(columns);
        }
        return buffer;
    }

    pub fn get_rows(&self) -> i32 { self.rows }
    pub fn get_attribute(&mut self) -> &mut Attribute { &mut self.attribute }
    pub fn get_display(&mut self) -> &mut Display { &mut self.display }

    fn line_for_row(&mut self, row_in_viewport: i32) -> Option<&mut LineRef>
    {
        let row_in_buffer = row_in_viewport - self.viewport_offset;
        if row_in_buffer >= self.rows as i32 {
            return None;
        }

        if row_in_buffer >= 0
        {
            let line = &mut self.content[row_in_buffer as usize];
            return Some( line );
        }
        else if -row_in_buffer <= self.scrollback.len() as i32
        {
            let row_in_scrollback = self.scrollback.len() as i32 + row_in_buffer;
            let line = &mut self.scrollback[row_in_scrollback as usize];
            return Some( line );
        }
        return None;
    }

    fn draw_row(&mut self, row_in_viewport: i32) -> Vec<(Rune, CursorPos)>
    {
        let line = self.line_for_row(row_in_viewport);
        if line.is_none() {
            return Vec::new();
        }
        return line.unwrap().borrow_mut().draw(row_in_viewport);
    }

    fn invalidate_row(&mut self, row_in_viewport: i32)
    {
        let line = self.line_for_row(row_in_viewport);
        if line.is_some() {
            line.unwrap().borrow_mut().invalidate();
        }
    }

    pub fn draw(&mut self) -> Vec<(Rune, CursorPos)>
    {
        let mut runes = Vec::<(Rune, CursorPos)>::new();
        runes.reserve((self.columns * self.rows) as usize);
        for row in 0..self.rows {
            runes.append(&mut self.draw_row(row));
        }
        return runes;
    }

    pub fn redraw(&mut self)
    {
        for row in 0..self.rows {
            self.invalidate_row(row);
        }
        self.flush();
    }

    fn flush_scroll_buffer(&mut self)
    {
        if self.scroll_buffer != 0
        {
            self.display.draw_scroll(self.scroll_buffer, 
                self.scroll_region_top, self.scroll_region_bottom);
            self.scroll_buffer = 0;
        }
    }

    pub fn flush(&mut self)
    {
        self.flush_scroll_buffer();
        let mut cursor = self.cursor.clone();
        cursor.move_by(self.viewport_offset, 0);

        let mut runes = self.draw();
        if (0..self.rows).contains(&cursor.get_row())
        {
            let rune = self.rune_at_cursor(&cursor).clone();
            let mut inverted_rune = rune.clone();
            inverted_rune.attribute = inverted_rune.attribute.inverted();
            runes.push((inverted_rune, cursor.clone()));

            self.display.draw_runes(&runes);
            self.display.flush();
            self.display.draw_runes(&[(rune, cursor)]);
        }
        else
        {
            self.display.draw_runes(&runes);
            self.display.flush();
        }
    }

    fn rune_at_cursor(&mut self, pos: &CursorPos) -> Rune
    {
        let line = &mut self.content[pos.get_row() as usize].borrow_mut();
        return line.get(pos.get_column()).clone();
    }
    fn set_rune_at_cursor(&mut self, pos: &CursorPos, rune: Rune)
    {
        let line = &mut self.content[pos.get_row() as usize].borrow_mut();
        line.set(pos.get_column(), rune);
    }

    pub fn resize(&mut self, rows: i32, columns: i32)
    {
        // Noop
        if self.rows == rows && self.columns == columns {
            return;
        }

        // Update main content
        self.content.resize(rows as usize, Line::new(columns));
        for line in &mut self.content {
            line.borrow_mut().resize(columns);
        }
        
        // Update scrollback
        // TODO: Actually resize this instead of just resetting
        self.scrollback.clear();

        // Update sizes
        self.scroll_region_top = 0;
        self.scroll_region_bottom = rows;
        self.rows = rows;
        self.columns = columns;
    }

    pub fn set_scroll_region(&mut self, top: i32, bottom: i32)
    {
        self.flush_scroll_buffer();
        self.scroll_region_top = top;
        self.scroll_region_bottom = bottom;
    }

    fn scroll_up(&mut self, mut amount: i32, top: i32, bottom: i32)
    {
        amount = min(amount, bottom - top);
        let start_row = top + amount;
        let end_row = bottom;
        
        for row in (start_row..end_row).rev() {
            self.content[row as usize] = self.content[(row - amount) as usize].clone();
        }
        for row in top..start_row {
            self.content[row as usize] = Line::new(self.columns);
        }
    }

    fn scroll_down(&mut self, mut amount: i32, top: i32, bottom: i32)
    {
        amount = min(amount, bottom - top);
        let start_row = top;
        let end_row = bottom - amount;

        // Copy old rows into scrollback
        let old_start_index = top as usize;
        let old_end_index = (top + amount) as usize;
        self.scrollback.append(&mut self.content[old_start_index..old_end_index].to_vec());

        // Move rows up
        for row in start_row..end_row {
            self.content[row as usize] = self.content[(row + amount) as usize].clone();
        }

        // Clear new rows
        for row in end_row..bottom {
            self.content[row as usize] = Line::new(self.columns);
        }
    }

    fn scroll_in_bounds(&mut self, amount: i32, top: i32, bottom: i32)
    {
        if amount < 0 {
            self.scroll_down(-amount, top, bottom);
        } else if amount > 0 {
            self.scroll_up(amount, top, bottom);
        }

        if top == self.scroll_region_top && bottom == self.scroll_region_bottom {
            self.scroll_buffer += amount;
        } else {
            self.display.draw_scroll(amount, top, bottom);
        }
        self.cursor_move(amount, 0);
    }

    pub fn scroll(&mut self, amount: i32)
    {
        let top = self.scroll_region_top;
        let bottom = self.scroll_region_bottom;
        self.scroll_in_bounds(amount, top, bottom);
    }

    pub fn scroll_viewport(&mut self, amount: i32)
    {
        // Move viewport
        self.viewport_offset += amount;
        
        // If we scroll more then a screen height, 
        // just redraw the whole thing
        if i32::abs(amount) > self.rows
        {
            self.redraw();
            return;
        }

        // Otherwise, only redraw what we need to
        self.display.draw_scroll(amount, 0, self.rows);
        let range = 
            if amount < 0 { self.rows+amount..self.rows }
            else { 0..amount };
        for row in range {
            self.invalidate_row(row);
        }

        self.flush();
    }

    pub fn reset_viewport(&mut self)
    {
        if self.viewport_offset != 0 {
            self.scroll_viewport(-self.viewport_offset);
        }
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

        self.set_rune_at_cursor(at, rune);
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

        let line = &mut self.content[row as usize].borrow_mut();
        for column in (start..end).rev()
        {
            let rune = line.get(column - count as i32).clone();
            line.set(column, rune);
        }
    }

    pub fn delete(&mut self, count: usize)
    {
        let start = self.cursor.get_column();
        let end = self.columns - count as i32;
        let row = self.cursor.get_row();

        let line = &mut self.content[row as usize].borrow_mut();
        for column in start..end
        {
            let rune = line.get(column + count as i32).clone();
            line.set(column, rune);
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
    
    fn clear_line_range(&mut self, start: i32, end: i32)
    {
        let row = self.cursor.get_row();
        if !(0..=self.rows).contains(&row) 
            || !(0..=self.columns).contains(&start) 
            || !(0..=self.columns).contains(&end) 
        {
            return;
        }

        let line = &mut self.content[row as usize].borrow_mut();
        for column in start..end {
            line.set(column, self.rune_from_code_point(' ' as u32));
        }
        self.display.draw_clear(&self.attribute, row, start, end - start, 1);
    }

    fn clear_block_range(&mut self, start: i32, end: i32)
    {
        if !(0..=self.rows).contains(&start) 
            || !(0..=self.rows).contains(&end) 
        {
            return;
        }
        
        for row in start..end {
            self.content[row as usize].borrow_mut().clear();
        }
        self.display.draw_clear(&self.attribute, start, 0, self.columns, end - start);
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

    pub fn erase(&mut self, count: i32) 
    {
        let column = self.cursor.get_column();
        self.clear_line_range(column, column + count);
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

