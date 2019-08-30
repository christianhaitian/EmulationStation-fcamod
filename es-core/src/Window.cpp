#include "Window.h"

#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "resources/Font.h"
#include "resources/TextureResource.h"
#include "InputManager.h"
#include "Log.h"
#include "Scripting.h"
#include <algorithm>
#include <iomanip>
#include <SDL_events.h>

Window::Window() : mNormalizeNextUpdate(false), mFrameTimeElapsed(0), mFrameCountElapsed(0), mAverageDeltaTime(10),
	mAllowSleep(true), mSleeping(false), mTimeSinceLastInput(0), mScreenSaver(NULL), mRenderScreenSaver(false), mInfoPopup(NULL)
{
	mHelp = new HelpComponent(this);
	mBackgroundOverlay = new ImageComponent(this);	

	mSplash = NULL;	
}

Window::~Window()
{
	delete mBackgroundOverlay;

	// delete all our GUIs
	while(peekGui())
		delete peekGui();

	delete mHelp;
}

void Window::pushGui(GuiComponent* gui)
{
	if (mGuiStack.size() > 0)
	{
		auto& top = mGuiStack.back();
		top->topWindow(false);
	}
	mGuiStack.push_back(gui);
	gui->updateHelpPrompts();
}

void Window::removeGui(GuiComponent* gui)
{
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
	{
		if(*i == gui)
		{
			i = mGuiStack.erase(i);

			if(i == mGuiStack.cend() && mGuiStack.size()) // we just popped the stack and the stack is not empty
			{
				mGuiStack.back()->updateHelpPrompts();
				mGuiStack.back()->topWindow(true);
			}

			return;
		}
	}
}

GuiComponent* Window::peekGui()
{
	if(mGuiStack.size() == 0)
		return NULL;

	return mGuiStack.back();
}

bool Window::init(bool initRenderer)
{
	LOG(LogInfo) << "Window::init";
	
	if (initRenderer)
	{
		if (!Renderer::init())
		{
			LOG(LogError) << "Renderer failed to initialize!";
			return false;
		}

		InputManager::getInstance()->init();
	}
	else
		Renderer::activateWindow();
		
	ResourceManager::getInstance()->reloadAll();

	//keep a reference to the default fonts, so they don't keep getting destroyed/recreated
	if(mDefaultFonts.empty())
	{
		mDefaultFonts.push_back(Font::get(FONT_SIZE_SMALL));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_MEDIUM));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_LARGE));
	}

	mBackgroundOverlay->setImage(":/scroll_gradient.png");
	mBackgroundOverlay->setResize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	// update our help because font sizes probably changed
	if (peekGui())
		peekGui()->updateHelpPrompts();
	
	return true;
}

void Window::deinit(bool deinitRenderer)
{
	// Hide all GUI elements on uninitialisation - this disable
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
	{
		(*i)->onHide();
	}

	if (deinitRenderer)
		InputManager::getInstance()->deinit();

	TextureResource::resetCache();
	ResourceManager::getInstance()->unloadAll();

	if (deinitRenderer)
		Renderer::deinit();
}

void Window::textInput(const char* text)
{
	if(peekGui())
		peekGui()->textInput(text);
}

void Window::input(InputConfig* config, Input input)
{
	if (mScreenSaver) {
		if(mScreenSaver->isScreenSaverActive() && Settings::getInstance()->getBool("ScreenSaverControls") &&
		   (Settings::getInstance()->getString("ScreenSaverBehavior") == "random video"))
		{
			if(mScreenSaver->getCurrentGame() != NULL && (config->isMappedLike("right", input) || config->isMappedTo("start", input) || config->isMappedTo("select", input)))
			{
				if(config->isMappedLike("right", input) || config->isMappedTo("select", input))
				{
					if (input.value != 0) {
						// handle screensaver control
						mScreenSaver->nextVideo();
					}
					return;
				}
				else if(config->isMappedTo("start", input) && input.value != 0)
				{
					// launch game!
					cancelScreenSaver();
					mScreenSaver->launchGame();
					// to force handling the wake up process
					mSleeping = true;
				}
			}
		}
	}

	if(mSleeping)
	{
		// wake up
		mTimeSinceLastInput = 0;
		cancelScreenSaver();
		mSleeping = false;
		onWake();
		return;
	}

	mTimeSinceLastInput = 0;
	if (cancelScreenSaver())
		return;

	if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_g && SDL_GetModState() & KMOD_LCTRL/* && Settings::getInstance()->getBool("Debug")*/)
	{
		// toggle debug grid with Ctrl-G
		Settings::getInstance()->setBool("DebugGrid", !Settings::getInstance()->getBool("DebugGrid"));
	}
	else if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_t && SDL_GetModState() & KMOD_LCTRL/* && Settings::getInstance()->getBool("Debug")*/)
	{
		// toggle TextComponent debug view with Ctrl-T
		Settings::getInstance()->setBool("DebugText", !Settings::getInstance()->getBool("DebugText"));
	}
	else if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_i && SDL_GetModState() & KMOD_LCTRL/* && Settings::getInstance()->getBool("Debug")*/)
	{
		// toggle TextComponent debug view with Ctrl-I
		Settings::getInstance()->setBool("DebugImage", !Settings::getInstance()->getBool("DebugImage"));
	}
	else
	{
		if (peekGui())
		{
			this->peekGui()->input(config, input); // this is where the majority of inputs will be consumed: the GuiComponent Stack
		}
	}
}

void Window::update(int deltaTime)
{	
	if(mNormalizeNextUpdate)
	{
		mNormalizeNextUpdate = false;
		if(deltaTime > mAverageDeltaTime)
			deltaTime = mAverageDeltaTime;
	}

	mFrameTimeElapsed += deltaTime;
	mFrameCountElapsed++;
	if(mFrameTimeElapsed > 500)
	{
		mAverageDeltaTime = mFrameTimeElapsed / mFrameCountElapsed;

		if(Settings::getInstance()->getBool("DrawFramerate"))
		{
			std::stringstream ss;

			// fps
			ss << std::fixed << std::setprecision(1) << (1000.0f * (float)mFrameCountElapsed / (float)mFrameTimeElapsed) << "fps, ";
			ss << std::fixed << std::setprecision(2) << ((float)mFrameTimeElapsed / (float)mFrameCountElapsed) << "ms";

			// vram
			float textureVramUsageMb = TextureResource::getTotalMemUsage() / 1000.0f / 1000.0f;
			float textureTotalUsageMb = TextureResource::getTotalTextureSize() / 1000.0f / 1000.0f;
			float fontVramUsageMb = Font::getTotalMemUsage() / 1000.0f / 1000.0f;

			ss << "\nFont VRAM: " << fontVramUsageMb << " Tex VRAM: " << textureVramUsageMb <<
				  " Tex Max: " << textureTotalUsageMb;
			mFrameDataText = std::unique_ptr<TextCache>(mDefaultFonts.at(1)->buildTextCache(ss.str(), 50.f, 50.f, 0xFF00FFFF));
		}

		mFrameTimeElapsed = 0;
		mFrameCountElapsed = 0;
	}

	mTimeSinceLastInput += deltaTime;

	if(peekGui())
		peekGui()->update(deltaTime);

	// Update the screensaver
	if (mScreenSaver)
		mScreenSaver->update(deltaTime);
}

void Window::render()
{
	Transform4x4f transform = Transform4x4f::Identity();

	mRenderedHelpPrompts = false;

	// draw only bottom and top of GuiStack (if they are different)
	if(mGuiStack.size())
	{
		auto& bottom = mGuiStack.front();
		auto& top = mGuiStack.back();

		bottom->render(transform);
		if(bottom != top)
		{
			mBackgroundOverlay->render(transform);
			top->render(transform);
		}
	}

	if(!mRenderedHelpPrompts)
		mHelp->render(transform);

	if(Settings::getInstance()->getBool("DrawFramerate") && mFrameDataText)
	{
		Renderer::setMatrix(Transform4x4f::Identity());
		mDefaultFonts.at(1)->renderTextCache(mFrameDataText.get());
	}

	unsigned int screensaverTime = (unsigned int)Settings::getInstance()->getInt("ScreenSaverTime");
	if(mTimeSinceLastInput >= screensaverTime && screensaverTime != 0)
		startScreenSaver();

	// Always call the screensaver render function regardless of whether the screensaver is active
	// or not because it may perform a fade on transition
	renderScreenSaver();

	if(!mRenderScreenSaver && mInfoPopup)
	{
		mInfoPopup->render(transform);
	}

	if(mTimeSinceLastInput >= screensaverTime && screensaverTime != 0)
	{
		if (!isProcessing() && mAllowSleep && (!mScreenSaver || mScreenSaver->allowSleep()))
		{
			// go to sleep
			if (mSleeping == false) {
				mSleeping = true;
				onSleep();
			}
		}
	}
}

void Window::normalizeNextUpdate()
{
	mNormalizeNextUpdate = true;
}

bool Window::getAllowSleep()
{
	return mAllowSleep;
}

void Window::setAllowSleep(bool sleep)
{
	mAllowSleep = sleep;
}

void Window::endRenderLoadingScreen()
{
	mSplash = NULL;
	mCustomSplash = "";
}

void Window::renderLoadingScreen(std::string text, float percent, unsigned char opacity)
{	
	if (mSplash == NULL)
		mSplash = TextureResource::get(":/splash.svg", false, true, false, false);

	Transform4x4f trans = Transform4x4f::Identity();
	Renderer::setMatrix(trans);
	Renderer::drawRect(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x00000000 | opacity);
	
	if (percent >= 0)
	{
		float baseHeight = 0.04f;

		float w = Renderer::getScreenWidth() / 2;
		float h = Renderer::getScreenHeight() * baseHeight;

		float x = Renderer::getScreenWidth() / 2 - w / 2;
		float y = Renderer::getScreenHeight() - (Renderer::getScreenHeight() * 3 * baseHeight);

		Renderer::drawRect(x, y, w, h, 0x25252500 | opacity);
		Renderer::drawGradientRect(x, y, (w*percent), h, 0x006C9E00 | opacity, 0x003E5C00 | opacity); // 0xFFFFFFFF
	}
	
	ImageComponent splash(this, true);
	splash.setResize(Renderer::getScreenWidth() * 0.4f, 0.0f);	

	if (mSplash != NULL)
		splash.setImage(mSplash);
	else
		splash.setImage(":/splash.svg");

	splash.setPosition((Renderer::getScreenWidth() - splash.getSize().x()) / 2, (Renderer::getScreenHeight() - splash.getSize().y()) / 2 * 0.7f);
	splash.render(trans);
	
	auto& font = mDefaultFonts.at(1);
	TextCache* cache = font->buildTextCache(text, 0, 0, 0x65656500 | opacity);

	float x = Math::round((Renderer::getScreenWidth() - cache->metrics.size.x()) / 2.0f);
	float y = Math::round(Renderer::getScreenHeight() * 0.78f); // 35
	trans = trans.translate(Vector3f(x, y, 0.0f));
	Renderer::setMatrix(trans);
 	font->renderTextCache(cache);
	delete cache;

	Renderer::swapBuffers();
	
#if defined(_WIN32)
	// Avoid Window Freezing on Windows
	SDL_Event event;
	while (SDL_PollEvent(&event));
#endif
}

void Window::loadCustomImageLoadingScreen(std::string imagePath, std::string customText)
{
	if (!Utils::FileSystem::exists(imagePath))
		return;

	if (Settings::getInstance()->getBool("HideWindow"))
		return;

	if (mSplash != NULL)
		endRenderLoadingScreen();

	mSplash = TextureResource::get(imagePath, false, true, false, false, MaxSizeInfo(Renderer::getScreenWidth() * 0.60f, Renderer::getScreenHeight() * 0.60f));
	mCustomSplash = customText;
	
	std::shared_ptr<ResourceManager>& rm = ResourceManager::getInstance();
	rm->removeReloadable(mSplash);
}

void Window::renderGameLoadingScreen(float opacity, bool swapBuffers)
{
	if (mSplash == NULL)
		mSplash = TextureResource::get(":/splash.svg", false, true, false, false);

	Transform4x4f trans = Transform4x4f::Identity();
	Renderer::setMatrix(trans);
	Renderer::drawRect(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x00000000 | (unsigned char)(opacity * 255));

	ImageComponent splash(this, true);

	if (mSplash != NULL)
		splash.setImage(mSplash);
	else
		splash.setImage(":/splash.svg");

	splash.setOrigin(0.5, 0.5);
	splash.setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() * 0.835f / 2.0f);
	splash.setMaxSize(Renderer::getScreenWidth() * 0.60f, Renderer::getScreenHeight() * 0.60f);

	if (!mCustomSplash.empty())
		splash.setColorShift(0xFFFFFF00 | (unsigned char)(opacity * 210));
	else
		splash.setColorShift(0xFFFFFF00 | (unsigned char)(opacity * 255));

	splash.render(trans);

	auto& font = mDefaultFonts.at(1);
	font->reload(); // Ensure font is loaded

	TextCache* cache = font->buildTextCache(mCustomSplash.empty() ? _("Loading...") : mCustomSplash, 0, 0, 0x65656500 | (unsigned char)(opacity * 255));

	float x = Math::round((Renderer::getScreenWidth() - cache->metrics.size.x()) / 2.0f);
	float y = Math::round(Renderer::getScreenHeight() * 0.835f);
	trans = trans.translate(Vector3f(x, y, 0.0f));
	Renderer::setMatrix(trans);
	font->renderTextCache(cache);
	delete cache;

	if (swapBuffers)
		Renderer::swapBuffers();
}


void Window::renderHelpPromptsEarly()
{
	mHelp->render(Transform4x4f::Identity());
	mRenderedHelpPrompts = true;
}

void Window::setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style)
{
	mHelp->clearPrompts();
	mHelp->setStyle(style);

	std::vector<HelpPrompt> addPrompts;

	std::map<std::string, bool> inputSeenMap;
	std::map<std::string, int> mappedToSeenMap;
	for(auto it = prompts.cbegin(); it != prompts.cend(); it++)
	{
		// only add it if the same icon hasn't already been added
		if(inputSeenMap.emplace(it->first, true).second)
		{
			// this symbol hasn't been seen yet, what about the action name?
			auto mappedTo = mappedToSeenMap.find(it->second);
			if(mappedTo != mappedToSeenMap.cend())
			{
				// yes, it has!

				// can we combine? (dpad only)
				if((it->first == "up/down" && addPrompts.at(mappedTo->second).first != "left/right") ||
					(it->first == "left/right" && addPrompts.at(mappedTo->second).first != "up/down"))
				{
					// yes!
					addPrompts.at(mappedTo->second).first = "up/down/left/right";
					// don't need to add this to addPrompts since we just merged
				}else{
					// no, we can't combine!
					addPrompts.push_back(*it);
				}
			}else{
				// no, it hasn't!
				mappedToSeenMap.emplace(it->second, (int)addPrompts.size());
				addPrompts.push_back(*it);
			}
		}
	}

	// sort prompts so it goes [dpad_all] [dpad_u/d] [dpad_l/r] [a/b/x/y/l/r] [start/select]
	std::sort(addPrompts.begin(), addPrompts.end(), [](const HelpPrompt& a, const HelpPrompt& b) -> bool {

		static const char* map[] = {
			"up/down/left/right",
			"up/down",
			"left/right",
			"a", "b", "x", "y", "l", "r",
			"start", "select",
			NULL
		};

		int i = 0;
		int aVal = 0;
		int bVal = 0;
		while(map[i] != NULL)
		{
			if(a.first == map[i])
				aVal = i;
			if(b.first == map[i])
				bVal = i;
			i++;
		}

		return aVal > bVal;
	});

	mHelp->setPrompts(addPrompts);
}


void Window::onSleep()
{
	Scripting::fireEvent("sleep");
}

void Window::onWake()
{
	Scripting::fireEvent("wake");
}

bool Window::isProcessing()
{
	return count_if(mGuiStack.cbegin(), mGuiStack.cend(), [](GuiComponent* c) { return c->isProcessing(); }) > 0;
}

void Window::startScreenSaver()
{
	if (mScreenSaver && !mRenderScreenSaver)
	{
		// Tell the GUI components the screensaver is starting
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverActivate();

		mScreenSaver->startScreenSaver();
		mRenderScreenSaver = true;
	}
}

bool Window::cancelScreenSaver()
{
	if (mScreenSaver && mRenderScreenSaver)
	{
		mScreenSaver->stopScreenSaver();
		mRenderScreenSaver = false;
		mScreenSaver->resetCounts();

		// Tell the GUI components the screensaver has stopped
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverDeactivate();

		return true;
	}

	return false;
}

void Window::renderScreenSaver()
{
	if (mScreenSaver)
		mScreenSaver->renderScreenSaver();
}
