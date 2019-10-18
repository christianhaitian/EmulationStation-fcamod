#ifndef API_SYSTEM
#define API_SYSTEM

#include <string>
#include <functional>


class Window;

namespace UpdateState
{
	enum State
	{
		NO_UPDATE,
		UPDATER_RUNNING,
		UPDATE_READY
	};
}

class ApiSystem 
{
public:
	static UpdateState::State state;

	static std::pair<std::string, int> updateSystem(const std::function<void(const std::string)>& func = nullptr);
	static std::string checkUpdateVersion();
	static void startUpdate(Window* c);
};

#endif