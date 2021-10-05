#include "FormatConversion.h"

int AsciiToUTF16(Byte* asciiStr, size_t asciiLen, wchar_t* utf16Str, size_t& utf16Len)
{
    return 0;
}

int UTF8ToUTF16(Byte* utf8Str, size_t utf8Len, wchar_t* utf16Str, size_t& utf16Len)
{
    return 0;
}

size_t UTF8ToUTF32(Byte* utf8Str, size_t utf8Len, char32_t & pch32)
{
    return size_t();
}

int CopyUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t & destLen)
{
    return 0;
}

int SwapUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t & destLen)
{
    return 0;
}

int UTF16ToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len)
{
    return 0;
}

int UTF16BEToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len)
{
    return 0;
}