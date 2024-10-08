use crate::buffer::rune::{ColorType, StandardColor};
use std::io::Write;
use action::Action;

pub mod action;

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

fn decode_non_16color<Func>(args: &Vec<i32>, on_action: Func)
    where Func: Fn(Action)
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

    on_action(Action::set_color(color_type.clone(),
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
        })
    );
}

fn decode_color<Func>(mut code: i32, on_action: &Func)
    where Func: Fn(Action)
{
    let original_code = code;

    match code
    {
        // Reset all attributes
        0 =>
        {
            let background = StandardColor::DefaultBackground.color();
            let foreground = StandardColor::DefaultForeground.color();
            on_action(Action::set_color(ColorType::Background, background));
            on_action(Action::set_color(ColorType::Foreground, foreground));
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
            on_action(Action::color_invert());
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

    on_action(Action::set_color(color_type, color.unwrap().color()));
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

    fn finish_escape<Func>(&mut self, on_action: &Func)
        where Func: Fn(Action)
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
                    decode_color(0, on_action);
                } 
                else if args[0] == 38 || args[0] == 48 
                {
                    decode_non_16color(args, on_action);
                } 
                else 
                {
                    for color_code in args {
                        decode_color(*color_code, on_action);
                    }
                }
            },

            // Horizontal and Vertical Position [row;column] (default = [1,1]) (HVP)
            'H' | 'f' =>
            {
                let row = if args.len() == 2 {args[0] - 1} else {0};
                let column = if args.len() == 2 {args[1] - 1} else {0};
                on_action(Action::cursor_set(row, column));
            },

            'A' => on_action(Action::cursor_movement(action::CursorDirection::Up, def_or_zero(1))),
            'B' => on_action(Action::cursor_movement(action::CursorDirection::Down, def_or_zero(1))),
            'C' => on_action(Action::cursor_movement(action::CursorDirection::Right, def_or_zero(1))),
            'D' => on_action(Action::cursor_movement(action::CursorDirection::Left, def_or_zero(1))),
            
            'K' =>
            {
                match def(0)
                {
                    0 => on_action(Action::clear_from_cursor(action::CursorDirection::Right)),
                    1 => on_action(Action::clear_from_cursor(action::CursorDirection::Left)),
                    2 => on_action(Action::clear_line()),
                    3 => {},
                    _ => panic!(),
                }
            },

            'J' =>
            {
                match def(0)
                {
                    0 => on_action(Action::clear_from_cursor(action::CursorDirection::Down)),
                    1 => on_action(Action::clear_from_cursor(action::CursorDirection::Up)),
                    2 => on_action(Action::clear_screen()),
                    3 => {},
                    _ => panic!(),
                }
            },

            // Line Position Absolute [row] (default = [1,column]) (VPA)
            'd' => on_action(Action::cursor_set_row(def(1) - 1)),
            
            // Cursor Character Absolute [column] (default = [row,1]) (CHA)
            'G' => on_action(Action::cursor_set_column(def(1) - 1)),

            '@' => on_action(Action::insert(def(1))),
            'P' => on_action(Action::delete(def(1))),
            'X' => on_action(Action::erase(def(1))),

            'L' => on_action(Action::insert_lines(def(1))),
            'M' => on_action(Action::delete_lines(def(1))),

            'r' =>
            {
                let top = if args.len() == 2 { Some( args[0] - 1) } else { None };
                let bottom = if args.len() == 2 { Some( args[1] ) } else { None };
                on_action(Action::set_scroll_region(top, bottom));
            },

            'c' =>
            {
                assert!(def(0) == 0);
                on_action(Action::response("\x1b[1;2c"
                    .as_bytes()
                    .to_vec()));
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

    fn finish_single_char_code<Func>(&mut self, on_action: &Func)
        where Func: Fn(Action)
    {
        match self.command
        {
            'D' => on_action(Action::cursor_movement(action::CursorDirection::Down, 1)),
            'M' => on_action(Action::cursor_movement(action::CursorDirection::Up, 1)),
            'E' => on_action(Action::new_line()),

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

    fn finish_hash<Func>(&mut self, on_action: &Func)
        where Func: Fn(Action)
    {
        match self.command
        {
            '8' => on_action(Action::fill('E' as u32)),

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

    pub fn decode<Func>(&mut self, output: &[u8], on_action: &Func)
        where Func: Fn(Action)
    {
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
                            '\n' => on_action(Action::new_line()),
                            '\r' => on_action(Action::carriage_return()),
                            '\x07' => println!("Bell!!!"),
                            '\x08' => on_action(Action::cursor_movement(action::CursorDirection::Left, 1)),
                            '\x1b' => self.state = State::Escape,
                            _ => on_action(Action::type_code_point(self.c as u32)),
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
                        on_action(Action::type_code_point(self.curr_rune));
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
                            self.finish_single_char_code(on_action);
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
                            self.finish_escape(on_action);
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
                    self.finish_hash(on_action);
                },
                
            }
        }
    }

}
