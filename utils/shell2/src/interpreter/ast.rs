
pub trait NodeObject
{
    fn execute(&self, node: &Node) -> i32;
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

    pub fn execute(&self) -> i32
    {
        self.data.execute(self)
    }

}
