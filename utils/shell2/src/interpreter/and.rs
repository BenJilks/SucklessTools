use crate::interpreter::ast::{Node, NodeObject, NodeBlockObject};

pub struct And;

impl NodeBlockObject for And
{

    fn new() -> Box<Self>
    {
        Box::from(Self {})
    }

}

impl NodeObject for And
{
    
    fn execute(&self, node: &Node) -> i32
    {
        let result = node.left().execute();
        if result != 0 {
            return result;
        }

        return node.right().execute();
    }

}
