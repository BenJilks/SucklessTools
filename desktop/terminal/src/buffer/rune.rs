
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
            StandardColor::Black => 0x121212FF,
            StandardColor::Red => 0xEE494CFF,
            StandardColor::Green => 0x9EEE69FF,
            StandardColor::Yellow => 0xECEE65FF,
            StandardColor::Blue => 0x83EEEAFF,
            StandardColor::Magenta => 0xAE64EEFF,
            StandardColor::Cyan => 0x163EEEFF,
            StandardColor::White => 0xFFFFFFFF,
            StandardColor::DefaultBackground => 0x181C1FFF,
            StandardColor::DefaultForeground => 0xB7CCD3FF,
        }
    }

}

#[derive(Clone, PartialEq)]
pub struct Attribute
{
    pub background: u32,
    pub foreground: u32,
}

#[derive(Clone, PartialEq)]
pub enum ColorType
{
    Foreground,
    Background,
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

    pub fn from_type(&mut self, color_type: &ColorType) -> &mut u32
    {
        match color_type
        {
            ColorType::Background => &mut self.background,
            ColorType::Foreground => &mut self.foreground,
        }
    }

}

