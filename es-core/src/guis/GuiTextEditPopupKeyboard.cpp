#include "guis/GuiTextEditPopupKeyboard.h"
#include "components/MenuComponent.h"
#include "Log.h"
#include "utils/StringUtil.h"

std::vector<std::vector<UNICODE_CHARTYPE>> kbFrench {
	{ UNICODE_CHARS("&"), UNICODE_CHARS("é"), UNICODE_CHARS("\""), UNICODE_CHARS("'"), UNICODE_CHARS("("), UNICODE_CHARS("#"), UNICODE_CHARS("è"), UNICODE_CHARS("!"), UNICODE_CHARS("ç"), UNICODE_CHARS("à"), UNICODE_CHARS(")"), UNICODE_CHARS("-") },
	{ UNICODE_CHARS("1"), UNICODE_CHARS("2"), UNICODE_CHARS("3"), UNICODE_CHARS("4"), UNICODE_CHARS("5"), UNICODE_CHARS("6"), UNICODE_CHARS("7"), UNICODE_CHARS("8"), UNICODE_CHARS("9"), UNICODE_CHARS("0"), UNICODE_CHARS("@"), UNICODE_CHARS("_") },
	/*
	{ UNICODE_CHARS("à"), UNICODE_CHARS("ä"), UNICODE_CHARS("ë"), UNICODE_CHARS("ì"), UNICODE_CHARS("ï"), UNICODE_CHARS("ò"), UNICODE_CHARS("ö"), UNICODE_CHARS("ü"), UNICODE_CHARS("\\"), UNICODE_CHARS("|"), UNICODE_CHARS("§"), UNICODE_CHARS("°") },
	{ UNICODE_CHARS("á"), UNICODE_CHARS("â"), UNICODE_CHARS("ê"), UNICODE_CHARS("í"), UNICODE_CHARS("î"), UNICODE_CHARS("ó"), UNICODE_CHARS("ô"), UNICODE_CHARS("ú",  "û"), UNICODE_CHARS("ñ"), UNICODE_CHARS("¡"), UNICODE_CHARS("¿") },
	*/
	{ UNICODE_CHARS("a"), UNICODE_CHARS("z"), UNICODE_CHARS("e"), UNICODE_CHARS("r"), UNICODE_CHARS("t"), UNICODE_CHARS("y"), UNICODE_CHARS("u"), UNICODE_CHARS("i"), UNICODE_CHARS("o"), UNICODE_CHARS("p"), UNICODE_CHARS("^"), UNICODE_CHARS("$") },
	{ UNICODE_CHARS("A"), UNICODE_CHARS("Z"), UNICODE_CHARS("E"), UNICODE_CHARS("R"), UNICODE_CHARS("T"), UNICODE_CHARS("Y"), UNICODE_CHARS("U"), UNICODE_CHARS("I"), UNICODE_CHARS("O"), UNICODE_CHARS("P"), UNICODE_CHARS("¨"), UNICODE_CHARS("*") },

	{ UNICODE_CHARS("q"), UNICODE_CHARS("s"), UNICODE_CHARS("d"), UNICODE_CHARS("f"), UNICODE_CHARS("g"), UNICODE_CHARS("h"), UNICODE_CHARS("j"), UNICODE_CHARS("k"), UNICODE_CHARS("l"), UNICODE_CHARS("m"), UNICODE_CHARS("ù"), UNICODE_CHARS("`") },
	{ UNICODE_CHARS("Q"), UNICODE_CHARS("S"), UNICODE_CHARS("D"), UNICODE_CHARS("F"), UNICODE_CHARS("G"), UNICODE_CHARS("H"), UNICODE_CHARS("J"), UNICODE_CHARS("K"), UNICODE_CHARS("L"), UNICODE_CHARS("M"), UNICODE_CHARS("%"), UNICODE_CHARS("£") },

	 //SHIFT key at position 0
	{ UNICODE_CHARS("SHIFT"), UNICODE_CHARS("<"), UNICODE_CHARS("w"), UNICODE_CHARS("x"), UNICODE_CHARS("c"), UNICODE_CHARS("v"), UNICODE_CHARS("b"), UNICODE_CHARS("n"), UNICODE_CHARS(","), UNICODE_CHARS(";"), UNICODE_CHARS(":"), UNICODE_CHARS("=") },
	{ UNICODE_CHARS("SHIFT"), UNICODE_CHARS(">"), UNICODE_CHARS("W"), UNICODE_CHARS("X"), UNICODE_CHARS("C"), UNICODE_CHARS("V"), UNICODE_CHARS("B"), UNICODE_CHARS("N"), UNICODE_CHARS("?"), UNICODE_CHARS("."), UNICODE_CHARS("/"), UNICODE_CHARS("+") }
};

std::vector<std::vector<UNICODE_CHARTYPE>> kbUs {

	{ UNICODE_CHARS("1"), UNICODE_CHARS("2"), UNICODE_CHARS("3"), UNICODE_CHARS("4"), UNICODE_CHARS("5"), UNICODE_CHARS("6"), UNICODE_CHARS("7"), UNICODE_CHARS("8"), UNICODE_CHARS("9"), UNICODE_CHARS("0"), UNICODE_CHARS("_"), UNICODE_CHARS("+") },
	{ UNICODE_CHARS("!"), UNICODE_CHARS("@"), UNICODE_CHARS("#"), UNICODE_CHARS("$"), UNICODE_CHARS("%"), UNICODE_CHARS("^"), UNICODE_CHARS("&"), UNICODE_CHARS("*"), UNICODE_CHARS("("), UNICODE_CHARS(")"), UNICODE_CHARS("-"), UNICODE_CHARS("=") },
/*
	{ UNICODE_CHARS("à"), UNICODE_CHARS("ä"), UNICODE_CHARS("è"), UNICODE_CHARS("ë"), UNICODE_CHARS("ì"), UNICODE_CHARS("ï"), UNICODE_CHARS("ò"), UNICODE_CHARS("ö"), UNICODE_CHARS("ù"), UNICODE_CHARS("ü"), UNICODE_CHARS("¨"), UNICODE_CHARS("¿") },
	{ UNICODE_CHARS("á"), UNICODE_CHARS("â"), UNICODE_CHARS("é"), UNICODE_CHARS("ê"), UNICODE_CHARS("í"), UNICODE_CHARS("î"), UNICODE_CHARS("ó"), UNICODE_CHARS("ô"), UNICODE_CHARS("ú"), UNICODE_CHARS("û"), UNICODE_CHARS("ñ"), UNICODE_CHARS("¡") },
	*/
	{ UNICODE_CHARS("q"), UNICODE_CHARS("w"), UNICODE_CHARS("e"), UNICODE_CHARS("r"), UNICODE_CHARS("t"), UNICODE_CHARS("y"), UNICODE_CHARS("u"), UNICODE_CHARS("i"), UNICODE_CHARS("o"), UNICODE_CHARS("p"), UNICODE_CHARS("{"), UNICODE_CHARS("}") },
	{ UNICODE_CHARS("Q"), UNICODE_CHARS("W"), UNICODE_CHARS("E"), UNICODE_CHARS("R"), UNICODE_CHARS("T"), UNICODE_CHARS("Y"), UNICODE_CHARS("U"), UNICODE_CHARS("I"), UNICODE_CHARS("O"), UNICODE_CHARS("P"), UNICODE_CHARS("["), UNICODE_CHARS("]") },

	{ UNICODE_CHARS("a"), UNICODE_CHARS("s"), UNICODE_CHARS("d"), UNICODE_CHARS("f"), UNICODE_CHARS("g"), UNICODE_CHARS("h"), UNICODE_CHARS("j"), UNICODE_CHARS("k"), UNICODE_CHARS("l"), UNICODE_CHARS(";"), UNICODE_CHARS("\""), UNICODE_CHARS("|") },
	{ UNICODE_CHARS("A"), UNICODE_CHARS("S"), UNICODE_CHARS("D"), UNICODE_CHARS("F"), UNICODE_CHARS("G"), UNICODE_CHARS("H"), UNICODE_CHARS("J"), UNICODE_CHARS("K"), UNICODE_CHARS("L"), UNICODE_CHARS(":"), UNICODE_CHARS("'"), UNICODE_CHARS("\\") },

	{ UNICODE_CHARS("SHIFT"), UNICODE_CHARS("~"), UNICODE_CHARS("z"), UNICODE_CHARS("x"), UNICODE_CHARS("c"), UNICODE_CHARS("v"), UNICODE_CHARS("b"), UNICODE_CHARS("n"), UNICODE_CHARS("m"), UNICODE_CHARS(","), UNICODE_CHARS("."), UNICODE_CHARS("?") },
	{ UNICODE_CHARS("SHIFT"), UNICODE_CHARS("`"), UNICODE_CHARS("Z"), UNICODE_CHARS("X"), UNICODE_CHARS("C"), UNICODE_CHARS("V"), UNICODE_CHARS("B"), UNICODE_CHARS("N"), UNICODE_CHARS("M"), UNICODE_CHARS("<"), UNICODE_CHARS(">"), UNICODE_CHARS("/") },
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
					mShiftButton = std::make_shared<ButtonComponent>(mWindow, "SHIFT", _T("SHIFTS FOR UPPER,LOWER, AND SPECIAL"), [this] {
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
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _T(acceptBtnText), _T(acceptBtnText), [this, okCallback] { okCallback(mText->getValue()); delete this; }));

	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _T("SPACE"), _T("SPACE"), [this] {
		mText->startEditing();
		mText->textInput(" ");
		mText->stopEditing();
	}));

	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _T("DELETE"), _T("DELETE A CHAR"), [this] {
		mText->startEditing();
		mText->textInput("\b");
		mText->stopEditing();
	}));

	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _T("CANCEL"), _T("discard changes"), [this] { delete this; }));

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
	prompts.push_back(HelpPrompt("x", _T("SHIFT")));
	prompts.push_back(HelpPrompt("b", _T("BACK")));
	prompts.push_back(HelpPrompt("r", _T("SPACE")));
	prompts.push_back(HelpPrompt("l", _T("DELETE")));
	return prompts;
}

