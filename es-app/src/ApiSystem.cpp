#include "ApiSystem.h"
#include "HttpReq.h"
#include "utils/FileSystemUtil.h"
#include <thread>

std::string ApiSystem::checkUpdateVersion()
{
	std::string localVersion;
	std::string localVersionFile = Utils::FileSystem::getExePath() + "/version.info";
	if (Utils::FileSystem::exists(localVersion))
		localVersion = Utils::FileSystem::readAllText(localVersionFile);

	std::shared_ptr<HttpReq> httpreq = std::make_shared<HttpReq>("https://github.com/fabricecaruso/EmulationStation/releases/download/continuous-master/version.info");

	while (httpreq->status() == HttpReq::REQ_IN_PROGRESS)
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

	if (httpreq->status() == HttpReq::REQ_SUCCESS)
	{
		std::string serverVersion = httpreq->getContent();
		if (!serverVersion.empty() && serverVersion != localVersion)
			return serverVersion;
	}
	
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
#endif

/*
// BusyComponent* ui
std::pair<std::string, int> ApiSystem::updateSystem(const std::function<void(const std::string)>& func)
{
#if defined(WIN32) && defined(_DEBUG)
	for (int i = 0; i < 100; i += 2)
	{
		if (func != nullptr)
			func(std::string("Downloading >>> " + std::to_string(i) + " %"));

		::Sleep(200);
	}

	if (func != nullptr)
		func(std::string("Extracting files"));

	::Sleep(750);

	return std::pair<std::string, int>("done.", 0);
#endif

	LOG(LogDebug) << "ApiSystem::updateSystem";

	std::string updatecommand = "batocera-upgrade";

	FILE *pipe = popen(updatecommand.c_str(), "r");
	if (pipe == nullptr)
		return std::pair<std::string, int>(std::string("Cannot call update command"), -1);

	char line[1024] = "";
	FILE *flog = fopen("/userdata/system/logs/batocera-upgrade.log", "w");
	while (fgets(line, 1024, pipe))
	{
		strtok(line, "\n");
		if (flog != nullptr)
			fprintf(flog, "%s\n", line);

		if (func != nullptr)
			func(std::string(line));
	}

	int exitCode = pclose(pipe);

	if (flog != NULL)
	{
		fprintf(flog, "Exit code : %d\n", exitCode);
		fclose(flog);
	}

	return std::pair<std::string, int>(std::string(line), exitCode);
}*/