#pragma once
#include "datatype.hpp"
#include <string>

namespace Structure
{
	
	class Function
	{
	public:
		Function(const std::string &name)
			: m_name(name) {}

		inline void dump(int indent = 0) const
		{
			for (int i = 0; i < indent; i++)
				std::cout << "\t";
			std::cout << "Function (" << m_name << ") -> " << m_return_type.str() << ":\n";
			for (const auto &param : m_params)
			{
				for (int i = 0; i < indent + 1; i++)
					std::cout << "\t";
				std::cout << param.first << ": " << param.second.str() << "\n";
			}
		}


	private:
		std::string m_name;
		std::vector<std::pair<std::string, DataType>> m_params;
		DataType m_return_type;

	};

}

