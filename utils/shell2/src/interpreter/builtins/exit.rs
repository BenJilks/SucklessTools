use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;
use crate::interpreter::Environment;
use crate::parser::token::Token;

pub struct Exit;

impl BuiltIn for Exit
{

    fn program() -> &'static str { "exit" }

    fn new(_: Vec<Token>) -> Box<Self>
    {
        Box::from(Self {})
    }

}

impl NodeObject for Exit
{

    fn execute(&self, environment: &mut Environment, _: &Node) -> i32
    {
        environment.should_exit = true;
        return 0;
    }

    fn dump(&self)
    {
        println!("exit");
    }

}
