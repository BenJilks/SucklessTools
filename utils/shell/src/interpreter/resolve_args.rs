use crate::parser::token::{Token, TokenType};
use crate::parser::Lexer;
use crate::parser;
use crate::path::ShellPath;
use std::env;
use std::path::PathBuf;

fn variable(variable: &str) -> String
{
    let value_or_error = env::var(variable);
    if value_or_error.is_err() {
        return String::new();
    }

    return value_or_error.unwrap();
}

fn possible_path(possible_path: &str) -> String
{
    if !possible_path.starts_with("~") {
        return possible_path.to_owned();
    }

    let path = PathBuf::from(possible_path).resolve();
    return path.to_str().unwrap().to_owned();
}

fn sub_command(command: &str, output: &mut Vec<String>)
{
    let root = parser::parse(Lexer::new(command.as_bytes())).unwrap();
    let command_output = root.execute_capture_output();
    
    let lexer = Lexer::new(command_output.as_bytes());
    for token_or_error in lexer
    {
        if token_or_error.is_err() {
            panic!(); // TODO
        }

        let token = token_or_error.unwrap();
        output.push(token.data);
    }
}

enum State
{
    String,
    Variable,
    SubCommand,
}

pub fn resolve_string_variables(string: &str) -> String
{
    let mut output = String::new();
    let mut buffer = String::new();
    let mut state = State::String;
    let mut depth = 0;

    for c in string.chars()
    {
        match state
        {
            State::String =>
            {
                match c
                {
                    '$' => state = State::Variable,
                    _ => output.push(c),
                }
            },

            State::Variable => 
            {
                match c
                {
                    x if x.is_whitespace() =>
                    {
                        if buffer.is_empty() {
                            output.push('$');
                        } else {
                            output.push_str(&variable(&buffer));
                        }

                        output.push(c);
                        state = State::String;
                        buffer = String::new();
                    },

                    '(' => state = State::SubCommand,
                    _ => buffer.push(c),
                }
            },

            State::SubCommand =>
            {
                match c
                {
                    ')' =>
                    {
                        depth -= 1;
                        if depth < 0 
                        {
                            let mut tokens = Vec::<String>::new();
                            sub_command(&buffer, &mut tokens);
                            output.push_str(&tokens.join(" "));

                            state = State::String;
                            buffer = String::new();
                            depth = 0;
                        }
                    },

                    '(' => 
                    {
                        depth += 1;
                        buffer.push(c)
                    },

                    _ => buffer.push(c),
                }
            }
        }
    }

    output
}

pub fn resolve(args: &[Token]) -> Vec<String>
{
    let mut output = Vec::<String>::new();
    for arg in args 
    {
        match arg.token_type
        {
            TokenType::Identifier => output.push(possible_path(&arg.data)),
            TokenType::String => output.push(resolve_string_variables(&arg.data)),
            TokenType::Variable => output.push(variable(&arg.data)),
            TokenType::SubCommand => sub_command(&arg.data, &mut output),
            _ => panic!(),
        }
    }

    output
}
