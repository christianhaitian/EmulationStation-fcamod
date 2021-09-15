#include "guis/GuiThemeInstall.h"

#include "ApiSystem.h"
#include "components/OptionListComponent.h"
#include "guis/GuiSettings.h"
#include "views/ViewController.h"
#include "utils/StringUtil.h"
#include "components/ComponentGrid.h"
#include "components/MultiLineMenuEntry.h"
#include "EsLocale.h"
#include "ContentInstaller.h"

GuiThemeInstall::GuiThemeInstall(Window* window)
	: GuiComponent(window), mMenu(window, _("SELECT THEME TO INSTALL").c_str())
{
	auto theme = ThemeData::getMenuTheme();

	addChild(&mMenu);
	
	auto themes = ApiSystem::getThemesList();
	for(auto utheme : themes)
	{
		std::string themeName = utheme.name;

		ComponentListRow row;

		// icon
		/*
		auto icon = std::make_shared<ImageComponent>(mWindow);
		icon->setImage(utheme.installed ? ":/star_filled.svg" : ":/star_unfilled.svg");
		icon->setColorShift(theme->Text.color);
		icon->setResize(0, theme->Text.font->getLetterHeight() * 1.25f);
		row.addElement(icon, false);
		*/

		auto icon = std::make_shared<TextComponent>(mWindow);		
		icon->setColor(theme->Text.color);

		if (utheme.installed)
			icon->setOpacity(192);

		icon->setFont(theme->Text.font);
		icon->setText(utheme.installed ? _U("\uF021") : _U("\uF019"));
		icon->setSize(theme->Text.font->getLetterHeight() * 1.5f, 0);
		//icon->setResize(0, theme->Text.font->getLetterHeight() * 1.25f);
		row.addElement(icon, false);


		// spacer between icon and text
		auto spacer = std::make_shared<GuiComponent>(mWindow);
		spacer->setSize(10, 0);
		row.addElement(spacer, false);

		auto grid = std::make_shared<MultiLineMenuEntry>(window, utheme.name, utheme.url);
		row.addElement(grid, true);
		row.makeAcceptInputHandler([this, themeName] { start(themeName); });

		mMenu.addRow(row);	
	}

	mMenu.addButton(_("BACK"), "back", [&] { delete this; });

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiThemeInstall::start(std::string themeName)
{
	if (themeName.empty())
		return;

	char trstring[256];
	snprintf(trstring, 256, _("'%s' ADDED TO DOWNLOAD QUEUE").c_str(), themeName.c_str()); // batocera
	mWindow->displayNotificationMessage(_U("\uF019 ") + std::string(trstring));

	ContentInstaller::Enqueue(mWindow, ContentInstaller::CONTENT_THEME, themeName);
    delete this;
}

bool GuiThemeInstall::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;
	
	if(input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		delete this;
		return true;
	}

	if(config->isMappedTo("start", input) && input.value != 0)
	{
		// close everything
		Window* window = mWindow;
		while(window->peekGui() && window->peekGui() != ViewController::get())
			delete window->peekGui();
	}
	return false;
}

std::vector<HelpPrompt> GuiThemeInstall::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	return prompts;
}
