use std::io::Read;
use std::collections::VecDeque;
use crate::parser::token_source::
{
    Token, 
    TokenType, 
    TokenError, 
    TokenSource
};

enum State
{
    Initial,
    Name,
    And,
}

pub struct Lexer<Source: Read>
{
    source: Source,
    peek_queue: VecDeque<Token>,
    should_reconsume: bool,
    curr_byte: u8,
}

impl<Source: Read> Lexer<Source>
{

    pub fn new(source: Source) -> Self
    {
        Self
        {
            source: source,
            peek_queue: VecDeque::new(),
            should_reconsume: false,
            curr_byte: 0,
        }
    }

    fn is_name_char(c: char) -> bool
    {
        c.is_alphabetic() || ['-', '/', '.'].contains(&c)
    }

    fn read_next(&mut self) -> Option<Token>
    {
        let mut buffer = String::new();
        let mut state = State::Initial;
    
        loop
        {
            if !self.should_reconsume 
            {
                let mut buffer: [u8; 1] = [0];
                self.source.read(&mut buffer).unwrap();
                self.curr_byte = buffer[0];
            }
            self.should_reconsume = false;

            let c = self.curr_byte as char;
            match state
            {
                State::Initial =>
                {
                    if Self::is_name_char(c)
                    {
                        self.should_reconsume = true;
                        state = State::Name;
                    }
                    else if self.curr_byte == 0
                    {
                        return None;
                    }
                    else
                    {
                        match c
                        {
                            '&' => state = State::And,
                            ';' | '\n' => return Token::new(TokenType::SemiColon, ";"),
                            ' ' | '\t' => {},
                            _ => panic!(),
                        }
                    }
                },

                State::Name =>
                {
                    if (!Self::is_name_char(c) && !c.is_digit(10)) || self.curr_byte == 0 
                    {
                        self.should_reconsume = true;
                        return Token::new(TokenType::Identifier, &buffer);
                    }
                    buffer.push(c);
                },

                State::And =>
                {
                    if c == '&' {
                        return Token::new(TokenType::DoubleAnd, "&&");
                    }
                    
                    self.should_reconsume = true;
                    return Token::new(TokenType::And, "&");
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

    fn assume(&mut self, token_type: TokenType, name: &str) -> Result<Token, TokenError>
    {
        let next_or_none = self.peek(0);
        if next_or_none.is_none()  {
            return Err(TokenError::new(name, "<end of file>"));
        }
        
        let next = next_or_none.unwrap();
        if next.token_type != token_type {
            return Err(TokenError::new(name, &next.data));
        }

        return Ok(self.consume().unwrap());
    }

}