mod line;
mod interpreter;
mod parser;
use line::Line;
use interpreter::Environment;
use parser::lexer::Lexer;

fn main() 
{
    let mut environment = Environment::new();

    while !environment.should_exit
    {
        let line = Line::get("shell> ");
        let lexer = Lexer::new(line.as_bytes());
        let script_or_error = parser::parse(lexer);
        if script_or_error.is_err() 
        {
            let error = script_or_error.err().unwrap();
            println!("Error: expected '{}', got '{}'", 
                error.expected, error.got);
            return;
        }

        let script = script_or_error.ok().unwrap();
        if script.is_some() {
            script.unwrap().execute(&mut environment);
        }
    }
}
