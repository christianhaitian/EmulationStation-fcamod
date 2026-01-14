#include <string>
#pragma once
#ifndef ES_CORE_BRIGHTNESS_CONTROL_H
#define ES_CORE_BRIGHTNESS_CONTROL_H

#include <memory>
#include <unistd.h>
#include <fcntl.h>

/*!
Singleton pattern. Call getInstance() to get an object.
*/
class BrightnessControl
{
	static std::weak_ptr<BrightnessControl> sInstance;

	BrightnessControl();	

public:
	static std::shared_ptr<BrightnessControl> & getInstance();

	void init();
	void deinit();

	bool isAvailable();

	int getBrightness();
	int getMaxBrightness() const;
	int getBrightnessLevel();
	void setBrightnessLevel(int brightness_level);

	~BrightnessControl();
private:
	bool mBrightnessctlExist;

};

#endif // ES_CORE_BRIGHTNESS_CONTROL_H
