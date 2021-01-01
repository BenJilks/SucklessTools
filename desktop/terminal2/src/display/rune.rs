
pub enum StandardColor
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

impl StandardColor
{
    
    pub fn color(&self) -> u32
    {
        match self
        {
            StandardColor::Black => 0x000000FF,
            StandardColor::Red => 0xFF0000FF,
            StandardColor::Green => 0x00FF00FF,
            StandardColor::Yellow => 0xFFFF00FF,
            StandardColor::Blue => 0x0000FFFF,
            StandardColor::Magenta => 0x00FFFFFF,
            StandardColor::Cyan => 0xFF00FFFF,
            StandardColor::White => 0xFFFFFFFF,
            StandardColor::DefaultBackground => 0x000000FF,
            StandardColor::DefaultForeground => 0xFFFFFFFF,
        }
    }

}

#[derive(Clone, PartialEq)]
pub struct Attribute
{
    pub background: u32,
    pub foreground: u32,
}

impl Default for Attribute
{
    fn default() -> Self
    {
        return Self
        {
            background: StandardColor::DefaultBackground.color(),
            foreground: StandardColor::DefaultForeground.color(),
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

