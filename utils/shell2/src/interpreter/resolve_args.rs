use crate::parser::token::{Token, TokenType};
use std::env;

fn resolve_variable(variable: &str) -> String
{
    let value_or_error = env::var(variable);
    if value_or_error.is_err() {
        return String::new();
    }

    return value_or_error.unwrap();
}

pub fn resolve(args: &Vec<Token>) -> Vec<String>
{
    let mut output = Vec::<String>::new();
    for arg in args 
    {
        match arg.token_type
        {
            TokenType::Identifier => output.push(arg.data.clone()),
            TokenType::Variable => output.push(resolve_variable(&arg.data)),
            _ => panic!(),
        }
    }

    output
}
