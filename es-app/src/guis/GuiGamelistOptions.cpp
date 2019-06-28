#include "GuiGamelistOptions.h"

#include "guis/GuiGamelistFilter.h"
#include "scrapers/Scraper.h"
#include "views/gamelist/IGameListView.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "GuiMetaDataEd.h"
#include "SystemData.h"

#include "animations/LambdaAnimation.h"

std::vector<std::string> GuiGamelistOptions::gridSizes {
	"automatic",
	
	"1x1",

	"2x1",
	"2x2",
	"2x3",	
	"2x4",
	"2x5",
	"2x6",
	"2x7",

	"3x1",
	"3x2",
	"3x3",
	"3x4",
	"3x5",
	"3x6",
	"3x7",
	
	"4x1",
	"4x2",
	"4x3",
	"4x4",
	"4x5",
	"4x6",
	"4x7",

	"5x1",
	"5x2",
	"5x3",
	"5x4",
	"5x5",
	"5x6",
	"5x7",

	"6x1",
	"6x2",
	"6x3",
	"6x4",
	"6x5",
	"6x6",
	"6x7",

	"7x1",
	"7x2",
	"7x3",
	"7x4",
	"7x5",
	"7x6",
	"7x7"
};

GuiGamelistOptions::GuiGamelistOptions(Window* window, SystemData* system, bool showGridFeatures) : GuiComponent(window),
	mSystem(system), mMenu(window, "OPTIONS"), fromPlaceholder(false), mFiltersChanged(false)
{	
	mGridSize = NULL;
	addChild(&mMenu);

	// check it's not a placeholder folder - if it is, only show "Filter Options"
	FileData* file = getGamelist()->getCursor();
	fromPlaceholder = file->isPlaceHolder();
	ComponentListRow row;

	if (!fromPlaceholder) {
		// jump to letter
		row.elements.clear();

		// define supported character range
		// this range includes all numbers, capital letters, and most reasonable symbols
		char startChar = '!';
		char endChar = '_';

		char curChar = (char)toupper(getGamelist()->getCursor()->getName()[0]);
		if(curChar < startChar || curChar > endChar)
			curChar = startChar;

		mJumpToLetterList = std::make_shared<LetterList>(mWindow, _T("JUMP TO LETTER"), false);
		for (char c = startChar; c <= endChar; c++)
		{
			// check if c is a valid first letter in current list
			const std::vector<FileData*>& files = getGamelist()->getCursor()->getParent()->getChildrenListToDisplay();
			for (auto file : files)
			{
				char candidate = (char)toupper(file->getName()[0]);
				if (c == candidate)
				{
					mJumpToLetterList->add(std::string(1, c), c, c == curChar);
					break;
				}
			}
		}

		row.addElement(std::make_shared<TextComponent>(mWindow, _T("JUMP TO LETTER"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
		row.addElement(mJumpToLetterList, false);
		row.input_handler = [&](InputConfig* config, Input input) 
		{
			if(config->isMappedTo("a", input) && input.value)
			{
				jumpToLetter();
				return true;
			}
			else if(mJumpToLetterList->input(config, input))
			{
				return true;
			}
			return false;
		};
		mMenu.addRow(row);

		// sort list by
		mListSort = std::make_shared<SortList>(mWindow, _T("SORT GAMES BY"), false);
		for(unsigned int i = 0; i < FileSorts::SortTypes.size(); i++)
		{
			const FolderData::SortType& sort = FileSorts::SortTypes.at(i);
			mListSort->add(_L(Utils::String::toUpper(sort.description)), &sort, i == 0); // TODO - actually make the sort type persistent
		}

		mMenu.addWithLabel(_T("SORT GAMES BY"), mListSort);
	}

	// GameList view style
	mViewMode = std::make_shared< OptionListComponent<std::string> >(mWindow, _T("GAMELIST VIEW STYLE"), false);
	std::vector<std::string> styles;
	styles.push_back("automatic");

	auto mViews = system->getTheme()->getViewsOfTheme();
	for (auto it = mViews.cbegin(); it != mViews.cend(); ++it)
		styles.push_back(*it);

	std::string viewMode = system->getSystemViewMode();

	bool found = false;
	for (auto it = styles.cbegin(); it != styles.cend(); it++)
	{
		bool sel = (viewMode.empty() && *it == "automatic") || viewMode == *it;
		if (sel)
			found = true;

		mViewMode->add(_L(*it), *it, sel);
	}

	if (!found)
		mViewMode->selectFirstItem();

	mMenu.addWithLabel(_T("GAMELIST VIEW STYLE"), mViewMode);	
	
	

	// Grid size override
	if (showGridFeatures)
	{
		auto gridOverride = system->getGridSizeOverride();
		auto ovv = std::to_string((int)gridOverride.x()) + "x" + std::to_string((int)gridOverride.y());

		mGridSize = std::make_shared<OptionListComponent<std::string>>(mWindow, _T("GRID SIZE"), false);

		found = false;
		for (auto it = gridSizes.cbegin(); it != gridSizes.cend(); it++)
		{
			bool sel = (gridOverride == Vector2f(0, 0) && *it == "automatic") || ovv == *it;
			if (sel)
				found = true;

			mGridSize->add(_L(*it), *it, sel);
		}

		if (!found)
			mGridSize->selectFirstItem();

		mMenu.addWithLabel(_T("GRID SIZE"), mGridSize);
	}

	// show filtered menu
	if(!Settings::getInstance()->getBool("ForceDisableFilters"))
	{
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(mWindow, _T("APPLY FILTER"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
		row.addElement(makeArrow(mWindow), false);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::openGamelistFilter, this));
		mMenu.addRow(row);		
	}



	/*
	// maximum vram
	auto max_vram = std::make_shared<SliderComponent>(mWindow, 0.f, 2000.f, 10.f, "Mb");
	max_vram->setValue((float)(Settings::getInstance()->getInt("MaxVRAM")));
	s->addWithLabel(_T("VRAM LIMIT"), max_vram);
	s->addSaveFunc([max_vram] { Settings::getInstance()->setInt("MaxVRAM", (int)Math::round(max_vram->getValue())); });
	*/
















	std::map<std::string, CollectionSystemData> customCollections = CollectionSystemManager::get()->getCustomCollectionSystems();

	if(UIModeController::getInstance()->isUIModeFull() &&
		((customCollections.find(system->getName()) != customCollections.cend() && CollectionSystemManager::get()->getEditingCollection() != system->getName()) ||
		CollectionSystemManager::get()->getCustomCollectionsBundle()->getName() == system->getName()))
	{
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(mWindow, "ADD/REMOVE GAMES TO THIS GAME COLLECTION", ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::startEditMode, this));
		mMenu.addRow(row);
	}

	if(UIModeController::getInstance()->isUIModeFull() && CollectionSystemManager::get()->isEditing())
	{
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(mWindow, "FINISH EDITING '" + Utils::String::toUpper(CollectionSystemManager::get()->getEditingCollection()) + "' COLLECTION", ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::exitEditMode, this));
		mMenu.addRow(row);
	}

	if (UIModeController::getInstance()->isUIModeFull() && !fromPlaceholder && !(mSystem->isCollection() && file->getType() == FOLDER))
	{
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(mWindow, _T("EDIT THIS GAME'S METADATA"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color), true);
		row.addElement(makeArrow(mWindow), false);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::openMetaDataEd, this));
		mMenu.addRow(row);
	}

	// center the menu
	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());	

	float x0 = (mSize.x() - mMenu.getSize().x()) / 2;

	float y1 = Renderer::getScreenHeight();
	float y2 = (mSize.y() - mMenu.getSize().y()) / 2;
	
	if (Settings::getInstance()->getString("PowerSaverMode") == "instant" || Settings::getInstance()->getString("TransitionStyle") == "instant")
		setPosition(x0, y2);
	else
	{
		setPosition(x0, y1);

		auto fadeFunc = [this, x0, y1, y2](float t) {

			t -= 1; // cubic ease out
			float pct = Math::lerp(0, 1, t*t*t + 1);

			float y = y1 * (1 - pct) + y2 * pct;
			setPosition(x0, y);
		};

		setAnimation(new LambdaAnimation(fadeFunc, 350), 0, [this, fadeFunc, x0, y2]
		{
			setPosition(x0, y2);
		});

		setPosition(x0, y2);
	}
}

GuiGamelistOptions::~GuiGamelistOptions()
{
	// apply sort
	if (!fromPlaceholder) {
		FolderData* root = mSystem->getRootFolder();
		root->sort(*mListSort->getSelected()); // will also recursively sort children

		// notify that the root folder was sorted
		getGamelist()->onFileChanged(root, FILE_SORTED);
	}

	Vector2f gridSizeOverride(0, 0);

	if (mGridSize != NULL)
	{
		auto str = mGridSize->getSelected();

		size_t divider = str.find('x');
		if (divider != std::string::npos)
		{
			std::string first = str.substr(0, divider);
			std::string second = str.substr(divider+1, std::string::npos);

			gridSizeOverride = Vector2f((float)atof(first.c_str()), (float)atof(second.c_str()));
		}
	}
	
	bool viewModeChanged = mSystem->setSystemViewMode(mViewMode->getSelected(), gridSizeOverride);
	if (viewModeChanged || mFiltersChanged)
	{
		if (viewModeChanged)
			Settings::getInstance()->saveFile();

		// only reload full view if we came from a placeholder
		// as we need to re-display the remaining elements for whatever new
		// game is selected
		ViewController::get()->reloadGameListView(mSystem);
	}
}

void GuiGamelistOptions::openGamelistFilter()
{
	mFiltersChanged = true;
	GuiGamelistFilter* ggf = new GuiGamelistFilter(mWindow, mSystem);
	mWindow->pushGui(ggf);
}

void GuiGamelistOptions::startEditMode()
{
	std::string editingSystem = mSystem->getName();
	// need to check if we're editing the collections bundle, as we will want to edit the selected collection within
	if(editingSystem == CollectionSystemManager::get()->getCustomCollectionsBundle()->getName())
	{
		FileData* file = getGamelist()->getCursor();
		// do we have the cursor on a specific collection?
		if (file->getType() == FOLDER)
		{
			editingSystem = file->getName();
		}
		else
		{
			// we are inside a specific collection. We want to edit that one.
			editingSystem = file->getSystem()->getName();
		}
	}
	CollectionSystemManager::get()->setEditMode(editingSystem);
	delete this;
}

void GuiGamelistOptions::exitEditMode()
{
	CollectionSystemManager::get()->exitEditMode();
	delete this;
}

void GuiGamelistOptions::openMetaDataEd()
{
	// open metadata editor
	// get the FileData that hosts the original metadata
	FileData* file = getGamelist()->getCursor()->getSourceFileData();
	ScraperSearchParams p;
	p.game = file;
	p.system = file->getSystem();

	std::function<void()> deleteBtnFunc;

	if (file->getType() == FOLDER)
	{
		deleteBtnFunc = NULL;
	}
	else
	{
		deleteBtnFunc = [this, file] {
			CollectionSystemManager::get()->deleteCollectionFiles(file);
			ViewController::get()->getGameListView(file->getSystem()).get()->remove(file, true);
		};
	}

	mWindow->pushGui(new GuiMetaDataEd(mWindow, &file->metadata, file->metadata.getMDD(), p, Utils::FileSystem::getFileName(file->getPath()),
		std::bind(&IGameListView::onFileChanged, ViewController::get()->getGameListView(file->getSystem()).get(), file, FILE_METADATA_CHANGED), deleteBtnFunc, file));
}

void GuiGamelistOptions::jumpToLetter()
{
	char letter = mJumpToLetterList->getSelected();
	IGameListView* gamelist = getGamelist();

	// this is a really shitty way to get a list of files
	const std::vector<FileData*>& files = gamelist->getCursor()->getParent()->getChildrenListToDisplay();

	long min = 0;
	long max = (long)files.size() - 1;
	long mid = 0;

	while(max >= min)
	{
		mid = ((max - min) / 2) + min;

		// game somehow has no first character to check
		if(files.at(mid)->getName().empty())
			continue;

		char checkLetter = (char)toupper(files.at(mid)->getName()[0]);

		if(checkLetter < letter)
			min = mid + 1;
		else if(checkLetter > letter || (mid > 0 && (letter == toupper(files.at(mid - 1)->getName()[0]))))
			max = mid - 1;
		else
			break; //exact match found
	}

	gamelist->setCursor(files.at(mid));

	delete this;
}

bool GuiGamelistOptions::input(InputConfig* config, Input input)
{
	if((config->isMappedTo("b", input) || config->isMappedTo("select", input)) && input.value)
	{
		delete this;
		return true;
	}

	return mMenu.input(config, input);
}

HelpStyle GuiGamelistOptions::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(mSystem->getTheme(), "system");
	return style;
}

std::vector<HelpPrompt> GuiGamelistOptions::getHelpPrompts()
{
	auto prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", _T("CLOSE")));
	return prompts;
}

IGameListView* GuiGamelistOptions::getGamelist()
{
	return ViewController::get()->getGameListView(mSystem).get();
}
