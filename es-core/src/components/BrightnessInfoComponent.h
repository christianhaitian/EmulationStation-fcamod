#pragma once

#include <mutex>
#include "GuiComponent.h"

class ComponentGrid;
class NinePatchComponent;
class TextComponent;
class Window;

class BrightnessInfoComponent : public GuiComponent
{
public:
	BrightnessInfoComponent(Window* window, bool actionLine = true);
	~BrightnessInfoComponent();

	void render(const Transform4x4f& parentTrans) override;
	void update(int deltaTime) override;

	void reset() { mBrightness = 1; }

private:
	NinePatchComponent* mFrame;
	TextComponent*		mLabel;

	int mBrightness;

	int mCheckTime;
	int mDisplayTime;
};
