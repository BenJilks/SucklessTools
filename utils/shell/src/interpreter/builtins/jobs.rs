use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;
use crate::interpreter::Environment;
use crate::parser::token::Token;

pub struct Jobs
{
    args: Vec<Token>,
}

impl BuiltIn for Jobs
{

    fn program() -> &'static str { "jobs" }

    fn new(args: Vec<Token>) -> Box<Self>
    {
        Box::from(Self
        {
            args: args,
        })
    }

}

impl NodeObject for Jobs
{

    fn execute(&self, environment: &mut Environment, _: &Node) -> i32
    {
        let mut id = 1;
        for job in &environment.jobs
        {
            println!("[{}] {} {}", id, job.status, job.name);
            id += 1;
        }

        0
    }

    fn dump(&self)
    {
        println!("jobs");
    }

}
