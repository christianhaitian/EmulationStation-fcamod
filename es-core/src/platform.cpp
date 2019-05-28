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

#include "GuiComponent.h"

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

std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
	{
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

void split_cmd(const std::string& cmd,
	std::string* executable,
	std::string* parameters)
{
	std::string c(cmd);
	size_t exec_end;

	c = trim(c);
	//boost::trim_all(c);

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

int runSystemCommand(const std::string& cmd_utf8, const std::string& name, Window* window)
{
#ifdef WIN32
	if (window != NULL)
		window->renderGameLoadingScreen();

	// on Windows we use _wsystem to support non-ASCII paths
	// which requires converting from utf8 to a wstring
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;
	std::wstring wchar_str = converter.from_bytes(cmd_utf8);
	
	std::string exe;
	std::string args;

	split_cmd(cmd_utf8, &exe, &args);
	
	SHELLEXECUTEINFO lpExecInfo;
	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	lpExecInfo.lpFile = exe.c_str();	
	lpExecInfo.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS;
	lpExecInfo.hwnd = NULL;
	lpExecInfo.lpVerb = "open"; // to open  program
	lpExecInfo.lpParameters = args.c_str(); //  file name as an argument
	lpExecInfo.lpDirectory = NULL;
	lpExecInfo.nShow = SW_SHOW;  // show command prompt with normal window size 
	lpExecInfo.hInstApp = (HINSTANCE)SE_ERR_DDEFAIL;   //WINSHELLAPI BOOL WINAPI result;
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
		
		CloseHandle(lpExecInfo.hProcess);
		return 0;
	}
	
	return 1;
	//return _wsystem(wchar_str.c_str());
#else
	return system(cmd_utf8.c_str());
#endif
}

int quitES(const std::string& filename)
{
	if (!filename.empty())
		touch(filename);
	SDL_Event* quit = new SDL_Event();
	quit->type = SDL_QUIT;
	SDL_PushEvent(quit);
	return 0;
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