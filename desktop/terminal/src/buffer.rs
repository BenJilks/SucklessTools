use crate::decoder::action::{Action, ActionType, CursorDirection};
use crate::display::cursor::CursorPos;
use crate::display::rune::{Rune, Attribute};
use crate::display;
use std::cmp::min;
use std::mem::swap;
use std::rc::Rc;
use std::cell::RefCell;

#[derive(Clone)]
struct LineElement
{
    rune: Rune,
    dirty: bool,
    selected: bool,
}

impl Default for LineElement
{
    fn default() -> Self
    {
        Self
        {
            rune: Rune::default(),
            dirty: true,
            selected: false,
        }
    }
}

struct Line
{
    content: Vec<LineElement>,
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
            content: vec![LineElement::default(); columns as usize],
            columns: columns as usize,
            dirty: true,
        }));
    }

    pub fn resize(&mut self, columns: i32)
    {
        if columns == self.content.len() as i32 {
            return;
        }

        self.content.resize(columns as usize, LineElement::default());
        self.columns = columns as usize;
    }

    pub fn clear(&mut self)
    {
        for i in 0..self.columns {
            let element = &mut self.content[i];
            element.rune = Rune::default();
            element.dirty = true;
        }
        self.dirty = true;
    }

    pub fn set(&mut self, column: i32, rune: Rune)
    {
        let element = &mut self.content[column as usize];
        element.rune = rune;
        element.dirty = true;
        self.dirty = true;
    }

    pub fn set_selected(&mut self, column: i32, selected: bool)
    {
        let element = &mut self.content[column as usize];
        element.selected = selected;
        element.dirty = true;
        self.dirty = true;
    }

    pub fn get(&self, column: i32) -> &Rune
    {
        &self.content[column as usize].rune
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
            let element = &mut self.content[i];
            if element.dirty
            {
                let mut rune = element.rune.clone();
                if element.selected {
                    rune.attribute = rune.attribute.inverted();
                }

                let pos = CursorPos::new(row, i as i32);
                runes.push((rune, pos));
                element.dirty = false;
            }
        }
        self.dirty = false;
        return runes;
    }

    pub fn invalidate(&mut self)
    {
        for it in &mut self.content {
            it.dirty = true;
        }
        self.dirty = true;
    }
}

pub struct Buffer<Display>
    where Display: display::Display
{
    // State
    cursor: CursorPos,
    attribute: Attribute,
    display: Display,

    // Dimensions
    rows: i32,
    columns: i32,
    scroll_region_top: i32,
    scroll_region_bottom: i32,
    scroll_buffer: i32,

    // Selection
    selection_start_pos: CursorPos,
    selection_end_pos: CursorPos,
    in_selection: bool,

    // Data
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

            selection_start_pos: CursorPos::new(0,0),
            selection_end_pos: CursorPos::new(0,0),
            in_selection: false,

            content: vec![Line::new(columns); (rows * columns) as usize],
            scrollback: Vec::new(),
            viewport_offset: 0,
        };

        for row in 0..rows {
            buffer.content[row as usize] = Line::new(columns);
        }
        return buffer;
    }

    pub fn get_display(&mut self) -> &mut Display { &mut self.display }

    /* Drawing */

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
            line.borrow_mut().resize(self.columns);
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

    fn draw(&mut self) -> Vec<(Rune, CursorPos)>
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
        self.display.clear_screen();
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
        if cursor.in_bounds(self.rows, self.columns)
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

    /* Selection */

    fn for_each_in_selection<Func>(&mut self, callback: &mut Func)
        where Func: FnMut(&mut Line, i32)
    {
        // Order points by row first, if they're the same then by column
        let mut start = self.selection_start_pos.clone();
        let mut end = self.selection_end_pos.clone();
        if start.get_row() > end.get_row() || 
            start.get_row() == end.get_row() && start.get_column() > end.get_column()
        {
            swap(&mut start, &mut end);
        }

        // Fetch starting state
        let (mut row, mut column) = (start.get_row(), start.get_column());
        let start_line = self.line_for_row(row);
        if start_line.is_none() {
            return;
        }
        let mut line = start_line.unwrap().clone();

        // Iterate though each column of each line, right-down
        while row < end.get_row() || column <= end.get_column()
        {
            callback(&mut line.borrow_mut(), column);

            column += 1;
            if column >= self.columns {
                row += 1;
                column = 0;
                
                let new_line = self.line_for_row(row);
                if new_line.is_none() {
                    break;
                }
                line = new_line.unwrap().clone();
            }
        }
    }

    fn selection_end(&mut self)
    {
        // Noop
        if !self.in_selection {
            return;
        }

        // Deselect all
        self.for_each_in_selection(&mut move |line, column| {
            line.set_selected(column, false);
        });
        self.in_selection = false;
        self.flush();
    }

    pub fn selection_start(&mut self, row: i32, column: i32)
    {
        self.selection_end();
        self.selection_start_pos = CursorPos::new(row, column);
        self.selection_end_pos = self.selection_start_pos.clone();
    }

    pub fn selection_update(&mut self, row: i32, column: i32)
    {
        // Check for nooop
        let curr = &self.selection_end_pos;
        if curr.get_row() == row && curr.get_column() == column {
            return;
        }

        self.in_selection = true;
        self.for_each_in_selection(&mut move |line, column| {
            line.set_selected(column, false);
        });

        self.selection_end_pos = CursorPos::new(row, column);

        self.for_each_in_selection(&mut move |line, column| {
            line.set_selected(column, true);
        });
        self.flush();
    }

    pub fn selection_word(&mut self, row: i32, column: i32)
    {
        self.selection_end();
        {
            let line_ref = self.line_for_row(row).unwrap().clone();
            let line = line_ref.borrow_mut();

            let mut start = column;
            while start > 0
            {
                let c = line.get(start).code_point as u8 as char;
                if c.is_whitespace() || c == '\0'
                {
                    start += 1;
                    break;
                }
                start -= 1;
            }

            let mut end = column;
            while end < self.columns - 1
            {
                let c = line.get(end).code_point as u8 as char;
                if c.is_whitespace() || c == '\0'
                {
                    end -= 1;
                    break;
                }
                end += 1;
            }

            self.selection_start_pos = CursorPos::new(row, start);
            self.selection_end_pos = CursorPos::new(row, end);
            self.in_selection = true;
        }

        self.for_each_in_selection(&mut move |line, column| {
            line.set_selected(column, true);
        });
        self.flush();
    }

    /* Misc */

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
        if self.rows == rows && self.columns == columns 
        {
            self.redraw();
            return;
        }

        // Update main content
        self.content.resize(rows as usize, Line::new(columns));
        for i in 0..self.content.len() 
        {
            let line = &mut self.content[i];
            if i >= self.rows as usize {
                *line = Line::new(columns);
            } else {
                line.borrow_mut().resize(columns);
            }
        }

        // Update sizes
        self.scroll_region_top = 0;
        self.scroll_region_bottom = rows;
        self.rows = rows;
        self.columns = columns;
        self.redraw();
    }

    fn set_scroll_region(&mut self, top: i32, bottom: i32)
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
        self.selection_end();
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

    fn scroll(&mut self, amount: i32)
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

        if self.in_selection {
            self.selection_start_pos.move_by(amount, 0);
            self.selection_end_pos.move_by(amount, 0);
        }
        self.flush();
    }

    pub fn reset_viewport(&mut self)
    {
        self.selection_end();
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

    fn type_rune(&mut self, code_point: u32)
    {
        let cursor = self.cursor.clone();
        let rune = self.rune_from_code_point(code_point);
        self.set_rune_at(&cursor, rune);
        self.cursor_move(0, 1);
    }

    fn fill(&mut self, code_point: u32)
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

    fn insert(&mut self, count: usize)
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

    fn delete(&mut self, count: usize)
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

    fn insert_lines(&mut self, count: i32)
    {
        let top = self.cursor.get_row();
        let bottom = self.scroll_region_bottom;
        self.scroll_in_bounds(count, top, bottom);
    }
    
    fn delete_lines(&mut self, count: i32)
    {
        let top = self.cursor.get_row();
        let bottom = self.scroll_region_bottom;
        self.scroll_in_bounds(-count, top, bottom);
    }

    fn new_line(&mut self)
    {
        self.cursor_move(1, 0);
        self.cursor.carriage_return();
    }

    fn carriage_return(&mut self)
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
    
    fn cursor_set(&mut self, row: i32, column: i32) 
    { 
        self.cursor.move_to(row, column);
        self.cursor_check_move();
    }

    fn cursor_set_row(&mut self, row: i32)
    {
        self.cursor_set(row, self.cursor.get_column());
    }
    
    fn cursor_set_column(&mut self, column: i32)
    {
        self.cursor_set(self.cursor.get_row(), column);
    }

    fn cursor_left(&mut self, amount: i32) { self.cursor_move(0, -amount) }
    fn cursor_right(&mut self, amount: i32) { self.cursor_move(0, amount) }
    fn cursor_up(&mut self, amount: i32) { self.cursor_move(-amount, 0) }
    fn cursor_down(&mut self, amount: i32) { self.cursor_move(amount, 0) }

    /* Clear lines */
    
    fn clear_line_range(&mut self, start: i32, end: i32)
    {
        let row = self.cursor.get_row();
        if !(0..self.rows).contains(&row) 
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
        if !(0..self.rows).contains(&start) 
            || !(0..=self.rows).contains(&end)
        {
            return;
        }
        
        for row in start..end {
            self.content[row as usize].borrow_mut().clear();
        }
        self.display.draw_clear(&self.attribute, start, 0, self.columns, end - start);
    }

    fn clear_from_cursor_right(&mut self) {
        self.clear_line_range(self.cursor.get_column(), self.columns);
    }
    fn clear_from_cursor_left(&mut self) { 
        self.clear_line_range(0, self.cursor.get_column());
    }
    fn clear_whole_line(&mut self) { 
        self.clear_line_range(0, self.columns);
    }

    fn erase(&mut self, count: i32) 
    {
        let column = self.cursor.get_column();
        self.clear_line_range(column, column + count);
    }
    
    fn clear_from_cursor_down(&mut self) {
        self.clear_from_cursor_right();
        self.clear_block_range(self.cursor.get_row() + 1, self.rows);
    }
    fn clear_from_cursor_up(&mut self) {
        self.clear_from_cursor_left();
        self.clear_block_range(0, self.cursor.get_row());
    }
    fn clear_whole_screen(&mut self) {
        self.clear_block_range(0, self.rows);
    }

    pub fn do_action(&mut self, action: Action) -> Vec<u8>
    {
        match action.action_type
        {
            ActionType::None => panic!(),

            ActionType::TypeCodePoint => self.type_rune(action.code_point.unwrap()),
            ActionType::Insert => self.insert(action.amount.unwrap() as usize),
            ActionType::Delete => self.delete(action.amount.unwrap() as usize),
            ActionType::Erase => self.erase(action.amount.unwrap()),

            ActionType::NewLine => self.new_line(),
            ActionType::CarriageReturn => self.carriage_return(),
            ActionType::InsetLines => self.insert_lines(action.amount.unwrap()),
            ActionType::DeleteLines => self.delete_lines(action.amount.unwrap()),

            ActionType::CursorMovement =>
            {
                match action.cursor_direction.unwrap()
                {
                    CursorDirection::Up => self.cursor_up(action.amount.unwrap()),
                    CursorDirection::Down => self.cursor_down(action.amount.unwrap()),
                    CursorDirection::Left => self.cursor_left(action.amount.unwrap()),
                    CursorDirection::Right => self.cursor_right(action.amount.unwrap()),
                }
            },
            ActionType::CursorSet => self.cursor_set(action.row.unwrap(), action.column.unwrap()),
            ActionType::CursorSetRow => self.cursor_set_row(action.row.unwrap()),
            ActionType::CursorSetColumn => self.cursor_set_column(action.column.unwrap()),

            ActionType::ClearFromCursor =>
            {
                match action.cursor_direction.unwrap()
                {
                    CursorDirection::Up => self.clear_from_cursor_up(),
                    CursorDirection::Down => self.clear_from_cursor_down(),
                    CursorDirection::Left => self.clear_from_cursor_left(),
                    CursorDirection::Right => self.clear_from_cursor_right(),
                }
            },
            ActionType::ClearLine => self.clear_whole_line(),
            ActionType::ClearScreen => self.clear_whole_screen(),

            ActionType::SetScrollRegion =>
            {
                let top = action.top.unwrap_or(0);
                let bottom = action.bottom.unwrap_or(self.rows);
                self.set_scroll_region(top, bottom);
            },
            ActionType::Response => return action.message.unwrap(),
            ActionType::Fill => self.fill(action.code_point.unwrap()),

            ActionType::SetColor => *self.attribute.from_type(&action.color_type.unwrap()) = action.color.unwrap(),
            ActionType::ColorInvert => self.attribute = self.attribute.inverted(),
        };

        return Vec::new();
    }

}
