use crate::interpreter::Environment;

pub trait NodeObject
{
    fn execute(&self, environment: &mut Environment, node: &Node) -> i32;
    fn dump(&self);
}

pub trait NodeBlockObject
{
    fn new() -> Box<Self>;
}

pub struct Node
{
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
    data: Box<dyn NodeObject>,
}

impl Node
{

    pub fn leaf(data: Box<dyn NodeObject>) -> Node
    {
        Self
        {
            left: None,
            right: None,
            data: data,
        }
    }

    pub fn operation(left: Node, right: Node, data: Box<dyn NodeObject>) -> Node
    {
        Self
        {
            left: Some(Box::from(left)),
            right: Some(Box::from(right)),
            data: data,
        }
    }

    pub fn left(&self) -> &Node { &self.left.as_ref().unwrap() }
    pub fn right(&self) -> &Node { self.right.as_ref().unwrap() }

    pub fn execute(&self, environment: &mut Environment) -> i32
    {
        self.data.execute(environment, self)
    }

    pub fn dump(&self, indent: usize)
    {
        for _ in 0..indent { print!(" ") }
        self.data.dump();

        if self.left.is_some() 
        {
            for _ in 0..indent { print!(" ") }
            self.left.as_ref().unwrap().dump(indent + 1);
        }
        if self.right.is_some() 
        {
            for _ in 0..indent { print!(" ") }
            self.right.as_ref().unwrap().dump(indent + 1);
        }
    }

}
