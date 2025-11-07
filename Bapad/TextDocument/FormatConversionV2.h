#pragma once
#include "pch.h"

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

template<typename T>
inline auto SwapWord16(T& ch16) noexcept
{
  T left_val = ((ch16) << 8);
  T right_val = ((ch16) >> 8);
  ch16 = left_val | right_val;
  return ch16;
}


bool IsUTF8(std::vector<unsigned char> buffer) noexcept;

std::vector<wchar_t> RawToUtf16(std::vector<unsigned char>& rawData, const CP_TYPE rawDataCodpage);
std::vector<unsigned char> Utf16toRaw(std::vector<wchar_t>& utf16Data, const CP_TYPE rawDataCodpage);