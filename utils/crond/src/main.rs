#[macro_use]
extern crate ini;
mod job;
mod job_runner;
use std::path::PathBuf;
use std::time::Duration;

fn run_background_process()
{
    let config = ini!("/usr/etc/crond.conf");
    let cron_path = config["main"]["path"].clone().unwrap();
    let verbose = config["main"].contains_key("verbose");

    loop
    {
        if verbose {
            println!("Doing loop");
        }

        let result = job_runner::do_check_cycle(&PathBuf::from(&cron_path));
        if result.is_err() {
            println!("Error: {}", result.unwrap_err());
        } else {
            if verbose {
                println!("Done {} job(s)", result.unwrap());
            }
        }

        std::thread::sleep(Duration::from_secs(60));
    }
}

fn main() 
{
    run_background_process();
}

