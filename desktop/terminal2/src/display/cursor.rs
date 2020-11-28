
#[derive(Clone)]
pub struct CursorPos
{
    row: i32,
    column: i32,
}

impl CursorPos
{
    
    pub fn new(row: i32, column: i32) -> Self
    {
        return Self
        {
            row: row,
            column: column,
        };
    }

    pub fn move_by(&mut self, row: i32, column: i32)
    {
        self.row += row;
        self.column += column;
        assert!(self.row >= 0 && self.column >= 0);
    }

    pub fn carriage_return(&mut self)
    {
        self.column = 0;
    }

    pub fn next_line(&mut self)
    {
        self.row += 1;
        self.column = 0;
    }

    pub fn get_row(&self) -> i32 { self.row }
    pub fn get_column(&self) -> i32 { self.column }

}

