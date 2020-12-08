
#[derive(Clone, PartialEq, Debug)]
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
    }

    pub fn move_to(&mut self, row: i32, column: i32)
    {
        self.row = row;
        self.column = column;
    }

    pub fn clamp(&mut self, width: i32, height: i32)
    {
        self.row = 
            if self.row < 0 { 0 }
            else if self.row > height { 0 }
            else { self.row };

        self.column = 
            if self.column < 0 { 0 }
            else if self.column > width { 0 }
            else { self.column };
    }

    pub fn carriage_return(&mut self)
    {
        self.column = 0;
    }

    pub fn get_row(&self) -> i32 { self.row }
    pub fn get_column(&self) -> i32 { self.column }

}

