pub mod lexer;
pub mod token;
pub mod assume;

pub use assume::Assume;
pub use lexer::Lexer;
use crate::interpreter::ast::{Node, NodeObject, NodeBlockObject};
use crate::interpreter::builtins;
use crate::interpreter::Block;
use crate::interpreter::And;
use crate::interpreter::Or;
use crate::interpreter::Pipe;
use crate::interpreter::With;
use crate::interpreter::Assignmnet;
use token::{Token, TokenType, Error, UnexpectedError};
use std::io::Read;
use std::iter::Peekable;

fn parse_command<S: Read>(src: &mut Peekable<Lexer<S>>) -> Result<Node, Error>
{
    let program = src.next().unwrap()?;
    let mut args = Vec::<Token>::new();

    loop
    {
        let arg_or_none = src.peek();
        if arg_or_none.is_none() {
            break;
        }
        
        let arg = arg_or_none.unwrap().clone();
        if !arg?.is_command_token() {
            break;
        }
        
        args.push(src.next().unwrap()?);
    }

    let command = builtins::node_object_for_program(program.data, args);
    Ok(Node::leaf(command))
}

fn parse_assignment<S: Read>(src: &mut Peekable<Lexer<S>>) -> Result<Node, Error>
{
    let token = src.next().unwrap()?;
    let value = src.next().unwrap()?;
    Ok(Node::leaf(Assignmnet::new(token.data, value.data)))
}

fn parse_statement<S: Read>(src: &mut Peekable<Lexer<S>>) -> Result<Option<Node>, Error>
{
    let next_or_none = src.peek();
    if next_or_none.is_none() {
        return Ok(None);
    }

    let next = next_or_none.unwrap().clone()?;
    match next.token_type
    {
        TokenType::Identifier => Ok(Some(parse_command(src)?)),
        TokenType::Assignement => Ok(Some(parse_assignment(src)?)),
        TokenType::SemiColon => { src.next(); Ok(None) },
        _ => Err(UnexpectedError::new("statement", &next.data)),
    }
}

fn operation_from_type(token_type: TokenType) -> Box<dyn NodeObject>
{
    match token_type
    {
        TokenType::DoubleAnd => And::new(),
        TokenType::DoublePipe => Or::new(),
        TokenType::Pipe => Pipe::new(),
        TokenType::And => With::new(),
        _ => panic!(),
    }
}

fn parse_operation<S, F>(src: &mut Peekable<Lexer<S>>, func: F, tokens: &[TokenType]) -> Result<Option<Node>, Error>
    where S: Read, F: Fn(&mut Peekable<Lexer<S>>) -> Result<Option<Node>, Error>
{
    let mut left = func(src)?;
    if left.is_none() {
        return Ok(None);
    }

    while src.peek().is_some()
    {
        let next = src.peek().unwrap().clone()?;
        if !tokens.contains(&next.token_type) {
            break;
        }
        src.next();

        let right = func(src)?;
        if right.is_none() {
            return Err(UnexpectedError::new("statement", "nothing"));
        }

        left = Some(Node::operation(
            left.unwrap(), right.unwrap(), 
            operation_from_type(next.token_type),
        ));
    }

    Ok(left)
}

fn parse_factor<S: Read>(src: &mut Peekable<Lexer<S>>) -> Result<Option<Node>, Error>
{
    parse_operation(src, parse_statement, &[TokenType::Pipe, TokenType::And])
}

fn parse_term<S: Read>(src: &mut Peekable<Lexer<S>>) -> Result<Option<Node>, Error>
{
    parse_operation(src, parse_factor, &[TokenType::DoubleAnd, TokenType::DoublePipe])
}

fn parse_block<S: Read>(src: &mut Peekable<Lexer<S>>) -> Result<Node, Error>
{
    let mut block = Vec::<Node>::new();
    loop
    {
        let statement_or_none = parse_term(src)?;
        if statement_or_none.is_some() {
            block.push(statement_or_none.unwrap());
        }

        let next_or_none = src.peek();
        if next_or_none.is_none() {
            break;
        } 

        let next = next_or_none.unwrap().clone();
        if next?.token_type != TokenType::SemiColon {
            break;
        }
        src.next();
    }

    Ok(Node::block(block, Block::new()))
}

pub fn parse<S: Read>(lexer: Lexer<S>) -> Result<Node, Error>
{
    parse_block(&mut lexer.peekable())
}
