extern crate nix;
use crate::interpreter::ast::{Node, NodeObject, NodeBlockObject};
use crate::interpreter::{Environment, Job};
use nix::unistd::{fork, ForkResult};
use nix::sys::wait::{waitpid, WaitStatus};
use std::process::exit;

pub struct With;

impl NodeBlockObject for With
{

    fn new() -> Box<dyn NodeObject>
    {
        Box::from(Self {})
    }

}

impl NodeObject for With
{
    
    fn execute(&self, environment: &mut Environment, node: &Node) -> i32
    {
        match unsafe { fork() }
        {
            Ok(ForkResult::Child) =>
            {
                exit(node.left().execute(environment))
            },

            Ok(ForkResult::Parent { child }) =>
            {
                if node.is_unary()
                {
                    environment.add_job(Job::new(child, "test"));
                    0
                }
                else
                {
                    node.right().execute(environment);
                    match waitpid(child, None)
                    {
                        Ok(WaitStatus::Exited(_, _)) => 0,
                        _ => 1,
                    }
                }
            }

            Err(_) => 1
        }
    }

    fn dump(&self)
    {
        println!("With");
    }

}
