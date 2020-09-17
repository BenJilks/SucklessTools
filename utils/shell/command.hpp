#pragma once
#include "lexer.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

typedef std::function<int(
	const std::vector<std::string>&, 
	const std::vector<std::pair<std::string, std::string>>&)> BuiltIn;
class Lexer;

class Command
{
public:
	Command() {}
    virtual ~Command() = default;

	virtual int execute() = 0;
	virtual void execute_and_exit() { exit(execute()); }
	
    static std::unique_ptr<Command> parse(Lexer &lexer);
	static void register_built_in(std::string name, BuiltIn func)
	{
        s_built_ins[name] = func;
	}

private:
	static std::pair<std::string, std::string> parse_assignment(Lexer &lexer);
	static std::unique_ptr<Command> parse_exec(Lexer &lexer);
	static std::unique_ptr<Command> parse_command(Lexer &lexer);

    static std::map<std::string, BuiltIn> s_built_ins;

};

class CommandList final : public Command
{
public:
	CommandList() {}

	void add(std::unique_ptr<Command> &command);
	virtual int execute() override;

private:
    std::vector<std::unique_ptr<Command>> m_commands;

};

class CommandBuiltIn final : public Command
{
public:
	CommandBuiltIn(BuiltIn func, 
				   std::vector<std::string> &args, 
				   std::vector<std::pair<std::string, std::string>> &assignments)
        : m_func(func)
        , m_args(args)
        , m_assignments(assignments)
	{
	}

	virtual int execute() override;

private:
    BuiltIn m_func;
    std::vector<std::string> m_args;
    std::vector<std::pair<std::string, std::string>> m_assignments;

};

class CommandSetEnv final : public Command
{
public:
	CommandSetEnv(std::vector<std::pair<std::string, std::string>> assignments) 
        : m_assignments(assignments)
	{
	}

	virtual int execute() override;

private:
    std::vector<std::pair<std::string, std::string>> m_assignments;

};

class CommandExec final : public Command
{
public:
	CommandExec(std::string program, 
		std::vector<std::string> arguments, 
		std::vector<std::pair<std::string, std::string>> assignments);
	
	virtual int execute() override;
	virtual void execute_and_exit() override;

private:
    std::string m_program;
    std::vector<std::string> m_arguments;
    std::vector<std::pair<std::string, std::string>> m_assignments;

    std::vector<const char*> m_raw_arguments;
    std::vector<const char*> m_raw_assignments;

};

class CommandOperation : public Command
{
public:
	CommandOperation(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
        : m_left(std::move(left))
        , m_right(std::move(right))
	{
	}

protected:
    std::unique_ptr<Command> m_left;
    std::unique_ptr<Command> m_right;

};

class CommandPipe final : public CommandOperation
{
public:
	CommandPipe(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
		: CommandOperation(std::move(left), std::move(right))
	{
	}

	virtual int execute() override;
	virtual void execute_and_exit() override;
};

class CommandRedirect final : public Command
{
public:
    enum class Mode
    {
        Override,
        Append,
    };

    enum class Capture
    {
        StdOut,
        StdErr,
        All
    };

    CommandRedirect(std::unique_ptr<Command> left, const std::string &right,
            Mode mode, Capture capture)
        : m_left(std::move(left))
        , m_right(std::move(right))
        , m_mode(mode)
        , m_capture(capture)
    {
    }

    virtual int execute() override;

private:
    std::unique_ptr<Command> m_left;
    std::string m_right;
    Mode m_mode;
    Capture m_capture;

};

class CommandAnd final : public CommandOperation
{
public:
	CommandAnd(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
		: CommandOperation(std::move(left), std::move(right))
	{
	}

	virtual int execute() override;

};

class CommandOr final : public CommandOperation
{
public:
	CommandOr(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
		: CommandOperation(std::move(left), std::move(right))
	{
	}

	virtual int execute() override;

};

class CommandWith final : public CommandOperation
{
public:
	CommandWith(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
		: CommandOperation(std::move(left), std::move(right))
	{
	}

	virtual int execute() override;
};

