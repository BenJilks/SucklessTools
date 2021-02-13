extern crate nix;
use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::Environment;
use crate::interpreter::resolve_args;
use crate::parser::token::Token;
use nix::unistd::{fork, execvp, ForkResult};
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

    fn execute(&self, environment: &mut Environment, node: &Node) -> i32
    {
        match unsafe { fork() }
        {
            Ok(ForkResult::Child) =>
            {
                self.execute_and_exit(environment, node);
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

    fn execute_and_exit(&self, environment: &mut Environment, _: &Node) -> !
    {
        // Resolve arguments and aliases
        let mut program = &self.program;
        let mut token_args = self.args.clone();
        if environment.aliases.contains_key(program)
        {
            let alias = &environment.aliases[program];
            program = &alias[0].data;
            token_args = alias[1..alias.len()].to_vec();
            token_args.extend(self.args.clone().into_iter());
        }
        let args = resolve_args::resolve(&token_args);

        // Create c-strings for program and arguments
        let c_program = make_c_string(program);
        let mut c_args = args.iter().map(|x| { make_c_string(x) }).collect::<Vec<_>>();
        c_args.insert(0, c_program.clone());
        
        // Execute the program
        let result = execvp(c_program.as_c_str(), &c_args);
        if result.is_err() {
            println!("shell: {}", result.unwrap_err());
        }

        // Exit with an error, as we should neve be here
        exit(1);
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
