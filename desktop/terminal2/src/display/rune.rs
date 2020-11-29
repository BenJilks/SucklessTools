
#[derive(Clone, PartialEq, Eq, Hash)]
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

impl Default for Attribute
{
    fn default() -> Self
    {
        return Self
        {
            background: Color::DefaultBackground,
            foreground: Color::DefaultForeground,
        };
    }
}

#[derive(Clone, PartialEq, Default)]
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

pub fn string_from_color(color: &Color) -> &'static str
{
    match color
    {
        Color::Black => return "000000FF",
        Color::Red => return "FF0000FF",
        Color::Green => return "00FF00FF",
        Color::Yellow => return "FFFF00FF",
        Color::Blue => return "0000FFFF",
        Color::Magenta => return "00FFFFFF",
        Color::Cyan => return "FF00FFFF",
        Color::White => return "FFFFFFFF",
        Color::DefaultBackground => return "000000FF",
        Color::DefaultForeground => return "FFFFFFFF",
    }
}

pub fn int_from_color(color: &Color) -> u32
{
    return u32::from_str_radix(string_from_color(color), 16)
        .expect("Invalid color");
}

