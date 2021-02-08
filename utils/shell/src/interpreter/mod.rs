pub mod ast;
pub mod block;
pub mod and;
pub mod or;
pub mod pipe;
pub mod with;
pub mod command;
pub mod assignment;
pub mod builtins;
mod resolve_args;

pub use block::Block;
pub use and::And;
pub use or::Or;
pub use pipe::Pipe;
pub use with::With;
pub use command::Command;
pub use assignment::Assignmnet;

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
