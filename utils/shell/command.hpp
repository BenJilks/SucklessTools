#pragma once
#include <string>
#include <vector>
#include <memory>

class Command
{
public:
	Command() {}

	virtual void execute() = 0;
	int execute_in_process();
	
	static std::unique_ptr<Command> parse(const std::string &source);

private:
};

class CommandExec : public Command
{
public:
	CommandExec(std::string program, 
		std::vector<std::string> arguments, 
		std::vector<std::pair<std::string, std::string>> assignments);
	
	virtual void execute() override;

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

	virtual void execute() override;

private:
	std::unique_ptr<Command> left;
	std::unique_ptr<Command> right;

};

