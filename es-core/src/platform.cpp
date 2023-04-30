#include "platform.h"
#include <SDL_events.h>

#ifdef WIN32
#include <codecvt>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include "Window.h"
#include "Log.h"

#include "GuiComponent.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

int runShutdownCommand()
{
#ifdef WIN32 // windows
	return system("shutdown -s -t 0");
#else // osx / linux
	return system("sudo shutdown -h now");
#endif
}

int runRestartCommand()
{
#ifdef WIN32 // windows
	return system("shutdown -r -t 0");
#else // osx / linux
	return system("sudo shutdown -r now");
#endif
}

void splitCommand(std::string cmd, std::string* executable, std::string* parameters)
{
	std::string c = Utils::String::trim(cmd);
	size_t exec_end;

	if (c[0] == '\"')
	{
		exec_end = c.find_first_of('\"', 1);
		if (std::string::npos != exec_end)
		{
			*executable = c.substr(1, exec_end - 1);
			*parameters = c.substr(exec_end + 1);
		}
		else
		{
			*executable = c.substr(1, exec_end);
			std::string().swap(*parameters);
		}
	}
	else
	{
		exec_end = c.find_first_of(' ', 0);
		if (std::string::npos != exec_end)
		{
			*executable = c.substr(0, exec_end);
			*parameters = c.substr(exec_end + 1);
		}
		else
		{
			*executable = c.substr(0, exec_end);
			std::string().swap(*parameters);
		}
	}
}

#ifdef WIN32
int _monitorEnumIndex = 0;
HMONITOR _monitorEnumHandle = 0;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{	
	if (_monitorEnumIndex == dwData)
	{
		_monitorEnumHandle = hMonitor;
		return FALSE;
	}

	_monitorEnumIndex++;
	return TRUE;  // continue enumerating
}
#endif

int runSystemCommand(const std::string& cmd_utf8, const std::string& name, Window* window)
{
#ifdef WIN32

#if _DEBUG
	return 0;
#endif

	if (window != NULL)
		window->renderGameLoadingScreen();

	// on Windows we use _wsystem to support non-ASCII paths
	// which requires converting from utf8 to a wstring
	//typedef std::codecvt_utf8<wchar_t> convert_type;
	//std::wstring_convert<convert_type, wchar_t> converter;
	//std::wstring wchar_str = converter.from_bytes(cmd_utf8);
	std::string command = cmd_utf8;

	#define BUFFER_SIZE 8192

	TCHAR szEnvPath[BUFFER_SIZE];
	DWORD dwLen = ExpandEnvironmentStringsA(command.c_str(), szEnvPath, BUFFER_SIZE);
	if (dwLen > 0 && dwLen < BUFFER_SIZE)
		command = std::string(szEnvPath);

	std::string exe;
	std::string args;

	splitCommand(command, &exe, &args);
	exe = Utils::FileSystem::getPreferredPath(exe);
	
	SHELLEXECUTEINFO lpExecInfo;
	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	lpExecInfo.lpFile = exe.c_str();	
	lpExecInfo.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS;
	lpExecInfo.hwnd = NULL;
	lpExecInfo.lpVerb = "open"; // to open  program

	lpExecInfo.lpDirectory = NULL;
	lpExecInfo.nShow = SW_SHOW;  // show command prompt with normal window size 
	lpExecInfo.hInstApp = (HINSTANCE)SE_ERR_DDEFAIL;   //WINSHELLAPI BOOL WINAPI result;
	
	std::string extraConfigFile;

	int monitorId = Settings::getInstance()->getInt("MonitorID");
	if (monitorId > 0)
	{
		_monitorEnumIndex = 0;
		_monitorEnumHandle = 0;
		EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, monitorId);
		if (_monitorEnumHandle != 0)
		{
			// Special processing for retroarch -> Set monitor index in the extra config file ( and disable save_on_exit )
			if (Utils::String::toLower(exe).find("retroarch.") != std::string::npos)
			{
				std::string video_monitor_index = "video_monitor_index = \""+ std::to_string(monitorId+1) +"\"\r\nconfig_save_on_exit = \"false\"\r\n";
				extraConfigFile = Utils::FileSystem::getGenericPath(Utils::FileSystem::getHomePath() + "/retroarch.custom.cfg");
				Utils::FileSystem::writeAllText(extraConfigFile, video_monitor_index);
				args = args + " --appendconfig \""+ extraConfigFile +"\"";
			}

			lpExecInfo.fMask |= SEE_MASK_HMONITOR;
			lpExecInfo.hIcon = _monitorEnumHandle;
		}
	}
	
	lpExecInfo.lpParameters = args.c_str(); //  file name as an argument	

	// Don't set directory for relative paths
	if (!Utils::String::startsWith(exe, ".") && !Utils::String::startsWith(exe, "/") && !Utils::String::startsWith(exe, "\\"))
		lpExecInfo.lpDirectory = Utils::FileSystem::getAbsolutePath(Utils::FileSystem::getParent(exe)).c_str();

	ShellExecuteEx(&lpExecInfo);

	if (lpExecInfo.hProcess != NULL)
	{		
		if (window == NULL)
			WaitForSingleObject(lpExecInfo.hProcess, INFINITE);
		else
		{
			while (WaitForSingleObject(lpExecInfo.hProcess, 50) == 0x00000102L)
			{
				bool polled = false;

				SDL_Event event;
				while (SDL_PollEvent(&event))
					polled = true;

				if (window != NULL && polled)
					window->renderGameLoadingScreen();
			}
		}
	
		if (Utils::FileSystem::exists(extraConfigFile))
			Utils::FileSystem::removeFile(extraConfigFile);

		CloseHandle(lpExecInfo.hProcess);
		return 0;
	}
	
	if (Utils::FileSystem::exists(extraConfigFile))
		Utils::FileSystem::removeFile(extraConfigFile);

	return 1;
	//return _wsystem(wchar_str.c_str());
#else
	return system(cmd_utf8.c_str());
#endif
}

QuitMode quitMode = QuitMode::QUIT;

int quitES(QuitMode mode)
{
	quitMode = mode;

	SDL_Event *quit = new SDL_Event();
	quit->type = SDL_QUIT;
	SDL_PushEvent(quit);
	return 0;
}

bool executeSystemScript(const std::string command)
{
	LOG(LogInfo) << "Platform::executeSystemScript() - Running -> " << command;

	if (system(command.c_str()) == 0)
		return true;

	LOG(LogError) << "Platform::executeSystemScript() - Error executing " << command;
	return false;
}

std::pair<std::string, int> executeSystemScript(const std::string command, const std::function<void(const std::string)>& func)
{
	LOG(LogInfo) << "Platform::executeSystemScript() - Running -> " << command;

	FILE *pipe = popen(command.c_str(), "r");
	if (pipe == NULL)
	{
		LOG(LogError) << "Platform::executeSystemScript() - Error executing " << command;
		return std::pair<std::string, int>("Error starting command : " + command, -1);
	}

	char line[1024];
	while (fgets(line, 1024, pipe))
	{
		strtok(line, "\n");

		// Long theme names/URL can crash the GUI MsgBox
		// "48" found by trials and errors. Ideally should be fixed
		// in es-core MsgBox -- FIXME
		if (strlen(line) > 48)
			line[47] = '\0';

		if (func != nullptr)
			func(std::string(line));
	}

	int exitCode = WEXITSTATUS(pclose(pipe));
	return std::pair<std::string, int>(line, exitCode);
}

void touch(const std::string& filename)
{
#ifdef WIN32
	FILE* fp = fopen(filename.c_str(), "ab+");
	if (fp != NULL)
		fclose(fp);
#else
	int fd = open(filename.c_str(), O_CREAT|O_WRONLY, 0644);
	if (fd >= 0)
		close(fd);
#endif
}

void processQuitMode()
{
	switch (quitMode)
	{
	case QuitMode::RESTART:
		LOG(LogInfo) << "Restarting EmulationStation";
		touch("/tmp/es-restart");
		break;
	case QuitMode::REBOOT:
		LOG(LogInfo) << "Rebooting system";
		touch("/tmp/es-sysrestart");
		runRestartCommand();
		break;
	case QuitMode::SHUTDOWN:
		LOG(LogInfo) << "Shutting system down";
		touch("/tmp/es-shutdown");
		runShutdownCommand();
		break;
	}
}

std::string queryBatteryRootPath()
{
	static std::string batteryRootPath;

	if (batteryRootPath.empty())
	{
		auto files = Utils::FileSystem::getDirContent("/sys/class/power_supply");
		for (auto file : files)
		{
			if (Utils::String::toLower(file).find("/bat") != std::string::npos)
			{
				batteryRootPath = file;
				break;
			}
		}
	}
	return batteryRootPath;
}

BatteryInformation queryBatteryInformation(bool summary)
{
	BatteryInformation ret;

	std::string batteryRootPath = queryBatteryRootPath();

	// Find battery path - only at the first call
	if (!queryBatteryRootPath().empty())
	{
		try
		{
			ret.hasBattery = true;
			ret.level = queryBatteryLevel();
			ret.isCharging = queryBatteryCharging();
			if (!summary)
			{
				ret.health = Utils::String::toLower( Utils::String::replace( Utils::FileSystem::readAllText(batteryRootPath + "/health"), "\n", "" ) );
				ret.max_capacity = std::atoi(Utils::FileSystem::readAllText(batteryRootPath + "/charge_full").c_str()) / 1000; // milli amperes
				ret.voltage = queryBatteryVoltage();
			}
		} catch (...) {
			LOG(LogError) << "Platform::queryBatteryInformation() - Error reading battery data!!!";
		}
	}

	return ret;
}

int queryBatteryLevel()
{
	std::string batteryCapacityPath = queryBatteryRootPath() + "/capacity";
	if ( Utils::FileSystem::exists(batteryCapacityPath) )
		return std::atoi(Utils::FileSystem::readAllText(batteryCapacityPath).c_str());

	return 0;
}

bool queryBatteryCharging()
{
	std::string batteryStatusPath = queryBatteryRootPath() + "/status";
	if ( Utils::FileSystem::exists(batteryStatusPath) )
		return Utils::String::compareIgnoreCase( Utils::String::replace(Utils::FileSystem::readAllText(batteryStatusPath), "\n", ""), "discharging" );

	return false;
}

float queryBatteryVoltage()
{
	std::string batteryVoltagePath = queryBatteryRootPath() + "/voltage_now";
	if ( Utils::FileSystem::exists(batteryVoltagePath) )
		return std::atof(Utils::FileSystem::readAllText(batteryVoltagePath).c_str()) / 1000000; // volts

	return false;
}

// Adapted from emuelec
std::string getShOutput(const std::string& mStr) 
{
    std::string result, file;
    FILE* pipe{popen(mStr.c_str(), "r")};
    char buffer[256];

    while(fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        file = buffer;
        result += file.substr(0, file.size() - 1);
    }

    pclose(pipe);
    return result;
}
