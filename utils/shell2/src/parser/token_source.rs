
#[derive(Clone, PartialEq, Debug)]
pub enum TokenType
{
    Identifier,
    SemiColon,
    And,
    DoubleAnd,
}

#[derive(Clone, Debug)]
pub struct Token
{
    pub token_type: TokenType,
    pub data: String,
}

impl Token
{

    pub fn new(token_type: TokenType, data: &str) -> Option<Self>
    {
        Some(Self
        {
            token_type: token_type,
            data: data.to_owned(),
        })
    }

}

pub struct TokenError
{
    pub expected: String,
    pub got: String,
}

impl TokenError
{

    pub fn new(expected: &str, got: &str) -> Self
    {
        Self
        {
            expected: expected.to_owned(),
            got: got.to_owned(),
        }
    }

}

pub trait TokenSource
{
    fn peek(&mut self, count: usize) -> Option<Token>;
    fn consume(&mut self) -> Option<Token>;
    fn assume(&mut self, token_type: TokenType, name: &str) -> Result<Token, TokenError>;
}
