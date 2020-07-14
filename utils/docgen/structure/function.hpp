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

	private:
		std::string m_name;
		std::vector<std::pair<std::string, DataType>> m_params;
		DataType m_return_type;

	};

}

