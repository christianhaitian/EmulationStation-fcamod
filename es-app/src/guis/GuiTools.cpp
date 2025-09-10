#include "guis/GuiTools.h"
#include "components/MenuComponent.h"
#include "Window.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "platform.h"

#include <fstream>
#include <algorithm>

class SubMenuWrapper : public GuiComponent
{
public:
    SubMenuWrapper(Window* window, MenuComponent* menu)
        : GuiComponent(window), mMenu(menu)
    {
        addChild(menu);
    }

    // Called from BACK button or input
    void close()
    {
        mWindow->removeGui(this);
        // Do NOT delete manually; Window will handle it
    }

    bool input(InputConfig* config, Input input) override
    {
        if (input.value != 0 && config->isMappedTo("b", input))
        {
            close();  // B/Escape closes the submenu
            return true;
        }
        return GuiComponent::input(config, input);
    }

private:
    MenuComponent* mMenu;
};

GuiTools::GuiTools(Window* window)
    : GuiComponent(window), mMenu(window, "OPTIONS")
{
    addChild(&mMenu);

    addScriptsToMenu(mMenu, "/opt/system");

    // back button
    mMenu.addButton("BACK", "back", [this] { delete this; });
}

GuiTools::~GuiTools()
{
}

void GuiTools::addScriptsToMenu(MenuComponent& menu, const std::string& folderPath)
{
    auto items = Utils::FileSystem::getDirContent(folderPath);
    std::vector<std::pair<std::string, std::string>> scripts;
    std::vector<std::pair<std::string, std::string>> folders;

    for (auto& item : items)
    {
        std::string name = Utils::FileSystem::getFileName(item); // "Wifi.sh"
        std::string fullPath = item;

        if (Utils::FileSystem::isDirectory(fullPath))
        {
            folders.push_back(std::make_pair(name, fullPath));
        }
        else if (Utils::String::toLower(Utils::FileSystem::getExtension(name)) == ".sh")
        {
            std::string displayName = Utils::FileSystem::getStem(name);

            // Check for .name sidecar file
            std::string nameFile = fullPath + ".name";
            if (Utils::FileSystem::exists(nameFile))
            {
                std::ifstream f(nameFile.c_str());
                if (f)
                {
                    std::getline(f, displayName);
                    f.close();
                }
            }
            else
            {
                // Look for "# NAME:" in first 10 lines of script
                std::ifstream f(fullPath.c_str());
                if (f)
                {
                    std::string line;
                    const std::string marker = "# NAME:";
                    int linesChecked = 0;

                    while (linesChecked < 10 && std::getline(f, line))
                    {
                        linesChecked++;
                        if (line.find(marker) == 0)
                        {
                            std::string candidate = Utils::String::trim(line.substr(marker.size()));
                            if (!candidate.empty())
                            {
                                displayName = candidate;
                                break;
                            }
                        }
                    }
                    f.close();
                }
            }

            scripts.push_back(std::make_pair(displayName, fullPath));
        }
    }

    // Sort folders alphabetically, case-insensitive
    std::sort(folders.begin(), folders.end(),
        [](const std::pair<std::string, std::string>& a,
           const std::pair<std::string, std::string>& b) {
            return Utils::String::toLower(a.first) < Utils::String::toLower(b.first);
        });

    // Sort scripts alphabetically, case-insensitive
    std::sort(scripts.begin(), scripts.end(),
        [](const std::pair<std::string, std::string>& a,
           const std::pair<std::string, std::string>& b) {
            return Utils::String::toLower(a.first) < Utils::String::toLower(b.first);
        });

    // Add folders (submenus)
    for (size_t i = 0; i < folders.size(); i++)
    {
        const std::string folderName = folders[i].first;
        const std::string folderPathFull = folders[i].second;

        MenuComponent* subMenu = new MenuComponent(mWindow, folderName);
        addScriptsToMenu(*subMenu, folderPathFull);

        // Wrap it
        SubMenuWrapper* wrapper = new SubMenuWrapper(mWindow, subMenu);

        subMenu->addButton("BACK", "back", [wrapper]() {
            wrapper->close();
        });

        menu.addEntry(folderName, false, [this, wrapper] {
            mWindow->pushGui(wrapper);
        });
    }

    // Add scripts
    for (size_t i = 0; i < scripts.size(); i++)
    {
        const std::string displayName = scripts[i].first;
        const std::string path = scripts[i].second;

        menu.addEntry(displayName, false, [this, path] {
            launchTool(path);
        });
    }
}

void GuiTools::launchTool(const std::string& script)
{
    mWindow->deinit(true);  // free framebuffer

    system("sudo chmod 666 /dev/tty1");

    std::string cmd = "/bin/bash \"" + script + "\" 2>&1 > /dev/tty1";
    system(cmd.c_str());

    system("setterm -clear all > /dev/tty1");

    mWindow->init(true);    // restore ES
}

bool GuiTools::input(InputConfig* config, Input input)
{
    if (input.value != 0 && config->isMappedTo("b", input))
    {
        GuiComponent* top = mWindow->peekGui(); // get the top-most GUI
        if (top != this)
        {
            // If a submenu is on top, close it
            mWindow->removeGui(top);
            delete top;
            return true;
        }
        else
        {
            // If at root Tools menu, exit Tools
            delete this;
            return true;
        }
    }

    return GuiComponent::input(config, input);
}

void GuiTools::render(const Transform4x4f& parentTrans)
{
    GuiComponent::render(parentTrans);
}


