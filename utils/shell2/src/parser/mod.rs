pub mod lexer;
pub mod token_source;
use crate::interpreter::ast::{Node, NodeObject, NodeBlockObject};
use crate::interpreter::builtins;
use crate::interpreter::Block;
use crate::interpreter::And;
use token_source::{TokenSource, TokenType, TokenError};

type ParserResult = Result<Option<Node>, TokenError>;

fn parse_command<S: TokenSource>(src: &mut S) -> ParserResult
{
    let program_or_none = src.peek(0);
    if program_or_none.is_none() {
        return Ok(None);
    }

    let program = program_or_none.unwrap();
    if program.token_type != TokenType::Identifier {
        return Ok(None);
    }
    src.consume();
    
    let mut args = Vec::<String>::new();
    loop
    {
        let arg_or_none = src.peek(0);
        if arg_or_none.is_none() {
            break
        }

        let arg = arg_or_none.unwrap();
        if arg.token_type != TokenType::Identifier {
            break;
        }
        args.push(arg.data);
        src.consume();
    }

    let command = builtins::node_object_for_program(program.data, args);
    return Ok(Some(Node::leaf(command)));
}

fn parse_operation<Op, S, Func>(src: &mut S, func: Func, token: TokenType) -> ParserResult
    where S: TokenSource,
          Func: Fn(&mut S) -> ParserResult,
          Op: NodeObject + NodeBlockObject + 'static
{
    let lhs_or_error = func(src);
    if lhs_or_error.is_err() {
        return Err(lhs_or_error.err().unwrap());
    }

    let lhs = lhs_or_error?;
    if lhs.is_none() {
        return Ok(None);
    }

    let next = src.peek(0);
    if next.is_none() {
        return Ok(lhs);
    }

    if next.unwrap().token_type == token
    {
        src.consume();

        let rhs_or_error = parse_operation::<Op, S, Func>(src, func, token);
        if rhs_or_error.is_err() {
            return Err(rhs_or_error.err().unwrap());
        }

        let rhs = rhs_or_error?;
        if rhs.is_some() {
            return Ok(Some(Node::operation(lhs.unwrap(), rhs.unwrap(), Op::new())));
        }
    }

    return Ok(lhs);
}

fn parse_and<S: TokenSource>(src: &mut S) -> ParserResult
{
    return parse_operation::<And, _, _>(src, parse_command, TokenType::DoubleAnd);
}

fn parse_block<S: TokenSource>(src: &mut S) -> ParserResult
{
    return parse_operation::<Block, _, _>(src, parse_and, TokenType::SemiColon);
}

pub fn parse<S: TokenSource>(mut src: S) -> ParserResult
{
    return parse_block(&mut src);
}
