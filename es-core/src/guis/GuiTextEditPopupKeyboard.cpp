#include "guis/GuiTextEditPopupKeyboard.h"
#include "components/MenuComponent.h"
#include "Log.h"
#include "utils/StringUtil.h"

std::vector<std::vector<UNICODE_CHARTYPE>> kbFrench {
	{ _L("&"), _L("é"), _L("\""), _L("'"), _L("("), _L("#"), _L("è"), _L("!"), _L("ç"), _L("à"), _L(")"), _L("-") },
	{ _L("1"), _L("2"), _L("3"), _L("4"), _L("5"), _L("6"), _L("7"), _L("8"), _L("9"), _L("0"), _L("@"), _L("_") },
	/*
	{ _L("à"), _L("ä"), _L("ë"), _L("ì"), _L("ï"), _L("ò"), _L("ö"), _L("ü"), _L("\\"), _L("|"), _L("§"), _L("°") },
	{ _L("á"), _L("â"), _L("ê"), _L("í"), _L("î"), _L("ó"), _L("ô"), _L("ú",  "û"), _L("ñ"), _L("¡"), _L("¿") },
	*/
	{ _L("a"), _L("z"), _L("e"), _L("r"), _L("t"), _L("y"), _L("u"), _L("i"), _L("o"), _L("p"), _L("^"), _L("$") },
	{ _L("A"), _L("Z"), _L("E"), _L("R"), _L("T"), _L("Y"), _L("U"), _L("I"), _L("O"), _L("P"), _L("¨"), _L("*") },

	{ _L("q"), _L("s"), _L("d"), _L("f"), _L("g"), _L("h"), _L("j"), _L("k"), _L("l"), _L("m"), _L("ù"), _L("`") },
	{ _L("Q"), _L("S"), _L("D"), _L("F"), _L("G"), _L("H"), _L("J"), _L("K"), _L("L"), _L("M"), _L("%"), _L("£") },

	 //SHIFT key at position 0
	{ _L("SHIFT"), _L("<"), _L("w"), _L("x"), _L("c"), _L("v"), _L("b"), _L("n"), _L(","), _L(";"), _L(":"), _L("=") },
	{ _L("SHIFT"), _L(">"), _L("W"), _L("X"), _L("C"), _L("V"), _L("B"), _L("N"), _L("?"), _L("."), _L("/"), _L("+") }
};

std::vector<std::vector<UNICODE_CHARTYPE>> kbUs {

	{ _L("1"), _L("2"), _L("3"), _L("4"), _L("5"), _L("6"), _L("7"), _L("8"), _L("9"), _L("0"), _L("_"), _L("+") },
	{ _L("!"), _L("@"), _L("#"), _L("$"), _L("%"), _L("^"), _L("&"), _L("*"), _L("("), _L(")"), _L("-"), _L("=") },
/*
	{ _L("à"), _L("ä"), _L("è"), _L("ë"), _L("ì"), _L("ï"), _L("ò"), _L("ö"), _L("ù"), _L("ü"), _L("¨"), _L("¿") },
	{ _L("á"), _L("â"), _L("é"), _L("ê"), _L("í"), _L("î"), _L("ó"), _L("ô"), _L("ú"), _L("û"), _L("ñ"), _L("¡") },
	*/
	{ _L("q"), _L("w"), _L("e"), _L("r"), _L("t"), _L("y"), _L("u"), _L("i"), _L("o"), _L("p"), _L("{"), _L("}") },
	{ _L("Q"), _L("W"), _L("E"), _L("R"), _L("T"), _L("Y"), _L("U"), _L("I"), _L("O"), _L("P"), _L("["), _L("]") },

	{ _L("a"), _L("s"), _L("d"), _L("f"), _L("g"), _L("h"), _L("j"), _L("k"), _L("l"), _L(";"), _L("\""), _L("|") },
	{ _L("A"), _L("S"), _L("D"), _L("F"), _L("G"), _L("H"), _L("J"), _L("K"), _L("L"), _L(":"), _L("'"), _L("\\") },

	{ _L("SHIFT"), _L("~"), _L("z"), _L("x"), _L("c"), _L("v"), _L("b"), _L("n"), _L("m"), _L(","), _L("."), _L("?") },
	{ _L("SHIFT"), _L("`"), _L("Z"), _L("X"), _L("C"), _L("V"), _L("B"), _L("N"), _L("M"), _L("<"), _L(">"), _L("/") },
};

GuiTextEditPopupKeyboard::GuiTextEditPopupKeyboard(Window* window, const std::string& title, const std::string& initValue, 
	const std::function<void(const std::string&)>& okCallback, bool multiLine, const char* acceptBtnText)
	: GuiComponent(window), mBackground(window, ":/frame.png"), mGrid(window, Vector2i(1, 7)), mMultiLine(multiLine)
{
	auto theme = ThemeData::getMenuTheme();

	mBackground.setImagePath(theme->Background.path); // ":/frame.png"
	mBackground.setCenterColor(theme->Background.color);
	mBackground.setEdgeColor(theme->Background.color);

	addChild(&mBackground);
	addChild(&mGrid);

	mTitle = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(title), ThemeData::getMenuTheme()->Title.font, ThemeData::getMenuTheme()->Title.color, ALIGN_CENTER);

	mText = std::make_shared<TextEditComponent>(mWindow);
	mText->setValue(initValue);

	if(!multiLine)
		mText->setCursor(initValue.size());

	// Header
	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);

	// Text edit add
	mGrid.setEntry(mText, Vector2i(0, 1), true, false, Vector2i(1, 1));

	std::vector< std::vector< std::shared_ptr<ButtonComponent> > > buttonList;

	// Keyboard
	// Case for if multiline is enabled, then don't create the keyboard.
	if (!mMultiLine) 
	{		
		std::vector<std::vector<UNICODE_CHARTYPE>> &layout = kbUs;

		if (EsLocale::getLanguage() == "fr")
			layout = kbFrench;

		for (unsigned int i = 0; i < layout.size() / 2; i++)
		{			
			std::vector<std::shared_ptr<ButtonComponent>> buttons;
			for (unsigned int j = 0; j < layout[2 * i].size(); j++)
			{				
#ifdef WIN32
				std::wstring toConvert = layout[2 * i][j];
				std::string atj = Utils::String::convertFromWideString(toConvert);

				toConvert = layout[2 * i + 1][j];
				std::string atjs = Utils::String::convertFromWideString(toConvert);
#else
				std::string atj = layout[2 * i][j];
				std::string atjs = layout[2 * i + 1][j];
				
#endif
				
				if (atj == "SHIFT")
				{
					// Special case for shift key
					mShiftButton = std::make_shared<ButtonComponent>(mWindow, "SHIFT", _("SHIFTS FOR UPPER,LOWER, AND SPECIAL"), [this] {
						shiftKeys();
					});
					buttons.push_back(mShiftButton);
				}
				else 
					buttons.push_back(makeButton(atj, atjs));
			}
			buttonList.push_back(buttons);
		}
	}
	
	const float gridWidth = Renderer::getScreenWidth() * 0.85f;
	mKeyboardGrid = makeMultiDimButtonGrid(mWindow, buttonList, gridWidth - 20);
	mGrid.setEntry(mKeyboardGrid, Vector2i(0, 2), true, false);

	// Accept/Cancel buttons
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _(acceptBtnText), _(acceptBtnText), [this, okCallback] { okCallback(mText->getValue()); delete this; }));

	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("SPACE"), _("SPACE"), [this] {
		mText->startEditing();
		mText->textInput(" ");
		mText->stopEditing();
	}));

	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("DELETE"), _("DELETE A CHAR"), [this] {
		mText->startEditing();
		mText->textInput("\b");
		mText->stopEditing();
	}));

	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CANCEL"), _("discard changes"), [this] { delete this; }));

	mButtons = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtons, Vector2i(0, 3), true, false);

	// Determine size from text size
	float textHeight = mText->getFont()->getHeight();
	if (multiLine)
		textHeight *= 6;
	mText->setSize(gridWidth - 40, textHeight);

	// If multiline, set all diminsions back to default, else draw size for keyboard.
	if (mMultiLine)
	{
		setSize(Renderer::getScreenWidth() * 0.5f, mTitle->getFont()->getHeight() + textHeight + mKeyboardGrid->getSize().y() + 40);
		setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
	}
	else
	{
		setSize(gridWidth, mTitle->getFont()->getHeight() + textHeight + 40 + mKeyboardGrid->getSize().y() + mButtons->getSize().y());
		setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
	}
}

void GuiTextEditPopupKeyboard::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mText->setSize(mSize.x() - 40, mText->getSize().y());

	float fullHeight = mTitle->getFont()->getHeight() + mText->getSize().y() + mKeyboardGrid->getSize().y() + mButtons->getSize().y();

	// update grid
	mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / fullHeight);
	mGrid.setRowHeightPerc(1, mText->getSize().y() / fullHeight);
	mGrid.setRowHeightPerc(2, mKeyboardGrid->getSize().y() / fullHeight);
	mGrid.setRowHeightPerc(3, mButtons->getSize().y() / fullHeight);

	mGrid.setSize(mSize);

	mKeyboardGrid->onSizeChanged();
	/*
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mText->setSize(mSize.x() - 40, mText->getSize().y());

	// update grid
	mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y());
	mGrid.setRowHeightPerc(2, mKeyboardGrid->getSize().y() / mSize.y());

	mGrid.setSize(mSize);*/
}

bool GuiTextEditPopupKeyboard::input(InputConfig* config, Input input)
{
	if (GuiComponent::input(config, input))
		return true;

	// pressing back when not text editing closes us
	if (config->isMappedTo("b", input) && input.value)
	{
		delete this;
		return true;
	}

	// For deleting a chara (Left Top Button)
	if (config->isMappedTo("lefttop", input) && input.value) {
		mText->startEditing();
		mText->textInput("\b");
		mText->stopEditing();
	}

	// For Adding a space (Right Top Button)
	if (config->isMappedTo("righttop", input) && input.value) {
		mText->startEditing();
		mText->textInput(" ");
	}

	// For Shifting (X)
	if (config->isMappedTo("x", input) && input.value) {
		if (mShift) mShift = false;
		else mShift = true;
		shiftKeys();
	}

	

	return false;
}

void GuiTextEditPopupKeyboard::update(int deltatime) {

}

std::shared_ptr<ButtonComponent> GuiTextEditPopupKeyboard::makeButton(const std::string& key, const std::string& shiftedKey)
{
	std::shared_ptr<ButtonComponent> button = std::make_shared<ButtonComponent>(mWindow, key, key, [this, key, shiftedKey] {
		mText->startEditing();
		if (mShift)
			mText->textInput(shiftedKey.c_str());
		else
			mText->textInput(key.c_str());
		mText->stopEditing();
	}, false);
	KeyboardButton kb(button, key, shiftedKey);
	keyboardButtons.push_back(kb);
	return button;
}

// Shifts the keys when user hits the shift button.
void GuiTextEditPopupKeyboard::shiftKeys() 
{
	mShift = !mShift;

	if (mShift)
		mShiftButton->setColorShift(0xFF0000FF);
	else
		mShiftButton->removeColorShift();

	for (auto & kb : keyboardButtons)
	{
		const std::string& text = mShift ? kb.shiftedKey : kb.key;
		kb.button->setText(text, text, false);
	}
}

std::vector<HelpPrompt> GuiTextEditPopupKeyboard::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("SHIFT")));
	prompts.push_back(HelpPrompt("b", _("BACK")));
	prompts.push_back(HelpPrompt("r", _("SPACE")));
	prompts.push_back(HelpPrompt("l", _("DELETE")));
	return prompts;
}

