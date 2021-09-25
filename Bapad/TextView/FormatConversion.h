#include "pch.h"

//
//	Conversions to UTF-16
//
int	   ascii_to_utf16(BYTE* asciistr, size_t asciilen, wchar_t* utf16str, size_t* utf16len);
int    utf8_to_utf16(BYTE* utf8str, size_t utf8len, wchar_t* utf16str, size_t* utf16len);
size_t utf8_to_utf32(BYTE* utf8str, size_t utf8len, DWORD* pcp32);

int	   copy_utf16(wchar_t* src, size_t srclen, wchar_t* dest, size_t* destlen);
int	   swap_utf16(wchar_t* src, size_t srclen, wchar_t* dest, size_t* destlen);

//
//	Conversions to UTF-32
//
int    utf16_to_utf32(WCHAR* utf16str, size_t utf16len, ULONG* utf32str, size_t* utf32len);
int    utf16be_to_utf32(WCHAR* utf16str, size_t utf16len, ULONG* utf32str, size_t* utf32len);

