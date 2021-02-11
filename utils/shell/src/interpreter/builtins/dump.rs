use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;
use crate::interpreter::resolve_args;
use crate::interpreter::Environment;
use crate::parser::token::Token;
use crate::parser::Lexer;
use crate::parser;
use std::fs::File;

pub struct Dump
{
    args: Vec<Token>,
}

impl BuiltIn for Dump
{

    fn program() -> &'static str { "dump" }

    fn new(args: Vec<Token>) -> Box<Self>
    {
        Box::from(Self 
        {
            args
        })
    }

}

impl NodeObject for Dump
{

    fn execute(&self, _: &mut Environment, _: &Node) -> i32
    {
        let args = resolve_args::resolve(&self.args);
        if args.len() == 0 
        {
            println!("shell: dump: No filename");
            return 1;
        }
        if args.len() > 1 
        {
            println!("shell: dump: Too many arguments");
            return 1;
        }

        let file = File::open(&args[0]);
        if file.is_err() 
        {
            println!("shell: dump: {}", file.unwrap_err());
            return 1;
        }

        let lexer = Lexer::new(file.unwrap());
        let script = parser::parse(lexer);
        if script.is_err() 
        {
            println!("shell: source: {:?}", script.err().unwrap());
            return 1;
        }

        script.unwrap().dump(0);
        0
    }

    fn dump(&self)
    {
        println!("dump");
    }

}
