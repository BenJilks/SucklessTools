use super::buffer::*;
use super::display::rune::*;
use super::display;
use std::io::Write;

const SHOW_STREAM_LOG: bool = false;
const SHOW_UNKOWN_ESCAPES: bool = false;
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

fn decode_standard_color(code: &i32, color_type: &ColorType) -> Option<StandardColor>
{
    match code
    {
        0 => Some( StandardColor::Black ),
        1 => Some( StandardColor::Red ),
        2 => Some( StandardColor::Green ),
        3 => Some( StandardColor::Yellow ),
        4 => Some( StandardColor::Blue ),
        5 => Some( StandardColor::Magenta ),
        6 => Some( StandardColor::Cyan ),
        7 => Some( StandardColor::White ),
        9 =>
        {
            if *color_type == ColorType::Background {
                Some( StandardColor::DefaultBackground )
            } else {
                Some( StandardColor::DefaultForeground )
            }
        },
        _ => None,
    }
}

fn decode_non_16color<Display>(args: &Vec<i32>, buffer: &mut Buffer<Display>)
    where Display: display::Display
{
    assert!(args.len() == 3);
    assert!(args[1] == 5);

    let color_type = 
        if args[0] == 38 { 
            ColorType::Foreground 
        } else {
            ColorType::Background 
        };
    let code = args[2];
    let attribute = buffer.get_attribute();

    *attribute.from_type(&color_type) =
        if code <= 7
        {
            // Standard 
            let standard_color = decode_standard_color(&code, &color_type);
            standard_color.unwrap().color()
        }
        else if (16..=231).contains(&code) 
        {
            // Cube 
            let cube = code - 16;
            let rf = (cube / 36) as f32 / 6.0;
            let gf = ((cube % 36) / 6) as f32 / 6.0;
            let bf = (cube % 6) as f32 / 6.0;
            
            let r = (rf * 255.0) as u32;
            let g = (gf * 255.0) as u32;
            let b = (bf * 255.0) as u32;
            (r << 24) | (g << 16) | (b << 8)
        }
        else if (232..=255).contains(&code)
        {
            // Grayscale 
            let factor = (code - 232) as f32 / 24.0;
            let shade = (factor * 255.0) as u32;
            (shade << 24) | (shade << 16) | (shade << 8)
        }
        else 
        {
            if SHOW_UNKOWN_ESCAPES {
                println!("Invalid 246color code {}", code);
            }
            0
        };
}

fn decode_color<Display>(mut code: i32, buffer: &mut Buffer<Display>)
    where Display: display::Display
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
        if SHOW_UNKOWN_ESCAPES {
            println!("Invalid attribute: {}", original_code);
        }
        return;
    }

    *attribute.from_type(&color_type) = color.unwrap().color();
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

    fn finish_escape<Display>(&mut self, buffer: &mut Buffer<Display>, response: &mut Vec<u8>)
        where Display: display::Display
    {
        let args = &self.args;
        let def = |val| { if !args.is_empty() {args[0]} else {val} };
        let def_or_zero = |val| { if def(val) == 0 {val} else {def(val)} };

        match self.command
        {
            'm' =>
            {
                if args.is_empty() 
                {
                    decode_color(0, buffer);
                } 
                else if args[0] == 38 || args[0] == 48 
                {
                    decode_non_16color(args, buffer);
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
                    _ => panic!(),
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
                    _ => panic!(),
                }
            },

            // Line Position Absolute [row] (default = [1,column]) (VPA)
            'd' => buffer.cursor_set_row(def(1) - 1),
            
            // Cursor Character Absolute [column] (default = [row,1]) (CHA)
            'G' => buffer.cursor_set_column(def(1) - 1),

            '@' => buffer.insert(def(1) as usize),
            'P' => buffer.delete(def(1) as usize),

            'L' => buffer.insert_lines(def(1)),
            'M' => buffer.delete_lines(def(1)),
            'X' => buffer.erase(def(1)),

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
            
            _ => 
            {
                if SHOW_UNKOWN_ESCAPES
                {
                    println!("Unkown escape {}{:?}{}", 
                        if self.is_private {"?"} else {""}, args, self.command);
                }
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

    fn finish_single_char_code<Display>(&mut self, buffer: &mut Buffer<Display>)
        where Display: display::Display
    {
        match self.command
        {
            'D' => buffer.cursor_down(1),
            'M' => buffer.cursor_up(1),
            'E' => buffer.new_line(),

            _ =>
            {
                if SHOW_UNKOWN_ESCAPES {
                    println!("Unkown escape '{}'", self.command);
                }
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
                if SHOW_UNKOWN_ESCAPES
                {
                    println!("Unkown bracket escape {}", 
                        self.command);
                }
            }
        }
        
        if SHOW_ALL_ESCAPES {
            println!("Escape {}", self.command);
        }
        self.command = '\0';
    }

    fn finish_hash<Display>(&mut self, buffer: &mut Buffer<Display>)
        where Display: display::Display
    {
        match self.command
        {
            '8' =>
            {
                buffer.fill('E' as u32);
            },

            _ =>
            {
                if SHOW_UNKOWN_ESCAPES
                {
                    println!("Unkown bracket escape {}", 
                        self.command);
                }
            }
        }
        self.command = '\0';
    }

    fn finish_command(&mut self)
    {
        if SHOW_UNKOWN_ESCAPES {
            println!("Command {:?} {}", self.args, self.buffer);
        }

        self.buffer.clear();
        self.args.clear();
    }

    pub fn decode<Display>(&mut self, output: &[u8], buffer: &mut Buffer<Display>) -> Vec<u8>
        where Display: display::Display
    {
        let mut response = Vec::<u8>::new();
        let mut i = 0;

        loop
        {
            if !self.reconsume
            {
                if i >= output.len() {
                    break;
                }
                
                self.c = output[i as usize] as char;
                i += 1;
            
                if SHOW_STREAM_LOG 
                {
                    match self.c
                    {
                        '\x1b' => print!("\\033"),
                        '\r' => print!("\\r"),
                        '\n' => print!("\\n"),
                        '\t' => print!("\\t"),
                        _ => print!("{}", self.c),
                    }
                    std::io::stdout().flush().unwrap();
                }
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
                            '\n' => buffer.new_line(),
                            '\r' => buffer.carriage_return(),
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
                            panic!();
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

        buffer.flush();
        return response;
    }

}

