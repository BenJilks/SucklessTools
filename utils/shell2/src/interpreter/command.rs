extern crate libc;
use crate::interpreter::ast::{Node, NodeObject};
use std::ffi::CString;
use std::os::raw::c_char;

pub struct Command
{
    program: String,
    args: Vec<String>,
}

impl Command
{

    pub fn new(program: String, args: Vec<String>) -> Box<Self>
    {
        Box::from(Self
        {
            program: program,
            args: args,
        })
    }

}

fn perror(msg: &str)
{
    let cstr = CString::new(msg).unwrap();
    unsafe
    {
        libc::perror(cstr.as_ptr());
    }
}

impl NodeObject for Command 
{

    fn execute(&self, _: &Node) -> i32
    {
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
                let mut args = Vec::<*const c_char>::new();

                // Convert our args into a pointer of pointers
                args.push(program_cstr.as_ptr());
                for arg in &self.args
                {
                    arg_cstrs.push(CString::new(arg.clone()).unwrap());
                    args.push(arg_cstrs.last().unwrap().as_ptr());
                }
                args.push(std::ptr::null());

                // Execute the command
                if libc::execvp(program_cstr.as_ptr(), args.as_ptr()) < 0 {
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

}
