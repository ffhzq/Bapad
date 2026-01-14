#include "FormatConversionV2.h"

#include <unicode/ucnv.h>
#include <unicode/unistr.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "Windows.h"
#include "gsl/gsl"

std::vector<char16_t> RawToUtf16(std::vector<char>& rawData,
                                 const CP_TYPE rawDataCodpage) {
  UErrorCode errorCode = U_ZERO_ERROR;
  UConverter* converter = nullptr;
  std::vector<char16_t> utf16Str;
  std::string codepageName;

  switch (rawDataCodpage) {
    case CP_TYPE::ANSI:
      codepageName = std::string();
      break;
    case CP_TYPE::UTF8:
      codepageName = std::string("UTF8");
      break;
    case CP_TYPE::UTF16:
      codepageName = std::string("UTF16LE");
      break;
    case CP_TYPE::UTF16BE:
      codepageName = std::string("UTF16BE");
      break;
    default:
      throw std::runtime_error("Unsupported code page type.");
  }
  converter = ucnv_open(codepageName.c_str(), &errorCode);
  if (U_FAILURE(errorCode)) {
    std::cerr << "Error opening converter: " << u_errorName(errorCode)
              << std::endl;
    return std::vector<char16_t>();
  }

  const int32_t targetLength =
      ucnv_toUChars(converter, nullptr, 0, rawData.data(),
                    gsl::narrow_cast<int32_t>(rawData.size()), &errorCode);

  if (errorCode == U_BUFFER_OVERFLOW_ERROR) {
    errorCode = U_ZERO_ERROR;

    std::vector<UChar> ucharBuffer(targetLength);

    ucnv_toUChars(converter, ucharBuffer.data(), targetLength, rawData.data(),
                  gsl::narrow_cast<int32_t>(rawData.size()), &errorCode);

    if (U_SUCCESS(errorCode)) {
      utf16Str = std::move(ucharBuffer);
      // std::cout << "ICU Converted (as UTF-8): " << uString << std::endl;
    } else {
      std::cerr << "Conversion error: " << u_errorName(errorCode) << std::endl;
    }
  }
  ucnv_close(converter);
  return utf16Str;
}

std::vector<char> Utf16toRaw(std::vector<char16_t>& utf16Data,
                             const CP_TYPE rawDataCodpage) {
  std::vector<char> rawData;
  throw;  // todo:
  return rawData;
}

std::vector<char16_t> NormalizeLineEndings(
    const std::vector<char16_t>& utf16Data) {
  std::vector<char16_t> result;
  result.reserve(utf16Data.size());
  for (auto it = utf16Data.begin(); it != utf16Data.end(); ++it) {
    if (*it == u'\r') {
      auto next_it = std::next(it);
      if (next_it != utf16Data.end() && *next_it == u'\n') {
        result.push_back(u'\n');
        it = next_it;
      } else {
        result.push_back(u'\n');
      }
    } else {
      result.push_back(*it);
    }
  }
  return result;
}

struct _BOM_LOOKUP BOMLOOK[] = {
    // define longest headers first
    // bom, headerlen, encoding form
    {0x0000FEFF, 4, CP_TYPE::UTF32}, {0xFFFE0000, 4, CP_TYPE::UTF32BE},
    {0xBFBBEF, 3, CP_TYPE::UTF8},    {0xFFFE, 2, CP_TYPE::UTF16BE},
    {0xFEFF, 2, CP_TYPE::UTF16},     {0, 0, CP_TYPE::ANSI},
};

CP_TYPE DetectFileFormat(std::vector<char> docBuffer,
                         int& headerSize) noexcept {
  CP_TYPE res = CP_TYPE::UNKNOWN;
  if (docBuffer.empty()) return res;
  const size_t docLengthByBytes = docBuffer.size();
  for (auto i : BOMLOOK) {
    if (docLengthByBytes >= i.headerLength &&
        memcmp(&gsl::at(docBuffer, 0), &i.bom, i.headerLength) == 0) {
      headerSize = i.headerLength;
      res = i.codePageType;
      break;
    }
  }
  if (res == CP_TYPE::ANSI) {
    if (IsUTF8(docBuffer)) {
      res = CP_TYPE::UTF8;
      headerSize = 0;
    }
  }
  return res;
}

bool IsUTF8(std::vector<char> buffer) noexcept {
  if (buffer.empty()) return false;
  const size_t len = buffer.size();
  /*
 1byte :0xxxxxxx
 2bytes:110xxxxx 10xxxxxx
 3bytes:1110xxxx 10xxxxxx 10xxxxxx
 4bytes:11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
  const gsl::span<const char> bufferSpan(buffer);
  // unsigned char* bufferSpan = buffer.data();
  size_t i = 0;
  while (i < len) {
    if (bufferSpan[i] < 0x80) {
      i++;
    } else if ((bufferSpan[i] & 0xE0) == 0xC0) {
      if (i + 1 >= len) return false;
      if ((bufferSpan[i + 1] & 0xC0) != 0x80) return false;
      i += 2;
    } else if ((bufferSpan[i] & 0xF0) == 0xE0) {
      if (i + 2 >= len) return false;
      if ((bufferSpan[i + 1] & 0xC0) != 0x80 ||
          (bufferSpan[i + 2] & 0xC0) != 0x80)
        return false;
      i += 3;
    } else if ((bufferSpan[i] & 0xF8) == 0xF0) {
      if (i + 3 >= len) return false;
      if ((bufferSpan[i + 1] & 0xC0) != 0x80 ||
          (bufferSpan[i + 2] & 0xC0) != 0x80 ||
          (bufferSpan[i + 3] & 0xC0) != 0x80)
        return false;
      i += 4;
    } else {
      return false;
    }
  }
  return true;
}