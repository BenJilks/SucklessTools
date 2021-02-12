extern crate nix;
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

use crate::parser::token::Token;
use std::collections::HashMap;
use nix::unistd::Pid;
use nix::sys::wait::{waitpid, WaitPidFlag, WaitStatus};

pub struct Job
{
    pub pid: Pid,
    pub name: String,
}

impl Job
{

    pub fn new(pid: Pid, name: &str) -> Self
    {
        Self
        {
            pid: pid,
            name: name.to_owned(),
        }
    }

}

pub struct Environment
{
    pub should_exit: bool,
    pub aliases: HashMap<String, Vec<Token>>,
    pub jobs: Vec<Job>,
}

impl Environment
{

    pub fn new() -> Self
    {
        Self
        {
            should_exit: false,
            aliases: HashMap::new(),
            jobs: Vec::new(),
        }
    }

    pub fn add_job(&mut self, job: Job)
    {
        println!("[{}] {}", self.jobs.len() + 1, job.pid);
        self.jobs.push(job);
    }

    pub fn check_jobs(&mut self)
    {
        let mut jobs_done = Vec::<Pid>::new();
        for job in &self.jobs
        {
            match waitpid(job.pid, Some(WaitPidFlag::WNOHANG))
            {
                Ok(WaitStatus::Exited(_, _)) => jobs_done.push(job.pid),
                _ => {},
            }
        }

        for job in &jobs_done 
        {
            let index = self.jobs.iter().position(|x| { x.pid == *job }).unwrap();
            println!("[{}] Done", index + 1);
            
            self.jobs.remove(index);
        }
    }

}
