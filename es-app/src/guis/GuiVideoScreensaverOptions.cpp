#include "guis/GuiVideoScreensaverOptions.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Settings.h"

GuiVideoScreensaverOptions::GuiVideoScreensaverOptions(Window* window, const char* title) : GuiScreensaverOptions(window, title)
{
	// timeout to swap videos
	auto swap = std::make_shared<SliderComponent>(mWindow, 10.f, 1000.f, 1.f, "s");
	swap->setValue((float)(Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout") / (1000)));
	addWithLabel(_("SWAP VIDEO AFTER (SECS)"), swap);
	addSaveFunc([swap] {
		int playNextTimeout = (int)Math::round(swap->getValue()) * (1000);
		Settings::getInstance()->setInt("ScreenSaverSwapVideoTimeout", playNextTimeout);
		PowerSaver::updateTimeouts();
	});


#ifdef _RPI_
	auto ss_omx = std::make_shared<SwitchComponent>(mWindow);
	ss_omx->setState(Settings::getInstance()->getBool("ScreenSaverOmxPlayer"));
	addWithLabel(_("USE OMX PLAYER FOR SCREENSAVER"), ss_omx);
	addSaveFunc([ss_omx, this] { Settings::getInstance()->setBool("ScreenSaverOmxPlayer", ss_omx->getState()); });

	ss_omx->setOnChangedCallback([this, ss_omx, window]()
	{
		if (Settings::getInstance()->setBool("ScreenSaverOmxPlayer", ss_omx->getState()))
		{
			Window* pw = mWindow;
			delete this;
			pw->pushGui(new GuiVideoScreensaverOptions(pw, _("VIDEO SCREENSAVER").c_str()));
		}
	});
#endif

	// Render Video Game Name as subtitles
	auto ss_info = std::make_shared< OptionListComponent<std::string> >(mWindow, _("SHOW GAME INFO"), false);
	std::vector<std::string> info_type;
	info_type.push_back("always");
	info_type.push_back("start & end");
	info_type.push_back("never");
	for(auto it = info_type.cbegin(); it != info_type.cend(); it++)
		ss_info->add(_(it->c_str()), *it, Settings::getInstance()->getString("ScreenSaverGameInfo") == *it);
	addWithLabel(_("SHOW GAME INFO ON SCREENSAVER"), ss_info);
	addSaveFunc([ss_info, this] { Settings::getInstance()->setString("ScreenSaverGameInfo", ss_info->getSelected()); });

	// SHOW DATE TIME
	bool showDateTime = Settings::getInstance()->getBool("ScreenSaverDateTime");
	auto datetime_screensaver = std::make_shared<SwitchComponent>(mWindow);
	datetime_screensaver->setState(showDateTime);
	addWithLabel(_("SHOW DATE TIME"), datetime_screensaver);
	datetime_screensaver->setOnChangedCallback([this, datetime_screensaver]() {
		if (Settings::getInstance()->setBool("ScreenSaverDateTime", datetime_screensaver->getState()))
		{
			Window* pw = mWindow;
			delete this;
			pw->pushGui(new GuiVideoScreensaverOptions(pw, _("VIDEO SCREENSAVER").c_str()));
		}
	});
	if (showDateTime)
	{
		auto sss_date_format = std::make_shared< OptionListComponent<std::string> >(mWindow, _("DATE FORMAT"), false);
		sss_date_format->addRange({ "%Y-%m-%d", "%d-%m-%Y", "%A, %B %d", "%b %d, %Y" }, Settings::getInstance()->getString("ScreenSaverDateFormat"));
		addWithLabel(_("DATE FORMAT"), sss_date_format);
		addSaveFunc([sss_date_format] { Settings::getInstance()->setString("ScreenSaverDateFormat", sss_date_format->getSelected()); });

		auto sss_time_format = std::make_shared< OptionListComponent<std::string> >(mWindow, _("TIME FORMAT"), false);
		sss_time_format->addRange({ "%H:%M:%S", "%I:%M %p", "%p %I:%M" }, Settings::getInstance()->getString("ScreenSaverTimeFormat"));
		addWithLabel(_("TIME FORMAT"), sss_time_format);
		addSaveFunc([sss_time_format] { Settings::getInstance()->setString("ScreenSaverTimeFormat", sss_time_format->getSelected()); });
	}

	bool advancedOptions = true;

//#ifdef _RPI_
	advancedOptions = !Settings::getInstance()->getBool("ScreenSaverOmxPlayer");
//#endif

	if (advancedOptions)
	{
		auto marquee_screensaver = std::make_shared<SwitchComponent>(mWindow);
		marquee_screensaver->setState(Settings::getInstance()->getBool("ScreenSaverMarquee"));
		addWithLabel(_("USE MARQUEE AS GAME INFO"), marquee_screensaver);
		addSaveFunc([marquee_screensaver] { Settings::getInstance()->setBool("ScreenSaverMarquee", marquee_screensaver->getState()); });
/*
		auto decoration_screensaver = std::make_shared<SwitchComponent>(mWindow);
		decoration_screensaver->setState(Settings::getInstance()->getBool("ScreenSaverDecoration"));
		addWithLabel(_("USE RANDOM DECORATION"), decoration_screensaver);
		addSaveFunc([decoration_screensaver] { Settings::getInstance()->setBool("ScreenSaverDecoration", decoration_screensaver->getState()); });
*/
	}

	auto stretch_screensaver = std::make_shared<SwitchComponent>(mWindow);
	stretch_screensaver->setState(Settings::getInstance()->getBool("StretchVideoOnScreenSaver"));
	addWithLabel(_("STRETCH VIDEO ON SCREENSAVER"), stretch_screensaver);
	addSaveFunc([stretch_screensaver] { Settings::getInstance()->setBool("StretchVideoOnScreenSaver", stretch_screensaver->getState()); });
}

GuiVideoScreensaverOptions::~GuiVideoScreensaverOptions()
{
}

void GuiVideoScreensaverOptions::save()
{
#ifdef _RPI_
	bool startingStatusNotRisky = (Settings::getInstance()->getString("ScreenSaverGameInfo") == "never" || !Settings::getInstance()->getBool("ScreenSaverOmxPlayer"));
#endif
	GuiScreensaverOptions::save();

#ifdef _RPI_
	bool endStatusRisky = (Settings::getInstance()->getString("ScreenSaverGameInfo") != "never" && Settings::getInstance()->getBool("ScreenSaverOmxPlayer"));
	if (startingStatusNotRisky && endStatusRisky) {
		// if before it wasn't risky but now there's a risk of problems, show warning
		mWindow->pushGui(new GuiMsgBox(mWindow,
		"Using OMX Player and displaying Game Info may result in the video flickering in some TV modes. If that happens, consider:\n\n• Disabling the \"Show Game Info\" option;\n• Disabling \"Overscan\" on the Pi configuration menu might help:\nRetroPie > Raspi-Config > Advanced Options > Overscan > \"No\".\n• Disabling the use of OMX Player for the screensaver.",
			"GOT IT!", [] { return; }));
	}
#endif
}
