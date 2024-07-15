#include "pch.h"
#include "FormatConversion.h"
#include "MyException.h"
#include <stdexcept>

size_t UTF8ToUTF32(UTF8* utf8Str, size_t utf8Len, UTF32* pch32)
{
	static ULONG nonshortest[] = { 0, 0x80, 0x800, 0x10000, 0xffffffff, 0xffffffff };
	UTF32 val32 = 0;
	int trailing = 0;
	UTF8 ch = *utf8Str;
	utf8Str++;

	if (utf8Str == 0 || utf8Len <= 0 || pch32 == 0)
		return 0;
	const char ASCII_MAX = 0x80 - 1;
	if (ch <= ASCII_MAX)
	{
		*pch32 = ch;
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
		*pch32 = UNI_REPLACEMENT_CHAR;
		return 1;
	}
	// any other ILLEGAL form.
	else
	{
		*pch32 = UNI_REPLACEMENT_CHAR;
		return 1;
	}

	size_t i = 0, len = 1;
	// process trailing bytes
	for (; i < trailing && len < utf8Len; i++)
	{
		ch = *utf8Str++;

		// Valid trail-byte: 10xxxxxx
		if ((ch&0xC0)==0x80)
		{
			val32 = (val32 << 6) + (ch & 0x7f);
			len++;
		}
		// Anything else is an error
		else
		{
			*pch32 = UNI_REPLACEMENT_CHAR;
			return len;
		}
	}

	// did we 
	if (val32 < nonshortest[trailing] || i != trailing)
		*pch32 = UNI_REPLACEMENT_CHAR;
	else
		*pch32 = val32;

	if (utf8Len!=0&&len == 0)
	{
		throw MyException("Invalid UTF-8 sequence", -1, MyException::ConversionType::FromUtf8ToUtf32);
	}

	return len;
}
size_t UTF32ToUTF8(UTF32 ch32, UTF8* utf8Str, size_t &utf8Len)
{
	size_t len = 0;

	// validate parameters
	if (utf8Str == 0 || utf8Len == 0)
		return 0;

	// ASCII is the easiest
	if (ch32 < 0x80)
	{
		*utf8Str = (UTF8)ch32;
		return 1;
	}

	// make sure we have a legal utf32 char
	if (ch32 > UNI_MAX_LEGAL_UTF32)
		ch32 = UNI_REPLACEMENT_CHAR;

	// cannot encode the surrogate range
	if (ch32 >= UNI_SUR_HIGH_START && ch32 <= UNI_SUR_LOW_END)
		ch32 = UNI_REPLACEMENT_CHAR;

	// 2-byte sequence
	if (ch32 < 0x800 && utf8Len >= 2)
	{
		*utf8Str++ = (UTF8)((ch32 >> 6) | 0xC0);
		*utf8Str++ = (UTF8)((ch32 & 0x3f) | 0x80);
		len = 2;
	}
	// 3-byte sequence
	else if (ch32 < 0x10000 && utf8Len >= 3)
	{
		*utf8Str++ = (UTF8)((ch32 >> 12) | 0xE0);
		*utf8Str++ = (UTF8)((ch32 >> 6) & 0x3f | 0x80);
		*utf8Str++ = (UTF8)((ch32 & 0x3f) | 0x80);
		len = 3;
	}
	// 4-byte sequence
	else if (ch32 <= UNI_MAX_LEGAL_UTF32 && utf8Len >= 4)
	{
		*utf8Str++ = (UTF8)((ch32 >> 18) | 0xF0);
		*utf8Str++ = (UTF8)((ch32 >> 12) & 0x3f | 0x80);
		*utf8Str++ = (UTF8)((ch32 >> 6) & 0x3f | 0x80);
		*utf8Str++ = (UTF8)((ch32 & 0x3f) | 0x80);
		len = 4;
	}

	// 5/6 byte sequences never occur because we limit using UNI_MAX_LEGAL_UTF32
	if (len == 0) {
		throw MyException("Invalid UTF-32 character", -1, MyException::ConversionType::FromUtf32ToUtf8);
	}
	return len;
}



size_t AsciiToUTF16(UTF8* asciiStr, size_t asciiLen, UTF16* utf16Str, size_t& utf16Len)
{
	size_t len = min(utf16Len, asciiLen);
	if (len > static_cast<size_t>((std::numeric_limits<int>::max)()))
	{
		throw std::overflow_error(
			"Input ascii string too long, size_t doesn't fit into int.");
	}
	int lenInt = static_cast<int>(len);
	int retVal=MultiByteToWideChar(CP_ACP, 0, (CCHAR*)asciiStr, lenInt, (WCHAR *)utf16Str, lenInt);
	utf16Len = len;
	if (lenInt!=0 && retVal == 0)
	{
		        throw MyException("Invalid ASCII character", GetLastError(), MyException::ConversionType::FromAsciiToUtf16);
	}
	return len;
}

size_t UTF8ToUTF16(UTF8* utf8Str, size_t utf8Len, UTF16* utf16Str, size_t& utf16Len)
{
	UTF16* utf16start = utf16Str;
	UTF8* utf8start = utf8Str;

	size_t len;
	size_t tmp16len;
	UTF32  ch32;

	while (utf8Len > 0 && utf16Len > 0)
	{
		// convert to utf-32
		len = UTF8ToUTF32(utf8Str, utf8Len, &ch32);
		utf8Str += len;
		utf8Len -= len;

		// convert to utf-16
		tmp16len = utf16Len;
		len = UTF32ToUTF16(&ch32, 1, utf16Str, tmp16len);
		utf16Str += len;
		utf16Len -= len;
	}

	utf16Len = utf16Str - utf16start;
	if (utf8Str - utf8start!=0&&utf16Len == 0)
	{
		        throw MyException("Invalid UTF-8 character", -1, MyException::ConversionType::FromUtf8ToUtf16);
    
	}

	return utf8Str - utf8start;
}




size_t CopyUTF16(UTF16* src, size_t srcLen, UTF16* dest, size_t & destLen)
{
	size_t len = min(destLen, srcLen);
	memcpy(dest, src, len * sizeof(UTF16));

	destLen = len;
	if (srcLen != 0 && len == 0)
	{
		throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::CopyUtf16);
	}
	return len;
}

size_t SwapUTF16(UTF16* src, size_t srcLen, UTF16* dest, size_t & destLen)
{
	size_t len = min(destLen, srcLen);

	for (size_t i = 0; i < len; i++)
		dest[i] = SwapWord(src[i]);

	destLen = len;
	if (srcLen!=0 && len == 0)
	{
		throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::SwapUtf16);
	}
	return len;
}

size_t UTF16ToUTF32(UTF16* utf16Str, size_t utf16Len, UTF32* utf32Str, size_t& utf32Len)
{
	UTF16* utf16start = utf16Str;
	UTF32* utf32start = utf32Str;

	while (utf16Len > 0 && utf32Len > 0)
	{
		UTF32 charTemp1 = *utf16Str;

		// first of a surrogate pair?
		if (charTemp1 >= UNI_SUR_HIGH_START && charTemp1 < UNI_SUR_HIGH_END && utf16Len >= 2)
		{
			UTF32 charTemp2 = *(utf16Str + 1);

			// valid trailing surrogate unit?
			if (charTemp2 >= UNI_SUR_LOW_START && charTemp1 < UNI_SUR_LOW_END)
			{
				charTemp1 = ((charTemp1 - UNI_SUR_HIGH_START) << 10) +
					((charTemp2 - UNI_SUR_LOW_START) + 0x00010000);

				utf16Str++;
				utf16Len--;
			}
			// illegal character
			else
			{
				charTemp1 = UNI_REPLACEMENT_CHAR;
			}
		}

		*utf32Str++ = charTemp1;
		utf32Len--;

		utf16Str++;
		utf16Len--;
	}
	
	utf32Len = utf32Str - utf32start;
	if (utf16Len!=0 && utf32Len == 0)
	{
		        throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::FromUtf16ToUtf32);
    
	}
	return utf16Str - utf16start;
}

size_t UTF32ToUTF16(UTF32* utf32Str, size_t utf32Len, UTF16* utf16Str, size_t &utf16Len)
{
	UTF16* utf16start = utf16Str;
	UTF32* utf32start = utf32Str;

	while (utf32Len > 0 && utf16Len > 0)
	{
		UTF32 ch32 = *utf32Str++;
		utf32Len--;

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
				*utf16Str++ = (WORD)ch32;
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
	}

	utf16Len = utf16Str - utf16start;
	if (utf32Str - utf32start !=0&&utf16Len == 0)
	{
		                throw MyException("Invalid UTF-32 character", -1, MyException::ConversionType::FromUtf32ToUtf16);
    
    
	}
	return utf32Str - utf32start;
}

size_t UTF16BEToUTF32(UTF16* utf16Str, size_t utf16Len, UTF32* utf32Str, size_t& utf32Len)
{
	UTF16* utf16start = utf16Str;
	UTF32* utf32start = utf32Str;

	while (utf16Len > 0 && utf32Len > 0)
	{
		ULONG ch = SwapWord(*utf16Str);

		// first of a surrogate pair?
		if (ch >= UNI_SUR_HIGH_START && ch < UNI_SUR_HIGH_END && utf16Len >= 2)
		{
			ULONG ch2 = SwapWord(*(utf16Str + 1));

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
	if (utf16Str - utf16start!=0&&utf32Len == 0)
	{
		                throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::FromUtf16BEToUtf32);
    
    
	}
	return utf16Str - utf16start;
}

size_t UTF16ToUTF8(UTF16* utf16Str, size_t utf16Len, UTF8* utf8Str, size_t& utf8Len)
{
	UTF16* utf16start = utf16Str;
	UTF8* utf8start = utf8Str;
	size_t  len;
	UTF32	ch32;
	size_t	ch32len;

	while (utf16Len > 0 && utf8Len > 0)
	{
		// convert to utf-32
		ch32len = 1;
		len = UTF16ToUTF32(utf16Str, utf16Len, &ch32, ch32len);
		utf16Str += len;
		utf16Len -= len;

		// convert to utf-8
		len = UTF32ToUTF8(ch32, utf8Str, utf8Len);
		utf8Str += len;
		utf8Len -= len;
	}

	utf8Len = utf8Str - utf8start;
	if (utf16Str - utf16start!=0&&utf8Len == 0)
	{
		        throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::FromUtf16ToUtf8);
    
	}
	return utf16Str - utf16start;
}



size_t UTF16ToAscii(UTF16* utf16Str, size_t utf16Len, UTF8* asciiStr, size_t& asciiLen) {

	size_t len = min(utf16Len, asciiLen);
	if (len > static_cast<size_t>((std::numeric_limits<int>::max)()))
	{
		throw std::overflow_error(
			"Input UTF16 string too long, size_t doesn't fit into int.");
	}
	int lenInt = static_cast<int>(len);

	if (asciiLen > static_cast<size_t>((std::numeric_limits<int>::max)()))
	{
		throw std::overflow_error(
			"Input UTF16 string too long, size_t doesn't fit into int.");
	}
	int receiveLenInt = static_cast<int>(asciiLen);

	int retVal=WideCharToMultiByte(CP_ACP, 0, (LPCWCH)utf16Str, lenInt, (LPSTR)asciiStr, receiveLenInt, 0, 0);
	asciiLen = lenInt;
	if (retVal == 0)
	{
		throw MyException("Invalid UTF-16 character", GetLastError(), MyException::ConversionType::FromUtf16ToAscii);
	}
	return lenInt;
}