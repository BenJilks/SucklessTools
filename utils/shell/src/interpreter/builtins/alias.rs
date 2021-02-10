use crate::interpreter::ast::{Node, NodeObject};
use crate::interpreter::builtins::BuiltIn;
use crate::interpreter::Environment;
use crate::parser::token::Token;
use crate::parser::Lexer;

pub struct Alias
{
    args: Vec<Token>,
}

impl BuiltIn for Alias
{

    fn program() -> &'static str { "alias" }

    fn new(args: Vec<Token>) -> Box<Self>
    {
        Box::from(Self
        {
            args: args,
        })
    }

}

fn print_alias(alias: &str, value: &Vec<Token>)
{
    let value_str = value
        .iter()
        .map(|x| { x.data.clone() })
        .collect::<Vec<_>>()
        .join(" ");

    println!("alias {}='{}'", alias, value_str);
}

impl NodeObject for Alias
{

    fn execute(&self, environment: &mut Environment, _: &Node) -> i32
    {
        if self.args.len() == 0 
        {
            for (alias, value) in &environment.aliases {
                print_alias(alias, value);
            }
            return 0;
        }

        if self.args.len() == 1 
        {
            let alias = &self.args[0].data;
            let value_or_none = environment.aliases.get(alias);
            if value_or_none.is_none() 
            {
                println!("shell: alias: {}: not found", alias);
                return 1;
            }

            print_alias(alias, value_or_none.unwrap());
            return 0;
        }

        for i in 0..(self.args.len() / 2)
        {
            let alias = self.args[i + 0].data.clone();
            let value_str = &self.args[i + 1].data;
            
            let value = Lexer::new(value_str.as_bytes())
                .map(|x| x.unwrap())
                .collect::<Vec<_>>();
            environment.aliases.insert(alias, value);
        }
        
        return 0;
    }

    fn dump(&self)
    {
        println!("alias");
    }

}
