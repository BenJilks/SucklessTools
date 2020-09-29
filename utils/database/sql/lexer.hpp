#pragma once
#include "../forward.hpp"
#include <string>
#include <vector>
#include <optional>

namespace DB::Sql
{

class Lexer
{
public:
    enum Type
    {
        None,
        Name,

        Select,
        From,
        Where,
        Insert,
        Into,
        OpenBrace,
        CloseBrace,
        Values,
        Create,
        Table,
        Update,
        Set,
        Delete,
        If,
        Not,
        Exists,

        Integer,
        Float,
        String,

        MoreThan,
        Equals,
        And,

        Star,
        Comma,
    };

    struct Token
    {
        std::string data;
        Type type;
    };

    Lexer(const std::string &query)
        : m_query(query)
        , m_should_reconsume(false) {}

    std::optional<Token> consume(Type type = None);
    std::optional<Token> peek(size_t count = 0);

private:
    enum class State
    {
        Normal,
        Name,
        Integer,
        Float,
        String,
    };

    std::optional<Token> next();
    Token parse_name(const std::string &buffer);

    std::string m_query;
    State m_state { State::Normal };
    char m_curr_char { 0 };

    bool m_should_reconsume { false };
    size_t m_pointer { 0 };
    std::vector<Token> m_peek_stack;

};

}
