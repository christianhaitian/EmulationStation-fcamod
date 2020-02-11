#include "MetaData.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include <pugixml/src/pugixml.hpp>
#include "SystemData.h"
#include "Settings.h"

MetaDataDecl gameDecls[] = {
	// key,         type,                   default,            statistic,  name in GuiMetaDataEd,  prompt in GuiMetaDataEd
	{ 0,  "name",        MD_STRING,              "",                 false,      "name",                 "enter game name"},
//	{ 1,  "sortname",    MD_STRING,              "",                 false,      "sortname",             "enter game sort name"},
	{ 2,  "desc",        MD_MULTILINE_STRING,    "",                 false,      "description",          "enter description"},
	{ 3,  "emulator",    MD_PLIST,				 "",                 false,      "emulator",			 "emulator" },
	{ 4,  "core",	     MD_PLIST,				 "",                 false,      "core",				 "core" },	
	{ 5,  "image",       MD_PATH,                "",                 false,      "image",                "enter path to image"},
	{ 6,  "video",       MD_PATH     ,           "",                 false,      "video",                "enter path to video"},
	{ 7,  "marquee",     MD_PATH,                "",                 false,      "marquee",              "enter path to marquee"},
	{ 8,  "thumbnail",   MD_PATH,                "",                 false,      "thumbnail",            "enter path to thumbnail"},
	{ 9,  "rating",      MD_RATING,              "0.000000",         false,      "rating",               "enter rating"},
	{ 10, "releasedate", MD_DATE,                "not-a-date-time",  false,      "release date",         "enter release date"},
	{ 11, "developer",   MD_STRING,              "unknown",          false,      "developer",            "enter game developer"},
	{ 12, "publisher",   MD_STRING,              "unknown",          false,      "publisher",            "enter game publisher"},
	{ 13, "genre",       MD_STRING,              "unknown",          false,      "genre",                "enter game genre"},
	{ 14, "players",     MD_INT,                 "1",                false,      "players",              "enter number of players"},
	{ 15, "favorite",    MD_BOOL,                "false",            false,      "favorite",             "enter favorite off/on"},
	{ 16, "hidden",      MD_BOOL,                "false",            false,      "hidden",               "enter hidden off/on" },
	{ 17, "kidgame",     MD_BOOL,                "false",            false,      "kidgame",              "enter kidgame off/on" },
	{ 18, "playcount",   MD_INT,                 "0",                true,       "play count",           "enter number of times played"},
	{ 19, "lastplayed",  MD_TIME,                "0",                true,       "last played",          "enter last played date"},
	{ 20, "arcadesystemname",  MD_STRING,        "",                 false,      "arcade system",        "enter arcade system name"}	
};

const std::vector<MetaDataDecl> gameMDD(gameDecls, gameDecls + sizeof(gameDecls) / sizeof(gameDecls[0]));

MetaDataDecl folderDecls[] = {
	{ 0,  "name",        MD_STRING,              "",                 false,      "name",                 "enter game name"},
//	{ 1,  "sortname",    MD_STRING,              "",                 false,      "sortname",             "enter game sort name"},
	{ 2,  "desc",        MD_MULTILINE_STRING,    "",                 false,      "description",          "enter description"},
	{ 3,  "image",       MD_PATH,                "",                 false,      "image",                "enter path to image"},
	{ 4,  "thumbnail",   MD_PATH,                "",                 false,      "thumbnail",            "enter path to thumbnail"},
	{ 5,  "video",       MD_PATH,                "",                 false,      "video",                "enter path to video"},
	{ 6,  "marquee",     MD_PATH,                "",                 false,      "marquee",              "enter path to marquee"},
	{ 7,  "rating",      MD_RATING,              "0.000000",         false,      "rating",               "enter rating"},
	{ 8,  "releasedate", MD_DATE,                "not-a-date-time",  false,      "release date",         "enter release date"},
	{ 9,  "developer",   MD_STRING,              "unknown",          false,      "developer",            "enter game developer"},
	{ 10, "publisher",   MD_STRING,              "unknown",          false,      "publisher",            "enter game publisher"},
	{ 11, "genre",       MD_STRING,              "unknown",          false,      "genre",                "enter game genre"},
	{ 12, "players",     MD_INT,                 "1",                false,      "players",              "enter number of players"},
	{ 13, "favorite",    MD_BOOL,                "false",            false,      "favorite",             "enter favorite off/on" },
	{ 14, "hidden",      MD_BOOL,                "false",            false,      "hidden",               "enter hidden off/on" },
};

const std::vector<MetaDataDecl> folderMDD(folderDecls, folderDecls + sizeof(folderDecls) / sizeof(folderDecls[0]));

std::map<std::string, unsigned char> MetaDataList::mGameIdMap = MetaDataList::BuildIdMap(GAME_METADATA);
std::map<std::string, unsigned char> MetaDataList::mFolderIdMap = MetaDataList::BuildIdMap(FOLDER_METADATA);

std::map<unsigned char, MetaDataType> MetaDataList::mGameTypeMap = MetaDataList::BuildTypeMap(GAME_METADATA);
std::map<unsigned char, MetaDataType> MetaDataList::mFolderTypeMap = MetaDataList::BuildTypeMap(FOLDER_METADATA);

std::map<unsigned char, std::string> MetaDataList::mDefaultGameMap = MetaDataList::BuildDefaultMap(GAME_METADATA);
std::map<unsigned char, std::string> MetaDataList::mDefaultFolderMap = MetaDataList::BuildDefaultMap(FOLDER_METADATA);

std::map<unsigned char, MetaDataType> MetaDataList::BuildTypeMap(MetaDataListType type)
{
	std::map<unsigned char, MetaDataType> ret;

	const std::vector<MetaDataDecl>& mdd = getMDDByType(type);
	for (auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
		ret[iter->id] = iter->type;

	return ret;
}

std::map<std::string, unsigned char> MetaDataList::BuildIdMap(MetaDataListType type)
{
	std::map<std::string, unsigned char> ret;

	const std::vector<MetaDataDecl>& mdd = getMDDByType(type);
	for (auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
		ret[iter->key] = iter->id;

	return ret;
}

std::map<unsigned char, std::string> MetaDataList::BuildDefaultMap(MetaDataListType type)
{
	std::map<unsigned char, std::string> ret;

	const std::vector<MetaDataDecl>& mdd = getMDDByType(type);
	for (auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
		ret[iter->id] = iter->defaultValue;		

	return ret;
}

MetaDataType MetaDataList::getType(unsigned char id) const
{
	if (mType == GAME_METADATA)
		return mGameTypeMap[id];

	return mFolderTypeMap[id];
}

unsigned char MetaDataList::getId(const std::string& key) const
{
	if (mType == GAME_METADATA)
		return mGameIdMap[key];

	return mFolderIdMap[key];
}

const std::vector<MetaDataDecl>& getMDDByType(MetaDataListType type)
{
	switch(type)
	{
	case GAME_METADATA:
		return gameMDD;
	case FOLDER_METADATA:
		return folderMDD;
	}

	LOG(LogError) << "Invalid MDD type";
	return gameMDD;
}

MetaDataList::MetaDataList(MetaDataListType type) : mType(type), mWasChanged(false), mRelativeTo(nullptr)
{ 

}

MetaDataList MetaDataList::createFromXML(MetaDataListType type, pugi::xml_node& node, SystemData* system)
{
	MetaDataList mdl(type);
	mdl.mRelativeTo = system;

	auto sz = sizeof(MetaDataList);

	const std::vector<MetaDataDecl>& mdd = mdl.getMDD();

	for(auto iter = mdd.cbegin(); iter != mdd.cend(); iter++)
	{
		pugi::xml_node md = node.child(iter->key.c_str());
		if (md)
		{			
			std::string value = md.text().get();

		//	if (iter->type == MD_PATH) // if it's a path, resolve relative paths
		//		value = Utils::FileSystem::resolveRelativePath(value, relativeTo, true);

			if (value == iter->defaultValue)
				continue;
			
			if (iter->type == MD_BOOL)
				value = Utils::String::toLower(value);

			// Players -> remove "1-"
			if (type == GAME_METADATA && iter->id == 14 && iter->type == MD_INT && Utils::String::startsWith(value, "1-")) // "players"
				value = Utils::String::replace(value, "1-", "");
			
			if (iter->id == 0)
				mdl.mName = value;
			else
				mdl.mMap[iter->id] = value;
		}
	}

	return mdl;
}

void MetaDataList::appendToXML(pugi::xml_node& parent, bool ignoreDefaults, const std::string& relativeTo) const
{
	const std::vector<MetaDataDecl>& mdd = getMDD();

	for(auto mddIter = mdd.cbegin(); mddIter != mdd.cend(); mddIter++)
	{
		if (mddIter->id == 0)
		{
			parent.append_child("name").text().set(mName.c_str());
			continue;
		}

		auto mapIter = mMap.find(mddIter->id);
		if (mapIter != mMap.cend())
		{
			// we have this value!
			// if it's just the default (and we ignore defaults), don't write it
			if (ignoreDefaults && mapIter->second == mddIter->defaultValue)
				continue;
			
			// try and make paths relative if we can
			std::string value = mapIter->second;
			if (mddIter->type == MD_PATH)
				value = Utils::FileSystem::createRelativePath(value, relativeTo, true);

			parent.append_child(mddIter->key.c_str()).text().set(value.c_str()); // mapIter->first
		}
	}
}

const std::string& MetaDataList::getName() const
{
	return mName;
}

void MetaDataList::set(const std::string& key, const std::string& value)
{
	if (key == "name")
	{
		if (mName == value)
			return;

		mName = value;
	}
	else
	{
		auto id = getId(key);
		
		// Players -> remove "1-"
		if (mType == GAME_METADATA && id == 14 && Utils::String::startsWith(value, "1-")) // "players"
		{
			mMap[id] = Utils::String::replace(value, "1-", "");
			return;
		}

		auto prev = mMap.find(id);
		if (prev != mMap.cend() && prev->second == value)
			return;

		mMap[id] = value;
	}

	mWasChanged = true;
}

const std::string MetaDataList::get(const std::string& key) const
{
	if (key == "name")
		return mName;

	auto id = getId(key);

	auto it = mMap.find(id);
	if (it != mMap.end())
	{		
		if (getType(id) == MD_PATH && mRelativeTo != nullptr) // if it's a path, resolve relative paths				
			return Utils::FileSystem::resolveRelativePath(it->second, mRelativeTo->getStartPath(), true);

		return it->second;
	}

	if (mType == GAME_METADATA)
		return mDefaultGameMap.at(id);

	return mDefaultFolderMap.at(id);
}

int MetaDataList::getInt(const std::string& key) const
{
	return atoi(get(key).c_str());
}

float MetaDataList::getFloat(const std::string& key) const
{
	return (float)atof(get(key).c_str());
}

bool MetaDataList::wasChanged() const
{
	return mWasChanged;
}

void MetaDataList::resetChangedFlag()
{
	mWasChanged = false;
}

void MetaDataList::importScrappedMetadata(const MetaDataList& source)
{
	int type = MetaDataImportType::Types::ALL;

	if (Settings::getInstance()->getString("Scraper") == "ScreenScraper")
	{
		if (Settings::getInstance()->getString("ScrapperImageSrc").empty())
			type &= ~MetaDataImportType::Types::IMAGE;

		if (Settings::getInstance()->getString("ScrapperThumbSrc").empty())
			type &= ~MetaDataImportType::Types::THUMB;

		if (Settings::getInstance()->getString("ScrapperLogoSrc").empty())
			type &= ~MetaDataImportType::Types::MARQUEE;

		if (!Settings::getInstance()->getBool("ScrapeVideos"))
			type &= ~MetaDataImportType::Types::VIDEO;
	}

	for (auto mdd : getMDD())
	{
		if (mdd.key == "favorite" || mdd.key == "playcount" || mdd.key == "lastplayed")
			continue;

		if (mdd.key == "image" && (type & MetaDataImportType::Types::IMAGE) != MetaDataImportType::Types::IMAGE)
			continue;

		if (mdd.key == "thumbnail" && (type & MetaDataImportType::Types::THUMB) != MetaDataImportType::Types::THUMB)
			continue;

		if (mdd.key == "marquee" && (type & MetaDataImportType::Types::MARQUEE) != MetaDataImportType::Types::MARQUEE)
			continue;

		if (mdd.key == "video" && (type & MetaDataImportType::Types::VIDEO) != MetaDataImportType::Types::VIDEO)
			continue;

		set(mdd.key, source.get(mdd.key));
	}
}