#include "libjson.hpp"
#include <iostream>

int main()
{
	Json::Object object;

	object.add("test", "lolz");
	std::cout << object << "\n";
	return 0;
}

