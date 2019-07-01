#include "guis/GuiMenu.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiGeneralScreensaverOptions.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperStart.h"
#include "guis/GuiSettings.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "EmulationStation.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include <SDL_events.h>
#include <algorithm>

#include "resources/TextureData.h"
#include "animations/LambdaAnimation.h"
#include "GuiGamelistOptions.h" // grid sizes

GuiMenu::GuiMenu(Window* window) : GuiComponent(window), mMenu(window, _T("MAIN MENU")), mVersion(window)
{
	auto theme = ThemeData::getMenuTheme();

	bool isFullUI = UIModeController::getInstance()->isUIModeFull();	
	
	if (isFullUI)
	{
		addEntry(_T("UI SETTINGS"), theme->Text.color, true, [this] { openUISettings(); }, theme->MenuIcons.ui);
		addEntry(_T("CONFIGURE INPUT"), theme->Text.color, true, [this] { openConfigInput(); }, theme->MenuIcons.controllers);
	}

	addEntry(_T("SOUND SETTINGS"), theme->Text.color, true, [this] { openSoundSettings(); }, theme->MenuIcons.sound);

	if (isFullUI)
		addEntry(_T("SCRAPER"), theme->Text.color, true, [this] { openScraperSettings(); }, theme->MenuIcons.scraper);

	if (isFullUI)
	{
		addEntry(_T("GAME COLLECTION SETTINGS"), theme->Text.color, true, [this] { openCollectionSystemSettings(); }, theme->MenuIcons.games);
		addEntry(_T("ADVANCED SETTINGS"), theme->Text.color, true, [this] { openOtherSettings(); }, theme->MenuIcons.advanced);
	}
	
#ifdef WIN32
	addEntry(_T("QUIT"), theme->Text.color, false, [this] {openQuitMenu(); }, theme->MenuIcons.quit);
#else
	addEntry(_T("QUIT"), theme->Text.color, true, [this] {openQuitMenu(); }, theme->MenuIcons.quit);
#endif

	addChild(&mMenu);
	addVersionInfo();

	setSize(mMenu.getSize());
	
	float y1 = Renderer::getScreenHeight();
	float y2 = (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2;

	if (Settings::getInstance()->getString("PowerSaverMode") == "instant" || Settings::getInstance()->getString("TransitionStyle") == "instant")
		setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, y2);
	else
	{
		setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, y1);

		auto fadeFunc = [this, y1, y2](float t) {

			t -= 1; // cubic ease out
			float pct = Math::lerp(0, 1, t*t*t + 1);

			float y = y1 * (1 - pct) + y2 * pct;
			setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, y);
		};

		setAnimation(new LambdaAnimation(fadeFunc, 350), 0, [this, fadeFunc, y2]
		{
			setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, y2);
		});

		setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, y2);
	}
}

void GuiMenu::openScraperSettings()
{
	auto s = new GuiSettings(mWindow, _T("SCRAPER"));

	// scrape from
	auto scraper_list = std::make_shared< OptionListComponent< std::string > >(mWindow, _T("SCRAPE FROM"), false);
	std::vector<std::string> scrapers = getScraperList();

	// Select either the first entry of the one read from the settings, just in case the scraper from settings has vanished.
	for(auto it = scrapers.cbegin(); it != scrapers.cend(); it++)
		scraper_list->add(*it, *it, *it == Settings::getInstance()->getString("Scraper"));

	s->addWithLabel(_T("SCRAPE FROM"), scraper_list);
	s->addSaveFunc([scraper_list] { Settings::getInstance()->setString("Scraper", scraper_list->getSelected()); });

	// scrape ratings
	auto scrape_ratings = std::make_shared<SwitchComponent>(mWindow);
	scrape_ratings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
	s->addWithLabel(_T("SCRAPE RATINGS"), scrape_ratings);
	s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState()); });

	// scrape now
	ComponentListRow row;
	auto openScrapeNow = [this] { mWindow->pushGui(new GuiScraperStart(mWindow)); };
	std::function<void()> openAndSave = openScrapeNow;
	openAndSave = [s, openAndSave] { s->save(); openAndSave(); };
	row.makeAcceptInputHandler(openAndSave);

	auto scrape_now = std::make_shared<TextComponent>(mWindow, _T("SCRAPE NOW"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
	auto bracket = makeArrow(mWindow);
	row.addElement(scrape_now, true);
	row.addElement(bracket, false);
	s->addRow(row);

	s->updatePosition();
	mWindow->pushGui(s);
}

void GuiMenu::openSoundSettings()
{
	auto s = new GuiSettings(mWindow, _T("SOUND SETTINGS"));
	
	// volume
	auto volume = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 1.f, "%");
	volume->setValue((float)VolumeControl::getInstance()->getVolume());
	s->addWithLabel(_T("SYSTEM VOLUME"), volume);
	s->addSaveFunc([volume] { VolumeControl::getInstance()->setVolume((int)Math::round(volume->getValue())); });

	if (UIModeController::getInstance()->isUIModeFull())
	{
#if defined(__linux__)
		// audio card
		auto audio_card = std::make_shared< OptionListComponent<std::string> >(mWindow, "AUDIO CARD", false);
		std::vector<std::string> audio_cards;
	#ifdef _RPI_
		// RPi Specific  Audio Cards
		audio_cards.push_back("local");
		audio_cards.push_back("hdmi");
		audio_cards.push_back("both");
	#endif
		audio_cards.push_back("default");
		audio_cards.push_back("sysdefault");
		audio_cards.push_back("dmix");
		audio_cards.push_back("hw");
		audio_cards.push_back("plughw");
		audio_cards.push_back("null");
		if (Settings::getInstance()->getString("AudioCard") != "") {
			if(std::find(audio_cards.begin(), audio_cards.end(), Settings::getInstance()->getString("AudioCard")) == audio_cards.end()) {
				audio_cards.push_back(Settings::getInstance()->getString("AudioCard"));
			}
		}
		for(auto ac = audio_cards.cbegin(); ac != audio_cards.cend(); ac++)
			audio_card->add(*ac, *ac, Settings::getInstance()->getString("AudioCard") == *ac);
		s->addWithLabel("AUDIO CARD", audio_card);
		s->addSaveFunc([audio_card] {
			Settings::getInstance()->setString("AudioCard", audio_card->getSelected());
			VolumeControl::getInstance()->deinit();
			VolumeControl::getInstance()->init();
		});

		// volume control device
		auto vol_dev = std::make_shared< OptionListComponent<std::string> >(mWindow, "AUDIO DEVICE", false);
		std::vector<std::string> transitions;
		transitions.push_back("PCM");
		transitions.push_back("Speaker");
		transitions.push_back("Master");
		transitions.push_back("Digital");
		transitions.push_back("Analogue");
		if (Settings::getInstance()->getString("AudioDevice") != "") {
			if(std::find(transitions.begin(), transitions.end(), Settings::getInstance()->getString("AudioDevice")) == transitions.end()) {
				transitions.push_back(Settings::getInstance()->getString("AudioDevice"));
			}
		}
		for(auto it = transitions.cbegin(); it != transitions.cend(); it++)
			vol_dev->add(*it, *it, Settings::getInstance()->getString("AudioDevice") == *it);
		s->addWithLabel("AUDIO DEVICE", vol_dev);
		s->addSaveFunc([vol_dev] {
			Settings::getInstance()->setString("AudioDevice", vol_dev->getSelected());
			VolumeControl::getInstance()->deinit();
			VolumeControl::getInstance()->init();
		});
#endif

		// disable sounds
		auto sounds_enabled = std::make_shared<SwitchComponent>(mWindow);
		sounds_enabled->setState(Settings::getInstance()->getBool("EnableSounds"));
		s->addWithLabel(_T("ENABLE NAVIGATION SOUNDS"), sounds_enabled);
		s->addSaveFunc([sounds_enabled] {
			if (sounds_enabled->getState()
				&& !Settings::getInstance()->getBool("EnableSounds")
				&& PowerSaver::getMode() == PowerSaver::INSTANT)
			{
				Settings::getInstance()->setString("PowerSaverMode", "default");
				PowerSaver::init();
			}
			Settings::getInstance()->setBool("EnableSounds", sounds_enabled->getState());
		});

		auto video_audio = std::make_shared<SwitchComponent>(mWindow);
		video_audio->setState(Settings::getInstance()->getBool("VideoAudio"));
		s->addWithLabel(_T("ENABLE VIDEO AUDIO"), video_audio);
		s->addSaveFunc([video_audio] { Settings::getInstance()->setBool("VideoAudio", video_audio->getState()); });

#ifdef _RPI_
		// OMX player Audio Device
		auto omx_audio_dev = std::make_shared< OptionListComponent<std::string> >(mWindow, "OMX PLAYER AUDIO DEVICE", false);
		std::vector<std::string> omx_cards;
		// RPi Specific  Audio Cards
		omx_cards.push_back("local");
		omx_cards.push_back("hdmi");
		omx_cards.push_back("both");
		omx_cards.push_back("alsa:hw:0,0");
		omx_cards.push_back("alsa:hw:1,0");
		if (Settings::getInstance()->getString("OMXAudioDev") != "") {
			if (std::find(omx_cards.begin(), omx_cards.end(), Settings::getInstance()->getString("OMXAudioDev")) == omx_cards.end()) {
				omx_cards.push_back(Settings::getInstance()->getString("OMXAudioDev"));
			}
		}
		for (auto it = omx_cards.cbegin(); it != omx_cards.cend(); it++)
			omx_audio_dev->add(*it, *it, Settings::getInstance()->getString("OMXAudioDev") == *it);
		s->addWithLabel("OMX PLAYER AUDIO DEVICE", omx_audio_dev);
		s->addSaveFunc([omx_audio_dev] {
			if (Settings::getInstance()->getString("OMXAudioDev") != omx_audio_dev->getSelected())
				Settings::getInstance()->setString("OMXAudioDev", omx_audio_dev->getSelected());
		});
#endif
	}

	s->updatePosition();
	mWindow->pushGui(s);

}

void GuiMenu::openUISettings()
{
	auto s = new GuiSettings(mWindow, _T("UI SETTINGS"));

	auto pthis = this;

	// theme set
	auto theme = ThemeData::getMenuTheme();
	auto themeSets = ThemeData::getThemeSets();
	auto system = ViewController::get()->getState().getSystem();

	if (!themeSets.empty())
	{
		std::map<std::string, ThemeSet>::const_iterator selectedSet = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
		if (selectedSet == themeSets.cend())
			selectedSet = themeSets.cbegin();

		auto theme_set = std::make_shared< OptionListComponent<std::string> >(mWindow, "THEME", false);
		for (auto it = themeSets.cbegin(); it != themeSets.cend(); it++)
			theme_set->add(it->first, it->first, it == selectedSet);
		s->addWithLabel(_T("THEME"), theme_set);

		Window* window = mWindow;
		s->addSaveFunc([window, theme_set, pthis]
		{
			std::string oldTheme = Settings::getInstance()->getString("ThemeSet");
			if (oldTheme != theme_set->getSelected())
			{
				Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());

				auto themeSubSets = ThemeData::getThemeSubSets(theme_set->getSelected());
				auto themeColorSets = ThemeData::sortThemeSubSets(themeSubSets, "colorset");
				auto themeIconSets = ThemeData::sortThemeSubSets(themeSubSets, "iconset");
				auto themeMenus = ThemeData::sortThemeSubSets(themeSubSets, "menu");
				auto themeSystemviewSets = ThemeData::sortThemeSubSets(themeSubSets, "systemview");
				auto themeGamelistViewSets = ThemeData::sortThemeSubSets(themeSubSets, "gamelistview");
				auto themeRegions = ThemeData::sortThemeSubSets(themeSubSets, "region");

				// theme changed without setting options, forcing options to avoid crash/blank theme
				if (themeRegions.empty())
					Settings::getInstance()->setString("ThemeRegionName", "");
				else
					Settings::getInstance()->setString("ThemeRegionName", themeRegions.begin()->first);

				if (themeColorSets.empty())
					Settings::getInstance()->setString("ThemeColorSet", "");
				else
					Settings::getInstance()->setString("ThemeColorSet", themeColorSets.begin()->first);

				if (themeIconSets.empty())
					Settings::getInstance()->setString("ThemeIconSet", "");
				else
					Settings::getInstance()->setString("ThemeIconSet", themeIconSets.begin()->first);

				if (themeMenus.empty())
					Settings::getInstance()->setString("ThemeMenu", "");
				else
					Settings::getInstance()->setString("ThemeMenu", themeMenus.begin()->first);

				if (themeSystemviewSets.empty())
					Settings::getInstance()->setString("ThemeSystemView", "");
				else
					Settings::getInstance()->setString("ThemeSystemView", themeSystemviewSets.begin()->first);

				if (themeGamelistViewSets.empty())
					Settings::getInstance()->setString("ThemeGamelistView", "");
				else
					Settings::getInstance()->setString("ThemeGamelistView", themeGamelistViewSets.begin()->first);



				Scripting::fireEvent("theme-changed", theme_set->getSelected(), oldTheme);
				CollectionSystemManager::get()->updateSystemsList();
				ViewController::get()->goToStart();
				ViewController::get()->reloadAll(); // TODO - replace this with some sort of signal-based implementation

				delete pthis;
				window->pushGui(new GuiMenu(window));
			}
		});
	
		if (system != NULL && system->getTheme()->hasSubsets())
		{

			// theme config
			std::function<void()> openGui = [this, theme_set, window, pthis] {
				auto themeconfig = new GuiSettings(mWindow, _T("THEME CONFIGURATION").c_str());

				auto SelectedTheme = theme_set->getSelected();

				auto themeSubSets = ThemeData::getThemeSubSets(SelectedTheme);
				auto themeColorSets = ThemeData::sortThemeSubSets(themeSubSets, "colorset");
				auto themeIconSets = ThemeData::sortThemeSubSets(themeSubSets, "iconset");
				auto themeMenus = ThemeData::sortThemeSubSets(themeSubSets, "menu");
				auto themeSystemviewSets = ThemeData::sortThemeSubSets(themeSubSets, "systemview");
				auto themeGamelistViewSets = ThemeData::sortThemeSubSets(themeSubSets, "gamelistview");
				auto themeRegions = ThemeData::sortThemeSubSets(themeSubSets, "region");

				// colorset

				auto selectedColorSet = themeColorSets.find(Settings::getInstance()->getString("ThemeColorSet"));
				if (selectedColorSet == themeColorSets.end())
					selectedColorSet = themeColorSets.begin();
				auto theme_colorset = std::make_shared<OptionListComponent<std::string> >(mWindow, _T("THEME COLORSET"), false);

				for (auto it = themeColorSets.begin(); it != themeColorSets.end(); it++)
					theme_colorset->add(it->first, it->first, it == selectedColorSet);

				if (!themeColorSets.empty())
					themeconfig->addWithLabel(_T("THEME COLORSET"), theme_colorset);

				// iconset

				auto selectedIconSet = themeIconSets.find(Settings::getInstance()->getString("ThemeIconSet"));
				if (selectedIconSet == themeIconSets.end())
					selectedIconSet = themeIconSets.begin();
				auto theme_iconset = std::make_shared<OptionListComponent<std::string> >(mWindow, _T("THEME ICONSET"), false);

				for (auto it = themeIconSets.begin(); it != themeIconSets.end(); it++)
					theme_iconset->add(it->first, it->first, it == selectedIconSet);

				if (!themeIconSets.empty())
					themeconfig->addWithLabel(_T("THEME ICONSET"), theme_iconset);

				// menu

				auto selectedMenu = themeMenus.find(Settings::getInstance()->getString("ThemeMenu"));
				if (selectedMenu == themeMenus.end())
					selectedMenu = themeMenus.begin();
				auto theme_menu = std::make_shared<OptionListComponent<std::string> >(mWindow, _T("THEME MENU"), false);

				for (auto it = themeMenus.begin(); it != themeMenus.end(); it++)
					theme_menu->add(it->first, it->first, it == selectedMenu);

				if (!themeMenus.empty())
					themeconfig->addWithLabel(_T("THEME MENU"), theme_menu);

				// systemview

				auto selectedSystemviewSet = themeSystemviewSets.find(Settings::getInstance()->getString("ThemeSystemView"));
				if (selectedSystemviewSet == themeSystemviewSets.end())
					selectedSystemviewSet = themeSystemviewSets.begin();

				auto theme_systemview = std::make_shared<OptionListComponent<std::string> >(mWindow, _T("THEME SYSTEMVIEW"), false);

				for (auto it = themeSystemviewSets.begin(); it != themeSystemviewSets.end(); it++)
					theme_systemview->add(it->first, it->first, it == selectedSystemviewSet);

				if (!themeSystemviewSets.empty())
					themeconfig->addWithLabel(_T("THEME SYSTEMVIEW"), theme_systemview);

				// gamelistview

				auto selectedGamelistViewSet = themeGamelistViewSets.find(Settings::getInstance()->getString("ThemeGamelistView"));
				if (selectedGamelistViewSet == themeGamelistViewSets.end())
					selectedGamelistViewSet = themeGamelistViewSets.begin();

				auto theme_gamelistview = std::make_shared<OptionListComponent<std::string> >(mWindow, _T("THEME GAMELISTVIEW"), false);

				for (auto it = themeGamelistViewSets.begin(); it != themeGamelistViewSets.end(); it++)
					theme_gamelistview->add(it->first, it->first, it == selectedGamelistViewSet);

				if (!themeGamelistViewSets.empty())
					themeconfig->addWithLabel(_T("THEME GAMELISTVIEW"), theme_gamelistview);

				// themeregion
				auto selectedRegion = themeRegions.find(Settings::getInstance()->getString("ThemeRegionName"));
				if (selectedRegion == themeRegions.end())
					selectedRegion = themeRegions.begin();

				auto theme_region = std::make_shared<OptionListComponent<std::string> >(mWindow, _T("THEME GAMELISTVIEW"), false);

				for (auto it = themeRegions.begin(); it != themeRegions.end(); it++)
					theme_region->add(it->first, it->first, it == selectedRegion);

				if (!themeRegions.empty())
					themeconfig->addWithLabel(_T("THEME REGION"), theme_region);
				
				themeconfig->addSaveFunc([this, window, theme_set, theme_colorset, theme_iconset, theme_menu, theme_systemview, theme_gamelistview, theme_region, pthis] {
					bool needReload = false;
					if (Settings::getInstance()->getString("ThemeColorSet") != theme_colorset->getSelected() && !theme_colorset->getSelected().empty())
						needReload = true;
					if (Settings::getInstance()->getString("ThemeIconSet") != theme_iconset->getSelected() && !theme_iconset->getSelected().empty())
						needReload = true;
					if (Settings::getInstance()->getString("ThemeMenu") != theme_menu->getSelected() && !theme_menu->getSelected().empty())
						needReload = true;
					if (Settings::getInstance()->getString("ThemeSystemView") != theme_systemview->getSelected() && !theme_systemview->getSelected().empty())
						needReload = true;
					if (Settings::getInstance()->getString("ThemeGamelistView") != theme_gamelistview->getSelected() && !theme_gamelistview->getSelected().empty())
						needReload = true;
					if (Settings::getInstance()->getString("ThemeRegionName") != theme_region->getSelected() && !theme_region->getSelected().empty())
						needReload = true;

					if (needReload) {
						Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());
						Settings::getInstance()->setString("ThemeColorSet", theme_colorset->getSelected());
						Settings::getInstance()->setString("ThemeIconSet", theme_iconset->getSelected());
						Settings::getInstance()->setString("ThemeMenu", theme_menu->getSelected());
						Settings::getInstance()->setString("ThemeSystemView", theme_systemview->getSelected());
						Settings::getInstance()->setString("ThemeGamelistView", theme_gamelistview->getSelected());
						Settings::getInstance()->setString("ThemeRegionName", theme_region->getSelected());
						//Settings::getInstance()->setBool("ThemeChanged", true);

						//reload theme
						std::string oldTheme = Settings::getInstance()->getString("ThemeSet");
						Scripting::fireEvent("theme-changed", theme_set->getSelected(), oldTheme);
						CollectionSystemManager::get()->updateSystemsList();
						ViewController::get()->goToStart();
						ViewController::get()->reloadAll(); // TODO - replace this with some sort of signal-based implementation

						delete pthis;
						window->pushGui(new GuiMenu(window));
						//ReloadAll();
					}
				});
				if (!themeRegions.empty() || !themeGamelistViewSets.empty() || !themeSystemviewSets.empty() || !themeIconSets.empty() || !themeMenus.empty() || !themeColorSets.empty())
				{
					themeconfig->updatePosition();
					mWindow->pushGui(themeconfig);
				}
				else
					mWindow->pushGui(new GuiMsgBox(window, _T("THIS THEME HAS NO OPTION"), _T("OK")));
			};

			s->addSubMenu(_T("THEME CONFIGURATION"), openGui);
		}
	}

	// GameList view style
	if (system != NULL && !system->getTheme()->hasSubsets())
	{
		auto gamelist_style = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("GAMELIST VIEW STYLE"), false);
		std::vector<std::string> styles;
		styles.push_back("automatic");

		auto system = ViewController::get()->getState().getSystem();
		if (system != NULL)
		{
			auto mViews = system->getTheme()->getViewsOfTheme();
			for (auto it = mViews.cbegin(); it != mViews.cend(); ++it)
				styles.push_back(*it);
		}
		else
		{
			styles.push_back("basic");
			styles.push_back("detailed");
			styles.push_back("video");
			styles.push_back("grid");
		}

		auto viewPreference = Settings::getInstance()->getString("GamelistViewStyle");
		if (!system->getTheme()->hasView(viewPreference))
			viewPreference = "automatic";

		for (auto it = styles.cbegin(); it != styles.cend(); it++)
			gamelist_style->add(_T(*it), *it, viewPreference == *it);

		s->addWithLabel(_T("GAMELIST VIEW STYLE"), gamelist_style);
		s->addSaveFunc([gamelist_style, viewPreference] {
			bool needReload = false;
			if (viewPreference != gamelist_style->getSelected())
				needReload = true;
			Settings::getInstance()->setString("GamelistViewStyle", gamelist_style->getSelected());
			if (needReload)
				ViewController::get()->reloadAll();
		});
	}
	
	// Default grid size
	if (system != NULL && system->getTheme()->hasView("grid"))
	{
		Vector2f gridOverride = Vector2f::parseString(Settings::getInstance()->getString("DefaultGridSize"));
		auto ovv = std::to_string((int)gridOverride.x()) + "x" + std::to_string((int)gridOverride.y());

		auto mGridSize = std::make_shared<OptionListComponent<std::string>>(mWindow, _T("DEFAULT GRID SIZE"), false);

		bool found = false;
		for (auto it = GuiGamelistOptions::gridSizes.cbegin(); it != GuiGamelistOptions::gridSizes.cend(); it++)
		{
			bool sel = (gridOverride == Vector2f(0, 0) && *it == "automatic") || ovv == *it;
			if (sel)
				found = true;

			mGridSize->add(_T(*it), *it, sel);
		}

		if (!found)
			mGridSize->selectFirstItem();

		s->addWithLabel(_T("DEFAULT GRID SIZE"), mGridSize);
		s->addSaveFunc([mGridSize]
		{
			std::string str = mGridSize->getSelected();
			std::string value = "";

			size_t divider = str.find('x');
			if (divider != std::string::npos)
			{
				std::string first = str.substr(0, divider);
				std::string second = str.substr(divider + 1, std::string::npos);

				Vector2f gridSizeOverride = Vector2f((float)atof(first.c_str()), (float)atof(second.c_str()));
				value = Utils::String::replace(Utils::String::replace(gridSizeOverride.toString(), ".000000", ""), "0 0", "");
			}
			
			if (value != Settings::getInstance()->getString("DefaultGridSize"))
			{
				Settings::getInstance()->setString("DefaultGridSize", value);
				ViewController::get()->reloadAll();
			}
		});
	}

	//#ifndef WIN32
		//UI mode
	auto UImodeSelection = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("UI MODE"), false);
	std::vector<std::string> UImodes = UIModeController::getInstance()->getUIModes();
	for (auto it = UImodes.cbegin(); it != UImodes.cend(); it++)
		UImodeSelection->add(_T(*it), *it, Settings::getInstance()->getString("UIMode") == *it);
	s->addWithLabel(_T("UI MODE"), UImodeSelection);
	Window* window = mWindow;
	s->addSaveFunc([UImodeSelection, window]
	{
		std::string selectedMode = UImodeSelection->getSelected();
		if (selectedMode != "Full")
		{
			std::string msg = "You are changing the UI to a restricted mode:\n" + selectedMode + "\n";
			msg += "This will hide most menu-options to prevent changes to the system.\n";
			msg += "To unlock and return to the full UI, enter this code: \n";
			msg += "\"" + UIModeController::getInstance()->getFormattedPassKeyStr() + "\"\n\n";
			msg += "Do you want to proceed?";
			window->pushGui(new GuiMsgBox(window, msg,
				"YES", [selectedMode] {
				LOG(LogDebug) << "Setting UI mode to " << selectedMode;
				Settings::getInstance()->setString("UIMode", selectedMode);
				Settings::getInstance()->saveFile();
			}, "NO", nullptr));
		}
	});
	//#endif

	// LANGUAGE

	std::vector<std::string> langues;
	langues.push_back("en");

	std::string xmlpath = ResourceManager::getInstance()->getResourcePath(":/splash.svg");
	if (xmlpath.length() > 0)
	{
		xmlpath = Utils::FileSystem::getParent(xmlpath) + "/locale/";

		Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(xmlpath, true);
		for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isDirectory(*it))
				continue;

			std::string name = *it;

			if (name.rfind("emulationstation2.po") == std::string::npos)
				continue;

			name = Utils::FileSystem::getParent(name);
			name = Utils::FileSystem::getFileName(name);

			if (name != "en")
				langues.push_back(name);
		}

		if (langues.size() > 1)
		{
			auto language = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("LANGUAGE"), false);

			for (auto it = langues.cbegin(); it != langues.cend(); it++)
				language->add(*it, *it, Settings::getInstance()->getString("Language") == *it);

			s->addWithLabel(_T("LANGUAGE"), language);
			s->addSaveFunc([language] {
				Settings::getInstance()->setString("Language", language->getSelected());
			});
		}
	}

	// transition style
	auto transition_style = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("TRANSITION STYLE"), false);
	std::vector<std::string> transitions;
	transitions.push_back("fade");
	transitions.push_back("slide");
	transitions.push_back("instant");
	for (auto it = transitions.cbegin(); it != transitions.cend(); it++)
		transition_style->add(_T(*it), *it, Settings::getInstance()->getString("TransitionStyle") == *it);

	s->addWithLabel(_T("TRANSITION STYLE"), transition_style);
	s->addSaveFunc([transition_style] {
		if (Settings::getInstance()->getString("TransitionStyle") == "instant"
			&& transition_style->getSelected() != "instant"
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setString("TransitionStyle", transition_style->getSelected());
		GuiComponent::ALLOWANIMATIONS = Settings::getInstance()->getString("TransitionStyle") != "instant";
	});


	auto transitionOfGames_style = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("GAME LAUNCH TRANSITION"), false);
	std::vector<std::string> gameTransitions;
	gameTransitions.push_back("fade");
	gameTransitions.push_back("slide");
	gameTransitions.push_back("instant");
	for (auto it = gameTransitions.cbegin(); it != gameTransitions.cend(); it++)
		transitionOfGames_style->add(_T(*it), *it, Settings::getInstance()->getString("GameTransitionStyle") == *it);

	s->addWithLabel(_T("GAME LAUNCH TRANSITION"), transitionOfGames_style);
	s->addSaveFunc([transitionOfGames_style] {
		if (Settings::getInstance()->getString("GameTransitionStyle") == "instant"
			&& transitionOfGames_style->getSelected() != "instant"
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setString("GameTransitionStyle", transitionOfGames_style->getSelected());
	});


	// Optionally start in selected system
	auto systemfocus_list = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("START ON SYSTEM"), false);
	systemfocus_list->add(_T("NONE"), "", Settings::getInstance()->getString("StartupSystem") == "");
	for (auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
	{
		if ("retropie" != (*it)->getName())
		{
			systemfocus_list->add((*it)->getName(), (*it)->getName(), Settings::getInstance()->getString("StartupSystem") == (*it)->getName());
		}
	}
	s->addWithLabel(_T("START ON SYSTEM"), systemfocus_list);
	s->addSaveFunc([systemfocus_list] {
		Settings::getInstance()->setString("StartupSystem", systemfocus_list->getSelected());
	});
	
	// Open gamelist at start
	auto bootOnGamelist = std::make_shared<SwitchComponent>(mWindow);
	bootOnGamelist->setState(Settings::getInstance()->getBool("StartupOnGameList"));
	s->addWithLabel(_T("BOOT ON GAMELIST"), bootOnGamelist);
	s->addSaveFunc([bootOnGamelist] { Settings::getInstance()->setBool("StartupOnGameList", bootOnGamelist->getState()); });

	// Hide system view
	auto hideSystemView = std::make_shared<SwitchComponent>(mWindow);
	hideSystemView->setState(Settings::getInstance()->getBool("HideSystemView"));
	s->addWithLabel(_T("HIDE SYSTEM VIEW"), hideSystemView);
	s->addSaveFunc([hideSystemView] { Settings::getInstance()->setBool("HideSystemView", hideSystemView->getState()); });
	

#if defined(_WIN32)
	// quick system select (left/right in game list view)
	auto hideWindowScreen = std::make_shared<SwitchComponent>(mWindow);
	hideWindowScreen->setState(Settings::getInstance()->getBool("HideWindow"));
	s->addWithLabel(_T("HIDE WHEN RUNNING GAME"), hideWindowScreen);
	s->addSaveFunc([hideWindowScreen] { Settings::getInstance()->setBool("HideWindow", hideWindowScreen->getState()); });
#endif

	// quick system select (left/right in game list view)
	auto quick_sys_select = std::make_shared<SwitchComponent>(mWindow);
	quick_sys_select->setState(Settings::getInstance()->getBool("QuickSystemSelect"));
	s->addWithLabel(_T("QUICK SYSTEM SELECT"), quick_sys_select);
	s->addSaveFunc([quick_sys_select] { Settings::getInstance()->setBool("QuickSystemSelect", quick_sys_select->getState()); });

	// carousel transition option
	auto move_carousel = std::make_shared<SwitchComponent>(mWindow);
	move_carousel->setState(Settings::getInstance()->getBool("MoveCarousel"));
	s->addWithLabel(_T("CAROUSEL TRANSITIONS"), move_carousel);
	s->addSaveFunc([move_carousel] {
		if (move_carousel->getState()
			&& !Settings::getInstance()->getBool("MoveCarousel")
			&& PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			Settings::getInstance()->setString("PowerSaverMode", "default");
			PowerSaver::init();
		}
		Settings::getInstance()->setBool("MoveCarousel", move_carousel->getState());
	});

	// show help
	auto show_help = std::make_shared<SwitchComponent>(mWindow);
	show_help->setState(Settings::getInstance()->getBool("ShowHelpPrompts"));
	s->addWithLabel(_T("ON-SCREEN HELP"), show_help);
	s->addSaveFunc([show_help] { Settings::getInstance()->setBool("ShowHelpPrompts", show_help->getState()); });

	// enable filters (ForceDisableFilters)
	auto enable_filter = std::make_shared<SwitchComponent>(mWindow);
	enable_filter->setState(!Settings::getInstance()->getBool("ForceDisableFilters"));
	s->addWithLabel(_T("ENABLE FILTERS"), enable_filter);
	s->addSaveFunc([enable_filter] { 
		bool filter_is_enabled = !Settings::getInstance()->getBool("ForceDisableFilters");
		Settings::getInstance()->setBool("ForceDisableFilters", !enable_filter->getState()); 
		if (enable_filter->getState() != filter_is_enabled) ViewController::get()->ReloadAndGoToStart();
	});


	// screensaver
	ComponentListRow screensaver_row;
	screensaver_row.elements.clear();
	screensaver_row.addElement(std::make_shared<TextComponent>(mWindow, _T("SCREENSAVER SETTINGS"), theme->Text.font, theme->Text.color), true);
	screensaver_row.addElement(makeArrow(mWindow), false);
	screensaver_row.makeAcceptInputHandler(std::bind(&GuiMenu::openScreensaverOptions, this));
	s->addRow(screensaver_row);


	s->updatePosition();
	mWindow->pushGui(s);
}

void GuiMenu::openOtherSettings()
{
	auto s = new GuiSettings(mWindow, _T("ADVANCED SETTINGS"));

	// maximum vram
	auto max_vram = std::make_shared<SliderComponent>(mWindow, 0.f, 2000.f, 10.f, "Mb");
	max_vram->setValue((float)(Settings::getInstance()->getInt("MaxVRAM")));
	s->addWithLabel(_T("VRAM LIMIT"), max_vram);
	s->addSaveFunc([max_vram] { Settings::getInstance()->setInt("MaxVRAM", (int)Math::round(max_vram->getValue())); });

	// power saver
	auto power_saver = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("POWER SAVER MODES"), false);
	std::vector<std::string> modes;
	modes.push_back("disabled");
	modes.push_back("default");
	modes.push_back("enhanced");
	modes.push_back("instant");
	for (auto it = modes.cbegin(); it != modes.cend(); it++)
		power_saver->add(*it, *it, Settings::getInstance()->getString("PowerSaverMode") == *it);

	s->addWithLabel(_T("POWER SAVER MODES"), power_saver);
	s->addSaveFunc([this, power_saver] {
		if (Settings::getInstance()->getString("PowerSaverMode") != "instant" && power_saver->getSelected() == "instant") {
			Settings::getInstance()->setString("TransitionStyle", "instant");
			Settings::getInstance()->setString("GameTransitionStyle", "instant");
			Settings::getInstance()->setBool("MoveCarousel", false);
			Settings::getInstance()->setBool("EnableSounds", false);
		}

		GuiComponent::ALLOWANIMATIONS = Settings::getInstance()->getString("TransitionStyle") != "instant";

		Settings::getInstance()->setString("PowerSaverMode", power_saver->getSelected());
		PowerSaver::init();
	});

	// gamelists
	auto save_gamelists = std::make_shared<SwitchComponent>(mWindow);
	save_gamelists->setState(Settings::getInstance()->getBool("SaveGamelistsOnExit"));
	s->addWithLabel(_T("SAVE METADATA ON EXIT"), save_gamelists);
	s->addSaveFunc([save_gamelists] { Settings::getInstance()->setBool("SaveGamelistsOnExit", save_gamelists->getState()); });

	auto parse_gamelists = std::make_shared<SwitchComponent>(mWindow);
	parse_gamelists->setState(Settings::getInstance()->getBool("ParseGamelistOnly"));
	s->addWithLabel(_T("PARSE GAMESLISTS ONLY"), parse_gamelists);
	s->addSaveFunc([parse_gamelists] { Settings::getInstance()->setBool("ParseGamelistOnly", parse_gamelists->getState()); });
	
#ifndef WIN32
	auto local_art = std::make_shared<SwitchComponent>(mWindow);
	local_art->setState(Settings::getInstance()->getBool("LocalArt"));
	s->addWithLabel(_T("SEARCH FOR LOCAL ART"), local_art);
	s->addSaveFunc([local_art] { Settings::getInstance()->setBool("LocalArt", local_art->getState()); });
#endif

	// hidden files
	auto hidden_files = std::make_shared<SwitchComponent>(mWindow);
	hidden_files->setState(Settings::getInstance()->getBool("ShowHiddenFiles"));
	s->addWithLabel(_T("SHOW HIDDEN FILES"), hidden_files);
	s->addSaveFunc([hidden_files] { Settings::getInstance()->setBool("ShowHiddenFiles", hidden_files->getState()); });

#ifdef _RPI_
	// Video Player - VideoOmxPlayer
	auto omx_player = std::make_shared<SwitchComponent>(mWindow);
	omx_player->setState(Settings::getInstance()->getBool("VideoOmxPlayer"));
	s->addWithLabel("USE OMX PLAYER (HW ACCELERATED)", omx_player);
	s->addSaveFunc([omx_player]
	{
		// need to reload all views to re-create the right video components
		bool needReload = false;
		if(Settings::getInstance()->getBool("VideoOmxPlayer") != omx_player->getState())
			needReload = true;

		Settings::getInstance()->setBool("VideoOmxPlayer", omx_player->getState());

		if(needReload)
			ViewController::get()->reloadAll();
	});

#endif

	// preload UI
	auto preloadUI = std::make_shared<SwitchComponent>(mWindow);
	preloadUI->setState(Settings::getInstance()->getBool("PreloadUI"));
	s->addWithLabel(_T("PRELOAD UI"), preloadUI);
	s->addSaveFunc([preloadUI] { Settings::getInstance()->setBool("PreloadUI", preloadUI->getState()); });
	
	// optimizeVram
	auto optimizeVram = std::make_shared<SwitchComponent>(mWindow);
	optimizeVram->setState(Settings::getInstance()->getBool("OptimizeVRAM"));
	s->addWithLabel(_T("OPTIMIZE IMAGES VRAM USE"), optimizeVram);
	s->addSaveFunc([optimizeVram]
	{
		TextureData::OPTIMIZEVRAM = optimizeVram->getState();
		Settings::getInstance()->setBool("OptimizeVRAM", optimizeVram->getState());
	});

#ifdef WIN32
	// vsync
	auto vsync = std::make_shared<SwitchComponent>(mWindow);
	vsync->setState(Settings::getInstance()->getBool("VSync"));
	s->addWithLabel(_T("VSYNC"), vsync);
	s->addSaveFunc([vsync] 
	{ 
		Settings::getInstance()->setBool("VSync", vsync->getState()); 

		// vsync
		if (Settings::getInstance()->getBool("VSync"))
		{
			// SDL_GL_SetSwapInterval(0) for immediate updates (no vsync, default), 
			// 1 for updates synchronized with the vertical retrace, 
			// or -1 for late swap tearing.
			// SDL_GL_SetSwapInterval returns 0 on success, -1 on error.
			// if vsync is requested, try normal vsync; if that doesn't work, try late swap tearing
			// if that doesn't work, report an error
			if (SDL_GL_SetSwapInterval(1) != 0 && SDL_GL_SetSwapInterval(-1) != 0)
				LOG(LogWarning) << "Tried to enable vsync, but failed! (" << SDL_GetError() << ")";
		}
		else
			SDL_GL_SetSwapInterval(0);
	});
#endif

	// framerate	
	auto framerate = std::make_shared<SwitchComponent>(mWindow);
	framerate->setState(Settings::getInstance()->getBool("DrawFramerate"));
	s->addWithLabel(_T("SHOW FRAMERATE"), framerate);
	s->addSaveFunc([framerate] { Settings::getInstance()->setBool("DrawFramerate", framerate->getState()); });

	// threaded loading
	auto threadedLoading = std::make_shared<SwitchComponent>(mWindow);
	threadedLoading->setState(Settings::getInstance()->getBool("ThreadedLoading"));
	s->addWithLabel(_T("THREADED LOADING"), threadedLoading);
	s->addSaveFunc([threadedLoading] { Settings::getInstance()->setBool("ThreadedLoading", threadedLoading->getState()); });

	s->updatePosition();
	mWindow->pushGui(s);

}

void GuiMenu::openConfigInput()
{
	Window* window = mWindow;
	window->pushGui(new GuiDetectDevice(window, false, nullptr));
		/*
	window->pushGui(new GuiMsgBox(window, _T("ARE YOU SURE YOU WANT TO CONFIGURE INPUT?"), _T("YES"),
		[window] {
		window->pushGui(new GuiDetectDevice(window, false, nullptr));
	}, _T("NO"), nullptr)
	);*/

}

void GuiMenu::openQuitMenu()
{
#ifdef WIN32
	Scripting::fireEvent("quit");
	quitES("");
	return;
#endif

	auto s = new GuiSettings(mWindow, _T("QUIT"));

	Window* window = mWindow;

	ComponentListRow row;
	if (UIModeController::getInstance()->isUIModeFull())
	{
		row.makeAcceptInputHandler([window] {
			window->pushGui(new GuiMsgBox(window, "REALLY RESTART?", "YES",
				[] {
				Scripting::fireEvent("quit");
				if(quitES("/tmp/es-restart") != 0)
					LOG(LogWarning) << "Restart terminated with non-zero result!";
			}, "NO", nullptr));
		});
		row.addElement(std::make_shared<TextComponent>(window, "RESTART EMULATIONSTATION", ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
		s->addRow(row);

		if(Settings::getInstance()->getBool("ShowExit"))
		{
			row.elements.clear();
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "REALLY QUIT?", "YES",
					[] {
					Scripting::fireEvent("quit");
					quitES("");
				}, "NO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "QUIT EMULATIONSTATION", ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
			s->addRow(row);
		}
	}
	row.elements.clear();
	row.makeAcceptInputHandler([window] {
		window->pushGui(new GuiMsgBox(window, "REALLY RESTART?", "YES",
			[] {
			Scripting::fireEvent("quit", "reboot");
			Scripting::fireEvent("reboot");
			if (quitES("/tmp/es-sysrestart") != 0)
				LOG(LogWarning) << "Restart terminated with non-zero result!";
		}, "NO", nullptr));
	});
	row.addElement(std::make_shared<TextComponent>(window, "RESTART SYSTEM", ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
	s->addRow(row);

	row.elements.clear();
	row.makeAcceptInputHandler([window] {
		window->pushGui(new GuiMsgBox(window, "REALLY SHUTDOWN?", "YES",
			[] {
			Scripting::fireEvent("quit", "shutdown");
			Scripting::fireEvent("shutdown");
			if (quitES("/tmp/es-shutdown") != 0)
				LOG(LogWarning) << "Shutdown terminated with non-zero result!";
		}, "NO", nullptr));
	});
	row.addElement(std::make_shared<TextComponent>(window, "SHUTDOWN SYSTEM", ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
	s->addRow(row);

	s->updatePosition();
	mWindow->pushGui(s);
}

void GuiMenu::addVersionInfo()
{
	std::string  buildDate = (Settings::getInstance()->getBool("Debug") ? std::string( "   (" + Utils::String::toUpper(PROGRAM_BUILT_STRING) + ")") : (""));

	auto theme = ThemeData::getMenuTheme();
//	mVersion.setFont(Font::get(FONT_SIZE_SMALL));
//	mVersion.setColor(0x5E5E5EFF);

	mVersion.setFont(theme->Footer.font);
	mVersion.setColor(theme->Footer.color);

	mVersion.setText("EMULATIONSTATION V" + Utils::String::toUpper(PROGRAM_VERSION_STRING) + buildDate);
	mVersion.setHorizontalAlignment(ALIGN_CENTER);
	addChild(&mVersion);
}

void GuiMenu::openScreensaverOptions() {
	mWindow->pushGui(new GuiGeneralScreensaverOptions(mWindow, _T("SCREENSAVER SETTINGS")));
}

void GuiMenu::openCollectionSystemSettings() {
	mWindow->pushGui(new GuiCollectionSystemsOptions(mWindow));
}

void GuiMenu::onSizeChanged()
{
	mVersion.setSize(mSize.x(), 0);
	mVersion.setPosition(0, mSize.y() - mVersion.getSize().y());
}

void GuiMenu::addEntry(std::string name, unsigned int color, bool add_arrow, const std::function<void()>& func, const std::string iconName)
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	color = theme->Text.color;

	// populate the list
	ComponentListRow row;

	if (!iconName.empty())
	{
		// icon
		auto icon = std::make_shared<ImageComponent>(mWindow);
		icon->setImage(iconName);
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.25f);
		row.addElement(icon, false);

		// spacer between icon and text
		auto spacer = std::make_shared<GuiComponent>(mWindow);
		spacer->setSize(10, 0);
		row.addElement(spacer, false);
	}

	row.addElement(std::make_shared<TextComponent>(mWindow, name, font, color), true);

	if (add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);
		row.addElement(bracket, false);
	}

	row.makeAcceptInputHandler(func);
	mMenu.addRow(row);
}

bool GuiMenu::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;

	if((config->isMappedTo("b", input) || config->isMappedTo("start", input)) && input.value != 0)
	{
		delete this;
		return true;
	}

	return false;
}

HelpStyle GuiMenu::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
	return style;
}

std::vector<HelpPrompt> GuiMenu::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("up/down", _T("CHOOSE")));
	prompts.push_back(HelpPrompt("a", _T("SELECT")));
	prompts.push_back(HelpPrompt("start", _T("CLOSE")));
	return prompts;
}
