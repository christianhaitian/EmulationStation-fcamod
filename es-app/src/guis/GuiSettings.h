#pragma once
#ifndef ES_APP_GUIS_GUI_SETTINGS_H
#define ES_APP_GUIS_GUI_SETTINGS_H

#include "components/MenuComponent.h"

// This is just a really simple template for a GUI that calls some save functions when closed.
class GuiSettings : public GuiComponent
{
public:
	GuiSettings(Window* window, std::string title);
	virtual ~GuiSettings(); // just calls save();
	
	void updatePosition();
	inline void addRow(const ComponentListRow& row) { mMenu.addRow(row); };
	inline void addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp, const std::string iconName = "") { mMenu.addWithLabel(label, comp, iconName); };
	inline void addSaveFunc(const std::function<void()>& func) { mSaveFuncs.push_back(func); };
	inline void addEntry(const std::string name, bool add_arrow, const std::function<void()>& func, const std::string iconName = "") { mMenu.addEntry(name, add_arrow, func, iconName); };

	inline void addSubMenu(const std::string& label, const std::function<void()>& func) {
		ComponentListRow row;
		row.makeAcceptInputHandler(func);

		auto theme = ThemeData::getMenuTheme();

		auto entryMenu = std::make_shared<TextComponent>(mWindow, label, theme->Text.font, theme->Text.color);
		row.addElement(entryMenu, true);
		row.addElement(makeArrow(mWindow), false);
		mMenu.addRow(row);
	};

	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	HelpStyle getHelpStyle() override;

	inline void setBeforeCloseFunc(const std::function<void()>& func) { mBeforeCloseFunc = func; mEnableBeforeCloseFunc = (mBeforeCloseFunc != nullptr); };
	void enableBeforeCloseFunc(bool use) { mEnableBeforeCloseFunc = use; }

	void save();

private:
	void saveAndClose();

	MenuComponent mMenu;
	std::vector< std::function<void()> > mSaveFuncs;
	std::function<void()> mBeforeCloseFunc;
	bool mEnableBeforeCloseFunc;
};

#endif // ES_APP_GUIS_GUI_SETTINGS_H
