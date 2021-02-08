
#[derive(Clone, PartialEq, Debug)]
pub enum TokenType
{
    Identifier,
    SemiColon,
    And,
    DoubleAnd,
    Pipe,
    DoublePipe,
    Variable,
    Assignement,
}

#[derive(Clone, Debug)]
pub struct Token
{
    pub token_type: TokenType,
    pub data: String,
    pub value: Option<String>,
}

impl Token
{

    pub fn new(token_type: TokenType, data: &str) -> Option<Result<Self, Error>>
    {
        Some(Ok(Self
        {
            token_type: token_type,
            data: data.to_owned(),
            value: None,
        }))
    }

    pub fn is_command_token(&self) -> bool
    {
        [
            TokenType::Identifier, 
            TokenType::Variable,
        ]
        .contains(&self.token_type)
    }

}

#[derive(Clone, Debug)]
pub struct UnexpectedError
{
    pub expected: Option<String>,
    pub got: String,
}

impl UnexpectedError
{

    pub fn new(expected: &str, got: &str) -> Error
    {
        Error::Unexpected(Self
        {
            expected: Some( expected.to_owned() ),
            got: got.to_owned(),
        })
    }

    pub fn new_char(got: char) -> Error
    {
        Error::Unexpected(Self
        {
            expected: None,
            got: String::from(got),
        })
    }

}

#[derive(Clone, Debug)]
pub enum Error
{
    Unexpected(UnexpectedError),
    IO(String),
}
