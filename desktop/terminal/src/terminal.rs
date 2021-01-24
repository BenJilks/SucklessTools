use crate::display;
use crate::decoder::action::Action;
use crate::decoder::Decoder;
use crate::buffer::Buffer;
use std::sync::mpsc::{channel, Sender, Receiver};
use std::os::raw::c_char;
use std::ptr;
use std::mem;
use std::ffi::CString;

const INPUT_BUFFER_SIZE: usize = 1024 * 10; // 10MB

fn c_str(string: &str) -> CString
{
    return CString::new(string)
        .expect("Could not create c string");
}

struct Terminal
{
    decoder: Decoder,
    master: i32,
    slave_pid: i32,
    input_buffer: Vec<u8>,
}

enum MessageType
{
    NotifyUpdate,
    ResetViewport,
    Flush,
    Action,
}

struct Message
{
    message_type: MessageType,
    action: Option<Action>,
}

impl Message
{
    pub fn new(message_type: MessageType) -> Self
    {
        Self
        {
            message_type: message_type,
            action: None,
        }
    }
}

fn read_from_terminal(term: &mut Terminal) -> Option<usize>
{
    let count_read = unsafe { libc::read(
        term.master, 
        term.input_buffer.as_mut_ptr() as *mut libc::c_void, 
        term.input_buffer.len()) } as i32;
    
    if count_read < 0 {
        unsafe { libc::perror(c_str("read").as_ptr()) };
        return None
    }
    return Some ( count_read as usize );
}

fn handle_output(term: &mut Terminal, display: &Sender<Message>)
{
    let count_read = read_from_terminal(term);
    if count_read.is_none() {
        return;
    }

    let on_action = |action: Action|
    {
        display.send(Message 
        { 
            message_type: MessageType::Action,
            action: Some( action ),
        }).unwrap();
    };

    display.send(Message::new(MessageType::ResetViewport)).unwrap();
    term.decoder.decode(&term.input_buffer[0..count_read.unwrap()], &on_action);
    display.send(Message::new(MessageType::Flush)).unwrap();
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
        libc::putenv(term.as_ptr() as *mut c_char);
        
        // Execute shell
        if libc::execl(shell.as_ptr(), shell.as_ptr(), 0) < 0 {
            libc::perror(c_str("execl").as_ptr());
        }
        libc::exit(1);
    }
}

fn open_terminal() -> Option<Terminal>
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

    return Some(Terminal
    {
        decoder: Decoder::new(),
        master: master, 
        slave_pid: slave_pid,
        input_buffer: vec![0; INPUT_BUFFER_SIZE],
    });
}

fn terminal_thread(mut term: Terminal, display_fd: i32, display: Sender<Message>)
{
    let master = term.master;
    let slave_pid = term.slave_pid;
    unsafe
    {
        let mut fds: libc::fd_set = mem::zeroed();
        loop
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
                handle_output(&mut term, &display);
            }

            if libc::FD_ISSET(display_fd, &mut fds) {
                display.send(Message::new(MessageType::NotifyUpdate)).unwrap();
            }
        }
    }
}

fn handle_resize<Display>(buffer: &mut Buffer<Display>, master: i32, result: &display::UpdateResult)
    where Display: display::Display
{
    buffer.resize(result.rows, result.columns);
    unsafe
    {
        let mut size = libc::winsize
        {
            ws_row: result.rows as u16,
            ws_col: result.columns as u16,
            ws_xpixel: result.width as u16,
            ws_ypixel: result.height as u16,
        };

        if libc::ioctl(master, libc::TIOCSWINSZ, &mut size) < 0 {
            libc::perror(c_str("ioctl(TIOCSWINSZ)").as_ptr());
        }
    }
}

fn handle_input(master: i32, input: &Vec<u8>)
{
    unsafe
    {
        if libc::write(
            master,
            input.as_ptr() as *const libc::c_void,
            input.len()) < 0
        {
            libc::perror(c_str("write").as_ptr());
        }
    }
}

fn handle_update<Display>(buffer: &mut Buffer<Display>, master: i32)
    where Display: display::Display
{
    let results = buffer.get_display().update();
    for result in &results
    {
        match result.result_type
        {
            display::UpdateResultType::Input => handle_input(master, &result.input),
            display::UpdateResultType::Resize => handle_resize(buffer, master, &result),
            display::UpdateResultType::RedrawRange => buffer.redraw_range(result.start, result.start + result.height),

            display::UpdateResultType::MouseDown => buffer.selection_start(result.rows, result.columns),
            display::UpdateResultType::MouseDrag => buffer.selection_update(result.rows, result.columns),
            display::UpdateResultType::DoubleClickDown => buffer.selection_word(result.rows, result.columns),
        }
    }
}

fn display_thread<Display>(display: Display, master: i32, terminal: Receiver<Message>)
    where Display: display::Display
{
    let mut buffer = Buffer::new(100, 50, display);
    for message in terminal
    {
        match message.message_type
        {
            MessageType::NotifyUpdate => handle_update(&mut buffer, master),
            MessageType::ResetViewport => buffer.reset_viewport(),
            MessageType::Flush => buffer.flush(),
            MessageType::Action =>
            {
                let result = buffer.do_action(message.action.unwrap());
                if !result.is_empty() {
                    handle_input(master, &result);
                }
            }
        };
    }
}

pub fn run<Display>(display: Display)
    where Display: display::Display
{
    let term_or_error = open_terminal();
    if term_or_error.is_none() {
        return;
    }
    let (message_sender, message_reciver) = channel::<Message>();

    let term = term_or_error.unwrap();
    let display_fd = display.get_fd();
    let master = term.master;
    std::thread::spawn(move ||
    {
        terminal_thread(term, display_fd, message_sender)
    });
    display_thread(display, master, message_reciver);
}
