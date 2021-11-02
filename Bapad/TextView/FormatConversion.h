#include "pch.h"

namespace zq {


    //char8_t
    using Byte = unsigned char;

    using Word = char16_t;//unsigned short;

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

//without side affect
#define SwapWord(val) (((WORD)(val) << 8) | ((WORD)(val) >> 8))

    template<typename T>
    inline auto SwapWord16(T & ch16)
    {
        return (ch16 = (((T)(ch16) << 8) | ((T)(ch16) >> 8)));
    }

    inline void LittleToBig32(char32_t & ch32)
    {
        unsigned char* data = reinterpret_cast<unsigned char*>(&ch32);

        ch32 = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

    }

    inline void BigToLittle32(char32_t & ch32)
    {
        unsigned char* data = reinterpret_cast<unsigned char*>(&ch32);

        ch32 = (data[3] << 0) | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
    }

    //
    //	Conversions to UTF-16
    //
    int	   AsciiToUTF16(Byte* asciiStr, size_t asciiLen, wchar_t* utf16Str, size_t& utf16Len);
    int    UTF8ToUTF16(Byte* utf8Str, size_t utf8Len, wchar_t* utf16Str, size_t& utf16Len);
    size_t UTF8ToUTF32(Byte* utf8Str, size_t utf8Len, char32_t& pch32);

    int	   CopyUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t& destLen);
    int	   SwapUTF16(wchar_t* src, size_t srcLen, wchar_t* dest, size_t& destLen);

    //
    //	Conversions to UTF-32
    //
    int    UTF16ToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len);
    int    UTF16BEToUTF32(wchar_t* utf16Str, size_t utf16Len, ULONG* utf32Str, size_t& utf32Len);


}
