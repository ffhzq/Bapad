#include "pch.h"
#include "FormatConversion.h"

typedef unsigned long	UTF32;	// at least 32 bits
typedef unsigned short	UTF16;	// at least 16 bits
typedef unsigned char	UTF8;	// typically 8 bits

// Some fundamental constants 
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP			 (UTF32)0x0000FFFF
#define UNI_MAX_UTF16		 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32		 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32  (UTF32)0x0010FFFF

#define UNI_SUR_HIGH_START   (UTF32)0xD800
#define UNI_SUR_HIGH_END     (UTF32)0xDBFF
#define UNI_SUR_LOW_START    (UTF32)0xDC00
#define UNI_SUR_LOW_END      (UTF32)0xDFFF

#define SWAPWORD(val) (((WORD)(val) << 8) | ((WORD)(val) >> 8))


int AsciiToUTF16(Byte* asciiStr, size_t asciiLen, wchar_t* utf16Str, size_t& utf16Len)
{
	int len = min(utf16Len, asciiLen);

	MultiByteToWideChar(CP_ACP, 0, (CCHAR*)asciiStr, len, utf16Str, len);
	utf16Len = len;
	return len;
}

int UTF8ToUTF16(Byte* utf8Str, size_t utf8Len, wchar_t* utf16Str, size_t& utf16Len)
{

	size_t len = 0;
	char32_t ch32 = 0;
	WCHAR* utf16Start = utf16Str;
	BYTE* utf8Start = utf8Str;

	while (utf8Len > 0 && utf16Len > 0)
	{
		len = UTF8ToUTF32(utf8Str, utf8Len, ch32);

		// target is a character <= 0xffff
		if (ch32 < 0xfffe)
		{
			// make sure we don't represent anything in UTF16 surrogate range
			// (this helps protect against non-shortest forms)
			if (ch32 >= UNI_SUR_HIGH_START && ch32 <= UNI_SUR_LOW_END)
			{
				*utf16Str++ = UNI_REPLACEMENT_CHAR;
				utf16Len--;
			}
			else
			{
				*utf16Str++ = static_cast<Word>(ch32);
				utf16Len--;
			}
		}
		// FFFE and FFFF are illegal mid-stream
		else if (ch32 == 0xfffe || ch32 == 0xffff)
		{
			*utf16Str++ = UNI_REPLACEMENT_CHAR;
			utf16Len--;
		}
		// target is illegal Unicode value
		else if (ch32 > UNI_MAX_UTF16)
		{
			*utf16Str++ = UNI_REPLACEMENT_CHAR;
			utf16Len--;
		}
		// target is in range 0xffff - 0x10ffff
		else if (utf16Len >= 2)
		{
			ch32 -= 0x0010000;

			*utf16Str++ = (WORD)((ch32 >> 10) + UNI_SUR_HIGH_START);
			*utf16Str++ = (WORD)((ch32 & 0x3ff) + UNI_SUR_LOW_START);

			utf16Len -= 2;
		}
		else
		{
			// no room to store result
			break;
		}

		utf8Str += len;
		utf8Len -= len;
	}

	utf16Len = utf16Str - utf16Start;

	return utf8Str - utf8Start;
}

size_t UTF8ToUTF32(Byte* utf8Str, size_t utf8Len, char32_t & pch32)
{
	static ULONG nonshortest[] = { 0, 0x80, 0x800, 0x10000, 0xffffffff, 0xffffffff };
    size_t val32 = 0;
    size_t trailing = 0;

    if (utf8Str == 0 || utf8Len <= 0 || pch32 == 0)
        return 0;

    Byte ch = *utf8Str;

    const char ASCII_MAX = 0x80 - 1;
    if (ch <= ASCII_MAX)
    {
        pch32 = static_cast<wchar_t>(ch);
        return 1;
    }
	// LEAD-byte of 2-byte seq: 110xxxxx 10xxxxxx
	else if ((ch & 0xE0) == 0xC0)
	{
		trailing = 1;
		val32 = ch & 0x1F;
	}
	// LEAD-byte of 3-byte seq: 1110xxxx 10xxxxxx 10xxxxxx
	else if ((ch & 0xF0) == 0xE0)
	{
		trailing = 2;
		val32 = ch & 0x0F;
	}
	// LEAD-byte of 4-byte seq: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	else if ((ch & 0xF8) == 0xF0)
	{
		trailing = 3;
		val32 = ch & 0x07;
	}
	// ILLEGAL 5-byte seq: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	else if ((ch & 0xFC) == 0xF8)
	{
		// range-checking the UTF32 result will catch this
		trailing = 4;
		val32 = ch & 0x03;
	}
	// ILLEGAL 6-byte seq: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	else if ((ch & 0xFE) == 0xFC)
	{
		// range-checking the UTF32 result will catch this
		trailing = 5;
		val32 = ch & 0x01;
	}
	// ILLEGAL continuation (trailing) byte by itself
	else if ((ch & 0xC0) == 0x80)
	{
		pch32 = UNI_REPLACEMENT_CHAR;
		return 1;
	}
	// any other ILLEGAL form.
	else
	{
		pch32 = UNI_REPLACEMENT_CHAR;
		return 1;
	}

	size_t i = 0, len = 0;
	// process trailing bytes
	for (; i < trailing && len < utf8Len; i++)
	{
		ch = *utf8Str++;

		// Valid trail-byte: 10xxxxxx
		if ((ch & 0xC0) == 0x80)
		{
			val32 = (val32 << 6) + (ch & 0x7f);
			len++;
		}
		// Anything else is an error
		else
		{
			pch32 = UNI_REPLACEMENT_CHAR;
			return len;
		}
	}

	// did we 
	if (val32 < nonshortest[trailing] || i != trailing)
		pch32 = UNI_REPLACEMENT_CHAR;
	else
		pch32 = val32;

	return len;
}

int CopyUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t & destLen)
{
	size_t len = min(destLen, srcLen);
	memcpy(dest, src, len * sizeof(WCHAR));

	destLen = len;
	return len * sizeof(WCHAR);
}

int SwapUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t & destLen)
{
	size_t len = min(destLen, srcLen);

	for (size_t i = 0; i < len; i++)
		dest[i] = SWAPWORD(src[i]);

	destLen = len;
	return len * sizeof(WCHAR);
}

int UTF16ToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len)
{
	WCHAR* utf16start = utf16Str;
	ULONG* utf32start = utf32Str;

	while (utf16Len > 0 && utf32Len > 0)
	{
		ULONG ch = *utf16Str;

		// first of a surrogate pair?
		if (ch >= UNI_SUR_HIGH_START && ch < UNI_SUR_HIGH_END && utf16Len >= 2)
		{
			ULONG ch2 = *(utf16Str + 1);

			// valid trailing surrogate unit?
			if (ch2 >= UNI_SUR_LOW_START && ch < UNI_SUR_LOW_END)
			{
				ch = ((ch - UNI_SUR_HIGH_START) << 10) +
					((ch2 - UNI_SUR_LOW_START) + 0x00010000);

				utf16Str++;
				utf16Len--;
			}
			// illegal character
			else
			{
				ch = UNI_REPLACEMENT_CHAR;
			}
		}

		*utf32Str++ = ch;
		utf32Len--;

		utf16Str++;
		utf16Len--;
	}

	utf32Len = utf32Str - utf32start;
	return utf16Str - utf16start;
}

int UTF16BEToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len)
{
	WCHAR* utf16start = utf16Str;
	ULONG* utf32start = utf32Str;

	while (utf16Len > 0 && utf32Len > 0)
	{
		ULONG ch = SWAPWORD(*utf16Str);

		// first of a surrogate pair?
		if (ch >= UNI_SUR_HIGH_START && ch < UNI_SUR_HIGH_END && utf16Len >= 2)
		{
			ULONG ch2 = SWAPWORD(*(utf16Str + 1));

			// valid trailing surrogate unit?
			if (ch2 >= UNI_SUR_LOW_START && ch < UNI_SUR_LOW_END)
			{
				ch = ((ch - UNI_SUR_HIGH_START) << 10) +
					((ch2 - UNI_SUR_LOW_START) + 0x00010000);

				utf16Str++;
				utf16Len--;
			}
			// illegal character
			else
			{
				ch = UNI_REPLACEMENT_CHAR;
			}
		}

		*utf32Str++ = ch;
		utf32Len--;

		utf16Str++;
		utf16Len--;
	}

	utf32Len = utf32Str - utf32start;
	return utf16Str - utf16start;
}