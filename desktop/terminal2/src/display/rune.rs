
#[derive(Clone, PartialEq)]
pub enum Color
{
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    DefaultBackground,
    DefaultForeground,
}

#[derive(Clone, PartialEq)]
pub struct Attribute
{
    pub background: Color,
    pub foreground: Color,
}

#[derive(Clone, PartialEq)]
pub struct Rune
{
    pub code_point: u32,
    pub attribute: Attribute,
}

impl Attribute
{

    pub fn inverted(&self) -> Self
    {
        return Self
        {
            background: self.foreground.clone(),
            foreground: self.background.clone(),
        };
    }

}

pub fn int_from_color(color: Color) -> u32
{
    match color
    {
        Color::Black => return 0x000000FF,
        Color::Red => return 0xFF0000FF,
        Color::Green => return 0x00FF00FF,
        Color::Yellow => return 0xFFFF00FF,
        Color::Blue => return 0x0000FFFF,
        Color::Magenta => return 0x00FFFFFF,
        Color::Cyan => return 0xFF00FFFF,
        Color::White => return 0xFFFFFFFF,
        Color::DefaultBackground => return 0x000000FF,
        Color::DefaultForeground => return 0xFFFFFFFF,
    }
}

