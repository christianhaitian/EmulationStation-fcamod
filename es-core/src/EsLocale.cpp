#include "EsLocale.h"
#include "resources/ResourceManager.h"
#include "Settings.h"
#include "utils/FileSystemUtil.h"

#include <fstream>

std::map<std::string, std::string> EsLocale::mItems;
std::string EsLocale::mCurrentLanguage = "en";
bool EsLocale::mCurrentLanguageLoaded = true; // By default, 'en' is considered loaded

void EsLocale::setLanguage(const std::string lang)
{
	mCurrentLanguage = lang;
	mCurrentLanguageLoaded = false;
}

const std::string EsLocale::getText(const std::string text)
{
	checkLocalisationLoaded();

	auto item = mItems.find(text);
	if (item != mItems.cend())
		return item->second;

	return text;
}

void EsLocale::checkLocalisationLoaded()
{
	if (mCurrentLanguageLoaded)
	{
		if (Settings::getInstance()->getString("Language") == mCurrentLanguage)
			return;

		mCurrentLanguage = Settings::getInstance()->getString("Language");
	}

	mCurrentLanguageLoaded = true;

	mItems.clear();

	std::string xmlpath = ResourceManager::getInstance()->getResourcePath(":/locale/" + mCurrentLanguage + "/emulationstation2.po");
	if (!Utils::FileSystem::exists(xmlpath))
		return;
	
	std::string	msgid;
	std::string	msgstr;
	std::string line;

	std::ifstream file(xmlpath);
	while (std::getline(file, line))
	{
		if (line.length() > 0 && line[0] == '#')
		{
			if (!msgid.empty() && !msgstr.empty())
				mItems[msgid] = msgstr;

			msgid = "";
			msgstr = "";
		}
		else if (line.find("msgid") == 0)
		{
			auto start = line.find("\"");
			if (start != std::string::npos)
			{
				auto end = line.find("\"", start + 1);
				if (end != std::string::npos)
					msgid = line.substr(start + 1, end - start - 1);
			}
		}
		else if (line.find("msgstr") == 0)
		{
			auto start = line.find("\"");
			if (start != std::string::npos)
			{
				auto end = line.find("\"", start + 1);
				if (end != std::string::npos)
					msgstr = line.substr(start + 1, end - start - 1);
			}
		}
	}

	if (!msgid.empty() && !msgstr.empty())
		mItems[msgid] = msgstr;
}


