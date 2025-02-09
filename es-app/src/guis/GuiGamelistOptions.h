#pragma once
#ifndef ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H
#define ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H

#include "components/MenuComponent.h"
#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "FileData.h"
#include "GuiComponent.h"

class IGameListView;
class SystemData;

class GuiGamelistOptions : public GuiComponent
{
public:
	GuiGamelistOptions(Window* window, SystemData* system, bool showGridFeatures = false);
	virtual ~GuiGamelistOptions();

	virtual bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	virtual HelpStyle getHelpStyle() override;

	static std::vector<std::string> gridSizes;

private:
	inline void addSaveFunc(const std::function<void()>& func) { mSaveFuncs.push_back(func); };

	void addTextFilterToMenu();
	void openGamelistFilter();
	void openMetaDataEd();
	void startEditMode();
	void exitEditMode();
	void jumpToLetter();

	MenuComponent mMenu;

	typedef OptionListComponent<std::string> LetterList;
	std::shared_ptr<LetterList> mJumpToLetterList;

	typedef OptionListComponent<unsigned int> SortList;
	std::shared_ptr<SortList> mListSort;

	std::shared_ptr<GuiComponent> mTextFilter;
	std::shared_ptr<OptionListComponent<std::string>> mViewMode;
	std::shared_ptr<OptionListComponent<std::string>> mGridSize;

	SystemData* mSystem;
	IGameListView* getGamelist();
	bool fromPlaceholder;
	bool mFiltersChanged;

	std::vector< std::function<void()> > mSaveFuncs;
	bool mReloadAll;
};

#endif // ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H
