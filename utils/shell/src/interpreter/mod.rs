pub mod ast;
pub mod block;
pub mod and;
pub mod or;
pub mod pipe;
pub mod with;
pub mod command;
pub mod assignment;
pub mod builtins;
pub mod resolve_args;
mod glob;

pub use block::Block;
pub use and::And;
pub use or::Or;
pub use pipe::Pipe;
pub use with::With;
pub use command::Command;
pub use assignment::Assignmnet;

use std::collections::HashMap;
use crate::parser::token::Token;

pub struct Environment
{
    pub should_exit: bool,
    pub aliases: HashMap<String, Vec<Token>>,
}

impl Environment
{

    pub fn new() -> Self
    {
        Self
        {
            should_exit: false,
            aliases: HashMap::new(),
        }
    }

}
