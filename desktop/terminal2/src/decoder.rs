use super::display::buffer::*;
use super::display::rune::*;

const SHOW_ALL_ESCAPES: bool = false;

#[derive(Debug)]
enum State
{
    Ascii,
    Utf8OneByteLeft,
    Utf8TwoBytesLeft,
    Utf8ThreeBytesLeft,
    Escape,
    Private,
    Argument,
    Command,
    CommandBody,
    Bracket,
    Hash,
}

pub struct Decoder
{
    state: State,
    buffer: String,
    c: char,
    reconsume: bool,

    curr_rune: u32,
    is_private: bool,
    args: Vec<i32>,
    command: char,
}

#[derive(PartialEq)]
enum ColorType
{
    Foreground,
    Background,
}

fn decode_standard_color(code: &i32, color_type: &ColorType) -> Option<StandardColor>
{
    match code
    {
        0 => return Some( StandardColor::Black ),
        1 => return Some( StandardColor::Red ),
        2 => return Some( StandardColor::Green ),
        3 => return Some( StandardColor::Yellow ),
        4 => return Some( StandardColor::Blue ),
        5 => return Some( StandardColor::Magenta ),
        6 => return Some( StandardColor::Cyan ),
        7 => return Some( StandardColor::White ),
        9 =>
        {
            if *color_type == ColorType::Background {
                return Some( StandardColor::DefaultBackground );
            } else {
                return Some( StandardColor::DefaultForeground );
            }
        },
        _ => return None,
    }
}

fn set_color_from_type(attribute: &mut Attribute, 
    color_type: &ColorType, color: u32)
{
    match color_type
    {
        ColorType::Background => attribute.background = color,
        ColorType::Foreground => attribute.foreground = color,
    }
}

fn decode_non_16bit_color(args: &Vec<i32>, buffer: &mut Buffer)
{
    assert!(args.len() == 3);
    assert!(args[1] == 5);

    let color_type = 
        if args[0] == 38 
            { ColorType::Foreground } 
        else 
            { ColorType::Background };
    let code = args[2];
    let attribute = buffer.get_attribute();

    let color = 
        if code <= 7
        {
            // Standard colors
            let standard_color = decode_standard_color(&code, &color_type);
            standard_color.unwrap().color()
        }
        else if code >= 16 && code <= 231 
        {
            // Cube colors
            let cube = code - 16;
            let rf = (cube / 36) as f32 / 6.0;
            let gf = (cube / 6) as f32 / 6.0;
            let bf = (cube % 6) as f32 / 6.0;
            
            let r = (rf * 255.0) as u32;
            let g = (gf * 255.0) as u32;
            let b = (bf * 255.0) as u32;
            (r << 24) | (g << 16) | (b << 8)
        }
        else if code >= 232 && code <= 255
        {
            // Grayscale colors
            let factor = (code - 232) as f32 / 24.0;
            let shade = (factor * 255.0) as u32;
            (shade << 24) | (shade << 16) | (shade << 8)
        }
        else 
        {
            panic!()
        };

    set_color_from_type(attribute, 
        &color_type, color);
}

fn decode_color(mut code: i32, buffer: &mut Buffer)
{
    let original_code = code;
    let attribute = buffer.get_attribute();

    match code
    {
        // Reset all attributes
        0 =>
        {
            attribute.background = StandardColor::DefaultBackground.color();
            attribute.foreground = StandardColor::DefaultForeground.color();
            //m_flags = 0;
            return;
        },

        1 => 
        {
            //m_flags |= Flag::Bold;
            return;
        },

        7 | 27 =>
        {
            *attribute = attribute.inverted();
            return;
        },

        _ => {}
    }

    let mut color_type = ColorType::Foreground;
    if code >= 90
    {
        // TODO: Bright
        code -= 60;
    }

    code -= 30;
    if code >= 10
    {
        color_type = ColorType::Background;
        code -= 10;
    }

    let color = decode_standard_color(&code, &color_type);
    if color.is_none() 
    {
        println!("Invalid attribute: {}", original_code);
        return;
    }

    set_color_from_type(attribute, 
        &color_type, color.unwrap().color());
}

impl Decoder
{

    pub fn new() -> Self
    {
        return Self
        {
            state: State::Ascii,
            buffer: String::new(),
            c: '\0',
            reconsume: false,

            curr_rune: 0,
            is_private: false,
            args: Vec::new(),
            command: '\0',
        };
    }

    fn finish_argument(&mut self)
    {
        if self.buffer.is_empty() {
            return;
        }

        self.args.push(self.buffer.parse::<i32>().unwrap());
        self.buffer.clear();
    }

    fn finish_escape(&mut self, buffer: &mut Buffer, response: &mut Vec<u8>)
    {
        let args = &self.args;
        let def = |val| { if args.len() >= 1 {args[0]} else {val} };
        let def_or_zero = |val| { if def(val) == 0 {val} else {def(val)} };

        match self.command
        {
            'm' =>
            {
                if args.len() == 0 
                {
                    decode_color(0, buffer);
                } 
                else if args[0] == 38 || args[0] == 48 
                {
                    decode_non_16bit_color(args, buffer);
                } 
                else 
                {
                    for color_code in args {
                        decode_color(*color_code, buffer);
                    }
                }
            },

            // Horizontal and Vertical Position [row;column] (default = [1,1]) (HVP)
            'H' | 'f' =>
            {
                let row = if args.len() == 2 {args[0] - 1} else {0};
                let column = if args.len() == 2 {args[1] - 1} else {0};
                buffer.cursor_set(row, column);
            },

            'A' => buffer.cursor_up(def_or_zero(1)),
            'B' => buffer.cursor_down(def_or_zero(1)),
            'C' => buffer.cursor_right(def_or_zero(1)),
            'D' => buffer.cursor_left(def_or_zero(1)),
            
            'K' =>
            {
                match def(0)
                {
                    0 => buffer.clear_from_cursor_right(),
                    1 => buffer.clear_from_cursor_left(),
                    2 => buffer.clear_whole_line(),
                    3 => {},
                    _ => assert!(false),
                }
            },

            'J' =>
            {
                match def(0)
                {
                    0 => buffer.clear_from_cursor_down(),
                    1 => buffer.clear_from_cursor_up(),
                    2 => buffer.clear_whole_screen(),
                    3 => {},
                    _ => assert!(false),
                }
            },

            // Line Position Absolute [row] (default = [1,column]) (VPA)
            'd' => buffer.cursor_set_row(def(1) - 1),
            
            // Cursor Character Absolute [column] (default = [row,1]) (CHA)
            'G' => buffer.cursor_set_column(def(1) - 1),

            '@' => buffer.insert(def(1) as usize),
            'P' => buffer.delete(def(1) as usize),

            'L' => buffer.insert_lines(def(1)),

            'r' =>
            {
                let top = if args.len() == 2 {args[0] - 1} else {0};
                let bottom = if args.len() == 2 {args[1]} else {buffer.get_rows()};
                buffer.set_scroll_region(top, bottom);
            },

            'c' =>
            {
                assert!(def(0) == 0);
                response.append(&mut "\x1b[1;2c"
                    .as_bytes()
                    .to_vec());
            },

            'X' =>
            {
                let count = def(1);
                for _ in 0..count {
                    buffer.type_rune(' ' as u32);
                }
            }

            _ => 
            {
                println!("Unkown escape {}{:?}{}", 
                    if self.is_private {"?"} else {""}, args, self.command);
            },
        }

        if SHOW_ALL_ESCAPES 
        {
            println!("Escape {}{:?}{}", 
                if self.is_private {"?"} else {""}, args, self.command);
        }

        self.is_private = false;
        self.args.clear();
        self.command = '\0';
    }

    fn finish_single_char_code(&mut self, buffer: &mut Buffer)
    {
        match self.command
        {
            'D' => buffer.cursor_down(1),
            'M' => buffer.cursor_up(1),
            'E' => buffer.type_special(Special::NewLine),

            _ =>
            {
                println!("Unkown escape '{}'", self.command);
            }
        }

        self.command = '\0';
    }

    fn finish_bracket(&mut self)
    {
        match self.command
        {
            _ =>
            {
                println!("Unkown bracket escape {}", 
                    self.command);
            }
        }
        
        if SHOW_ALL_ESCAPES {
            println!("Escape {}", self.command);
        }
        self.command = '\0';
    }

    fn finish_hash(&mut self, buffer: &mut Buffer)
    {
        match self.command
        {
            '8' =>
            {
                buffer.fill('E' as u32);
            },

            _ =>
            {
                println!("Unkown bracket escape {}", 
                    self.command);
            }
        }
        self.command = '\0';
    }

    fn finish_command(&mut self)
    {
        println!("Command {:?} {}", self.args, self.buffer);
        self.buffer.clear();
        self.args.clear();
    }

    pub fn decode(&mut self, output: &[u8], count_read: i32, buffer: &mut Buffer) -> Vec<u8>
    {
        let mut response = Vec::<u8>::new();
        let mut i = 0;

        loop
        {
            if !self.reconsume
            {
                if i >= count_read {
                    return response;
                }
                
                self.c = output[i as usize] as char;
                i += 1;
            }
            self.reconsume = false;
           
            match self.state
            {
                
                State::Ascii =>
                {
                    if (self.c as u32 & 0b11100000) == 0b11000000
                    {
                        self.curr_rune = (self.c as u32 & 0b00011111) << 6;
                        self.state = State::Utf8OneByteLeft;
                    }
                    else if (self.c as u32 & 0b11110000) == 0b11100000
                    {
                        self.curr_rune = (self.c as u32 & 0b00001111) << 12;
                        self.state = State::Utf8TwoBytesLeft;
                    }
                    else if (self.c as u32 & 0b11111000) == 0b11110000
                    {
                        self.curr_rune = (self.c as u32 & 0b00000111) << 18;
                        self.state = State::Utf8ThreeBytesLeft;
                    }
                    else
                    {
                        match self.c
                        {
                            '\n' => buffer.type_special(Special::NewLine),
                            '\r' => buffer.type_special(Special::Return),
                            '\x07' => println!("Bell!!!"),
                            '\x08' => buffer.cursor_left(1),
                            '\x1b' => self.state = State::Escape,
                            _ => buffer.type_rune(self.c as u32),
                        }
                    }
                },

                State::Utf8OneByteLeft =>
                {
                    if (self.c as u32 & 0b11000000) != 0b10000000
                    {
                        println!("Invalid UTF-8");
                        self.state = State::Ascii;
                        self.reconsume = true;
                    }
                    else
                    {
                        self.curr_rune |= self.c as u32 & 0b00111111;
                        self.state = State::Ascii;
                        buffer.type_rune(self.curr_rune);
                    }
                },
                
                State::Utf8TwoBytesLeft =>
                {
                    if (self.c as u32 & 0b11000000) != 0b10000000
                    {
                        println!("Invalid UTF-8");
                        self.state = State::Ascii;
                        self.reconsume = true;
                    }
                    else
                    {
                        self.curr_rune |= (self.c as u32 & 0b00111111) << 6;
                        self.state = State::Utf8OneByteLeft;
                    }
                },

                State::Utf8ThreeBytesLeft =>
                {
                    if (self.c as u32 & 0b11000000) != 0b10000000
                    {
                        println!("Invalid UTF-8");
                        self.state = State::Ascii;
                        self.reconsume = true;
                    }
                    else
                    {
                        self.curr_rune |= (self.c as u32 & 0b00111111) << 12;
                        self.state = State::Utf8TwoBytesLeft;
                    }
                },

                State::Escape =>
                {
                    match self.c
                    {
                        '[' => self.state = State::Private,
                        ']' => self.state = State::Command,
                        '(' => self.state = State::Bracket,
                        '#' => self.state = State::Hash,
                        _ =>
                        {
                            self.state = State::Ascii;
                            self.command = self.c;
                            self.finish_single_char_code(buffer);
                        },
                    }
                },

                State::Private =>
                {
                    match self.c
                    {
                        '?' => self.is_private = true,
                        _ => 
                        {
                            self.is_private = false;
                            self.reconsume = true;
                        },
                    }
                    self.state = State::Argument;
                },

                State::Argument =>
                {
                    match self.c
                    {
                        '0'..='9' => self.buffer.push(self.c),
                        ';' => self.finish_argument(),
                        ' ' => {},
                        '\x1b' => 
                        {
                            self.state = State::Ascii;
                            self.reconsume = true;
                        },
                        _ =>
                        {
                            self.state = State::Ascii;
                            self.command = self.c;
                            self.finish_argument();
                            self.finish_escape(buffer, &mut response);
                        },
                    }
                },

                State::Command =>
                {
                    match self.c
                    {
                        '0'..='9' => self.buffer.push(self.c),
                        '\x07' => 
                        {
                            self.finish_argument();
                            self.finish_command();
                            self.state = State::Ascii;
                        },
                        ';' =>
                        {
                            self.finish_argument();
                            self.state = State::CommandBody;
                        },
                        _ => 
                        {
                            println!("{}", self.c as u8);
                            assert!(false);
                        },
                    }
                },

                State::CommandBody =>
                {
                    match self.c
                    {
                        '\x07' =>
                        {
                            self.finish_command();
                            self.state = State::Ascii;
                        },
                        _ => self.buffer.push(self.c),
                    }
                },

                State::Bracket =>
                {
                    self.state = State::Ascii;
                    self.command = self.c;
                    self.finish_bracket();
                },

                State::Hash =>
                {
                    self.state = State::Ascii;
                    self.command = self.c;
                    self.finish_hash(buffer);
                },
                
            }
        }
    }

}

