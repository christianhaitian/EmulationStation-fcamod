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
	#define _L(x) L ## x
	#define _U(x) Utils::String::convertFromWideString(L ## x)
	
	#define _T(x) EsLocale::getText(x)
#else

	#define UNICODE_CHARTYPE char*
	#define _L(x) x
	#define _U(x) x

	#define _T(x) EsLocale::getText(x)
#endif // _WIN32

