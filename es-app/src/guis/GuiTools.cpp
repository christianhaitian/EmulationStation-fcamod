#include "guis/GuiTools.h"
#include "utils/FileSystemUtil.h"
#include "Log.h"
#include <cstdlib>
#include <unistd.h>

GuiTools::GuiTools(Window* window)
    : GuiComponent(window), mMenu(window, "TOOLS")
{
    std::string toolPath = "/opt/system/";

    auto files = Utils::FileSystem::getDirContent(toolPath);
    for (auto& file : Utils::FileSystem::getDirContent(toolPath)) {
        if (Utils::FileSystem::isRegularFile(file) &&
            access(file.c_str(), X_OK) == 0) 
        {
            std::string filename = Utils::FileSystem::getFileName(file);
            mMenu.addEntry(filename, false, [file, this] {
                runSystemCommand(file);
            });
        }
    }
    mMenu.addEntry("BACK", false, [this] {
        delete this;   // pops GuiTools off the stack
    });
    addChild(&mMenu);
    setSize(mMenu.getSize());
}

void GuiTools::runSystemCommand(const std::string& cmd)
{
    int ret = system(cmd.c_str());
    if (ret != 0) {
        LOG(LogError) << "Failed to run tool: " << cmd;
    }
}
