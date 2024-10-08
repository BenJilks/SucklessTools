use crate::buffer::{Buffer, DrawAction};
use crate::decoder::action::Action;
use crate::decoder::Decoder;
use crate::display;
use std::ffi::CString;
use std::mem;
use std::os::raw::c_char;
use std::ptr;
use std::sync::mpsc::{self, Receiver, Sender};

const INPUT_BUFFER_SIZE: usize = 1024 * 10; // 10MB

fn c_str(string: &str) -> CString {
    return CString::new(string).expect("Could not create c string");
}

struct Terminal {
    decoder: Decoder,
    master: i32,
    slave_pid: i32,
    input_buffer: Vec<u8>,
}

enum Message {
    ResetViewport,
    Flush,
    Event(display::Event),
    Action(Action),
    Close,
}

fn read_from_terminal(term: &mut Terminal) -> Option<usize> {
    let count_read = unsafe {
        libc::read(
            term.master,
            term.input_buffer.as_mut_ptr() as *mut libc::c_void,
            term.input_buffer.len(),
        )
    } as i32;

    if count_read < 0 {
        unsafe { libc::perror(c_str("read").as_ptr()) };
        return None;
    }
    return Some(count_read as usize);
}

fn handle_output(term: &mut Terminal, display: &Sender<Message>) {
    let count_read = read_from_terminal(term);
    if count_read.is_none() {
        return;
    }

    let on_action = |action: Action| {
        display.send(Message::Action(action)).unwrap();
    };

    display.send(Message::ResetViewport).unwrap();
    term.decoder
        .decode(&term.input_buffer[0..count_read.unwrap()], &on_action);
    display.send(Message::Flush).unwrap();
}

fn execute_child_process(master: i32, slave: i32) {
    let term = c_str("TERM=xterm-256color");
    let shell = c_str("/usr/bin/bash");

    unsafe {
        // Set up standard streams
        libc::setsid();
        libc::dup2(slave, 0);
        libc::dup2(slave, 1);
        libc::dup2(slave, 2);
        if libc::ioctl(slave, libc::TIOCSCTTY, 0) < 0 {
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

fn open_terminal() -> Option<Terminal> {
    let mut master: i32 = 0;
    let mut slave: i32 = 0;
    let slave_pid: libc::pid_t;

    unsafe {
        // Open the pty
        if libc::openpty(
            &mut master,
            &mut slave,
            ptr::null_mut(),
            ptr::null_mut(),
            ptr::null_mut(),
        ) < 0
        {
            libc::perror(c_str("openpty").as_ptr());
            return None;
        }

        // Fork off a new child process to
        // run the shell in
        slave_pid = libc::fork();
        if slave_pid < 0 {
            libc::perror(c_str("fork").as_ptr());
            return None;
        }

        if slave_pid == 0 {
            execute_child_process(master, slave);
        }

        libc::close(slave);
    }

    return Some(Terminal {
        decoder: Decoder::new(),
        master,
        slave_pid,
        input_buffer: vec![0; INPUT_BUFFER_SIZE],
    });
}

fn terminal_thread(mut term: Terminal, display: Sender<Message>) {
    let master = term.master;
    let slave_pid = term.slave_pid;
    unsafe {
        let mut fds: libc::fd_set = mem::zeroed();
        loop {
            libc::FD_ZERO(&mut fds);
            libc::FD_SET(master, &mut fds);
            if libc::select(
                master + 1,
                &mut fds,
                ptr::null_mut(),
                ptr::null_mut(),
                ptr::null_mut(),
            ) < 0
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
        }
    }

    let _ = display.send(Message::Close);
}

fn handle_resize(buffer: &mut Buffer, master: i32, event: &display::ResizeEvent) {
    buffer.resize(event.rows, event.columns);
    unsafe {
        let mut size = libc::winsize {
            ws_row: event.rows as u16,
            ws_col: event.columns as u16,
            ws_xpixel: event.width as u16,
            ws_ypixel: event.height as u16,
        };

        if libc::ioctl(master, libc::TIOCSWINSZ, &mut size) < 0 {
            libc::perror(c_str("ioctl(TIOCSWINSZ)").as_ptr());
        }
    }
}

fn handle_input(master: i32, input: &Vec<u8>) {
    unsafe {
        if libc::write(master, input.as_ptr() as *const libc::c_void, input.len()) < 0 {
            libc::perror(c_str("write").as_ptr());
        }
    }
}

fn handle_event(buffer: &mut Buffer, event: &display::Event, master: i32) {
    match event {
        display::Event::Input(input) => handle_input(master, &input),
        display::Event::Resize(resize_event) => handle_resize(buffer, master, resize_event),

        display::Event::RedrawRange { start, height } => {
            buffer.redraw_range(*start, *start + *height);
        }

        display::Event::MouseDown { row, column } => {
            buffer.selection_start(*row, *column);
        }

        display::Event::MouseDrag { row, column } => {
            buffer.selection_update(*row, *column);
        }

        display::Event::DoubleClick { row, column } => {
            buffer.selection_word(*row, *column);
        }
    }
}

fn display_thread(event_receiver: Receiver<display::Event>, message_sender: Sender<Message>) {
    for event in event_receiver {
        message_sender
            .send(Message::Event(event))
            .expect("Could send event message");
    }
}

fn buffer_thread(
    message_reciver: Receiver<Message>,
    draw_action_sender: Sender<DrawAction>,
    master: i32,
) {
    let mut buffer = Buffer::new(100, 50, draw_action_sender.clone());

    for message in message_reciver {
        match message {
            Message::ResetViewport => buffer.reset_viewport(),
            Message::Flush => buffer.flush(),
            Message::Event(event) => handle_event(&mut buffer, &event, master),
            Message::Close => break,

            Message::Action(action) => {
                let result = buffer.do_action(action);
                if !result.is_empty() {
                    handle_input(master, &result);
                }
            },
        }
    }

    let _ = draw_action_sender.send(DrawAction::Close);
}

pub fn run(event_receiver: Receiver<display::Event>, draw_action_sender: Sender<DrawAction>) {
    if let Some(term) = open_terminal() {
        let (message_sender, message_reciver) = mpsc::channel::<Message>();

        let display_message_sender = message_sender.clone();
        let master = term.master;
        std::thread::spawn(move || display_thread(event_receiver, display_message_sender));
        std::thread::spawn(move || terminal_thread(term, message_sender));
        std::thread::spawn(move || buffer_thread(message_reciver, draw_action_sender, master));
    } else {
        eprintln!("Error: Failed to open terminal");
    }
}
