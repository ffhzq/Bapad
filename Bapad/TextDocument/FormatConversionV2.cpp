#include "pch.h"
#include "FormatConversionV2.h"

std::vector<wchar_t> RawToUtf16(std::vector<unsigned char>& rawData, const CP_TYPE rawDataCodpage)
{
  std::vector<wchar_t> utf16Data;
  size_t targetLength = 0;
  const size_t srcLen = rawData.size();
  switch (rawDataCodpage)
  {
  case CP_TYPE::ANSI:
    assert(srcLen < static_cast<size_t>(INT_MAX));
    targetLength = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<CCHAR*>(rawData.data()), gsl::narrow_cast<int>(rawData.size()), nullptr, 0);
    utf16Data.resize(targetLength);
    assert(targetLength < static_cast<size_t>((std::numeric_limits<int>::max)()));
    MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<CCHAR*>(rawData.data()), gsl::narrow_cast<int>(rawData.size()), utf16Data.data(), gsl::narrow_cast<int>(targetLength));
    break;
  case CP_TYPE::UTF8:
    targetLength = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<CCHAR*>(rawData.data()), gsl::narrow_cast<int>(rawData.size()), nullptr, 0);
    utf16Data.resize(targetLength);
    assert(targetLength < static_cast<size_t>((std::numeric_limits<int>::max)()));
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<CCHAR*>(rawData.data()), gsl::narrow_cast<int>(rawData.size()), utf16Data.data(), gsl::narrow_cast<int>(targetLength));
    break;
  case CP_TYPE::UTF16:
    targetLength = srcLen / 2;
    utf16Data.resize(targetLength);
    std::memcpy(utf16Data.data(), rawData.data(), srcLen);
    break;
  case CP_TYPE::UTF16BE:
    targetLength = srcLen / 2;
    utf16Data.resize(targetLength);
    for (auto i{0U}; i < targetLength; ++i)
    {
      auto p = reinterpret_cast<wchar_t*>(&gsl::at(rawData, i * 2));
      gsl::at(utf16Data, i) = SwapWord16(*p);
    }
    break;
  default:
    throw;
  }
  return utf16Data;
}

std::vector<unsigned char> Utf16toRaw(std::vector<wchar_t>& utf16Data, const CP_TYPE rawDataCodpage)
{
  std::vector<unsigned char> rawData;
  size_t targetLength = 0;
  const size_t srcLen = utf16Data.size();
  switch (rawDataCodpage)
  {
  case CP_TYPE::ANSI:
    assert(srcLen < static_cast<size_t>(INT_MAX));
    targetLength = WideCharToMultiByte(CP_ACP, 0, utf16Data.data(), gsl::narrow_cast<int>(srcLen), nullptr, 0, nullptr, nullptr);
    rawData.resize(targetLength);
    WideCharToMultiByte(CP_ACP, 0, utf16Data.data(), gsl::narrow_cast<int>(srcLen), reinterpret_cast<char*>(rawData.data()), gsl::narrow_cast<int>(targetLength), nullptr, nullptr);
    break;
  case CP_TYPE::UTF8:
    assert(srcLen < static_cast<size_t>(INT_MAX));
    targetLength = WideCharToMultiByte(CP_UTF8, 0, utf16Data.data(), gsl::narrow_cast<int>(srcLen), nullptr, 0, nullptr, nullptr);
    rawData.resize(targetLength);
    WideCharToMultiByte(CP_UTF8, 0, utf16Data.data(), gsl::narrow_cast<int>(srcLen), reinterpret_cast<char*>(rawData.data()), gsl::narrow_cast<int>(targetLength), nullptr, nullptr);
    break;
  case CP_TYPE::UTF16:
    targetLength = srcLen * 2;
    rawData.resize(targetLength);
    std::memcpy(rawData.data(), utf16Data.data(), targetLength);
    break;
  case CP_TYPE::UTF16BE:
    targetLength = srcLen * 2;
    rawData.resize(targetLength);
    for (auto i{0U}; i < srcLen; ++i)
    {
      auto p = reinterpret_cast<wchar_t*>(&gsl::at(rawData, i * 2));
      *p = SwapWord16(gsl::at(utf16Data, i));
    }
    break;
  default:
    throw;
  }
  return rawData;
}

struct _BOM_LOOKUP BOMLOOK[] =
{
  // define longest headers first
  //bom, headerlen, encoding form
  { 0x0000FEFF, 4, CP_TYPE::UTF32 },
  { 0xFFFE0000, 4, CP_TYPE::UTF32BE },
  { 0xBFBBEF, 3, CP_TYPE::UTF8 },
  { 0xFFFE, 2, CP_TYPE::UTF16BE },
  { 0xFEFF, 2, CP_TYPE::UTF16 },
  { 0, 0, CP_TYPE::ANSI },
};



CP_TYPE DetectFileFormat(std::vector<unsigned char> docBuffer, int& headerSize) noexcept
{
  CP_TYPE res = CP_TYPE::UNKNOWN;
  if (docBuffer.empty()) return res;
  const size_t docLengthByBytes = docBuffer.size();
  for (auto i : BOMLOOK)
  {
    if (docLengthByBytes >= i.headerLength
      && memcmp(&gsl::at(docBuffer, 0), &i.bom, i.headerLength) == 0)
    {
      headerSize = i.headerLength;
      res = i.codePageType;
      break;
    }
  }
  if (res == CP_TYPE::ANSI)
  {
    if (IsUTF8(docBuffer))
    {
      res = CP_TYPE::UTF8;
      headerSize = 0;
    }
  }
  return res;

}


bool IsUTF8(std::vector<unsigned char> buffer) noexcept
{
  if (buffer.empty()) return false;
  const size_t len = buffer.size();
  /*
 1byte :0xxxxxxx
 2bytes:110xxxxx 10xxxxxx
 3bytes:1110xxxx 10xxxxxx 10xxxxxx
 4bytes:11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
  const gsl::span<const unsigned char> bufferSpan(buffer);
  //unsigned char* bufferSpan = buffer.data();
  size_t i = 0;
  while (i < len)
  {
    if (bufferSpan[i] < 0x80)
    {
      i++;
    }
    else if ((bufferSpan[i] & 0xE0) == 0xC0)
    {
      if (i + 1 >= len)
        return false;
      if ((bufferSpan[i + 1] & 0xC0) != 0x80)
        return false;
      i += 2;
    }
    else if ((bufferSpan[i] & 0xF0) == 0xE0)
    {
      if (i + 2 >= len)
        return false;
      if ((bufferSpan[i + 1] & 0xC0) != 0x80 || (bufferSpan[i + 2] & 0xC0) != 0x80)
        return false;
      i += 3;
    }
    else if ((bufferSpan[i] & 0xF8) == 0xF0)
    {
      if (i + 3 >= len)
        return false;
      if ((bufferSpan[i + 1] & 0xC0) != 0x80 || (bufferSpan[i + 2] & 0xC0) != 0x80 || (bufferSpan[i + 3] & 0xC0) != 0x80)
        return false;
      i += 4;
    }
    else
    {
      return false;
    }
  }
  return true;

}