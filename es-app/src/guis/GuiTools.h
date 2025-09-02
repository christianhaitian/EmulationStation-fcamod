#pragma once
#include "components/MenuComponent.h"
#include "Window.h"

class GuiTools : public GuiComponent {
public:
    GuiTools(Window* window);

private:
    MenuComponent mMenu;
    void runSystemCommand(const std::string& cmd);
};
