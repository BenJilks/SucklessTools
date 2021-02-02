use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;

pub struct Exit;

impl BuiltIn for Exit
{

    fn program() -> &'static str { "exit" }

    fn new(_: Vec<String>) -> Box<Self>
    {
        Box::from(Self {})
    }

}

impl NodeObject for Exit
{

    fn execute(&self, _: &Node) -> i32
    {
        println!("This is a exit command");
        0
    }

}
