#ifndef API_SYSTEM
#define API_SYSTEM

#include <string>

class ApiSystem 
{
public:
	//std::pair<std::string, int> updateSystem(const std::function<void(const std::string)>& func = nullptr);
	std::string checkUpdateVersion();

};

#endif