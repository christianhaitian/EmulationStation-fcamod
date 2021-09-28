#pragma once
#ifndef ES_APP_SYSTEM_DATA_H
#define ES_APP_SYSTEM_DATA_H

#include "PlatformId.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <pugixml/src/pugixml.hpp>
#include "math/Vector2f.h"
#include <unordered_map>
#include <unordered_set>

#include "FileFilterIndex.h"
#include "Settings.h"

class FileData;
class FolderData;
class ThemeData;
class Window;

struct EmulatorData
{
	std::string mName;
	std::string mCommandLine;
	std::vector<std::string> mCores;
};

struct SystemEnvironmentData
{
	std::string mSystemName;

	std::string mStartPath;
	std::unordered_set<std::string> mSearchExtensions;
	std::string mLaunchCommand;
	std::vector<PlatformIds::PlatformId> mPlatformIds;
	std::vector<EmulatorData> mEmulators;
	std::string mGroup;

	bool isValidExtension(const std::string extension)
	{
		return mSearchExtensions.find(extension) != mSearchExtensions.cend();		
	}

	std::vector<std::string> getCores(std::string emulatorName) 
	{
		std::vector<std::string> list;

		for (auto& emulator : mEmulators)
			if (emulatorName == emulator.mName)
				return emulator.mCores;

		return list;
	}

	std::string getDefaultEmulator()
	{
		std::string currentEmul = Settings::getInstance()->getString(mSystemName + ".emulator");

		for (auto& emulator : mEmulators)
			if (currentEmul == emulator.mName)
				return emulator.mName;

		for (auto& emulator : mEmulators)
			return emulator.mName;

		return "";
	}

	std::string getDefaultCore(std::string emulatorName)
	{
		std::string currentCore = Settings::getInstance()->getString(mSystemName + ".core");

		for (auto& emulator : mEmulators)
		{
			if (emulatorName == emulator.mName)
			{
				for (auto core : emulator.mCores)
					if (core == currentCore)
						return core;

				for (auto core : emulator.mCores)
					return core;
			}
		}

		return "";
	}

	std::string getEmulatorCommandLine(std::string emulatorName)
	{
		for (auto& emulator : mEmulators)
			if (emulatorName == emulator.mName)
				return emulator.mCommandLine;

		return "";
	}
};

class SystemData
{
public:
	SystemData(const std::string& name, const std::string& fullName, SystemEnvironmentData* envData, const std::string& themeFolder, bool CollectionSystem = false, bool groupedSystem = false);
	~SystemData();

	static SystemData* getSystem(const std::string name);

	inline FolderData* getRootFolder() const { return mRootFolder; };
	inline const std::string& getName() const { return mName; }
	inline const std::string& getFullName() const { return mFullName; }
	inline const std::string& getStartPath() const { return mEnvData->mStartPath; }
	//inline const std::vector<std::string>& getExtensions() const { return mEnvData->mSearchExtensions; }
	inline const std::string& getThemeFolder() const { return mThemeFolder; }
	inline SystemEnvironmentData* getSystemEnvData() const { return mEnvData; }
	inline const std::vector<PlatformIds::PlatformId>& getPlatformIds() const { return mEnvData->mPlatformIds; }
	inline bool hasPlatformId(PlatformIds::PlatformId id) { if (!mEnvData) return false; return std::find(mEnvData->mPlatformIds.cbegin(), mEnvData->mPlatformIds.cend(), id) != mEnvData->mPlatformIds.cend(); }

	inline const std::shared_ptr<ThemeData>& getTheme() const { return mTheme; }

	std::string getSystemViewMode() const { if (mViewMode == "automatic") return ""; else return mViewMode; };
	bool setSystemViewMode(std::string newViewMode, Vector2f gridSizeOverride = Vector2f(0,0), bool setChanged = true);

	Vector2f getGridSizeOverride();

	std::string getGamelistPath(bool forWrite) const;
	bool hasGamelist() const;
	std::string getThemePath() const;

	unsigned int getGameCount() const;
	
	int getDisplayedGameCount();
	void updateDisplayedGameCount();

	static bool hasDirtySystems();
	static void deleteSystems();
	static bool loadConfig(Window* window); //Load the system config file at getConfigPath(). Returns true if no errors were encountered. An example will be written if the file doesn't exist.
	static void writeExampleConfig(const std::string& path);
	static std::string getConfigPath(bool forWrite); // if forWrite, will only return ~/.emulationstation/es_systems.cfg, never /etc/emulationstation/es_systems.cfg

	static std::vector<SystemData*> sSystemVector;

	inline std::vector<SystemData*>::const_iterator getIterator() const { return std::find(sSystemVector.cbegin(), sSystemVector.cend(), this); };
	inline std::vector<SystemData*>::const_reverse_iterator getRevIterator() const { return std::find(sSystemVector.crbegin(), sSystemVector.crend(), this); };
	inline bool isCollection() { return mIsCollectionSystem; };
	inline bool isGameSystem() { return mIsGameSystem; };

	inline bool isGroupSystem() { return mIsGroupSystem; };	
	bool isGroupChildSystem();

	bool isVisible();
	
	SystemData* getNext() const;
	SystemData* getPrev() const;
	static SystemData* getRandomSystem();
	FileData* getRandomGame();

	// Load or re-load theme.
	void loadTheme();

	FileFilterIndex* getIndex(bool createIndex = false);

	void removeFromIndex(FileData* game) {
		if (mFilterIndex != nullptr) mFilterIndex->removeFromIndex(game);
	};

	void addToIndex(FileData* game) {
		if (mFilterIndex != nullptr) mFilterIndex->addToIndex(game);
	};

	void resetFilters() {
		if (mFilterIndex != nullptr) mFilterIndex->resetFilters();
	};

	void resetIndex() {
		if (mFilterIndex != nullptr) mFilterIndex->resetIndex();
	};
	
	void setUIModeFilters() {
		if (mFilterIndex != nullptr) mFilterIndex->setUIModeFilters();
	}

	void deleteIndex();

	unsigned int getSortId() const { return mSortId; };
	void setSortId(const unsigned int sortId = 0);

	void setGamelistHash(size_t size) { mGameListHash = size; }
	size_t getGamelistHash() { return mGameListHash; }

	SystemData* getParentGroupSystem();

	static std::unordered_set<std::string> getAllGroupNames();
	static std::unordered_set<std::string> getGroupChildSystemNames(const std::string groupName);

private:
	static SystemData* loadSystem(pugi::xml_node system);
	static void createGroupedSystems();

	size_t mGameListHash;
	bool mIsCollectionSystem;
	bool mIsGameSystem;
	bool mIsGroupSystem;
	
	std::string mName;
	std::string mFullName;
	SystemEnvironmentData* mEnvData;
	std::string mThemeFolder;
	std::shared_ptr<ThemeData> mTheme;

	std::string mViewMode;
	Vector2f    mGridSizeOverride;
	bool mViewModeChanged;

	unsigned int mSortId;

	void populateFolder(FolderData* folder, std::unordered_map<std::string, FileData*>& fileMap);
	void indexAllGameFilters(const FolderData* folder);
	void setIsGameSystemStatus();

	FileFilterIndex* mFilterIndex;

	FolderData* mRootFolder;
	int			mGameCount;
};

#endif // ES_APP_SYSTEM_DATA_H
