//mod line;
mod parser;
use parser::token_source::TokenSource;
use parser::lexer::Lexer;

fn main() 
{
    use std::fs::File;
    let source = File::open("test.sh").unwrap();
    
    let mut lexer = Lexer::new(source);
    loop
    {
        let curr = lexer.consume();
        if curr.is_none() {
            break;
        }
        println!("Current: {}", curr.unwrap().data);

        let next = lexer.peek(0);
        if next.is_some() {
            println!("Next: {}", next.unwrap().data);
        }
        let next2 = lexer.peek(1);
        if next2.is_some() {
            println!("Next: {}", next2.unwrap().data);
        }
        println!();
    }
}
