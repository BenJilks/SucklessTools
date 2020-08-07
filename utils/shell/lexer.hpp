#pragma once
#include <string>
#include <optional>
#include <queue>

struct Token
{
	enum class Type
	{
		Name,
		Pipe,
		And,
		Or,
		With,
		Variable,
		VariableAssignment,
		SubCommand,
		EndCommand
	};

	std::string data;
    Type type;
};

class Lexer
{
public:
    Lexer(const std::string &source);

    std::optional<Token> peek(int count = 0);
    std::optional<Token> consume(Token::Type type);

    void insert_string(const std::string&);
    std::function<bool(Token&, int)> hook_on_token;

private:
    enum class State
    {
        Default,
        Name,
        Pipe,
        And,
        VariableStart,
        VariableCurly,
        Variable,
        VariableAssignment,
        VariableAssignmentString,
        SubCommand,
        Escape,
        Escape03,
        Escape033,
    };

    std::optional<Token> next();

    State m_state { State::Default };
    State m_return_state { State::Default };
    bool m_should_reconsume { false };
    char m_curr_char { 0 };
    int m_index { 0 };

    std::string m_source;
    std::vector<Token> m_peek_queue;
    int m_token_index { 0 };

};
