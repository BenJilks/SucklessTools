use std::io::Read;
use std::collections::VecDeque;
use crate::parser::token_source::{Token, TokenType, TokenSource};

enum State
{
    Initial,
    Name,
}

pub struct Lexer<Source: Read>
{
    source: Source,
    peek_queue: VecDeque<Token>,

    c: u8,
    should_reconsume: bool,
    state: State,
}

impl<Source: Read> Lexer<Source>
{

    pub fn new(source: Source) -> Self
    {
        Self
        {
            source: source,
            peek_queue: VecDeque::new(),

            c: 0,
            should_reconsume: false,
            state: State::Initial,
        }
    }

    fn read_next(&mut self) -> Option<Token>
    {
        let mut buffer = String::new();

        loop
        {
            if !self.should_reconsume 
            {
                let mut buffer: [u8; 1] = [0];
                self.source.read(&mut buffer).unwrap();
                self.c = buffer[0];
            }
            self.should_reconsume = false;

            let c = self.c as char;
            match self.state
            {
                State::Initial =>
                {
                    if c.is_alphabetic() 
                    {
                        self.state = State::Name;
                        self.should_reconsume = true;
                    }
                    else if self.c == 0
                    {
                        return None;
                    }
                },

                State::Name =>
                {
                    if !c.is_alphanumeric() 
                    {
                        self.state = State::Initial;
                        self.should_reconsume = true;
                        return Some(Token 
                        {
                            token_type: TokenType::Identifier,
                            data: buffer,
                        });
                    }
                    buffer.push(c);
                },
            }
        }
    }

}

impl<Source: Read> TokenSource for Lexer<Source>
{

    fn peek(&mut self, count: usize) -> Option<Token>
    {
        while self.peek_queue.len() <= count
        {
            let next = self.read_next();
            if next.is_none() {
                return None;
            }

            self.peek_queue.push_back(next?);
        }

        return Some( self.peek_queue[count].clone() );
    }

    fn consume(&mut self) -> Option<Token>
    {
        let next = self.peek(0);
        if next.is_none() {
            return None;
        }

        self.peek_queue.pop_front();
        return next;
    }

}
