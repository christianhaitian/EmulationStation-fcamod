#include "Settings.h"

#include "utils/FileSystemUtil.h"
#include "Log.h"
#include "Scripting.h"
#include "platform.h"
#include <pugixml/src/pugixml.hpp>
#include <algorithm>
#include <vector>
#include "utils/StringUtil.h"

Settings* Settings::sInstance = NULL;
static std::string mEmptyString = "";

// these values are NOT saved to es_settings.xml
// since they're set through command-line arguments, and not the in-program settings menu
std::vector<const char*> settings_dont_save {
	{ "Debug" },
	{ "DebugGrid" },
	{ "DebugText" },
	{ "DebugImage" },
	{ "ForceKid" },
	{ "ForceKiosk" },
	{ "IgnoreGamelist" },
	{ "HideConsole" },
	{ "ShowExit" },
	{ "SplashScreen" },
	{ "SplashScreenProgress" },
#if !defined(_WIN32)
	{ "VSync" },
#endif
	{ "FullscreenBorderless" },
	{ "Windowed" },
	{ "WindowWidth" },
	{ "WindowHeight" },
	{ "ScreenWidth" },
	{ "ScreenHeight" },
	{ "ScreenOffsetX" },
	{ "ScreenOffsetY" },
	{ "ScreenRotate" },
	{ "MonitorID" },
	{ "ExePath" },
	{ "HomePath" },
	{ "MusicDirectory"},
	{ "UserMusicDirectory" }
};

Settings::Settings()
{
	mHasConfigRoot = false;
	setDefaults();
	loadFile();
}

Settings* Settings::getInstance()
{
	if(sInstance == NULL)
		sInstance = new Settings();

	return sInstance;
}

void Settings::setDefaults()
{
	mWasChanged = false;

	mBoolMap.clear();
	mIntMap.clear();

	mBoolMap["BackgroundJoystickInput"] = false;
	mBoolMap["ParseGamelistOnly"] = false;
	mBoolMap["ShowHiddenFiles"] = false;
    mBoolMap["IgnoreLeadingArticles"] = false;
	mBoolMap["DrawFramerate"] = false;
	mBoolMap["ShowExit"] = true;		

#if WIN32
	mBoolMap["ShowOnlyExit"] = true;
	mBoolMap["FullscreenBorderless"] = true;
#else
	mBoolMap["ShowOnlyExit"] = false;
	mBoolMap["FullscreenBorderless"] = false;
#endif

	mBoolMap["Windowed"] = false;
	mBoolMap["SplashScreen"] = true;
	mBoolMap["SplashScreenProgress"] = true;
	mBoolMap["PreloadUI"] = false;
	mBoolMap["StartupOnGameList"] = false;
	mBoolMap["HideSystemView"] = false;
	mBoolMap["BrightnessPopup"] = true;
	
	mStringMap["StartupSystem"] = "";

	mStringMap["FolderViewMode"] = "never";

	mBoolMap["VSync"] = true;
	mBoolMap["EnableSounds"] = true;
	mBoolMap["ShowHelpPrompts"] = true;
	mBoolMap["ScrapeRatings"] = true;
	mBoolMap["IgnoreGamelist"] = false;
	mBoolMap["HideConsole"] = true;
	mBoolMap["QuickSystemSelect"] = true;
	mBoolMap["MoveCarousel"] = true;
	mStringMap["VerbalBatteryVoice"] = "male1";
	mStringMap["VerbalBatteryWarning"] = "on";
	mStringMap["GlobalPerformanceGovernor"] = "performance";
	mStringMap["AutoSuspendTimeout"] = "Off";
	mStringMap["GameLoadingImage"] = "default";
	mStringMap["GameLoadingIMode"] = "pic";
	mStringMap["ImagedelayTime"] = "1.5";
	mBoolMap["SaveGamelistsOnExit"] = true;
	mBoolMap["OptimizeVRAM"] = true;	
	mBoolMap["ThreadedLoading"] = true;	
	mBoolMap["MusicTitles"] = true;

	mBoolMap["Debug"] = false;
	mBoolMap["DebugGrid"] = false;
	mBoolMap["DebugText"] = false;
	mBoolMap["DebugImage"] = false;

	mBoolMap["InvertButtons"] = false;
	mBoolMap["InvertPwrBtn"] = false;
	mBoolMap["ScanPorts"] = false;
	mIntMap["ScreenSaverTime"] = 5*60*1000; // 5 minutes
	mIntMap["ScraperResizeWidth"] = 400;
	mIntMap["ScraperResizeHeight"] = 0;

#if defined(_WIN32)
	mIntMap["MaxVRAM"] = 256;
#else
	#ifdef _RPI_
		mIntMap["MaxVRAM"] = 80;
	#else
		mIntMap["MaxVRAM"] = 100;
	#endif
#endif

#if defined(_WIN32)
	mBoolMap["HideWindow"] = false;
#else
	mBoolMap["HideWindow"] = true;
#endif
	mStringMap["GameTransitionStyle"] = "fade";
	mStringMap["TransitionStyle"] = "auto";
	mStringMap["Language"] = "en";	
	mStringMap["ThemeSet"] = "";
	mStringMap["ScreenSaverBehavior"] = "dim";	
	mStringMap["GamelistViewStyle"] = "automatic";
	mStringMap["DefaultGridSize"] = "";
	mStringMap["HiddenSystems"] = "";

	mStringMap["ThemeColorSet"] = "";
	mStringMap["ThemeIconSet"] = "";
	mStringMap["ThemeMenu"] = "";
	mStringMap["ThemeSystemView"] = "";
	mStringMap["ThemeGamelistView"] = "";
	mStringMap["ThemeRegionName"] = "eu";

	mBoolMap["ScreenSaverControls"] = true;
	mStringMap["ScreenSaverGameInfo"] = "never";
	mBoolMap["StretchVideoOnScreenSaver"] = false;
	mStringMap["PowerSaverMode"] = "disabled";

	mIntMap["ScreenSaverSwapImageTimeout"] = 10000;
	mBoolMap["SlideshowScreenSaverStretch"] = false;
	mBoolMap["SlideshowScreenSaverCustomImageSource"] = false;
	mStringMap["SlideshowScreenSaverImageDir"] = Utils::FileSystem::getHomePath() + "/.emulationstation/slideshow/image";
	mStringMap["SlideshowScreenSaverImageFilter"] = ".png,.jpg";
	mBoolMap["SlideshowScreenSaverRecurse"] = false;
	mBoolMap["SlideshowScreenSaverGameName"] = true;
	
	mBoolMap["ShowFilenames"] = false;

	// This setting only applies to raspberry pi but set it for all platforms so
	// we don't get a warning if we encounter it on a different platform
	mBoolMap["VideoOmxPlayer"] = false;
	#ifdef _RPI_
		// we're defaulting to OMX Player for full screen video on the Pi
		mBoolMap["ScreenSaverOmxPlayer"] = true;
		// use OMX Player defaults
		mStringMap["SubtitleFont"] = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";
		mStringMap["SubtitleItalicFont"] = "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf";
		mIntMap["SubtitleSize"] = 55;
		mStringMap["SubtitleAlignment"] = "left";
	#else
		mBoolMap["ScreenSaverOmxPlayer"] = false;
	#endif

	mIntMap["ScreenSaverSwapVideoTimeout"] = 30000;

	mBoolMap["VideoAudio"] = true;
	mBoolMap["VolumePopup"] = true;
	mBoolMap["VideoLowersMusic"] = true;
	mBoolMap["CaptionsCompatibility"] = true;
	// Audio out device for Video playback using OMX player.
	mStringMap["OMXAudioDev"] = "both";
	mStringMap["CollectionSystemsAuto"] = "";
	mStringMap["CollectionSystemsCustom"] = "";
	mBoolMap["CollectionShowSystemInfo"] = true;
	mBoolMap["SortAllSystems"] = false;
	mBoolMap["UseCustomCollectionsSystem"] = true;
	mBoolMap["FavoritesFirst"] = true;
	

	mBoolMap["LocalArt"] = false;

	// Audio out device for volume control
	#ifdef _RPI_
		mStringMap["AudioDevice"] = "PCM";
	#else
		mStringMap["AudioDevice"] = "Master";
	#endif

	mStringMap["AudioCard"] = "default";
	mStringMap["UIMode"] = "Full";
	mStringMap["UIMode_passkey"] = "uuddlrlrba";
	mBoolMap["ForceKiosk"] = false;
	mBoolMap["ForceKid"] = false;
	mBoolMap["ForceDisableFilters"] = false;

	mIntMap["WindowWidth"]   = 0;
	mIntMap["WindowHeight"]  = 0;
	mIntMap["ScreenWidth"]   = 0;
	mIntMap["ScreenHeight"]  = 0;
	mIntMap["ScreenOffsetX"] = 0;
	mIntMap["ScreenOffsetY"] = 0;
	mIntMap["ScreenRotate"]  = 0;
	mIntMap["MonitorID"] = -1;	
	mStringMap["ExePath"] = "";

	mStringMap["Scraper"] = "ScreenScraper";
	mStringMap["ScrapperImageSrc"] = "ss";
	mStringMap["ScrapperThumbSrc"] = "box-2D";
	mStringMap["ScrapperLogoSrc"] = "wheel";
	mStringMap["ScrapperRegionSrc"] = "US";

	mBoolMap["ScrapeVideos"] = false;
	
	mBoolMap["audio.bgmusic"] = true;
	mBoolMap["audio.persystem"] = false;
	mBoolMap["audio.thememusics"] = true;
	
	mStringMap["MusicDirectory"] = "";
	mStringMap["UserMusicDirectory"] = "";

	mBoolMap["updates.enabled"] = false;
	
	mBoolMap["DrawClock"] = true;

	mBoolMap["UseOSK"] = true;

	mDefaultBoolMap = mBoolMap;
	mDefaultIntMap = mIntMap;
	mDefaultFloatMap = mFloatMap;
	mDefaultStringMap = mStringMap;
}

template <typename K, typename V>
void saveMap(pugi::xml_node& doc, std::map<K, V>& map, const char* type, std::map<K, V>& defaultMap)
{
	for(auto iter = map.cbegin(); iter != map.cend(); iter++)
	{
		// key is on the "don't save" list, so don't save it
		if(std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), iter->first) != settings_dont_save.cend())
			continue;

		auto def = defaultMap.find(iter->first);
		if (def != defaultMap.cend() && def->second == iter->second)
			continue;

		pugi::xml_node node = doc.append_child(type);
		node.append_attribute("name").set_value(iter->first.c_str());
		node.append_attribute("value").set_value(iter->second);
	}
}

bool Settings::saveFile()
{
	if (!mWasChanged)
		return false;

	mWasChanged = false;

	LOG(LogDebug) << "Settings::saveFile() : Saving Settings to file.";
	const std::string path = Utils::FileSystem::getHomePath() + "/.emulationstation/es_settings.cfg";

	pugi::xml_document doc;
	pugi::xml_node root = doc;

	if (mHasConfigRoot)
		root = doc.append_child("config"); // batocera, root element

	saveMap<std::string, bool>(root, mBoolMap, "bool", mDefaultBoolMap);
	saveMap<std::string, int>(root, mIntMap, "int", mDefaultIntMap);
	saveMap<std::string, float>(root, mFloatMap, "float", mDefaultFloatMap);

	//saveMap<std::string, std::string>(doc, mStringMap, "string");
	for(auto iter = mStringMap.cbegin(); iter != mStringMap.cend(); iter++)
	{
		// key is on the "don't save" list, so don't save it
		if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), iter->first) != settings_dont_save.cend())
			continue;

		auto def = mDefaultStringMap.find(iter->first);
		if (def == mDefaultStringMap.cend() && iter->second.empty())
			continue;

		if (def != mDefaultStringMap.cend() && def->second == iter->second)
			continue;
		
		pugi::xml_node node = root.append_child("string");
		node.append_attribute("name").set_value(iter->first.c_str());
		node.append_attribute("value").set_value(iter->second.c_str());
	}

	doc.save_file(path.c_str());

	Scripting::fireEvent("config-changed");
	Scripting::fireEvent("settings-changed");

	return true;
}

void Settings::loadFile()
{
	const std::string path = Utils::FileSystem::getHomePath() + "/.emulationstation/es_settings.cfg";

	if(!Utils::FileSystem::exists(path))
		return;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.c_str());
	if(!result)
	{
		LOG(LogError) << "Could not parse Settings file!\n   " << result.description();
		return;
	}

	pugi::xml_node root = doc;

	// Batocera has a <config> root element, learn reading them
	pugi::xml_node config = doc.child("config");
	if (config)
	{
		mHasConfigRoot = true;
		root = config;
	}

	for(pugi::xml_node node = root.child("bool"); node; node = node.next_sibling("bool"))
		setBool(node.attribute("name").as_string(), node.attribute("value").as_bool());
	for(pugi::xml_node node = root.child("int"); node; node = node.next_sibling("int"))
		setInt(node.attribute("name").as_string(), node.attribute("value").as_int());
	for(pugi::xml_node node = root.child("float"); node; node = node.next_sibling("float"))
		setFloat(node.attribute("name").as_string(), node.attribute("value").as_float());
	for(pugi::xml_node node = root.child("string"); node; node = node.next_sibling("string"))
		setString(node.attribute("name").as_string(), node.attribute("value").as_string());

	mWasChanged = false;
}

//Print a warning message if the setting we're trying to get doesn't already exist in the map, then return the value in the map.
#define SETTINGS_GETSET(type, mapName, getMethodName, setMethodName, defaultValue) type Settings::getMethodName(const std::string& name) \
{ \
	if(mapName.find(name) == mapName.cend()) \
	{ \
		/*LOG(LogError) << "Tried to use unset setting " << name << "!";*/ \
		return defaultValue; \
	} \
	return mapName[name]; \
} \
bool Settings::setMethodName(const std::string& name, type value) \
{ \
	if (mapName.count(name) == 0 || mapName[name] != value) { \
		mapName[name] = value; \
\
		if (std::find(settings_dont_save.cbegin(), settings_dont_save.cend(), name) == settings_dont_save.cend()) \
			mWasChanged = true; \
\
		return true; \
	} \
	return false; \
}

SETTINGS_GETSET(bool, mBoolMap, getBool, setBool, false);
SETTINGS_GETSET(int, mIntMap, getInt, setInt, 0);
SETTINGS_GETSET(float, mFloatMap, getFloat, setFloat, 0.0f);
SETTINGS_GETSET(const std::string&, mStringMap, getString, setString, mEmptyString);
