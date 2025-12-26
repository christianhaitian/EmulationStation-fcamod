#pragma once
#ifndef ES_CORE_UTILS_STRING_UTIL_H
#define ES_CORE_UTILS_STRING_UTIL_H

#include <string>
#include <vector>

namespace Utils
{
	namespace String
	{
		typedef std::vector<std::string> stringVector;

		unsigned int chars2Unicode      (const std::string& _string, size_t& _cursor);
		std::string  unicode2Chars      (const unsigned int _unicode);
		size_t       nextCursor         (const std::string& _string, const size_t _cursor);
		size_t       prevCursor         (const std::string& _string, const size_t _cursor);
		size_t       moveCursor         (const std::string& _string, const size_t _cursor, const int _amount);
		std::string  toLower            (const std::string& _string);
		std::string  toUpper            (const std::string& _string);
		std::string  trim               (const std::string& _string);
		std::string  replace            (const std::string& _string, const std::string& _replace, const std::string& _with);
		bool         startsWith         (const std::string& _string, const std::string& _start);
		bool         endsWith           (const std::string& _string, const std::string& _end);
		std::string  removeParenthesis  (const std::string& _string);
		stringVector commaStringToVector(const std::string& _string);
		std::string  vectorToCommaString(stringVector _vector);
		std::string  format             (const char* _string, ...);
		std::string  scramble           (const std::string& _input, const std::string& key);
		std::vector<std::string> split	(const std::string& s, char seperator);	
		std::vector<std::string> splitAny(const std::string& s, const std::string& seperator);
		std::string join(const std::vector<std::string>& items, std::string separator);
        int			compareIgnoreCase(const std::string& name1, const std::string& name2);
		const std::string boolToString(bool value, bool uppercase = false);

		// for Korean text input
		const std::vector<const char*> KOREAN_CHOSUNG_LIST = { "ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ" };
		const std::vector<const char*> KOREAN_JOONGSUNG_LIST = { "ㅏ", "ㅐ", "ㅑ", "ㅒ", "ㅓ", "ㅔ", "ㅕ", "ㅖ", "ㅗ", "ㅘ", "ㅙ", "ㅚ", "ㅛ", "ㅜ", "ㅝ", "ㅞ", "ㅟ", "ㅠ", "ㅡ", "ㅢ", "ㅣ" };
		const std::vector<const char*> KOREAN_JONGSUNG_LIST = { " ", "ㄱ", "ㄲ", "ㄳ", "ㄴ", "ㄵ", "ㄶ", "ㄷ", "ㄹ", "ㄺ", "ㄻ", "ㄼ", "ㄽ", "ㄾ", "ㄿ", "ㅀ", "ㅁ", "ㅂ", "ㅄ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ" };
		const std::vector<const char*> KOREAN_GYEOP_BATCHIM_LIST = { "ㄳ", "ㄵ", "ㄶ", "ㄺ", "ㄻ", "ㄼ", "ㄽ", "ㄾ", "ㄿ", "ㅀ", "ㅄ" };
		const std::vector<const char*> KOREAN_GYEOP_BATCHIM_COMBINATIONS = { "ㄱㅅ", "ㄴㅈ", "ㄴㅎ", "ㄹㄱ", "ㄹㅁ", "ㄹㅂ", "ㄹㅅ", "ㄹㅌ", "ㄹㅍ", "ㄹㅎ", "ㅂㅅ" };
		const std::vector<const char*> KOREAN_IJUNG_MOEUM_LIST = { "ㅘ", "ㅙ", "ㅚ", "ㅝ", "ㅞ", "ㅟ", "ㅢ" };
		const std::vector<const char*> KOREAN_IJUNG_MOEUM_COMBINATIONS = { "ㅗㅏ", "ㅗㅐ", "ㅗㅣ", "ㅜㅓ", "ㅜㅔ", "ㅜㅣ", "ㅡㅣ" };
		enum KoreanCharType : unsigned int
		{
			NONE = 0,
			JAEUM = 1,
			MOEUM = 2,
		};
		bool            isKorean(const unsigned int uni);
		bool            isKorean(const char* _string, bool checkFirstChar = true);
		KoreanCharType  getKoreanCharType(const char* _string);
		bool            splitHangulSyllable(const char* input, const char** chosung, const char** joongsung = nullptr, const char** jongsung = nullptr);
		void            koreanTextInput(const char* text, std::string& componentText, unsigned int& componentCursor);
		// end Korean text input

#if defined(_WIN32)
		const std::string convertFromWideString(const std::wstring wstring);
		const std::wstring convertToWideString(const std::string string);
#endif

		// Pinyin-aware substring match: allow ASCII/pinyin query to hit Chinese titles.
		bool containsIgnoreCasePinyin(const std::string& s, const std::string& what);
	} // String::

} // Utils::

#endif // ES_CORE_UTILS_STRING_UTIL_H
