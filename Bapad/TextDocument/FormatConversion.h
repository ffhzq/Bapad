#include "pch.h"

//unsigned,8 bits long,BYTE
using Byte = unsigned char;

//unsigned,16 bits long,WORD
using Word = wchar_t;

//signed,8 bits long,CCHAR
using Cchar = char;

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

#define BCP_ASCII		0
#define BCP_UTF8		1
#define BCP_UTF16		2
#define BCP_UTF16BE		3
#define BCP_UTF32		4
#define BCP_UTF32BE		5

//Byte Order Mark
struct _BOM_LOOKUP
{
	DWORD  bom;
	ULONG  headerLength;
	int    type;
};

int DetectFileFormat(const unsigned char* docBuffer, const size_t docLengthByBytes, size_t& headerSize);

//without side affect
#define SwapWord(val) (((WORD)(val) << 8) | ((WORD)(val) >> 8))

template<typename T>
inline auto SwapWord16(T& ch16)
{
    return (ch16 = (((T)(ch16) << 8) | ((T)(ch16) >> 8)));
}

inline void LittleToBig32(char32_t& ch32)
{
    unsigned char* data = reinterpret_cast<unsigned char*>(&ch32);
    ch32 = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

inline void BigToLittle32(char32_t& ch32)
{
    unsigned char* data = reinterpret_cast<unsigned char*>(&ch32);
    ch32 = (data[3] << 0) | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);

}

inline void LittleToBig32(char32_t& ch32)
{
	unsigned char* data = reinterpret_cast<unsigned char*>(&ch32);
	ch32 = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

inline void BigToLittle32(char32_t& ch32)
{
	unsigned char* data = reinterpret_cast<unsigned char*>(&ch32);
	ch32 = (data[3] << 0) | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
}

size_t  UTF8ToUTF16(UTF8* utf8Str, size_t utf8Len, UTF16* utf16Str, size_t& utf16Len);
size_t  UTF16ToUTF8(UTF16* utf16Str, size_t utf16Len, UTF8* utf8Str, size_t& utf8Len);

size_t  UTF16ToUTF32(UTF16* utf16Str, size_t utf16Len, UTF32* utf32Str, size_t& utf32Len);
size_t  UTF32ToUTF16(UTF32* utf32Str, size_t utf32Len, UTF16* utf16Str, size_t& utf16Len);
size_t  UTF16BEToUTF32(UTF16* utf16Str, size_t utf16Len, UTF32* utf32Str, size_t& utf32Len);

size_t  AsciiToUTF16(UTF8* asciiStr, size_t asciiLen, UTF16* utf16Str, size_t& utf16Len);
size_t  UTF16ToAscii(UTF16* utf16Str, size_t utf16Len, UTF8* asciiStr, size_t& asciiLen);

size_t  CopyUTF16(UTF16* src, size_t srcLen, UTF16* dest, size_t& destLen);
size_t  SwapUTF16(UTF16* src, size_t srcLen, UTF16* dest, size_t& destLen);

bool IsUTF8(const unsigned char* buffer, size_t len);