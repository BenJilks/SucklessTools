use super::display::buffer::*;

enum State
{
    Initial,
    Escape,
}

pub struct Decoder
{
    state: State,
}

impl Decoder
{

    pub fn new() -> Self
    {
        return Self
        {
            state: State::Initial,
        };
    }

    pub fn decode(&mut self, output: &[u8], count_read: i32, buffer: &mut Buffer)
    {
        for i in 0..count_read
        {
            let c = output[i as usize] as char;
            match self.state
            {
                State::Initial =>
                {
                    match c
                    {
                        '\n' => buffer.type_special(Special::NewLine),
                        '\r' => buffer.type_special(Special::Return),
                        '\x07' => println!("Bell!!!"),
                        '\x08' => buffer.do_command(Command::CursorLeft, 1),
                        '\x1b' => self.state = State::Escape,
                        _ => buffer.type_rune(c as u32),
                    }
                },

                State::Escape =>
                {
                    println!("Escape: {}", c);
                    self.state = State::Initial;
                },
            }
        }
    }

}

