#pragma once
#include <string>

namespace Structure
{

	class DataType
	{
	public:
		enum class Flags
		{
			None = 0 << 0,
			Error = 1 << 0,
			Primitive = 1 << 1,
		};

		DataType()
			: m_name("Error")
			, m_flags(Flags::Error) {}

		DataType(const std::string &name, Flags flags = Flags::None)
			: m_name(name)
	       		, m_flags(flags) {}

	private:
		std::string m_name;
		Flags m_flags;

	};

}

