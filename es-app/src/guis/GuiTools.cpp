#include "AudioManager.h"
#include "VolumeControl.h"
#include "guis/GuiTools.h"
#include "components/MenuComponent.h"
#include "Window.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "platform.h"

#include <fstream>
#include <algorithm>

// ------------------- SubMenuWrapper -------------------
class SubMenuWrapper : public GuiComponent
{
public:
    SubMenuWrapper(Window* window, MenuComponent* menu)
        : GuiComponent(window), mMenu(menu)
    {
        addChild(menu);
    }

    void close()
    {
        mWindow->removeGui(this);
    }

    bool input(InputConfig* config, Input input) override
    {
        if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
        {
            close();
            return true;
        }
        return GuiComponent::input(config, input);
    }

private:
    MenuComponent* mMenu;
};

// ------------------- GuiTools -------------------
GuiTools::GuiTools(Window* window)
    : GuiComponent(window), mMenu(window, _("OPTIONS"))
{
    addChild(&mMenu);

    addScriptsToMenu(mMenu, "/opt/system");

    // Root BACK button
    mMenu.addButton(_("BACK"), "back", [this] { delete this; });

    Vector2f screenSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
    Vector2f menuSize = mMenu.getSize();

    if (screenSize.x() >= 1024 && screenSize.y() >= 600)
    {
        Vector3f pos;
        pos[0] = (screenSize.x() - menuSize.x()) / 2.0f;
        pos[1] = (screenSize.y() - menuSize.y()) / 2.0f;
        pos[2] = 0;
        mMenu.setPosition(pos);
    }
}

GuiTools::~GuiTools()
{
}

// ------------------- addScriptsToMenu -------------------
bool GuiTools::addScriptsToMenu(MenuComponent& menu, const std::string& folderPath)
{
    auto items = Utils::FileSystem::getDirContent(folderPath);
    std::vector<std::pair<std::string, SubMenuWrapper*> > folderWrappers;
    std::vector<std::pair<std::string, std::string> > scripts;

    for (auto& item : items)
    {
        std::string name = Utils::FileSystem::getFileName(item);
        std::string fullPath = item;

        if (Utils::FileSystem::isDirectory(fullPath))
        {
            std::string folderDisplayName = _U("\uF07B ") + name;
            MenuComponent* subMenu = new MenuComponent(mWindow, name);
            bool hasItems = addScriptsToMenu(*subMenu, fullPath);

            if (hasItems)
            {
                SubMenuWrapper* wrapper = new SubMenuWrapper(mWindow, subMenu);

                Vector2f screenSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
			    Vector2f menuSize = subMenu->getSize();

			    if (screenSize.x() >= 1024 && screenSize.y() >= 600)
			    {
			        Vector3f pos;
			        pos[0] = (screenSize.x() - menuSize.x()) / 2.0f;
			        pos[1] = (screenSize.y() - menuSize.y()) / 2.0f;
			        pos[2] = 0;
			        subMenu->setPosition(pos);
    			}

                // On-screen BACK button closes submenu
                subMenu->addButton(_("BACK"), "back", [wrapper] {
                    wrapper->close();
                });

                folderWrappers.push_back(std::make_pair(folderDisplayName, wrapper));
            }
            else
            {
                delete subMenu;
            }
        }
        else if (Utils::String::toLower(Utils::FileSystem::getExtension(name)) == ".sh")
        {
            std::string displayName = Utils::FileSystem::getStem(name);

            // .name sidecar
            std::string nameFile = fullPath + ".name";
            if (Utils::FileSystem::exists(nameFile))
            {
                std::ifstream f(nameFile.c_str());
                if (f)
                    std::getline(f, displayName);
            }
            else
            {
                // First 10 lines: look for "# NAME:"
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
                }
            }
            std::string scriptDisplayName = _U("\uF013 ") + displayName;
            scripts.push_back(std::make_pair(scriptDisplayName, fullPath));
        }
    }

    // Sort case-insensitive
    std::sort(folderWrappers.begin(), folderWrappers.end(),
        [](const std::pair<std::string, SubMenuWrapper*>& a,
           const std::pair<std::string, SubMenuWrapper*>& b) {
            return Utils::String::toLower(a.first) < Utils::String::toLower(b.first);
        });

    std::sort(scripts.begin(), scripts.end(),
        [](const std::pair<std::string, std::string>& a,
           const std::pair<std::string, std::string>& b) {
            return Utils::String::toLower(a.first) < Utils::String::toLower(b.first);
        });

    // Add folder entries
    for (size_t i = 0; i < folderWrappers.size(); ++i)
    {
        const std::string folderName = folderWrappers[i].first;
        SubMenuWrapper* wrapper = folderWrappers[i].second;

        menu.addEntry(folderName, true, [this, wrapper] {
            mWindow->pushGui(wrapper);
        }, "");
    }

    // Add script entries
    for (size_t i = 0; i < scripts.size(); ++i)
    {
        const std::string displayName = scripts[i].first;
        const std::string path = scripts[i].second;

        menu.addEntry(displayName, false, [this, path] {
            launchTool(path);
        }, "");
    }

    return (!folderWrappers.empty() || !scripts.empty());
}

// ------------------- launchTool -------------------
void GuiTools::launchTool(const std::string& script)
{
    AudioManager::getInstance()->deinit();
    VolumeControl::getInstance()->deinit();
    // Hide ES temporarily
    mWindow->deinit(true);

    system("sudo chmod 666 /dev/tty1");
    std::string cmd = "/bin/bash \"" + script + "\" 2>&1 > /dev/tty1";
    int ret = system(cmd.c_str());
    (void)ret;

    // Clear framebuffer in case script left garbage
    system("setterm -clear all > /dev/tty1");

    // Restore ES
    mWindow->init(true);
    VolumeControl::getInstance()->init();
    AudioManager::getInstance()->init();
}

// ------------------- input -------------------
bool GuiTools::input(InputConfig* config, Input input)
{
    if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
    {
        GuiComponent* top = mWindow->peekGui();
        if (top != this)
        {
            mWindow->removeGui(top);
            return true;
        }
        else
        {
            delete this;
            return true;
        }
    }
    return GuiComponent::input(config, input);
}

// ------------------- render -------------------
void GuiTools::render(const Transform4x4f& parentTrans)
{
    GuiComponent::render(parentTrans);
}
