#include <string>
#include "BrightnessControl.h"

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

std::weak_ptr<BrightnessControl> BrightnessControl::sInstance;


BrightnessControl::BrightnessControl()
{
	mBrightnessctlExist = false;
	init();
	LOG(LogInfo) << "BrightnessControl::BrightnessControl() - Initialized, command 'brightnessctl' exist: " << Utils::String::boolToString(mBrightnessctlExist);
}

BrightnessControl::~BrightnessControl()
{
	//set original volume levels for system
	//setVolume(originalVolume);

	deinit();
	LOG(LogInfo) << "BrightnessControl::BrightnessControl() - Deinitialized";
}

std::shared_ptr<BrightnessControl> & BrightnessControl::getInstance()
{
	//check if an BrightnessControl instance is already created, if not create one
	static std::shared_ptr<BrightnessControl> sharedInstance = sInstance.lock();
	if (sharedInstance == nullptr) {
		sharedInstance.reset(new BrightnessControl);
		sInstance = sharedInstance;
	}
	return sharedInstance;
}

void BrightnessControl::init()
{
	//initialize audio mixer interface
	mBrightnessctlExist = Utils::FileSystem::exists(BRIGHTNESSCTL_PATH);
}

void BrightnessControl::deinit()
{
	mBrightnessctlExist = false;
}

int BrightnessControl::getBrightness()
{
	if (mBrightnessctlExist)
		return std::atoi( getShOutput(*BRIGHTNESSCTL_PATH + " g").c_str() );
	else if (Utils::FileSystem::exists(ACTUAL_BRIGHTNESS_MAX_NAME))
		return std::atoi( getShOutput("cat " + *ACTUAL_BRIGHTNESS_MAX_NAME).c_str() );

	return 0;
}

int BrightnessControl::getMaxBrightness() const
{
	if (mBrightnessctlExist)
		return std::atoi( getShOutput(*BRIGHTNESSCTL_PATH + " m").c_str() );
	else if (Utils::FileSystem::exists(BACKLIGHT_BRIGHTNESS_MAX_NAME))
		return std::atoi( getShOutput("cat " + *BACKLIGHT_BRIGHTNESS_MAX_NAME).c_str() );

	return 0;
}

int BrightnessControl::getBrightnessLevel()
{
	if (mBrightnessctlExist)
		return std::atoi( getShOutput(*BRIGHTNESSCTL_PATH + " -m | awk -F',|%' '{print $4}')").c_str() );

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

void BrightnessControl::setBrightnessLevel(int brightness_level)
{
	bool setted = false;
	if (mBrightnessctlExist)
		setted = executeSystemScript(*BRIGHTNESSCTL_PATH + " s " + std::to_string(brightness_level) + "% &");

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

bool BrightnessControl::isAvailable()
{
	return true;
}
