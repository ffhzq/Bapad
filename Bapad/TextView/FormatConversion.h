#include "pch.h"

//char8_t
using Byte = unsigned char;

using Word = char16_t;//unsigned short;

//
//	Conversions to UTF-16
//
int	   AsciiToUTF16(Byte* asciiStr, size_t asciiLen, wchar_t* utf16Str, size_t& utf16Len);
int    UTF8ToUTF16(Byte* utf8Str, size_t utf8Len, wchar_t* utf16Str, size_t& utf16Len);
size_t UTF8ToUTF32(Byte* utf8Str, size_t utf8Len, char32_t & pch32);

int	   CopyUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t & destLen);
int	   SwapUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t & destLen);

//
//	Conversions to UTF-32
//
int    UTF16ToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len);
int    UTF16BEToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len);

