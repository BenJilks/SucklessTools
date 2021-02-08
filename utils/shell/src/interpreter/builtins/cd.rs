use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;
use crate::interpreter::Environment;
use crate::parser::token::Token;
use crate::path::ShellPath;
use std::path::PathBuf;

pub struct Cd
{
    args: Vec<Token>,
}

impl BuiltIn for Cd
{

    fn program() -> &'static str { "cd" }

    fn new(args: Vec<Token>) -> Box<Self>
    {
        Box::from(Self
        {
            args: args,
        })
    }

}

impl NodeObject for Cd
{

    fn execute(&self, _: &mut Environment, _: &Node) -> i32
    {
        if self.args.len() > 1 
        {
            println!("shell: cd: too many arguments");
            return 1;
        }

        // TODO: Get the home dir in some kind of path util
        let path = 
            if self.args.len() == 0 { 
                PathBuf::from("~")
            } else {
                PathBuf::from(&self.args[0].data)
            };
        
        let result = std::env::set_current_dir(path.resolve());
        if result.is_err() 
        {
            println!("shell: cd: {}", result.unwrap_err());
            return 1;
        }

        return 0;
    }

    fn dump(&self)
    {
        println!("cd");
    }

}
