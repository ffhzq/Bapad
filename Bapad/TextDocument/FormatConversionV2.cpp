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
