#pragma once

#include <string>
#include <map>
#include "utils/StringUtil.h"

class EsLocale
{
public:
	static const std::string getText(const std::string text);

	static void setLanguage(const std::string lang);
	static const std::string getLanguage() { return mCurrentLanguage; }

private:
	static void checkLocalisationLoaded();
	static std::map<std::string, std::string> mItems;
	static std::string mCurrentLanguage;
	static bool mCurrentLanguageLoaded;
};

#if defined(_WIN32)
	#define UNICODE_CHARTYPE wchar_t*
	#define UNICODE_CHARS(x) L ## x
	#define UNICODE_STRING(x) Utils::String::convertFromWideString(L ## x)
#else

	#define UNICODE_CHARTYPE char*
	#define UNICODE_CHARS(x) x
	#define UNICODE_STRING(x) x
#endif // _WIN32

#define _T(x) EsLocale::getText(x)
