#include "DisplayPanelControl.h"

#include "Log.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "platform.h"
#include <cstring>


#define BACKLIGHT_BUFFER_SIZE 127
const char* BACKLIGHT_BRIGHTNESS_NAME = "/sys/class/backlight/backlight/brightness";
const char* BACKLIGHT_BRIGHTNESS_MAX_NAME = "/sys/class/backlight/backlight/max_brightness";
const char* ACTUAL_BRIGHTNESS_MAX_NAME = "/sys/devices/platform/backlight/backlight/backlight/actual_brightness";
const char* BRIGHTNESSCTL_PATH = "/usr/bin/brightnessctl";

const char* PANEL_DRM_TOOL_PATH = "/usr/local/bin/panel_drm_tool";

const char* GET_COMMAND_TEMPLATE_P1 = "sudo /usr/local/bin/panel_drm_tool list | grep -A 9 133 | grep ";
const char* GET_COMMAND_TEMPLATE_P2 = " | awk -F ' = ' '{print $2}'";
const char* SET_COMMAND_TEMPLATE = "sudo /usr/local/bin/panel_drm_tool set /dev/dri/card0 133 ";

std::weak_ptr<DisplayPanelControl> DisplayPanelControl::sInstance;


DisplayPanelControl::DisplayPanelControl()
{
	mExistPanelDrmTool = false;
	init();
	LOG(LogInfo) << "DisplayPanelControl::DisplayPanelControl() - Initialized, command '" << BRIGHTNESSCTL_PATH << "' exist: " << Utils::String::boolToString(mExistBrightnessctl);
	LOG(LogInfo) << "DisplayPanelControl::DisplayPanelControl() - Initialized, command '" << PANEL_DRM_TOOL_PATH << "' exist: " << Utils::String::boolToString(mExistPanelDrmTool);
}

DisplayPanelControl::~DisplayPanelControl()
{
	//set original volume levels for system
	//setVolume(originalVolume);

	deinit();
	LOG(LogInfo) << "DisplayPanelControl::DisplayPanelControl() - Deinitialized";
}

std::shared_ptr<DisplayPanelControl> & DisplayPanelControl::getInstance()
{
	//check if an DisplayPanelControl instance is already created, if not create one
	static std::shared_ptr<DisplayPanelControl> sharedInstance = sInstance.lock();
	if (sharedInstance == nullptr) {
		sharedInstance.reset(new DisplayPanelControl);
		sInstance = sharedInstance;
	}
	return sharedInstance;
}

void DisplayPanelControl::init()
{
	//initialize audio mixer interface
	mExistBrightnessctl = Utils::FileSystem::exists(BRIGHTNESSCTL_PATH);
	mExistPanelDrmTool = Utils::FileSystem::exists(PANEL_DRM_TOOL_PATH);
}

void DisplayPanelControl::deinit()
{
	mExistPanelDrmTool = false;
}

int DisplayPanelControl::checkValue(int value)
{
	int checked_value = 50;
	if (value < 1)
		return 1;

	if (value > 100)
		return 100;
	
	return value;
}

int DisplayPanelControl::getBrightness()
{
	if (mExistBrightnessctl)
		return std::atoi( getShOutput(std::string(BRIGHTNESSCTL_PATH).append(" g")).c_str() );
	else if (Utils::FileSystem::exists(ACTUAL_BRIGHTNESS_MAX_NAME))
		return std::atoi( getShOutput("cat " + *ACTUAL_BRIGHTNESS_MAX_NAME).c_str() );

	return 0;
}

int DisplayPanelControl::getMaxBrightness() const
{
	if (mExistBrightnessctl)
		return std::atoi( getShOutput(std::string(BRIGHTNESSCTL_PATH).append(" m")).c_str() );
	else if (Utils::FileSystem::exists(BACKLIGHT_BRIGHTNESS_MAX_NAME))
		return std::atoi( getShOutput("cat " + *BACKLIGHT_BRIGHTNESS_MAX_NAME).c_str() );

	return 0;
}

int DisplayPanelControl::getBrightnessLevel()
{
	if (mExistBrightnessctl)
		return std::atoi( getShOutput(std::string(BRIGHTNESSCTL_PATH).append(" -m | awk -F',|%' '{print $4}'")).c_str() );


	int value,
			fd,
			max = 255;
	char buffer[BACKLIGHT_BUFFER_SIZE + 1];
	ssize_t count;

	fd = open(BACKLIGHT_BRIGHTNESS_MAX_NAME, O_RDONLY);
	if (fd < 0)
		return false;

	memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

	count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
	if (count > 0)
		max = atoi(buffer);

	close(fd);

	if (max == 0)
		return 0;

	fd = open(BACKLIGHT_BRIGHTNESS_NAME, O_RDONLY);
	if (fd < 0)
		return false;

	memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

	count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
	if (count > 0)
		value = atoi(buffer);

	close(fd);

	return (uint32_t) ((value / (float)max * 100.0f) + 0.5f);
}

void DisplayPanelControl::setBrightnessLevel(int brightness_level)
{
	bool setted = false;
	if (mExistBrightnessctl)
		setted = executeSystemScript(std::string(BRIGHTNESSCTL_PATH).append(" s ").append(std::to_string(brightness_level)).append("% &"));

	if (!setted)
	{
		if (brightness_level < 5)
			brightness_level = 5;

		if (brightness_level > 100)
			brightness_level = 100;

		int fd,
				max = 255;
		char buffer[BACKLIGHT_BUFFER_SIZE + 1];
		ssize_t count;

		fd = open(BACKLIGHT_BRIGHTNESS_MAX_NAME, O_RDONLY);
		if (fd < 0)
			return;

		memset(buffer, 0, BACKLIGHT_BUFFER_SIZE + 1);

		count = read(fd, buffer, BACKLIGHT_BUFFER_SIZE);
		if (count > 0)
			max = atoi(buffer);

		close(fd);

		if (max == 0)
			return;

		fd = open(BACKLIGHT_BRIGHTNESS_NAME, O_WRONLY);
		if (fd < 0)
			return;

		float percent = (brightness_level / 100.0f * (float)max) + 0.5f;
		sprintf(buffer, "%d\n", (uint32_t)percent);

		count = write(fd, buffer, strlen(buffer));
		if (count < 0)
			LOG(LogError) << "Platform::saveBrightnessLevel failed";
		else
			setted = true;

		close(fd);
	}
}

std::string DisplayPanelControl::getCommnad(const std::string property)
{
	return std::string(GET_COMMAND_TEMPLATE_P1).append(property).append(GET_COMMAND_TEMPLATE_P2);
}

std::string DisplayPanelControl::setCommnad(const std::string property, int value)
{
	return std::string(SET_COMMAND_TEMPLATE).append(property).append(" ").append(std::to_string(value));
}

void DisplayPanelControl::setGammaLevel(int gammaLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript(setCommnad("brightness", checkValue(gammaLevel)));
}

int DisplayPanelControl::getGammaLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(getCommnad("brightness")).c_str() ));
}

void DisplayPanelControl::setContrastLevel(int contrastLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript(setCommnad("contrast", checkValue(contrastLevel)));
}

int DisplayPanelControl::getContrastLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(getCommnad("contrast")).c_str() ));
}

void DisplayPanelControl::setSaturationLevel(int saturationLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript(setCommnad("saturation", checkValue(saturationLevel)));
}

int DisplayPanelControl::getSaturationLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(getCommnad("saturation")).c_str() ));
}

void DisplayPanelControl::setHueLevel(int hueLevel)
{
	if (!mExistPanelDrmTool)
		return;

	executeSystemScript(setCommnad("hue", checkValue(hueLevel)));
}

int DisplayPanelControl::getHueLevel()
{
	if (!mExistPanelDrmTool)
		return 50;

	return checkValue(std::atoi( getShOutput(getCommnad("hue")).c_str() ));
}

void DisplayPanelControl::resetDisplayPanelSettings()
{
	if (!mExistPanelDrmTool)
		return;
	
	setGammaLevel(50);
	setContrastLevel(50);
	setSaturationLevel(50);
	setHueLevel(50);
}

bool DisplayPanelControl::isAvailable()
{
	return true;
}
