#pragma once
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

// Bapad codepage
enum class CP_TYPE {
  ANSI,
  UTF8,
  UTF16,
  UTF16BE,
  UTF32,
  UTF32BE,
  UNKNOWN = -1
};

//Byte Order Mark
struct _BOM_LOOKUP {
  unsigned long  bom;
  int  headerLength;
  CP_TYPE codePageType;
};

CP_TYPE DetectFileFormat(std::vector<unsigned char> docBuffer, int& headerSize) noexcept;

//without side affect
#define SwapWord(val) (((WORD)(val) << 8) | ((WORD)(val) >> 8))

template<typename T>
inline auto SwapWord16(T& ch16) noexcept
{
  T left_val = ((ch16) << 8);
  T right_val = ((ch16) >> 8);
  ch16 = left_val | right_val;
  return ch16;
}


size_t  UTF8ToUTF32(const unsigned char* utf8Str, size_t utf8Len, unsigned long* pch32);
size_t  UTF32ToUTF8(unsigned long ch32, unsigned char* utf8Str, const size_t utf8Len);

size_t  UTF8ToUTF16(unsigned char* utf8Str, size_t utf8Len, unsigned short* utf16Str, size_t& utf16Len);
size_t  UTF16ToUTF8(unsigned short* utf16Str, size_t utf16Len, unsigned char* utf8Str, size_t& utf8Len);

size_t  UTF16ToUTF32(unsigned short* utf16Str, size_t utf16Len, unsigned long* utf32Str, size_t& utf32Len);
size_t  UTF32ToUTF16(unsigned long* utf32Str, size_t utf32Len, unsigned short* utf16Str, size_t& utf16Len);
size_t  UTF16BEToUTF32(unsigned short* utf16Str, size_t utf16Len, unsigned long* utf32Str, size_t& utf32Len);

size_t  AsciiToUTF16(unsigned char* asciiStr, size_t asciiLen, unsigned short* utf16Str, size_t& utf16Len);
size_t  UTF16ToAscii(const unsigned short* utf16Str, size_t utf16Len, unsigned char* asciiStr, size_t& asciiLen);

size_t  CopyUTF16(const unsigned short* src, size_t srcLen, unsigned short* dest, size_t& destLen);
size_t  SwapUTF16(unsigned short* src, size_t srcLen, unsigned short* dest, size_t& destLen);

bool IsUTF8(std::vector<unsigned char> buffer) noexcept;