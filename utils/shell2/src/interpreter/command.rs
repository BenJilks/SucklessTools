extern crate libc;
use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::Environment;
use crate::interpreter::perror;
use crate::interpreter::resolve_args;
use crate::parser::token_source::Token;
use std::ffi::CString;
use std::os::raw::c_char;

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

        unsafe
        {
            let pid = libc::fork();
            if pid < 0 
            {
                perror("fork");
                return 1;
            }

            if pid == 0 
            {
                // Child
                let program_cstr = CString::new(self.program.clone()).unwrap();
                let mut arg_cstrs = Vec::<CString>::new();
                let mut argv = Vec::<*const c_char>::new();

                // Convert our args into a pointer of pointers
                argv.push(program_cstr.as_ptr());
                for arg in &args
                {
                    arg_cstrs.push(CString::new(arg.clone()).unwrap());
                    argv.push(arg_cstrs.last().unwrap().as_ptr());
                }
                argv.push(std::ptr::null());

                // Execute the command
                if libc::execvp(program_cstr.as_ptr(), argv.as_ptr()) < 0 {
                    perror("shell");
                }
                libc::exit(1);
            }

            // Wait for it to finish
            let mut status: i32 = 1;
            libc::waitpid(pid, &mut status, 0);
            return status;
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
