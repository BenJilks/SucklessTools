use crate::interpreter::ast::{Node, NodeBlockObject, NodeObject};
use crate::interpreter::Environment;

pub struct Block;

impl NodeBlockObject for Block
{

    fn new() -> Box<dyn NodeObject>
    {
        Box::from(Self{})
    }

}

impl NodeObject for Block
{
    
    fn execute(&self, environment: &mut Environment, node: &Node) -> i32
    {
        for child in node.children() {
            child.execute(environment);
        }
        
        0
    }

    fn dump(&self)
    {
        println!("Block");
    }

}
