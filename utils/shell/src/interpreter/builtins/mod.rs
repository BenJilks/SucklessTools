pub mod cd;
pub mod exit;
pub mod source;
pub mod dump;
pub mod alias;
pub mod jobs;
use crate::interpreter::ast::NodeObject;
use crate::interpreter::command::Command;
use crate::parser::token::Token;
use exit::Exit;
use cd::Cd;
use source::Source;
use dump::Dump;
use alias::Alias;
use jobs::Jobs;

pub trait BuiltIn
{
    fn program() -> &'static str;
    fn new(args: Vec<Token>) -> Box<Self>;
}

macro_rules! check_each_type
{
    ( $program:expr, $args:expr, $( $t:ident ),+ ) => 
    { 
        $(
            if $t::program() == $program {
                return $t::new($args)
            }
        )*
        return Command::new($program, $args);
    }
}

pub fn node_object_for_program(program: String, args: Vec<Token>) -> Box<dyn NodeObject>
{
    check_each_type!(program, args
        ,Cd
        ,Exit
        ,Source
        ,Dump
        ,Alias
        ,Jobs
    );
}
