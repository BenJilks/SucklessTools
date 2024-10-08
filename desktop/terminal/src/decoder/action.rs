use crate::buffer::rune::ColorType;

pub enum ActionType
{
    None,

    TypeCodePoint,
    Insert,
    Delete,
    Erase,

    NewLine,
    CarriageReturn,
    InsetLines,
    DeleteLines,

    CursorMovement,
    CursorSet,
    CursorSetRow,
    CursorSetColumn,

    ClearFromCursor,
    ClearLine,
    ClearScreen,

    SetScrollRegion,
    Response,
    Fill,

    SetColor,
    ColorInvert,
}

pub enum CursorDirection
{
    Left,
    Right,
    Up,
    Down,
}

pub struct Action
{
    pub action_type: ActionType,
    pub code_point: Option<u32>,

    pub cursor_direction: Option<CursorDirection>,
    pub amount: Option<i32>,

    pub row: Option<i32>,
    pub column: Option<i32>,

    pub top: Option<i32>,
    pub bottom: Option<i32>,
    pub message: Option<Vec<u8>>,

    pub color_type: Option<ColorType>,
    pub color: Option<u32>,
}

impl Default for Action
{
    fn default() -> Self
    {
        Self
        {
            action_type: ActionType::None,
            code_point: None,
            cursor_direction: None,
            amount: None,
            row: None,
            column: None,
            top: None,
            bottom: None,
            message: None,
            color_type: None,
            color: None,
        }
    }
}

impl Action
{
    pub fn type_code_point(code_point: u32) -> Self
    {
        Self
        {
            action_type: ActionType::TypeCodePoint,
            code_point: Some( code_point ),
            ..Self::default()
        }
    }
    pub fn insert(amount: i32) -> Self
    {
        Self
        {
            action_type: ActionType::Insert,
            amount: Some( amount ),
            ..Self::default()
        }
    }
    pub fn delete(amount: i32) -> Self
    {
        Self
        {
            action_type: ActionType::Delete,
            amount: Some( amount ),
            ..Self::default()
        }
    }
    pub fn erase(amount: i32) -> Self
    {
        Self
        {
            action_type: ActionType::Erase,
            amount: Some( amount ),
            ..Self::default()
        }
    }

    pub fn new_line() -> Self
    {
        Self
        {
            action_type: ActionType::NewLine,
            ..Self::default()
        }
    }
    pub fn carriage_return() -> Self
    {
        Self
        {
            action_type: ActionType::CarriageReturn,
            ..Self::default()
        }
    }
    pub fn insert_lines(amount: i32) -> Self
    {
        Self
        {
            action_type: ActionType::InsetLines,
            amount: Some( amount ),
            ..Self::default()
        }
    }
    pub fn delete_lines(amount: i32) -> Self
    {
        Self
        {
            action_type: ActionType::DeleteLines,
            amount: Some( amount ),
            ..Self::default()
        }
    }

    pub fn cursor_movement(direction: CursorDirection, amount: i32) -> Self
    {
        Self
        {
            action_type: ActionType::CursorMovement,
            cursor_direction: Some( direction ),
            amount: Some( amount ),
            ..Self::default()
        }
    }

    pub fn cursor_set(row: i32, column: i32) -> Self
    {
        Self
        {
            action_type: ActionType::CursorSet,
            row: Some( row ),
            column: Some( column ),
            ..Self::default()
        }
    }

    pub fn cursor_set_row(row: i32) -> Self
    {
        Self
        {
            action_type: ActionType::CursorSetRow,
            row: Some( row ),
            ..Self::default()
        }
    }
    pub fn cursor_set_column(column: i32) -> Self
    {
        Self
        {
            action_type: ActionType::CursorSetColumn,
            column: Some( column ),
            ..Self::default()
        }
    }

    pub fn clear_from_cursor(direction: CursorDirection) -> Self
    {
        Self
        {
            action_type: ActionType::ClearFromCursor,
            cursor_direction: Some( direction ),
            ..Self::default()
        }
    }

    pub fn clear_line() -> Self
    {
        Self
        {
            action_type: ActionType::ClearLine,
            ..Self::default()
        }
    }

    pub fn clear_screen() -> Self
    {
        Self
        {
            action_type: ActionType::ClearScreen,
            ..Self::default()
        }
    }

    pub fn set_scroll_region(top: Option<i32>, bottom: Option<i32>) -> Self
    {
        Self
        {
            action_type: ActionType::SetScrollRegion,
            top: top,
            bottom: bottom,
            ..Self::default()
        }
    }
    pub fn response(message: Vec<u8>) -> Self
    {
        Self
        {
            action_type: ActionType::Response,
            message: Some( message ),
            ..Self::default()
        }
    }
    pub fn fill(code_point: u32) -> Self
    {
        Self
        {
            action_type: ActionType::Fill,
            code_point: Some( code_point ),
            ..Self::default()
        }
    }

    pub fn set_color(color_type: ColorType, color: u32) -> Self
    {
        Self
        {
            action_type: ActionType::SetColor,
            color_type: Some( color_type ),
            color: Some( color ),
            ..Self::default()
        }
    }
    pub fn color_invert() -> Self
    {
        Self
        {
            action_type: ActionType::ColorInvert,
            ..Self::default()
        }
    }
}
