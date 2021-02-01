#[macro_use]
extern crate ini;
extern crate daemonize;
mod job;
mod job_runner;
use std::path::PathBuf;
use std::time::Duration;
use std::fs::File;
use daemonize::Daemonize;

fn run_background_process()
{
    let config = ini!("/usr/etc/crond.conf");
    let cron_path = config["main"]["path"].clone().unwrap();
    let verbose = config["main"]["verbose"].clone();

    loop
    {
        if verbose.is_some() {
            println!("Doing loop");
        }

        let result = job_runner::do_check_cycle(&PathBuf::from(&cron_path));
        if result.is_err() {
            println!("Error: {}", result.unwrap_err());
        } else {
            if verbose.is_some() {
                println!("Done {} job(s)", result.unwrap());
            }
        }

        std::thread::sleep(Duration::from_secs(60));
    }
}

fn main() 
{
    let stdout = File::create("/tmp/crond.out").unwrap();
    let daemonize = Daemonize::new()
        .pid_file("/tmp/crond.pid")
        .chown_pid_file(true)
        .stdout(stdout);

    let result = daemonize.start();
    if result.is_err() 
    {
        println!("Unable to daemonize: {}", result.unwrap_err());
        return;
    }

    run_background_process();
}
