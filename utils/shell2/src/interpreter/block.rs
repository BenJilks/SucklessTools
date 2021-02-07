use crate::interpreter::ast::{Node, NodeBlockObject, NodeObject};
use crate::interpreter::Environment;

pub struct Block;

impl NodeBlockObject for Block
{

    fn new() -> Box<Self>
    {
        Box::from(Self{})
    }

}

impl NodeObject for Block
{
    
    fn execute(&self, environment: &mut Environment, node: &Node) -> i32
    {
        node.left().execute(environment);
        node.right().execute(environment);
        return 0;
    }

    fn dump(&self)
    {
        println!("Block");
    }

}
