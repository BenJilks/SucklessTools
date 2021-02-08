use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::Environment;
use crate::interpreter::resolve_args;
use crate::parser::token::Token;
use std::process;

pub struct Command
{
    program: String,
    args: Vec<Token>,
}

impl Command
{

    pub fn new(program: String, args: Vec<Token>) -> Box<Self>
    {
        Box::from(Self
        {
            program: program,
            args: args,
        })
    }

}

impl NodeObject for Command 
{

    fn execute(&self, _: &mut Environment, _: &Node) -> i32
    {
        let args = resolve_args::resolve(&self.args);
        let process = process::Command::new(&self.program)
            .args(args)
            .spawn();
        
        if process.is_err() 
        {
            println!("shell: {}", process.unwrap_err());
            return 1;
        }    
        
        let result = process.unwrap().wait();
        if result.is_err() {
            println!("shell: {}", result.as_ref().unwrap_err());
        }

        result.unwrap().code().unwrap_or(1)
    }

    fn dump(&self)
    {
        println!("{} {}", self.program, self.args
            .clone()
            .into_iter()
            .map(|x| x.data + " ")
            .collect::<String>());
    }

}
