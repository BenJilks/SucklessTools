#pragma once
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

	virtual int execute() = 0;
	virtual void execute_and_exit() { exit(execute()); }
	
	static std::unique_ptr<Command> parse(const std::string &source);
	static void register_built_in(std::string name, BuiltIn func)
	{
		built_ins[name] = func;
	}

	static void register_alias(std::string name, std::string value)
	{
		aliases[name] = value;
	}

	static void log_aliases();
	static void log_alias(const std::string &name);

private:
	static std::pair<std::string, std::string> parse_assignment(Lexer &lexer);
	static std::unique_ptr<Command> parse_exec(Lexer &lexer);
	static std::unique_ptr<Command> parse_command(Lexer &lexer);

	static std::map<std::string, BuiltIn> built_ins;
	static std::map<std::string, std::string> aliases;

};

class CommandList final : public Command
{
public:
	CommandList() {}

	void add(std::unique_ptr<Command> &command);
	virtual int execute() override;

private:
	std::vector<std::unique_ptr<Command>> commands;

};

class CommandBuiltIn final : public Command
{
public:
	CommandBuiltIn(BuiltIn func, 
				   std::vector<std::string> &args, 
				   std::vector<std::pair<std::string, std::string>> &assignments)
		: func(func)
		, args(args)
		, assignments(assignments)
	{
	}

	virtual int execute() override;

private:
	BuiltIn func;
	std::vector<std::string> args;
	std::vector<std::pair<std::string, std::string>> assignments;

};

class CommandSetEnv final : public Command
{
public:
	CommandSetEnv(std::vector<std::pair<std::string, std::string>> assignments) 
		: assignments(assignments)
	{
	}

	virtual int execute() override;

private:
	std::vector<std::pair<std::string, std::string>> assignments;

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
	std::string program;
	std::vector<std::string> arguments;
	std::vector<std::pair<std::string, std::string>> assignments;

	std::vector<const char*> raw_arguments;
	std::vector<const char*> raw_assignments;

};

class CommandOperation : public Command
{
public:
	CommandOperation(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
		: left(std::move(left))
		, right(std::move(right))
	{
	}

protected:
	std::unique_ptr<Command> left;
	std::unique_ptr<Command> right;

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

