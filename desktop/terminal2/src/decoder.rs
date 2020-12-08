use super::display::buffer::*;
use super::display::rune::*;

#[derive(Debug)]
enum State
{
    Initial,
    Escape,
    Private,
    Argument,
    Command,
    CommandBody,
    Bracket,
}

pub struct Decoder
{
    state: State,
    buffer: String,
    c: char,
    reconsume: bool,

    is_private: bool,
    args: Vec<i32>,
    command: char,
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
            attribute.background = Color::DefaultBackground;
            attribute.foreground = Color::DefaultForeground;
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

    #[derive(PartialEq)]
    enum ColorType
    {
        Foreground,
        Background,
    };

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

    let color: Color;
    match code
    {
        0 => color = Color::Black,
        1 => color = Color::Red,
        2 => color = Color::Green,
        3 => color = Color::Yellow,
        4 => color = Color::Blue,
        5 => color = Color::Magenta,
        6 => color = Color::Cyan,
        7 => color = Color::White,
        8 | 9 =>
        {
            if color_type == ColorType::Background {
                color = Color::DefaultBackground;
            } else {
                color = Color::DefaultForeground;
            }
        },
        _ =>
        {
            println!("Invalid attribute: {}", original_code);
            return;
        }
    }

    match color_type
    {
        ColorType::Background => attribute.background = color,
        ColorType::Foreground => attribute.foreground = color,
    }
}

impl Decoder
{

    pub fn new() -> Self
    {
        return Self
        {
            state: State::Initial,
            buffer: String::new(),
            c: '\0',
            reconsume: false,

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

    fn finish_escape(&mut self, buffer: &mut Buffer)
    {
        let args = &self.args;
        let def = |val| { if args.len() >= 1 {args[0]} else {val} };

        match self.command
        {
            'm' =>
            {
                if args.len() == 0 {
                    decode_color(0, buffer);
                }
                for color_code in args {
                    decode_color(*color_code, buffer);
                }
            },

            // Horizontal and Vertical Position [row;column] (default = [1,1]) (HVP)
            'H' | 'f' =>
            {
                let row = if args.len() == 2 {args[0] - 1} else {0};
                let column = if args.len() == 2 {args[1] - 1} else {0};
                buffer.cursor_set(row, column);
            },

            'A' => buffer.cursor_up(def(1)),
            'B' => buffer.cursor_down(def(1)),
            'C' => buffer.cursor_right(def(1)),
            'D' => buffer.cursor_left(def(1)),
            
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
                let bottom = if args.len() == 2 {args[1] - 1} else {buffer.get_rows()};
                buffer.set_scroll_region(top, bottom);
            },

            _ => 
            {
                println!("Unkown escape {}{:?}{}", 
                    if self.is_private {"?"} else {""}, args, self.command);
            },
        }

        println!("Escape {}{:?}{}", 
            if self.is_private {"?"} else {""}, args, self.command);

        self.is_private = false;
        self.args.clear();
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
        self.command = '\0';
    }

    fn finish_command(&mut self)
    {
        println!("Command {:?} {}", self.args, self.buffer);
        self.buffer.clear();
        self.args.clear();
    }

    pub fn decode(&mut self, output: &[u8], count_read: i32, buffer: &mut Buffer)
    {
        let mut i = 0;
        while i < count_read
        {
            if !self.reconsume
            {
                self.c = output[i as usize] as char;
                i += 1;
            }
            self.reconsume = false;
            
            // Enable raw stream debugging
            if false
            {
                match self.c
                { 
                    '\x1b' => print!("\\033"),
                    '\x06' => print!("\\0x06"),
                    '\x07' => print!("\\0x07"),
                    '\x08' => print!("\\b"),
                    '\r' => print!("\\r"),
                    _ => print!("{}", self.c),
                }
                use std::io::Write;
                std::io::stdout().flush().unwrap();
            }

            match self.state
            {

                State::Initial =>
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
                },

                State::Escape =>
                {
                    match self.c
                    {
                        '[' => self.state = State::Private,
                        ']' => self.state = State::Command,
                        '(' => self.state = State::Bracket,
                        _ =>
                        {
                            self.state = State::Initial;
                            self.reconsume = true;
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
                            self.state = State::Initial;
                            self.reconsume = true;
                        },
                        _ =>
                        {
                            self.state = State::Initial;
                            self.command = self.c;
                            self.finish_argument();
                            self.finish_escape(buffer);
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
                            self.state = State::Initial;
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
                            self.state = State::Initial;
                        },
                        _ => self.buffer.push(self.c),
                    }
                },

                State::Bracket =>
                {
                    self.state = State::Initial;
                    self.command = self.c;
                    self.finish_bracket();
                },
                
            }
        }
    }

}

