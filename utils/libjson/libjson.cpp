#include "libjson.hpp"
#include <sstream>

#define TINYLEX_IMPLEMENT
#include "lexer.hpp"

using namespace Json;

static std::shared_ptr<Value> parse_value(Lexer &lexer);

static std::shared_ptr<Value> error(Lexer &lexer, std::string msg)
{
	lexer.error(msg);
	std::cout << "Error: " << msg << "\n";
	return nullptr;
}

static std::shared_ptr<Value> parse_object(Lexer &lexer)
{
	if (!lexer.consume(Token::TokenType::OpenObject))
		return nullptr;
	
	auto object = std::make_shared<Object>();
	while (lexer.peek() && lexer.peek()->type != Token::TokenType::CloseObject)
	{
		auto name = lexer.consume(Token::TokenType::String);
		if (!name)
			return error(lexer, "Expected a name");

		if (!lexer.consume(Token::TokenType::Is))
			return error(lexer, "Expected ':'");

		auto value = parse_value(lexer);
		if (!value)
			return error(lexer, "Expected value");

		auto name_str = lexer.read_string(*name);
		name_str = name_str.substr(1, name_str.length() - 2);
		object->set(name_str, value);

		if (!lexer.consume(Token::TokenType::Next))
			break;
	}

	if (!lexer.consume(Token::TokenType::CloseObject))
		return error(lexer, "Expected '}'");

	return object;
}

static std::shared_ptr<Value> parse_array(Lexer &lexer)
{
	if (!lexer.consume(Token::TokenType::OpenArray))
		return nullptr;
	
	auto array = std::make_shared<Array>();
	while (lexer.peek() && lexer.peek()->type != Token::TokenType::CloseArray)
	{
		auto value = parse_value(lexer);
		if (!value)
			return error(lexer, "Expected value");

		array->append(value);

		if (!lexer.consume(Token::TokenType::Next))
			break;
	}

	if (!lexer.consume(Token::TokenType::CloseArray))
		return error(lexer, "Expected ']'");
	
	return array;
}

static std::shared_ptr<Value> parse_string(Lexer &lexer)
{
	auto token = lexer.consume(Token::TokenType::String);
	if (!token)
		return error(lexer, "Expected string");

	auto str = lexer.read_string(*token);
	str = str.substr(1, str.length() - 2);
	return std::make_shared<String>(str);
}

static std::shared_ptr<Value> parse_number(Lexer &lexer)
{
	auto token = lexer.consume(Token::TokenType::Number);
	if (!token)
		return error(lexer, "Expected number");

	auto number = std::atof(lexer.read_string(*token).c_str());
	return std::make_shared<Number>(number);
}

static std::shared_ptr<Value> parse_boolean(Lexer &lexer)
{
	auto token = lexer.consume(Token::TokenType::Boolean);
	if (!token)
		return error(lexer, "Expected boolean");

	auto boolean = lexer.read_string(*token) == "true";
	return std::make_shared<Boolean>(boolean);
}

static std::shared_ptr<Value> parse_value(Lexer &lexer)
{
	auto peek = lexer.peek();
	if (!peek)
		return nullptr;
	
	switch (peek->type)
	{
		case Token::TokenType::OpenObject: return parse_object(lexer);
		case Token::TokenType::OpenArray: return parse_array(lexer);
		case Token::TokenType::Number: return parse_number(lexer);
		case Token::TokenType::String: return parse_string(lexer);
		case Token::TokenType::Boolean: return parse_boolean(lexer);
		default: return nullptr;
	}
}

std::shared_ptr<Value> Json::parse(const std::string &file_path)
{
	Lexer lexer(file_path);
	return parse_value(lexer);
}

static std::string print_indent(int indent)
{
	std::string out;
	for (int i = 0; i < indent; i++)
		out += "\t";
	return out;
}

static std::string serialize(const Value &value, bool pretty_print, int indent = 0)
{
	if (value.is<String>())
		return "\"" + value.as<Json::String>()->get() + "\"";
	
	if (value.is<Number>())
	{
		std::stringstream stream;
		stream << std::noshowpoint << value.to_double(); 
		return stream.str();
	}
	
	if (value.is<Boolean>())
		return value.as<Json::Boolean>()->get() ? "true" : "false";
	
	if (value.is<Object>())
	{
		std::string out = "{";

		auto object = value.as<Json::Object>();
		bool is_first = true;
		int new_indent = indent + (pretty_print ? 1 : 0);
		for (const auto &i : *object)
		{
			if (!is_first)
				out += ", ";
		
			if (pretty_print)
				out += "\n";

			out += print_indent(new_indent);
			out += "\"" + i.first + "\": ";

			if (pretty_print && (i.second->is<Object>() || i.second->is<Array>()))
				out += "\n" + print_indent(new_indent);

			out += serialize(*i.second, pretty_print, 
				new_indent);
			
			is_first = false;
		}

		if (pretty_print)
			out += "\n" + print_indent(indent);

		return out + "}";
	}

	if (value.is<Array>())
	{
		std::string out = "[";

		auto array = value.as<Json::Array>();
		bool is_first = true;
		int new_indent = indent + (pretty_print ? 1 : 0);
		for (const auto &item : *array)
		{
			if (!is_first)
				out += ", ";
			
			if (pretty_print)
				out += "\n";

			out += print_indent(new_indent);
			out += serialize(*item, pretty_print, new_indent);
			
			is_first = false;
		}

		if (pretty_print)
			out += "\n" + print_indent(indent);

		return out + "]";
	}

	return "";
}

std::string Value::to_string(bool pretty_print) const
{
	return serialize(*this, pretty_print);
}

float Value::to_float() const
{
	return static_cast<float>(as<Json::Number>()->get());
}

double Value::to_double() const
{
	return as<Json::Number>()->get();
}

int Value::to_int() const
{
	return static_cast<int>(as<Json::Number>()->get());
}

bool Value::to_bool() const
{
	return as<Json::Boolean>()->get();
}

void Object::add(const std::string &name, const std::string str) 
{ 
	data[name] = std::make_shared<String>(str); 
}

void Object::add(const std::string &name, const char *str) 
{ 
	data[name] = std::make_shared<String>(std::string(str)); 
}

void Object::add(const std::string &name, double number) 
{ 
	data[name] = std::make_shared<Number>(number); 
}

void Object::add(const std::string &name, bool boolean) 
{ 
	data[name] = std::make_shared<Boolean>(boolean); 
}

std::ostream &Json::operator<< (std::ostream &out, const Value &value)
{
	out << value.to_string();
	return out;
}

std::ostream &Json::operator<< (std::ostream &out, std::shared_ptr<Value> &value)
{
	if (!value)
	{
		out << "<Null value>";
		return out;
	}

	out << *value;
	return out;
}

