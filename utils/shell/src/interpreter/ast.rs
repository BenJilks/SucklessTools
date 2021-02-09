extern crate nix;
use crate::interpreter::Environment;
use nix::unistd::{fork, pipe, dup2, close, read, ForkResult};
use nix::sys::wait::{waitpid, WaitPidFlag};
use std::process::exit;

pub trait NodeObject
{
    fn execute(&self, environment: &mut Environment, node: &Node) -> i32;
    fn dump(&self);
}

pub trait NodeBlockObject
{
    fn new() -> Box<dyn NodeObject>;
}

pub struct Node
{
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
    children: Vec<Node>,
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
            children: Vec::new(),
            data: data,
        }
    }

    pub fn operation(left: Node, right: Node, data: Box<dyn NodeObject>) -> Node
    {
        Self
        {
            left: Some(Box::from(left)),
            right: Some(Box::from(right)),
            children: Vec::new(),
            data: data,
        }
    }

    pub fn block(children: Vec<Node>, data: Box<dyn NodeObject>) -> Node
    {
        Self
        {
            left: None,
            right: None,
            children: children,
            data: data,
        }
    }

    pub fn left(&self) -> &Node { self.left.as_ref().unwrap() }
    pub fn right(&self) -> &Node { self.right.as_ref().unwrap() }
    pub fn children(&self) -> &Vec<Node> { &self.children }

    pub fn execute(&self, environment: &mut Environment) -> i32
    {
        self.data.execute(environment, self)
    }

    pub fn execute_capture_output(&self) -> String
    {
        let (read_fd, write_fd) = pipe().unwrap();
        match unsafe { fork() }
        {
            Ok(ForkResult::Child) =>
            {
                dup2(write_fd, 1).unwrap();
                close(write_fd).unwrap();
                close(read_fd).unwrap();

                let mut environment = Environment::new();
                self.execute(&mut environment);
                exit(0)
            },

            Ok(ForkResult::Parent{child}) =>
            {
                close(write_fd).unwrap();

                let mut output = String::new();
                loop
                {
                    let status = waitpid(child, Some(WaitPidFlag::WNOHANG)).unwrap();
                    if status.pid().is_some() {
                        break;
                    }

                    let mut buffer = [0u8; 80];
                    let read = read(read_fd, &mut buffer).unwrap();
                    if read == 0 {
                        break;
                    }

                    output.push_str(&String::from_utf8(buffer[0..read].to_vec()).unwrap());
                }
                close(read_fd).unwrap();
                
                output
            },

            _ => String::new()
        }
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
        for child in self.children() 
        {
            for _ in 0..indent { print!(" ") }
            child.dump(indent + 1);
        }
    }

}
