#include "views/gamelist/BasicGameListView.h"

#include "utils/FileSystemUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"

BasicGameListView::BasicGameListView(Window* window, FolderData* root)
	: ISimpleGameListView(window, root), mList(window)
{
	mLoaded = false;

	mList.setSize(mSize.x(), mSize.y() * 0.8f);
	mList.setPosition(0, mSize.y() * 0.2f);
	mList.setDefaultZIndex(20);
	addChild(&mList);		

	populateList(mRoot->getChildrenListToDisplay());
}

void BasicGameListView::onShow()
{
	/*
	if (!mLoaded)
	{
		populateList(mRoot->getChildrenListToDisplay());
		mLoaded = true;
	}
	*/
	ISimpleGameListView::onShow();
}

void BasicGameListView::setThemeName(std::string name)
{
	ISimpleGameListView::setThemeName(name);
	// mGrid.setThemeName(getName());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	using namespace ThemeFlags;
	mList.applyTheme(theme, getName(), "gamelist", ALL);

	sortChildren();
}

void BasicGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	if (change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

	ISimpleGameListView::onFileChanged(file, change);
}

void BasicGameListView::populateList(const std::vector<FileData*>& files)
{
	mList.clear();

	std::string systemName = mRoot->getSystem()->getFullName();
	mHeaderText.setText(systemName);

	std::string systemShortName = mRoot->getSystem()->getName();

	bool favoritesFirst = Settings::getInstance()->getBool("FavoritesFirst");
	bool showFavoriteIcon = (systemName != "favorites" && systemShortName != "recent");
	if (!showFavoriteIcon)
		favoritesFirst = false;	

	if (files.size() > 0)
	{
		if (mCursorStack.size())
		{
			FileData* placeholder = new FileData(PLACEHOLDER, "..", this->mRoot->getSystem());
			mList.add(". .", placeholder, (placeholder->getType() == PLACEHOLDER));
		}

		if (favoritesFirst)
		{
			for (auto file : files)
			{
				if (!file->getFavorite())
					continue;

				if (showFavoriteIcon)
					mList.add(_U("\u2605 ") + file->getName(), file, file->getType() == FOLDER);
				else if (file->getType() == FOLDER)
					mList.add(_U("\u272B ") + file->getName(), file, true);
				else
					mList.add(file->getName(), file, false);
			}
		}

		for (auto file : files)
		{
			if (file->getFavorite())
			{
				if (favoritesFirst)
					continue;

				if (showFavoriteIcon)
				{
					mList.add(_U("\u2605 ") + file->getName(), file, file->getType() == FOLDER);
					continue;
				}
			}

			if (file->getType() == FOLDER)
				mList.add(_U("\uf07B ") + file->getName(), file, true);
			else
				mList.add(file->getName(), file, false);
		}
	}
	else
	{
		addPlaceholder();
	}
}

FileData* BasicGameListView::getCursor()
{
	if (mList.size() == 0)
		return nullptr;

	return mList.getSelected();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	if(!mList.setCursor(cursor) && (!cursor->isPlaceHolder()))
	{
		auto children = mRoot->getChildrenListToDisplay();

		auto gameIter = std::find(children.cbegin(), children.cend(), cursor);
		if (gameIter == children.cend())
		{
			children = cursor->getParent()->getChildrenListToDisplay();

			// update our cursor stack in case our cursor just got set to some folder we weren't in before
			if (mCursorStack.empty() || mCursorStack.top() != cursor->getParent())
			{
				std::stack<FileData*> tmp;
				FileData* ptr = cursor->getParent();
				while (ptr && ptr != mRoot)
				{
					tmp.push(ptr);
					ptr = ptr->getParent();
				}

				// flip the stack and put it in mCursorStack
				mCursorStack = std::stack<FileData*>();
				while (!tmp.empty())
				{
					mCursorStack.push(tmp.top());
					tmp.pop();
				}
			}
		}
	
		populateList(children);
		mList.setCursor(cursor);
	}
}

void BasicGameListView::addPlaceholder()
{
	// empty list - add a placeholder
	FileData* placeholder = new FileData(PLACEHOLDER, "<" + _("No Entries Found") + ">", mRoot->getSystem());	
	mList.add(placeholder->getName(), placeholder, true);
}

std::string BasicGameListView::getQuickSystemSelectRightButton()
{
	return "right";
}

std::string BasicGameListView::getQuickSystemSelectLeftButton()
{
	return "left";
}

void BasicGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

void BasicGameListView::remove(FileData *game, bool deleteFile)
{
	if (deleteFile)
		Utils::FileSystem::removeFile(game->getPath());  // actually delete the file on the filesystem

	FolderData* parent = game->getParent();
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
		auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
		unsigned int gamePos = (int)std::distance(siblings.cbegin(), gameIter);
		if (gameIter != siblings.cend())
		{
			if ((gamePos + 1) < siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if (gamePos > 1) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
	}
	mList.remove(game);
	if(mList.size() == 0)
	{
		addPlaceholder();
	}
	delete game;                                 // remove before repopulating (removes from parent)
	onFileChanged(parent, FILE_REMOVED);           // update the view, with game removed
}

std::vector<HelpPrompt> BasicGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect"))
		prompts.push_back(HelpPrompt("left/right", _("SYSTEM")));

	prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
	prompts.push_back(HelpPrompt(BUTTON_OK, _("LAUNCH")));
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	if(!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("select", _("OPTIONS")));
	
	if(mRoot->getSystem()->isGameSystem())
		prompts.push_back(HelpPrompt("x", _("RANDOM")));

	if(mRoot->getSystem()->isGameSystem() && !UIModeController::getInstance()->isUIModeKid())
	{
		std::string prompt = CollectionSystemManager::get()->getEditingCollection();
		if (prompt == "Favorites")
			prompt = _("FAVORITE");

		prompts.push_back(HelpPrompt("y", prompt));
	}
	return prompts;
}

std::vector<FileData*> BasicGameListView::getFileDataEntries()
{
	return mList.getObjects();
}
