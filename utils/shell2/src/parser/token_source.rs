
#[derive(Clone)]
pub enum TokenType
{
    Identifier,
}

#[derive(Clone)]
pub struct Token
{
    pub token_type: TokenType,
    pub data: String,
}

pub trait TokenSource
{
    fn peek(&mut self, count: usize) -> Option<Token>;
    fn consume(&mut self) -> Option<Token>;
}
