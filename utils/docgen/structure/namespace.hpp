#pragma once
#include "function.hpp"
#include <string>
#include <memory>

namespace Structure
{

	class Namespace
	{
	public:
		Namespace(const std::string &name)
			: m_name(name) {}

		inline void dump(int indent = 0) const
		{
			for (int i = 0; i < indent; i++)
				std::cout << "\t";
			std::cout << "Namespace (" << m_name << "):\n";

			for (const auto &sub : m_sub_namespaces)
				sub->dump(indent + 1);
			for (const auto &func : m_functions)
				func->dump(indent + 1);
		}

	private:
		std::string m_name;
		std::vector<std::unique_ptr<Namespace>> m_sub_namespaces;
		std::vector<std::unique_ptr<Function>> m_functions;

	};

}

