#pragma once
#include "namespace.hpp"
#include <string>

namespace Structure
{
	
	class CodeUnit
	{
	public:
		CodeUnit(const std::string &name)
			: m_name(name) {}

		inline const std::string &name() const { return m_name; }

	private:
		std::string m_name;
		std::unique_ptr<Namespace> m_default_namespace;

	};

}

