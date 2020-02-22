#include "ApiSystem.h"
#include "HttpReq.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include <thread>
#include <codecvt> 
#include <locale> 
#include "Log.h"
#include "Window.h"
#include "components/AsyncNotificationComponent.h"

UpdateState::State ApiSystem::state = UpdateState::State::NO_UPDATE;

class ThreadedUpdater
{
public:
	ThreadedUpdater(Window* window) : mWindow(window)
	{
		ApiSystem::state = UpdateState::State::UPDATER_RUNNING;

		mWndNotification = new AsyncNotificationComponent(window, false);
		mWndNotification->updateTitle(_U("\uF019 ") + _("EMULATIONSTATION"));

		mWindow->registerNotificationComponent(mWndNotification);
		mHandle = new std::thread(&ThreadedUpdater::threadUpdate, this);
	}

	~ThreadedUpdater()
	{
		mWindow->unRegisterNotificationComponent(mWndNotification);
		delete mWndNotification;
	}

	void threadUpdate()
	{
		std::pair<std::string, int> updateStatus = ApiSystem::updateSystem([this](const std::string info)
		{
			auto pos = info.find(">>>");
			if (pos != std::string::npos)
			{
				std::string percent(info.substr(pos));
				percent = Utils::String::replace(percent, ">", "");
				percent = Utils::String::replace(percent, "%", "");
				percent = Utils::String::replace(percent, " ", "");

				int value = atoi(percent.c_str());

				std::string text(info.substr(0, pos));
				text = Utils::String::trim(text);

				mWndNotification->updatePercent(value);
				mWndNotification->updateText(text);
			}
			else
			{
				mWndNotification->updatePercent(-1);
				mWndNotification->updateText(info);
			}
		});

		if (updateStatus.second == 0)
		{
			ApiSystem::state = UpdateState::State::UPDATE_READY;

			mWndNotification->updateTitle(_U("\uF019 ") + _("UPDATE IS READY"));
			mWndNotification->updateText(_("RESTART EMULATIONSTATION TO APPLY"));

			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::hours(12));
		}
		else
		{
			ApiSystem::state = UpdateState::State::NO_UPDATE;

			std::string error = _("AN ERROR OCCURED") + std::string(": ") + updateStatus.first;
			mWindow->displayNotificationMessage(error);
		}

		delete this;
	}

private:
	std::thread*				mHandle;
	AsyncNotificationComponent* mWndNotification;
	Window*						mWindow;
};

void ApiSystem::startUpdate(Window* c)
{
#if WIN32
	new ThreadedUpdater(c);
#endif
}

std::string ApiSystem::checkUpdateVersion()
{
#if WIN32
	std::string localVersion;
	std::string localVersionFile = Utils::FileSystem::getExePath() + "/version.info";
	if (Utils::FileSystem::exists(localVersionFile))
	{
		localVersion = Utils::FileSystem::readAllText(localVersionFile);
		localVersion = Utils::String::replace(Utils::String::replace(localVersion, "\r", ""), "\n", "");
	}
	
	HttpReq httpreq("https://github.com/fabricecaruso/EmulationStation/releases/download/continuous-master/version.info");	
	if (httpreq.wait())
	{
		std::string serverVersion = httpreq.getContent();
		serverVersion = Utils::String::replace(Utils::String::replace(serverVersion, "\r", ""), "\n", "");
		if (!serverVersion.empty() && serverVersion != localVersion)
			return serverVersion;
	}
#endif

	return "";
}

#if WIN32
#include <ShlDisp.h>
#include <comutil.h> // #include for _bstr_t
#pragma comment(lib, "shell32.lib")
#pragma comment (lib, "comsuppw.lib" ) // link with "comsuppw.lib" (or debug version: "comsuppwd.lib")

bool unzipFile(const std::string fileName, const std::string dest)
{
	bool	ret = false;

	HRESULT          hResult;
	IShellDispatch*	 pISD;
	Folder*			 pFromZip = nullptr;
	VARIANT          vDir, vFile, vOpt;

	OleInitialize(NULL);
	CoInitialize(NULL);

	hResult = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&pISD);

	if (SUCCEEDED(hResult))
	{
		VariantInit(&vDir);
		vDir.vt = VT_BSTR;

		int zipDirLen = (lstrlenA(fileName.c_str()) + 1) * sizeof(WCHAR);
		BSTR bstrZip = SysAllocStringByteLen(NULL, zipDirLen);
		MultiByteToWideChar(CP_ACP, 0, fileName.c_str(), -1, bstrZip, zipDirLen);
		vDir.bstrVal = bstrZip;

		hResult = pISD->NameSpace(vDir, &pFromZip);

		if (hResult == S_OK && pFromZip != nullptr)
		{
			if (!Utils::FileSystem::exists(dest))
				Utils::FileSystem::createDirectory(dest);

			Folder *pToFolder = NULL;

			VariantInit(&vFile);
			vFile.vt = VT_BSTR;

			int fnLen = (lstrlenA(dest.c_str()) + 1) * sizeof(WCHAR);
			BSTR bstrFolder = SysAllocStringByteLen(NULL, fnLen);
			MultiByteToWideChar(CP_ACP, 0, dest.c_str(), -1, bstrFolder, fnLen);
			vFile.bstrVal = bstrFolder;

			hResult = pISD->NameSpace(vFile, &pToFolder);
			if (hResult == S_OK && pToFolder)
			{
				FolderItems *fi = NULL;
				pFromZip->Items(&fi);

				VariantInit(&vOpt);
				vOpt.vt = VT_I4;
				vOpt.lVal = FOF_NO_UI; //4; // Do not display a progress dialog box

				VARIANT newV;
				VariantInit(&newV);
				newV.vt = VT_DISPATCH;
				newV.pdispVal = fi;
				hResult = pToFolder->CopyHere(newV, vOpt);
				if (hResult == S_OK)
					ret = true;

				pFromZip->Release();
				pToFolder->Release();
			}
		}
		pISD->Release();
	}

	CoUninitialize();
	return ret;
}

bool downloadFile(const std::string url, const std::string fileName, const std::string label, const std::function<void(const std::string)>& func)
{
	if (func != nullptr)
		func("Downloading " + label);

	HttpReq httpreq(url, fileName);
	while (httpreq.status() == HttpReq::REQ_IN_PROGRESS)
	{
		if (func != nullptr)
			func(std::string("Downloading " + label + " >>> " + std::to_string(httpreq.getPercent()) + " %"));

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	if (httpreq.status() != HttpReq::REQ_SUCCESS)
		return false;

	return true;
}


void deleteDirectoryFiles(const std::string path)
{
	auto files = Utils::FileSystem::getDirContent(path, true, true);
	std::reverse(std::begin(files), std::end(files));
	for (auto file : files)
	{
		if (Utils::FileSystem::isDirectory(file))
			::RemoveDirectoryA(Utils::FileSystem::getPreferredPath(file).c_str());
		else
			Utils::FileSystem::removeFile(file);
	}
}
#endif

std::pair<std::string, int> ApiSystem::updateSystem(const std::function<void(const std::string)>& func)
{
#if WIN32
	std::string url = "https://github.com/fabricecaruso/EmulationStation/releases/download/continuous-master/EmulationStation-Win32-no-deps.zip";

	std::string fileName = Utils::FileSystem::getFileName(url);
	std::string path = Utils::FileSystem::getHomePath() + "/.emulationstation/update";

	if (!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);
	else
		deleteDirectoryFiles(path);

	std::string zipFile = path + "/" + fileName;

	if (downloadFile(url, zipFile, "update", func))
	{
		if (func != nullptr)
			func(std::string("Extracting update"));

		unzipFile(Utils::FileSystem::getPreferredPath(zipFile), Utils::FileSystem::getPreferredPath(path));
		Utils::FileSystem::removeFile(zipFile);

		auto files = Utils::FileSystem::getDirContent(path, true, true);
		for (auto file : files)
		{
			
			std::string relative = Utils::FileSystem::createRelativePath(file, path, false);
			if (Utils::String::startsWith(relative, "./"))
				relative = relative.substr(2);

			std::string localPath = Utils::FileSystem::getExePath() + "/" + relative;

			if (Utils::FileSystem::isDirectory(file))
			{
				if (!Utils::FileSystem::exists(localPath))
					Utils::FileSystem::createDirectory(localPath);
			}
			else
			{
				if (Utils::FileSystem::exists(localPath))
				{
					Utils::FileSystem::removeFile(localPath + ".old");
					rename(localPath.c_str(), (localPath + ".old").c_str());
				}

				if (Utils::FileSystem::copyFile(file, localPath))
				{
					Utils::FileSystem::removeFile(localPath + ".old");
					Utils::FileSystem::removeFile(file);
				}
			}
		}

		deleteDirectoryFiles(path);

		return std::pair<std::string, int>("done.", 0);
	}
#endif

	return std::pair<std::string, int>("error.", 1);
}

std::vector<ThemeDownloadInfo> ApiSystem::getThemesList()
{
	LOG(LogDebug) << "ApiSystem::getThemesList";

	std::vector<ThemeDownloadInfo> res;

	HttpReq httpreq("https://batocera.org/upgrades/themes.txt");
	if (httpreq.wait())
	{
		auto lines = Utils::String::split(httpreq.getContent(), '\n');
		for (auto line : lines)
		{
			auto parts = Utils::String::splitAny(line, " \t");
			if (parts.size() > 1)
			{
				auto themeName = parts[0];

				std::string themeUrl = parts[1];
				std::string themeFolder = Utils::FileSystem::getFileName(themeUrl);

				bool themeExists = false;

				std::vector<std::string> paths{
					Utils::FileSystem::getHomePath() + "/.emulationstation/themes",
					"/etc/emulationstation/themes",
					"/userdata/themes"
				};

				for (auto path : paths)
				{
					themeExists = Utils::FileSystem::isDirectory(path + "/" + themeName) ||
						Utils::FileSystem::isDirectory(path + "/" + themeFolder) ||
						Utils::FileSystem::isDirectory(path + "/" + themeFolder + "-master");
					
					if (themeExists)
						break;
				}

				ThemeDownloadInfo info;
				info.installed = themeExists;
				info.name = themeName;
				info.url = themeUrl;

				res.push_back(info);
			}
		}
	}

	return res;
}

bool downloadGitRepository(const std::string url, const std::string fileName, const std::string label, const std::function<void(const std::string)>& func)
{
	if (func != nullptr)
		func(_("Downloading") + " " + label);

	long downloadSize = 0;

	std::string statUrl = Utils::String::replace(url, "https://github.com/", "https://api.github.com/repos/");
	if (statUrl != url)
	{
		HttpReq statreq(statUrl);
		if (statreq.wait())
		{
			std::string content = statreq.getContent();
			auto pos = content.find("\"size\": ");
			if (pos != std::string::npos)
			{
				auto end = content.find(",", pos);
				if (end != std::string::npos)
					downloadSize = atoi(content.substr(pos + 8, end - pos - 8).c_str()) * 1024;
			}
		}
	}

	HttpReq httpreq(url + "/archive/master.zip", fileName);

	int curPos = -1;
	while (httpreq.status() == HttpReq::REQ_IN_PROGRESS)
	{
		if (downloadSize > 0)
		{
			double pos = httpreq.getPosition();
			if (pos > 0 && curPos != pos)
			{
				if (func != nullptr)
				{
					std::string pc = std::to_string((int)(pos * 100.0 / downloadSize));
					func(std::string(_("Downloading") + " " + label + " >>> " + pc + " %"));
				}

				curPos = pos;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	if (httpreq.status() != HttpReq::REQ_SUCCESS)
		return false;

	return true;
}

std::pair<std::string, int> ApiSystem::installTheme(std::string themeName, const std::function<void(const std::string)>& func)
{
#if WIN32
	for (auto theme : getThemesList())
	{		
		if (theme.name != themeName)
			continue;

		std::string themeFileName = Utils::FileSystem::getFileName(theme.url);
		std::string zipFile = Utils::FileSystem::getHomePath() + "/.emulationstation/themes/" + themeFileName + ".zip";
		zipFile = Utils::String::replace(zipFile, "/", "\\");

		if (downloadGitRepository(theme.url, zipFile, themeName, func))
		{
			if (func != nullptr)
				func(_("Extracting") + " " + themeName);

			if (!unzipFile(zipFile, Utils::String::replace(Utils::FileSystem::getHomePath() + "/.emulationstation/themes", "/", "\\")))
				return std::pair<std::string, int>(std::string("An error occured while extracting"), 1);

			std::string folderName = Utils::FileSystem::getHomePath() + "/.emulationstation/themes/" + themeFileName + "-master";
			if (Utils::FileSystem::isDirectory(folderName))
			{
				std::string finalfolderName = Utils::FileSystem::getParent(folderName) + "/" + themeName;

				if (Utils::FileSystem::isDirectory(finalfolderName))
					deleteDirectoryFiles(finalfolderName);

				rename(folderName.c_str(), finalfolderName.c_str());

				Utils::FileSystem::removeFile(zipFile);

				return std::pair<std::string, int>(std::string("OK"), 0);
			}			

			return std::pair<std::string, int>(std::string("Invalid extraction folder"), 1);
		}
				
		return std::pair<std::string, int>(std::string("An error occured while downloading"), 1);
	}
#endif

	return std::pair<std::string, int>(std::string("Theme not found"), 1);
}