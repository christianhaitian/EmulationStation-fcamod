#include "NetworkThread.h"
#include "ApiSystem.h"
#include "guis/GuiMsgBox.h"
#include "Log.h"
#include <chrono>
#include <thread>

NetworkThread::NetworkThread(Window* window) : mWindow(window)
{    
	LOG(LogDebug) << "NetworkThread : Starting";

    // creer le thread
    mFirstRun = true;
    mRunning = true;
	mThread = new std::thread(&NetworkThread::run, this);
}

NetworkThread::~NetworkThread() 
{
	LOG(LogDebug) << "NetworkThread : Exit";

	mRunning = false;
	mThread->join();
	delete mThread;
}

void NetworkThread::run() 
{
	while (mRunning) 
	{
		if (mFirstRun) 
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));
			mFirstRun = false;
		}
		else
			std::this_thread::sleep_for(std::chrono::hours(1));		

		if (Settings::getInstance()->getBool("updates.enabled")) 
		{
			LOG(LogDebug) << "NetworkThread : Checking for updates";

			std::string version = ApiSystem::checkUpdateVersion();
			if (!version.empty()) 
			{
				mWindow->displayNotificationMessage(_U("\uF019  ") + _("UPDATE AVAILABLE") + std::string(": ") + version);
				mRunning = false;
			}
			else
			{
				LOG(LogDebug) << "NetworkThread : No update found";				
			}
		}
	}
}