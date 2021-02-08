mod line;
mod interpreter;
mod parser;
mod path;
use line::Line;
use line::history::History;
use interpreter::Environment;
use parser::Lexer;
use std::fs::File;

fn cli()
{
    let mut environment = Environment::new();
    let mut history = History::new();

    while !environment.should_exit
    {
        let line = Line::get("shell> ", &mut history);
        let lexer = Lexer::new(line.as_bytes());
        let script_or_error = parser::parse(lexer);
        if script_or_error.is_err() 
        {
            let error = script_or_error.err().unwrap();
            println!("{:?}", error);
            return;
        }

        let script = script_or_error.ok().unwrap();
        script.execute(&mut environment);
        history.push(line);
    }
}

fn run_script(path: &str)
{
    let source_or_error = File::open(path);
    if source_or_error.is_err() 
    {
        println!("shell: {}", source_or_error.unwrap_err());
        return;
    }

    let lexer = Lexer::new(source_or_error.unwrap());
    let script_or_error = parser::parse(lexer);
    if script_or_error.is_err() 
    {
        let error = script_or_error.err().unwrap();
        println!("{:?}", error);
        return;
    }

    let mut environment = Environment::new();
    let script = script_or_error.ok().unwrap();
    script.dump(0);
    script.execute(&mut environment);
}

fn main()
{
    let mut args = std::env::args();
    if args.len() == 1 {
        cli();
        return;
    }

    assert!(args.len() >= 2);
    let script_path = args.nth(1).unwrap();
    run_script(&script_path);
}
