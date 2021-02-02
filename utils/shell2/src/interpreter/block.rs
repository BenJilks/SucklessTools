use crate::interpreter::ast::{Node, NodeBlockObject, NodeObject};

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
    
    fn execute(&self, node: &Node) -> i32
    {
        node.left().execute();
        node.right().execute();
        return 0;
    }

}
