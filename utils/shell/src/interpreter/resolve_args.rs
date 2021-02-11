use crate::parser::token::{Token, TokenType};
use crate::parser::Lexer;
use crate::parser;
use crate::interpreter::glob;
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

fn resolve_glob(glob_str: &str, output: &mut Vec<String>)
{
    // Split the glob path into parts
    let mut parts = glob_str.split("/").peekable();

    // Get the starting path, if it starts with an empty 
    // part, pop it and start at the root
    let mut current_paths = vec![
        if parts.clone().count() > 0 && parts.peek().unwrap().is_empty() {
            parts.next(); PathBuf::from("/")
        } else {
            PathBuf::from("")
        }];

    for part in parts
    {
        // If it ends in a '/', we're done
        if part.is_empty() {
            break;
        }

        let mut next_paths = Vec::<PathBuf>::new();
        for path in &mut current_paths
        {
            // If this isn't a directory, it's not a match
            if !path.resolve().is_dir() {
                continue;
            }

            let read_dir = path.resolve().read_dir().unwrap();
            for entry_or_error in read_dir 
            {
                // Get the filename of this entry
                let entry = entry_or_error.unwrap();
                let filename = entry.file_name().to_str().unwrap().to_owned();

                // If it matches the glob, add it as a match
                if glob::string_matches(&filename, part) {
                    next_paths.push(path.join(filename));
                }
            }
        }

        // Set the current matches to the new ones
        current_paths = next_paths;
    }

    // Return all the possible matches
    for path in current_paths {
        output.push(path.to_str().unwrap().to_owned());
    }
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
            TokenType::Glob => resolve_glob(&arg.data, &mut output),
            _ => panic!(),
        }
    }

    output
}