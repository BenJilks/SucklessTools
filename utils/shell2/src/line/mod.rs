extern crate nix;
pub mod history;
mod autocomplete;

use history::{History, HistoryDirection};
use autocomplete::Completion;
use nix::sys::termios;
use std::io::{Read, Write};
use std::io;

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
        let mut term = termios::tcgetattr(0).unwrap();
        term.local_flags.set(termios::LocalFlags::ICANON, self.buffered_io);
        term.local_flags.set(termios::LocalFlags::ECHO, self.echo);
        termios::tcsetattr(0, termios::SetArg::TCSANOW, &term).unwrap();
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

pub struct Line<'a>
{
    buffer: Vec<char>,
    cursor: i32,
    stdout: io::Stdout,
    
    state: State,
    arg_buffer: String,
    args: Vec<i32>,
    back_buffer: String,
    
    prompt: String,
    history: &'a mut History,
}

impl<'a> Line<'a>
{

    pub fn get(prompt: &str, history: &'a mut History) -> String
    {
        let mut line = Self
        {
            buffer: Vec::new(),
            cursor: 0,
            stdout: io::stdout(),

            state: State::Default,
            arg_buffer: String::new(),
            args: Vec::new(),
            back_buffer: String::new(),

            prompt: prompt.to_owned(),
            history: history,
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

    fn type_string(&mut self, string: &str)
    {
        for c in string.as_bytes() {
            self.type_code_point(*c as i32);
        }
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

    fn change_history(&mut self, direction: HistoryDirection)
    {
        let line_or_none = self.history.get_history(direction);
        if line_or_none.is_none() {
            return;
        }

        let line = line_or_none.unwrap();
        self.home();
        self.buffer = line.chars().collect();
        self.cursor = line.len() as i32;

        print!("{}\x1b[K", line);
        self.stdout.flush().unwrap();
    }

    fn replace_word(&mut self, start: usize, end: usize, with: &str)
    {
        for _ in start..end {
            self.buffer.remove(start);
        }
        for c in with.chars().rev() {
            self.buffer.insert(start, c);
        }

        self.cursor = (start + with.len()) as i32;
        print!("\x1b[{0}D\x1b[{0}P", end - start);
        print!("\x1b[{}@{}", with.len(), with);
        self.stdout.flush().unwrap();
    }

    fn options(&mut self, words: Vec<String>)
    {
        println!();
        for word in words {
            print!("{} ", word);
        }

        print!("\n{}{}", self.prompt, self.buffer.iter().collect::<String>());
        print!("\x1b[G\x1b[{}C", self.prompt.len() + self.cursor as usize);
        self.stdout.flush().unwrap();
    }

    fn complete_current_word(&mut self)
    {
        let mut word_start = self.cursor as usize;
        for i in (0..self.cursor).rev()
        {
            if self.buffer[i as usize].is_whitespace() {
                break;
            }

            word_start -= 1;
        }

        let word = &self.buffer[word_start..self.cursor as usize]
            .iter().collect::<String>();
        let completion = autocomplete::complete(word);
        if completion.is_none() {
            return;
        }

        match completion.unwrap()
        {
            Completion::Single(word) => self.replace_word(word_start, self.cursor as usize, &word),
            Completion::Multiple(words) => self.options(words),
        }
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
                    0x09 => self.complete_current_word(),
                    0x7f => self.type_backspace(),
                    0x1b => self.state = State::Escape,
                    _ => self.type_code_point(code_point),
                }
            },

            State::Escape =>
            {
                match c
                {
                    '[' => 
                    {
                        self.back_buffer = "\x1b[".to_owned();
                        self.state = State::Arg
                    },
                    _ => self.type_code_point(code_point),
                }
            },

            State::Arg =>
            {
                self.back_buffer.push(c);
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
                            'A' => self.change_history(HistoryDirection::Back),
                            'B' => self.change_history(HistoryDirection::Forward),
                            'C' => self.move_cursor(CursorDirection::Right),
                            'D' => self.move_cursor(CursorDirection::Left),
                            'H' => self.home(),
                            'F' => self.end(),
                            _ => self.type_string(&self.back_buffer.clone()),
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

        let input = io::stdin().bytes();
        for next_or_error in input
        {
            if next_or_error.is_err() {
                break;
            }

            let next = next_or_error.unwrap();
            if next == '\n' as u8
            {
                println!();
                return;
            }
            
            self.parse_code_point(next as i32);
        }
    }

}
