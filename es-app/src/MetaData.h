#pragma once
#ifndef ES_APP_META_DATA_H
#define ES_APP_META_DATA_H

#include <map>
#include <vector>

class SystemData;

namespace pugi { class xml_node; }

enum MetaDataType
{
	//generic types
	MD_STRING,
	MD_INT,
	MD_FLOAT,
	MD_BOOL,

	//specialized types
	MD_MULTILINE_STRING,
	MD_PATH,
	MD_RATING,
	MD_DATE,
	MD_TIME, //used for lastplayed
	MD_PLIST
};

namespace MetaDataImportType
{
	enum Types : int
	{
		IMAGE = 1,
		THUMB = 2,
		VIDEO = 4,
		MARQUEE = 8,
		ALL = IMAGE | THUMB | VIDEO | MARQUEE
	};
}

struct MetaDataDecl
{
	unsigned char id;

	std::string key;
	MetaDataType type;
	std::string defaultValue;
	bool isStatistic; //if true, ignore scraper values for this metadata
	std::string displayName; // displayed as this in editors
	std::string displayPrompt; // phrase displayed in editors when prompted to enter value (currently only for strings)
};

enum MetaDataListType
{
	GAME_METADATA,
	FOLDER_METADATA
};

const std::vector<MetaDataDecl>& getMDDByType(MetaDataListType type);

class MetaDataList
{
public:
	static MetaDataList createFromXML(MetaDataListType type, pugi::xml_node& node, SystemData* system);
	void appendToXML(pugi::xml_node& parent, bool ignoreDefaults, const std::string& relativeTo) const;

	MetaDataList(MetaDataListType type);

	void set(const std::string& key, const std::string& value);

	const std::string get(const std::string& key) const;
	int getInt(const std::string& key) const;
	float getFloat(const std::string& key) const;

	bool wasChanged() const;
	void resetChangedFlag();
	void setDirty() { mWasChanged = true; }

	inline MetaDataListType getType() const { return (MetaDataListType) mType; }
	inline const std::vector<MetaDataDecl>& getMDD() const { return getMDDByType(getType()); }
	const std::string& getName() const;

	void importScrappedMetadata(const MetaDataList& source);

private:
	std::string		mName;
	unsigned char	mType;
	bool			mWasChanged;
	SystemData*		mRelativeTo;

	std::map<unsigned char, std::string> mMap;

	unsigned char getId(const std::string& key) const;	
	MetaDataType getType(unsigned char id) const;


private: // Static maps

	static std::map<unsigned char, std::string> mDefaultGameMap;
	static std::map<unsigned char, std::string> mDefaultFolderMap;

	static std::map<std::string, unsigned char> mGameIdMap;
	static std::map<std::string, unsigned char> mFolderIdMap;

	static std::map<unsigned char, MetaDataType> mGameTypeMap;
	static std::map<unsigned char, MetaDataType> mFolderTypeMap;

	static std::map<unsigned char, std::string> BuildDefaultMap(MetaDataListType type);
	static std::map<std::string, unsigned char> BuildIdMap(MetaDataListType type);
	static std::map<unsigned char, MetaDataType> BuildTypeMap(MetaDataListType type);

};

#endif // ES_APP_META_DATA_H
