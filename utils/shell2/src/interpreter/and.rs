use crate::interpreter::ast::{Node, NodeObject, NodeBlockObject};
use crate::interpreter::Environment;

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
    
    fn execute(&self, environment: &mut Environment, node: &Node) -> i32
    {
        let result = node.left().execute(environment);
        if result != 0 {
            return result;
        }

        return node.right().execute(environment);
    }

}
