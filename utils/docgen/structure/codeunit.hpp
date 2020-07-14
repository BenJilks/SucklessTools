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

		inline void dump(int indent = 0) const
		{
			for (int i = 0; i < indent; i++)
				std::cout << "\t";
			std::cout << "Code Unit (" << m_name << "):\n";

			if (m_default_namespace)
				m_default_namespace->dump(indent + 1);
		}

	private:
		std::string m_name;
		std::unique_ptr<Namespace> m_default_namespace;

	};

}

