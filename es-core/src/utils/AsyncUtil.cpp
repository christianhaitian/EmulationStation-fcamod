#include "utils/AsyncUtil.h"

#include <thread>
#include "Settings.h"


namespace Utils
{
	namespace Async
	{

		bool isCanRunAsync()
		{
			return (std::thread::hardware_concurrency() > 2) && Settings::getInstance()->getBool("ThreadedLoading");
		}

	} // Async::

} // Utils::
