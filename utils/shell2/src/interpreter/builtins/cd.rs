extern crate libc;
use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;
use crate::interpreter::Environment;
use crate::interpreter::perror;
use crate::parser::token_source::Token;
use std::ffi::CString;

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
                std::env::home_dir().unwrap().to_str().unwrap().to_owned()
            } else {
                self.args[0].clone().data
            };

        let path_cstr = CString::new(path).unwrap();
        if unsafe { libc::chdir(path_cstr.as_ptr()) } < 0 
        {
            perror("shell: cd");
            return 1;
        }

        return 0;
    }

    fn dump(&self)
    {
        println!("cd");
    }

}
