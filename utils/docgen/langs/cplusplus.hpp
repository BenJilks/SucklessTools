#pragma once
#include "../structure/codeunit.hpp"
#include "../lexer.hpp"

namespace CPP
{

	class Parser
	{
	public:
		static Structure::CodeUnit parse(const std::string &path);
		static Structure::CodeUnit parse(std::istream&);

	private:
		Parser(Lexer &lexer)
			: m_lexer(lexer) {}

		void parse_function(Structure::Namespace &ns);
		Structure::CodeUnit run();

		Lexer &m_lexer;

	};

}
