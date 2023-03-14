#include "BrightnessInfoComponent.h"
#include "ThemeData.h"
#include "PowerSaver.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "EsLocale.h"
#include "BrightnessControl.h"
#include "Window.h"
#include "DisplayPanelControl.h"

#define PADDING_PX			(Renderer::getScreenWidth()*0.006)
#define PADDING_BAR			(Renderer::isSmallScreen() ? Renderer::getScreenWidth()*0.02 : Renderer::getScreenWidth()*0.006)

#define VISIBLE_TIME		2650
#define FADE_TIME			350
#define BASEOPACITY			200
#define CHECKBRIGHTNESSDELAY	40

BrightnessInfoComponent::BrightnessInfoComponent(Window* window, bool actionLine)
	: GuiComponent(window)
{
	mDisplayTime = -1;
	mBrightness = -1;
	mCheckTime = 0;

	auto theme = ThemeData::getMenuTheme();	

	auto font = theme->TextSmall.font;
	if (Renderer::isSmallScreen())
		font = theme->Text.font;

	Vector2f fullSize(
		2 * PADDING_PX + font->sizeText("100%").x(),
		2 * PADDING_PX + Renderer::getScreenHeight() * 0.20f);

	fullSize.y() = fullSize.x() * 2.5f;

	setSize(fullSize);

	mFrame = new NinePatchComponent(window);
	mFrame->setImagePath(theme->Background.path);
	mFrame->setEdgeColor(theme->Background.color);
	mFrame->setCenterColor(theme->Background.centerColor);
	mFrame->setCornerSize(theme->Background.cornerSize);
	mFrame->fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	addChild(mFrame);


	mLabel = new TextComponent(mWindow, "", font, theme->Text.color, ALIGN_CENTER);

	int h = font->sizeText("100%").y() + PADDING_PX;
	mLabel->setPosition(0, fullSize.y() - h);
	mLabel->setSize(fullSize.x(), h);
	addChild(mLabel);


	// FCA TopRight
	float posX = Renderer::getScreenWidth() - (Renderer::getScreenWidth() * 0.12f);
	float posY = Renderer::getScreenHeight() * 0.04f;

	setPosition(posX, posY, 0);
	setOpacity(BASEOPACITY);
}

BrightnessInfoComponent::~BrightnessInfoComponent()
{
	delete mLabel;
	delete mFrame;
}

void BrightnessInfoComponent::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mDisplayTime >= 0)
	{
		mDisplayTime += deltaTime;
		if (mDisplayTime > VISIBLE_TIME + FADE_TIME)
		{
			mDisplayTime = -1;

			if (isVisible())
			{
				setVisible(false);
				PowerSaver::resume();
			}
		}
	}

	mCheckTime += deltaTime;
	if (mCheckTime < CHECKBRIGHTNESSDELAY)
		return;

	mCheckTime = 0;

	int brightness = DisplayPanelControl::getInstance()->getBrightnessLevel();
	if (brightness != mBrightness)
	{
		bool firstTime = (mBrightness < 0);

		mBrightness = brightness;

		if (mBrightness == 0)
			mLabel->setText("X");
		else
			mLabel->setText(std::to_string(mBrightness) + "%");

		if (!firstTime)
		{
			mDisplayTime = 0;

			if (!isVisible())
			{
				setVisible(true);
				PowerSaver::pause();
			}
		}
	}
}

void BrightnessInfoComponent::render(const Transform4x4f& parentTrans)
{
	if (!mVisible || mDisplayTime < 0)
		return;

	int opacity = BASEOPACITY - Math::max(0, (mDisplayTime - VISIBLE_TIME) * BASEOPACITY / FADE_TIME);
	setOpacity(opacity);

	GuiComponent::render(parentTrans);

	Transform4x4f trans = parentTrans * getTransform();
	Renderer::setMatrix(trans);

	float x = PADDING_PX + PADDING_BAR;
	float y = PADDING_PX * 2;
	float w = getSize().x() - 2 * PADDING_PX - 2 * PADDING_BAR;
	float h = getSize().y() - mLabel->getSize().y() - PADDING_PX - PADDING_PX;

	auto theme = ThemeData::getMenuTheme();

	Renderer::drawRect(x, y, w, h, (theme->Text.color & 0xFFFFFF00) | (opacity / 2));

	float px = (h*mBrightness) / 100;
	Renderer::drawRect(x, y + h - px, w, px, (theme->TextSmall.color & 0xFFFFFF00) | opacity);
}
