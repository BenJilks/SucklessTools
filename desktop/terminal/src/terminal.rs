use super::display;
use super::decoder::*;
use super::buffer::*;
use std::{ptr, mem, ffi::CString};

fn c_str(string: &str) -> CString
{
    return CString::new(string)
        .expect("Could not create c string");
}

struct Terminal<Display>
    where Display: display::Display
{
    buffer: Buffer<Display>,
    decoder: Decoder,
    master: i32,
}

fn read_from_terminal<Display>(term: &mut Terminal<Display>, buffer: &mut [u8]) -> Option<usize>
    where Display: display::Display
{
    let count_read = unsafe { libc::read(
        term.master, 
        buffer.as_mut_ptr() as *mut libc::c_void, 
        buffer.len()) } as i32;
    
    if count_read < 0 {
        unsafe { libc::perror(c_str("read").as_ptr()) };
        return None
    }
    return Some ( count_read as usize );
}

fn handle_output<Display>(term: &mut Terminal<Display>)
    where Display: display::Display
{
    let mut buffer = [0 as u8; 80];
    let count_read = read_from_terminal(term, &mut buffer);
    if count_read.is_none() {
        return;
    }

    let response = term.decoder.decode(
        &buffer[0..count_read.unwrap()], &mut term.buffer);
    if response.len() > 0 {
        handle_input(term, &response);
    }
}

fn handle_input<Display>(term: &mut Terminal<Display>, input: &Vec<u8>)
    where Display: display::Display
{
    unsafe
    {
        if libc::write(
            term.master, 
            input.as_ptr() as *const libc::c_void, 
            input.len()) < 0 
        {
            libc::perror(c_str("write").as_ptr());
        }
    }
}

fn handle_resize<Display>(term: &mut Terminal<Display>, result: &display::UpdateResult)
    where Display: display::Display
{
    term.buffer.resize(result.rows, result.columns);
    unsafe
    {
        let mut size = libc::winsize
        {
            ws_row: result.rows as u16,
            ws_col: result.columns as u16,
            ws_xpixel: result.width as u16,
            ws_ypixel: result.height as u16,
        };

        if libc::ioctl(term.master, libc::TIOCSWINSZ, &mut size) < 0 {
            libc::perror(c_str("ioctl(TIOCSWINSZ)").as_ptr());
        }
    }
}

fn handle_update<Display>(term: &mut Terminal<Display>)
    where Display: display::Display
{
    let results = term.buffer.get_display().update();
    for result in &results
    {
        match result.result_type
        {
            display::UpdateResultType::Input => handle_input(term, &result.input),
            display::UpdateResultType::Resize => handle_resize(term, &result),
            display::UpdateResultType::Redraw => term.buffer.redraw(),
        }
    }
    term.buffer.flush();
}

fn execute_child_process(master: i32, slave: i32)
{
    let term = c_str("TERM=xterm-256color");
    let shell = c_str("/usr/bin/bash");
    
    unsafe
    {
        // Set up standard streams
        libc::setsid();
        libc::dup2(slave, 0);
        libc::dup2(slave, 1);
        libc::dup2(slave, 2);
        if libc::ioctl(slave, libc::TIOCSCTTY, 0) < 0
        {
            libc::perror(c_str("ioctl(TIOCSCTTY)").as_ptr());
            libc::exit(1);
        }

        // Close the old streams
        libc::close(slave);
        libc::close(master);

        // Setup envirement
        libc::putenv(term.as_ptr() as *mut i8);
        
        // Execute shell
        if libc::execl(shell.as_ptr(), shell.as_ptr(), 0) < 0 {
            libc::perror(c_str("execl").as_ptr());
        }
        libc::exit(1);
    }
}

fn open_terminal() -> Option<(i32, libc::pid_t)>
{
    let mut master: i32 = 0;
    let mut slave: i32 = 0;
    let slave_pid: libc::pid_t;

    unsafe
    {
        // Open the pty
        if libc::openpty(&mut master, &mut slave, 
            ptr::null_mut(), ptr::null_mut(), ptr::null_mut()) < 0
        {
            libc::perror(c_str("openpty").as_ptr());
            return None;
        }

        // Fork off a new child process to 
        // run the shell in
        slave_pid = libc::fork();
        if slave_pid < 0
        {
            libc::perror(c_str("fork").as_ptr()); 
            return None; 
        }
        
        if slave_pid == 0 {
            execute_child_process(master, slave);
        }

        libc::close(slave);
    }

    return Some( (master, slave_pid) );
}

pub fn run<Display>(display: Display)
    where Display: display::Display
{
    let fd_or_error = open_terminal();
    if fd_or_error.is_none() {
        return;
    }

    let (master, slave_pid) = fd_or_error.unwrap();
    let mut term = Terminal
    {
        buffer: Buffer::new(100, 50, display),
        decoder: Decoder::new(),
        master: master,
    };

    let display_fd = term.buffer.get_display().get_fd();
    unsafe
    {
        let mut fds: libc::fd_set = mem::zeroed();
        while !term.buffer.get_display().should_close()
        {
            libc::FD_ZERO(&mut fds);
            libc::FD_SET(master, &mut fds);
            libc::FD_SET(display_fd, &mut fds);
            if libc::select(master + display_fd + 1, &mut fds, 
                ptr::null_mut(), ptr::null_mut(), ptr::null_mut()) < 0
            {
                libc::perror(c_str("select").as_ptr());
                return;
            }
            
            let mut status: i32 = 0;
            if libc::waitpid(slave_pid, &mut status, libc::WNOHANG) != 0 {
                break;
            }

            if libc::FD_ISSET(master, &mut fds) {
                handle_output(&mut term);
            }

            if libc::FD_ISSET(display_fd, &mut fds) {
                handle_update(&mut term);
            }
        }
    }
}

