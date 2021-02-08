use crate::parser::token::{Token, TokenType};
use crate::path::ShellPath;
use std::env;
use std::path::PathBuf;

fn resolve_variable(variable: &str) -> String
{
    let value_or_error = env::var(variable);
    if value_or_error.is_err() {
        return String::new();
    }

    return value_or_error.unwrap();
}

fn resolve_possible_path(possible_path: &str) -> String
{
    if !possible_path.starts_with("~") {
        return possible_path.to_owned();
    }

    let path = PathBuf::from(possible_path).resolve();
    return path.to_str().unwrap().to_owned();
}

pub fn resolve(args: &Vec<Token>) -> Vec<String>
{
    let mut output = Vec::<String>::new();
    for arg in args 
    {
        match arg.token_type
        {
            TokenType::Identifier => output.push(resolve_possible_path(&arg.data)),
            TokenType::Variable => output.push(resolve_variable(&arg.data)),
            _ => panic!(),
        }
    }

    output
}
