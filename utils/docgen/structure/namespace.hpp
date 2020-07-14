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

	private:
		std::string m_name;
		std::vector<std::unique_ptr<Namespace>> m_sub_namespaces;
		std::vector<std::unique_ptr<Function>> m_functions;

	};

}

