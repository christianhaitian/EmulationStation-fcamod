#include "guis/GuiScraperStart.h"

#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperMulti.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "SystemData.h"

GuiScraperStart::GuiScraperStart(Window* window) : GuiComponent(window),
	mMenu(window, _T("SCRAPE NOW"))
{
	addChild(&mMenu);

	// add filters (with first one selected)
	mFilters = std::make_shared< OptionListComponent<GameFilterFunc> >(mWindow, _T("SCRAPE THESE GAMES"), false);
	mFilters->add(_T("All Games"),
		[](SystemData*, FileData*) -> bool { return true; }, false);
	mFilters->add(_T("Only missing image"),
		[](SystemData*, FileData* g) -> bool { return g->metadata.get("image").empty(); }, true);
	mMenu.addWithLabel(_T("FILTER"), mFilters);

	//add systems (all with a platformid specified selected)
	mSystems = std::make_shared< OptionListComponent<SystemData*> >(mWindow, _T("SCRAPE THESE SYSTEMS"), true);
	for(auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
	{
		if(!(*it)->hasPlatformId(PlatformIds::PLATFORM_IGNORE))
			mSystems->add((*it)->getFullName(), *it, !(*it)->getPlatformIds().empty());
	}
	mMenu.addWithLabel(_T("SYSTEMS"), mSystems);

	mApproveResults = std::make_shared<SwitchComponent>(mWindow);
	mApproveResults->setState(true);
	mMenu.addWithLabel(_T("USER DECIDES ON CONFLICTS"), mApproveResults);

	mMenu.addButton(_T("START"), _T("START"), std::bind(&GuiScraperStart::pressedStart, this));
	mMenu.addButton(_T("BACK"), _T("BACK"), [&] { delete this; });

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiScraperStart::pressedStart()
{
	std::vector<SystemData*> sys = mSystems->getSelectedObjects();
	for(auto it = sys.cbegin(); it != sys.cend(); it++)
	{
		if((*it)->getPlatformIds().empty())
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, 
				Utils::String::toUpper("Warning: some of your selected systems do not have a platform set. Results may be even more inaccurate than usual!\nContinue anyway?"), 
				_T("YES"), std::bind(&GuiScraperStart::start, this), 
				_T("NO"), nullptr));
			return;
		}
	}

	start();
}

void GuiScraperStart::start()
{
	std::queue<ScraperSearchParams> searches = getSearches(mSystems->getSelectedObjects(), mFilters->getSelected());

	if(searches.empty())
	{
		mWindow->pushGui(new GuiMsgBox(mWindow, _T("NO GAMES FIT THAT CRITERIA.")));
	}
	else
	{
		GuiScraperMulti* gsm = new GuiScraperMulti(mWindow, searches, mApproveResults->getState());
		mWindow->pushGui(gsm);
		delete this;
	}
}

std::queue<ScraperSearchParams> GuiScraperStart::getSearches(std::vector<SystemData*> systems, GameFilterFunc selector)
{
	std::queue<ScraperSearchParams> queue;
	for(auto sys = systems.cbegin(); sys != systems.cend(); sys++)
	{
		std::vector<FileData*> games = (*sys)->getRootFolder()->getFilesRecursive(GAME);
		for(auto game = games.cbegin(); game != games.cend(); game++)
		{
			if(selector((*sys), (*game)))
			{
				ScraperSearchParams search;
				search.game = *game;
				search.system = *sys;
				
				queue.push(search);
			}
		}
	}

	return queue;
}

bool GuiScraperStart::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if(consumed)
		return true;
	
	if(input.value != 0 && config->isMappedTo("b", input))
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

std::vector<HelpPrompt> GuiScraperStart::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", _T("BACK")));
	prompts.push_back(HelpPrompt("start", _T("CLOSE")));
	return prompts;
}
