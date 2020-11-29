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
}

pub struct Decoder
{
    state: State,
    buffer: String,

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
        match self.command
        {
            'm' =>
            {
                for color_code in args {
                    decode_color(*color_code, buffer);
                }
            },

            'H' =>
            {
                let row = if args.len() == 2 {args[0] - 1} else {0};
                let column = if args.len() == 2 {args[1] - 1} else {0};
                buffer.cursor_set(row, column);
            },

            'A' => buffer.cursor_up(if args.len() == 1 {args[0]} else {1}),
            'B' => buffer.cursor_down(if args.len() == 1 {args[0]} else {1}),
            'C' => buffer.cursor_right(if args.len() == 1 {args[0]} else {1}),
            'D' => buffer.cursor_left(if args.len() == 1 {args[0]} else {1}),
            
            'K' =>
            {
                match if args.len() == 0 {0} else {self.args[0]}
                {
                    0 => buffer.clear_from_cursor_right(),
                    1 => buffer.clear_from_cursor_left(),
                    2 => buffer.clear_whole_line(),
                    _ => {},
                }
            },

            'J' =>
            {
                match if args.len() == 0 {0} else {args[0]}
                {
                    0 => buffer.clear_from_cursor_down(),
                    1 => buffer.clear_from_cursor_up(),
                    2 => buffer.clear_whole_screen(),
                    _ => {},
                }
            },

            _ => 
            {
                println!("Unkown escape {}{:?}{}", 
                    if self.is_private {"?"} else {""}, args, self.command);
            },
        }

        self.is_private = false;
        self.args.clear();
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
        let (mut i, mut c, mut reconsume) = (0, '\0', false);
        while i < count_read
        {
            if !reconsume
            {
                c = output[i as usize] as char;
                i += 1;
            }
            reconsume = false;

            match self.state
            {

                State::Initial =>
                {
                    match c
                    {
                        '\n' => buffer.type_special(Special::NewLine),
                        '\r' => buffer.type_special(Special::Return),
                        '\x07' => println!("Bell!!!"),
                        '\x08' => buffer.cursor_left(1),
                        '\x1b' => self.state = State::Escape,
                        _ => buffer.type_rune(c as u32),
                    }
                },

                State::Escape =>
                {
                    match c
                    {
                        '[' => self.state = State::Private,
                        ']' => self.state = State::Command,
                        _ =>
                        {
                            self.state = State::Initial;
                            reconsume = true;
                        },
                    }
                },

                State::Private =>
                {
                    match c
                    {
                        '?' => self.is_private = true,
                        _ => 
                        {
                            self.is_private = false;
                            reconsume = true;
                        },
                    }
                    self.state = State::Argument;
                
                },

                State::Argument =>
                {
                    match c
                    {
                        '0'..='9' => self.buffer.push(c),
                        ';' => self.finish_argument(),
                        _ =>
                        {
                            self.state = State::Initial;
                            self.command = c;
                            self.finish_argument();
                            self.finish_escape(buffer);
                        },
                    }
                },

                State::Command =>
                {
                    match c
                    {
                        '0'..='9' => self.buffer.push(c),
                        ';' =>
                        {
                            self.finish_argument();
                            self.state = State::CommandBody;
                        },
                        _ => assert!(false),
                    }
                },

                State::CommandBody =>
                {
                    match c
                    {
                        '\x07' =>
                        {
                            self.finish_command();
                            self.state = State::Initial;
                        },
                        _ => self.buffer.push(c),
                    }
                },
                
            }
        }
    }

}

