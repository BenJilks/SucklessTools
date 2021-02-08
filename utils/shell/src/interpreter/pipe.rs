use crate::interpreter::ast::{Node, NodeObject, NodeBlockObject};
use crate::interpreter::Environment;
use nix::unistd::{fork, pipe, dup2, close, ForkResult};
use nix::sys::wait::{waitpid, WaitStatus};
use std::process::exit;

pub struct Pipe;

impl NodeBlockObject for Pipe
{

    fn new() -> Box<dyn NodeObject>
    {
        Box::from(Self {})
    }

}

impl NodeObject for Pipe
{
    
    fn execute(&self, environment: &mut Environment, node: &Node) -> i32
    {
        let (read_fd, write_fd) = pipe().unwrap();

        let left = unsafe { fork() }.unwrap();
        if left.is_child() 
        {
            dup2(write_fd, 1).unwrap();
            close(write_fd).unwrap();
            close(read_fd).unwrap();

            node.left().execute(environment);
            exit(0)
        }

        let right = unsafe { fork() }.unwrap();
        if right.is_child()
        {
            dup2(read_fd, 0).unwrap();
            close(write_fd).unwrap();
            close(read_fd).unwrap();

            node.right().execute(environment);
            exit(0)
        }

        let pid = 
            match right
            {
                ForkResult::Parent { child } => child,
                _ => panic!(),
            };
        
        close(write_fd).unwrap();
        close(read_fd).unwrap();
        match waitpid(pid, None)
        {
            Ok(WaitStatus::Exited(_, code)) => code,
            _ => 1
        }
    }

    fn dump(&self)
    {
        println!("Pipe");
    }

}
