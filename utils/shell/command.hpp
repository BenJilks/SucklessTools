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

private:
	static std::pair<std::string, std::string> parse_assignment(Lexer &lexer);
	static std::unique_ptr<Command> parse_exec(Lexer &lexer);
	static std::unique_ptr<Command> parse_command(Lexer &lexer);

	static std::map<std::string, BuiltIn> built_ins;

};

class CommandList : public Command
{
public:
	CommandList() {}

	void add(std::unique_ptr<Command> &command);
	virtual int execute() override;

private:
	std::vector<std::unique_ptr<Command>> commands;

};

class CommandBuiltIn : public Command
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

class CommandSetEnv : public Command
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

class CommandExec : public Command
{
public:
	CommandExec(std::string program, 
		std::vector<std::string> arguments, 
		std::vector<std::pair<std::string, std::string>> assignments);
	
	virtual int execute() override;
	virtual void execute_and_exit() override;

protected:
	virtual bool should_execute_in_process() const { return true; }

private:
	std::string program;
	std::vector<std::string> arguments;
	std::vector<std::pair<std::string, std::string>> assignments;

	std::vector<const char*> raw_arguments;
	std::vector<const char*> raw_assignments;

};

class CommandPipe : public Command
{
public:
	CommandPipe(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
		: left(std::move(left))
		, right(std::move(right))
	{
	}

	virtual int execute() override;
	virtual void execute_and_exit() override;

protected:
	virtual bool should_execute_in_process() const { return true; }

private:
	std::unique_ptr<Command> left;
	std::unique_ptr<Command> right;

};

class CommandAnd : public Command
{
public:
	CommandAnd(std::unique_ptr<Command> left, std::unique_ptr<Command> right)
		: left(std::move(left))
		, right(std::move(right))
	{
	}

	virtual int execute() override;

private:
	std::unique_ptr<Command> left;
	std::unique_ptr<Command> right;

};

