extern crate json;
extern crate libc;
mod timer;
use timer::Timer;
use std::fs::{File, OpenOptions};
use std::path::PathBuf;
use std::io::{Read, Write};
use std::ffi::{CString, c_void};
use chrono::{NaiveDateTime, DateTime, Local, Utc};
use json::JsonValue;

pub struct Job
{
    path: PathBuf,
    script: String,
    timer: Timer,
}

impl Job
{
    fn load(json_data: &JsonValue) -> Result<(String, Timer), String>
    {
        let script = &json_data["script"];
        if !script.is_string() {
            return Err("No script".to_owned());
        }

        let timer = Timer::load(&json_data["timer"]);
        if timer.is_err() {
            return Err(timer.err().unwrap());
        }

        return Ok((
            script.as_str().unwrap().to_owned(),
            timer.unwrap(),
        ));
    }

    pub fn open(path: PathBuf) -> Result<Self, String>
    {
        // Open file
        let file = File::open(path.join("job.json"));
        if file.is_err() {
            return Err(format!("{}", file.err().unwrap()));
        }

        // Read data
        let mut json_data = String::new();
        file.unwrap().read_to_string(&mut json_data).unwrap();

        // Parse json
        let job_json = json::parse(&json_data);
        if job_json.is_err() {
            return Err(format!("{}", job_json.err().unwrap()));
        }

        let load_result = Self::load(&job_json.ok().unwrap());
        if load_result.is_err() {
            return Err(load_result.err().unwrap());
        }

        let (script, timer) = load_result.ok().unwrap();
        return Ok(Self
        {
            path: path,
            script: script,
            timer: timer,
        });
    }

    fn valid_until(&self) -> DateTime<Local>
    {
        let file = File::open(self.path.join("valid_until"));
        if file.is_err() {
            return Local::now();
        }

        let mut buffer = String::new();
        file.unwrap().read_to_string(&mut buffer).unwrap();

        let timestamp = buffer.parse::<i64>().unwrap();
        let utc = DateTime::<Utc>::from_utc(NaiveDateTime::from_timestamp(timestamp, 0), Utc);
        return utc.with_timezone(&Local);
    }

    pub fn should_run(&self) -> bool
    {
        let valid_until = self.valid_until();
        if Local::now() < valid_until {
            return false;
        }

        return self.timer.is_condition_met();
    }

    fn perror(msg: &str)
    {
        let cstr = CString::new(msg).unwrap();
        unsafe { libc::perror(cstr.as_ptr()) };
    }

    fn write_to_log_file(&mut self, fd: i32, pid: i32, log_file_path: &PathBuf)
    {
        let log_file_or_error = OpenOptions::new()
            .create(true)
            .append(true)
            .open(log_file_path);
        if log_file_or_error.is_err() {
            return;
        }

        // Write out header
        let mut log_file = log_file_or_error.unwrap();
        let name = self.path.to_str().unwrap();
        let now = Local::now();
        log_file.write(format!("Execting '{}' at {}\n", name, now.format("%I:%M:%S %p")).as_bytes()).unwrap();
        log_file.write("==========================================\n".as_bytes()).unwrap();

        // Dump script output
        let mut buffer: [u8; 80] = [0; 80];
        loop
        { 
            let read = unsafe { libc::read(fd, buffer.as_mut_ptr() as *mut c_void, buffer.len()) };
            if read == 0 {
                break;
            }
            
            log_file.write(&buffer[0..read as usize]).unwrap();
        }

        let mut status: i32 = 0;
        unsafe { libc::waitpid(pid, &mut status, libc::WNOHANG) };

        // Write footer
        log_file.write("\n==========================================\n".as_bytes()).unwrap();
        log_file.write(format!("Exit status: {}\n\n", status).as_bytes()).unwrap();
    }

    fn run_script(&mut self, log_file_path: &PathBuf)
    {
        unsafe
        {
            // Open up a pipe
            let mut pipe: [i32; 2] = std::mem::zeroed();
            if libc::pipe(pipe.as_mut_ptr()) < 0 
            {
                Self::perror("pipe");
                return;
            }

            // Fork the current process
            let pid = libc::fork();
            if pid < 0 
            {
                Self::perror("fork");
                return;
            }

            if pid == 0
            {
                // child
                libc::close(pipe[0]);
                libc::dup2(pipe[1], 1);
                libc::dup2(pipe[1], 2);

                // Run the script
                let script_path = self.path.join(&self.script);
                let script = CString::new(script_path.to_str().unwrap()).unwrap();
                let rc = libc::execl(script.as_ptr(), script.as_ptr(), 0);
                if rc < 0 {
                    Self::perror("execl");
                }

                // We should never be here, so exit 
                // with and error
                libc::exit(1);
            }
            libc::close(pipe[1]);
            
            // Dump output to the log file
            self.write_to_log_file(pipe[0], pid, log_file_path);
            libc::close(pipe[0]);
        }
    }

    fn run_script_in_background(&mut self, log_file: &PathBuf)
    {
        unsafe
        {
            let pid = libc::fork();
            if pid < 0
            {
                Self::perror("fork");
                return;
            }

            if pid == 0 
            {
                // Child
                self.run_script(log_file);
                libc::exit(0);

                // TODO: This makes a zombie process, 
                //       catch sigchld or something.
            }
        }
    }

    pub fn run(&mut self, log_file: &PathBuf) -> Option<std::io::Error>
    {
        self.run_script_in_background(log_file);

        let file = File::create(self.path.join("valid_until"));
        if file.is_err() {
            return Some(file.err().unwrap());
        }

        let time = self.timer.valid_until();
        let timestamp_str = format!("{}", time.timestamp());
        file.unwrap().write(timestamp_str.as_bytes()).unwrap();
        return None;
    }

}
