use std::{ mem, io::{ self, Write } };
use libc;

struct TerminalMode
{
    buffered_io: bool,
    echo: bool,
}

impl TerminalMode
{
    
    pub fn default() -> Self
    {
        let mode = Self
        {
            buffered_io: true,
            echo: true,
        };

        mode.activate();
        return mode;
    }

    pub fn editing() -> Self
    {
        let mode = Self 
        { 
            buffered_io: false,
            echo: false,
        };

        mode.activate();
        return mode;
    }

    fn activate(&self)
    {
        unsafe
        {
            let mut term: libc::termios = mem::zeroed();
            libc::tcgetattr(0, &mut term);

            let mut set_bit = |bit: u32, value| {
                if value {
                    term.c_lflag |= bit;
                } else {
                    term.c_lflag &= !bit;
                }
            };
            
            set_bit(libc::ICANON, self.buffered_io);
            set_bit(libc::ECHO, self.echo);
            libc::tcsetattr(0, libc::TCSANOW, &term);
        }
    }

}

enum State
{
    Default,
    Escape,
    Arg,
}

enum CursorDirection
{
    Left,
    Right,
}

pub struct Line
{
    buffer: Vec<char>,
    cursor: i32,
    
    state: State,
    arg_buffer: String,
    args: Vec<i32>,
    
    prompt: String,
    stdout: io::Stdout,
}

impl Line
{

    pub fn get(prompt: &str) -> String
    {
        let mut line = Self
        {
            buffer: Vec::new(),
            cursor: 0,

            state: State::Default,
            arg_buffer: String::new(),
            args: Vec::new(),

            prompt: prompt.to_owned(),
            stdout: io::stdout(),
        };

        // TODO: Make this a scope thing?
        TerminalMode::editing();
        line.main_loop();
        TerminalMode::default();

        return line.buffer.into_iter().collect();
    }

    fn type_code_point(&mut self, code_point: i32)
    {
        let c = code_point as u8 as char;
        self.buffer.insert(self.cursor as usize, c);
        self.cursor += 1;

        if self.cursor < self.buffer.len() as i32 {
            print!("\x1b[@");
        }
        print!("{}", c);
        self.stdout.flush().unwrap();
    }

    fn type_backspace(&mut self)
    {
        if self.cursor > 0
        {
            self.cursor -= 1;
            self.buffer.remove(self.cursor as usize);

            print!("\x1b[D\x1b[P");
            self.stdout.flush().unwrap();
        }
    }

    fn move_cursor(&mut self, direction: CursorDirection)
    {
        // If there's no argument, the amount is 1. If the 
        // argument is 0, it should also be 1
        let amount = 
            if self.args.len() > 0 
            { 
                if self.args[0] == 0 { 1 } 
                else { self.args[0] }
            } 
            else { 1 };
        
        match direction
        {
            CursorDirection::Right =>
            {
                let count = std::cmp::min(amount, self.buffer.len() as i32 - self.cursor);
                if count > 0
                {
                    self.cursor += count;
                    print!("\x1b[{}C", count);
                }
            },

            CursorDirection::Left =>
            {
                let count = std::cmp::min(amount, self.cursor);
                if count > 0
                {
                    self.cursor -= count;
                    print!("\x1b[{}D", count);
                }
            },
        }
        self.stdout.flush().unwrap();
    }

    fn home(&mut self)
    {
        // Start of line
        self.cursor = 0;
        print!("\x1b[G\x1b[{}C", self.prompt.len());
        self.stdout.flush().unwrap();
    }

    fn end(&mut self)
    {
        // End of line
        self.cursor = self.buffer.len() as i32;
        print!("\x1b[{}G", self.cursor + 1);
        self.stdout.flush().unwrap();
    }

    fn parse_code_point(&mut self, code_point: i32)
    {
        let c = code_point as u8 as char;
        match self.state
        {
            State::Default => 
            {
                match code_point
                {
                    0x7f => self.type_backspace(),
                    0x1b => self.state = State::Escape,
                    _ => self.type_code_point(code_point),
                }
            },

            State::Escape =>
            {
                match c
                {
                    '[' => self.state = State::Arg,
                    _ => self.type_code_point(code_point),
                }
            },

            State::Arg =>
            {
                if !c.is_digit(10) 
                {
                    // Add it to the list of agrument if it's a valid i32
                    let arg_as_int = self.arg_buffer.parse::<i32>();
                    if arg_as_int.is_ok() {
                        self.args.push(arg_as_int.unwrap());
                    }

                    // Clear the buffer for the next argument
                    self.arg_buffer.clear();

                    // If there's no more arguments left
                    if c != ';'
                    {
                        match c
                        {
                            'C' => self.move_cursor(CursorDirection::Right),
                            'D' => self.move_cursor(CursorDirection::Left),
                            'H' => self.home(),
                            'F' => self.end(),
                            _ => println!("Unkown escape {}", c),
                        }

                        // Set the state to default and clear the argument 
                        // for the next escape code
                        self.state = State::Default;
                        self.args.clear();
                    }
                }
                else
                {
                    self.arg_buffer.push(c);
                }
            },
        }
    }

    fn main_loop(&mut self)
    {
        print!("{}", self.prompt);
        self.stdout.flush().unwrap();

        loop
        {
            // Fetch next char
            let c = unsafe { libc::getchar() };
            if c == '\n' as i32
            {
                println!();
                return;
            }
            
            self.parse_code_point(c);
        }
    }

}
