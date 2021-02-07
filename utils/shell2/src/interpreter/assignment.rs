use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::Environment;

pub struct Assignmnet
{
    variable: String,
    value: String,
}

impl Assignmnet
{

    pub fn new(variable: String, value: String) -> Box<Self>
    {
        Box::from(Self
        {
            variable,
            value,
        })
    }

}

impl NodeObject for Assignmnet
{

    fn execute(&self, _: &mut Environment, _: &Node) -> i32
    {
        std::env::set_var(&self.variable, &self.value);
        0
    }

    fn dump(&self)
    {
        println!("{} = {}", self.variable, self.value);
    }

}
