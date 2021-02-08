extern crate nix;
use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::Environment;
use crate::interpreter::resolve_args;
use crate::parser::token::Token;
use nix::unistd::{fork, execvp, ForkResult, Pid};
use nix::sys::wait::{waitpid, WaitStatus};
use std::process::exit;
use std::ffi::CString;

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

fn make_c_string<'a>(string: &String) -> CString
{
    CString::new(string.clone()).unwrap()
}

impl NodeObject for Command 
{

    fn execute(&self, _: &mut Environment, _: &Node) -> i32
    {
        let args = resolve_args::resolve(&self.args);

        match unsafe { fork() }
        {
            Ok(ForkResult::Child) =>
            {
                let c_program = make_c_string(&self.program);
                let mut c_args = args.iter().map(|x| { make_c_string(x) }).collect::<Vec<_>>();
                c_args.insert(0, c_program.clone());
                
                let result = execvp(c_program.as_c_str(), &c_args);
                if result.is_err() {
                    println!("{}", result.unwrap_err());
                }
                exit(1);
            },

            Ok(ForkResult::Parent { child }) =>
            {
                match waitpid(child, None)
                {
                    Ok(WaitStatus::Exited(_, code)) => code,
                    _ => 1,
                }
            },

            Err(err) => 
            {
                println!("shell: fork: {}", err);
                1
            },
        }
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
