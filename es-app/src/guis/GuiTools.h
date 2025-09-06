#pragma once
#ifndef ES_APP_GUIS_GUITOOLS_H
#define ES_APP_GUIS_GUITOOLS_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"

class GuiTools : public GuiComponent
{
public:
    GuiTools(Window* window);
    ~GuiTools() override;   // ✅ destructor matches .cpp

    bool input(InputConfig* config, Input input) override;

    void render(const Transform4x4f& parentTrans) override;   // ✅ render override

private:
    void addScriptsToMenu(MenuComponent& menu, const std::string& folderPath);
    void launchTool(const std::string& script);

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUITOOLS_H

