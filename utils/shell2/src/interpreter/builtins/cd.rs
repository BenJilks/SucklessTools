use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;

pub struct Cd
{
    args: Vec<String>,
}

impl BuiltIn for Cd
{

    fn program() -> &'static str { "cd" }

    fn new(args: Vec<String>) -> Box<Self>
    {
        Box::from(Self
        {
            args: args,
        })
    }

}

impl NodeObject for Cd
{

    fn execute(&self, _: &Node) -> i32
    {
        println!("This is a cd command {}", self.args.len());
        0
    }

}
