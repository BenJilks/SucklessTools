use crate::parser::token::{Token, TokenType};
use crate::parser::Lexer;
use crate::parser;
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

fn resolve_sub_command(command: &str, output: &mut Vec<String>)
{
    let lexer = Lexer::new(command.as_bytes());
    let root = parser::parse(lexer).unwrap();

    // TODO: This should produce multiple tokens
    output.push(root.execute_capture_output())
}

pub fn resolve(args: &[Token]) -> Vec<String>
{
    let mut output = Vec::<String>::new();
    for arg in args 
    {
        match arg.token_type
        {
            TokenType::Identifier => output.push(resolve_possible_path(&arg.data)),
            TokenType::Variable => output.push(resolve_variable(&arg.data)),
            TokenType::SubCommand => resolve_sub_command(&arg.data, &mut output),
            _ => panic!(),
        }
    }

    output
}
