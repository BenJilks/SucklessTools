extern crate chrono;
use crate::job::Job;
use std::path::PathBuf;
use chrono::prelude::{Local, Datelike};

fn get_log_file(path: &PathBuf) -> Result<PathBuf, String>
{
    let now = Local::today();
    let log_file_path = path
        .join("log")
        .join(format!("{}", now.year()))
        .join(format!("{}", now.month()));

    let result = std::fs::create_dir_all(&log_file_path);
    if result.is_err() {
        return Err(format!("{}", result.unwrap_err()));
    }

    return Ok(log_file_path.join(format!("{}-{}", now.weekday(), now.day())));
}

pub fn do_check_cycle(path: &PathBuf) -> Result<i32, String>
{
    let dir = std::fs::read_dir(path);
    if dir.is_err() {
        return Err(format!("{}", dir.err().unwrap()));
    }

    let log_file_or_error = get_log_file(path);
    if log_file_or_error.is_err() {
        return Err(log_file_or_error.unwrap_err());
    }

    let log_file = log_file_or_error.unwrap();
    let mut jobs_ran = 0;
    for path in dir.unwrap() 
    {
        let job_path = path.unwrap().path();
        if job_path.ends_with("log") {
            continue;
        }

        let job_or_error = Job::open(job_path);
        if job_or_error.is_err() {
            return Err(job_or_error.err().unwrap());
        }

        let mut job = job_or_error.unwrap();
        if job.should_run() 
        {
            job.run(&log_file);
            jobs_ran += 1;
        }
    }

    return Ok(jobs_ran);
}
