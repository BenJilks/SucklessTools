use crate::parser::token::{Token, TokenType, Error, UnexpectedError};
use crate::parser::Lexer;
use std::io::Read;
use std::iter::Peekable;

pub trait Assume
{
    fn assume(&mut self, token_type: TokenType, name: &str) -> Result<Token, Error>;
}

impl<S: Read> Assume for Peekable<Lexer<S>>
{

    fn assume(&mut self, token_type: TokenType, name: &str) -> Result<Token, Error>
    {
        let next_or_none = self.next();
        if next_or_none.is_none() {
            return Err(UnexpectedError::new(name, "<end of file>"));
        }

        let next = next_or_none.unwrap()?;
        if next.token_type != token_type {
            return Err(UnexpectedError::new(name, &next.data));
        }

        return Ok(next);
    }

}
