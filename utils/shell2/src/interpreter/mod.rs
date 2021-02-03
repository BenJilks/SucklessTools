pub mod ast;
pub mod block;
pub mod and;
pub mod command;
pub mod builtins;

pub use block::Block;
pub use and::And;
pub use command::Command;

pub struct Environment
{
    pub should_exit: bool,
}

impl Environment
{

    pub fn new() -> Self
    {
        Self
        {
            should_exit: false,
        }
    }

}

/* Command utils */

use std::ffi::CString;

fn perror(msg: &str)
{
    let cstr = CString::new(msg).unwrap();
    unsafe
    {
        libc::perror(cstr.as_ptr());
    }
}
