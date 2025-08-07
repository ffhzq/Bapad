#include "pch.h"
#include "FormatConversion.h"
#include "MyException.h"
#include <stdexcept>

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
      && memcmp(&docBuffer[0], &i.bom, i.headerLength) == 0)
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

size_t UTF8ToUTF32(const unsigned char* utf8Str, size_t utf8Len, unsigned long* pch32)
{
  assert(utf8Str != nullptr);
  static ULONG nonshortest[] = {0, 0x80, 0x800, 0x10000, 0xffffffff, 0xffffffff};
  UTF32 val32 = 0;
  int trailing = 0;
  const gsl::span<const unsigned char> utf8Span(utf8Str, utf8Len);
  size_t spanOffset = 0;
  UTF8 ch = utf8Span[spanOffset++];

  if (utf8Str == nullptr || utf8Len <= 0 || pch32 == nullptr)
    return 0;
  constexpr char ASCII_MAX = 0x80 - 1;
  if (ch <= ASCII_MAX)
  {
    *pch32 = ch;
    return 1;
  }
  // LEAD-byte of 2-byte seq: 110xxxxx 10xxxxxx
  else if ((ch & 0xE0) == 0xC0)
  {
    trailing = 1;
    val32 = ch & 0x1F;
  }
  // LEAD-byte of 3-byte seq: 1110xxxx 10xxxxxx 10xxxxxx
  else if ((ch & 0xF0) == 0xE0)
  {
    trailing = 2;
    val32 = ch & 0x0F;
  }
  // LEAD-byte of 4-byte seq: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  else if ((ch & 0xF8) == 0xF0)
  {
    trailing = 3;
    val32 = ch & 0x07;
  }
  // ILLEGAL 5-byte seq: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
  else if ((ch & 0xFC) == 0xF8)
  {
    // range-checking the UTF32 result will catch this
    trailing = 4;
    val32 = ch & 0x03;
  }
  // ILLEGAL 6-byte seq: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
  else if ((ch & 0xFE) == 0xFC)
  {
    // range-checking the UTF32 result will catch this
    trailing = 5;
    val32 = ch & 0x01;
  }
  // ILLEGAL continuation (trailing) byte by itself
  else if ((ch & 0xC0) == 0x80)
  {
    *pch32 = UNI_REPLACEMENT_CHAR;
    return 1;
  }
  // any other ILLEGAL form.
  else
  {
    *pch32 = UNI_REPLACEMENT_CHAR;
    return 1;
  }

  size_t i = 0, len = 1;
  // process trailing bytes
  for (; i < trailing && len < utf8Len; i++)
  {
    ch = utf8Span[spanOffset++];

    // Valid trail-byte: 10xxxxxx
    if ((ch & 0xC0) == 0x80)
    {
      val32 = (val32 << 6) + (ch & 0x7f);
      len++;
    }
    // Anything else is an error
    else
    {
      *pch32 = UNI_REPLACEMENT_CHAR;
      return len;
    }
  }

  // did we 
  if (val32 < gsl::at(nonshortest, trailing) || i != trailing)
    *pch32 = UNI_REPLACEMENT_CHAR;
  else
    *pch32 = val32;

  if (utf8Len != 0 && len == 0)
  {
    throw MyException("Invalid UTF-8 sequence", -1, MyException::ConversionType::FromUtf8ToUtf32);
  }

  return len;
}
size_t UTF32ToUTF8(unsigned long ch32, unsigned char* utf8Str, const size_t utf8Len)
{
  size_t len = 0;

  // validate parameters
  if (utf8Str == nullptr || utf8Len == 0)
    return 0;

  const gsl::span<unsigned char> utf8Span(utf8Str, utf8Len);
  size_t spanOffset = 0;

  // ASCII is the easiest
  if (ch32 < 0x80)
  {
    utf8Span[spanOffset] = gsl::narrow_cast<UTF8>(ch32);
    return 1;
  }

  // make sure we have a legal utf32 char
  if (ch32 > UNI_MAX_LEGAL_UTF32)
    ch32 = UNI_REPLACEMENT_CHAR;

  // cannot encode the surrogate range
  if (ch32 >= UNI_SUR_HIGH_START && ch32 <= UNI_SUR_LOW_END)
    ch32 = UNI_REPLACEMENT_CHAR;

  // 2-byte sequence
  if (ch32 < 0x800 && utf8Len >= 2)
  {
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 >> 6) | 0xC0);
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 & 0x3f) | 0x80);
    len = 2;
  }
  // 3-byte sequence
  else if (ch32 < 0x10000 && utf8Len >= 3)
  {
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 >> 12) | 0xE0);
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 >> 6) & 0x3f | 0x80);
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 & 0x3f) | 0x80);
    len = 3;
  }
  // 4-byte sequence
  else if (ch32 <= UNI_MAX_LEGAL_UTF32 && utf8Len >= 4)
  {
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 >> 18) | 0xF0);
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 >> 12) & 0x3f | 0x80);
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 >> 6) & 0x3f | 0x80);
    utf8Span[spanOffset++] = gsl::narrow_cast<UTF8>((ch32 & 0x3f) | 0x80);
    len = 4;
  }

  // 5/6 byte sequences never occur because we limit using UNI_MAX_LEGAL_UTF32
  if (len == 0)
  {
    throw MyException("Invalid UTF-32 character", -1, MyException::ConversionType::FromUtf32ToUtf8);
  }
  return len;
}



size_t AsciiToUTF16(unsigned char* asciiStr, size_t asciiLen, unsigned short* utf16Str, size_t& utf16Len)
{
  if (asciiLen > static_cast<size_t>((std::numeric_limits<int>::max)()))
  {
    throw std::overflow_error(
      "Input ascii string too long, size_t doesn't fit into int.");
  }
  utf16Len = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<CCHAR*>(asciiStr), gsl::narrow_cast<int>(asciiLen), reinterpret_cast<wchar_t*>(utf16Str), gsl::narrow_cast<int>(utf16Len));
  if (utf16Len == 0)
  {
    throw std::invalid_argument("WideChar Buffer size is too small.");
  }
  return asciiLen;
}

size_t UTF8ToUTF16(unsigned char* utf8Str, size_t utf8Len, unsigned short* utf16Str, size_t& utf16Len)
{
  const gsl::span<unsigned char> utf8Span(utf8Str, utf8Len);
  size_t utf8SpanOffset = 0;

  const gsl::span<unsigned short> utf16Span(utf16Str, utf16Len);// todo check utf16Len is valid
  size_t utf16SpanOffset = 0;

  size_t len = 0;
  size_t tmp16len = 0;
  UTF32  ch32 = 0;

  while (utf8Len > 0 && utf16Len > 0)
  {
    // convert to utf-32
    len = UTF8ToUTF32(&utf8Span[utf8SpanOffset], utf8Len, &ch32);
    utf8SpanOffset += len;
    utf8Len -= len;

    // convert to utf-16
    tmp16len = utf16Len;
    len = UTF32ToUTF16(&ch32, 1, &utf16Span[utf16SpanOffset], tmp16len);
    utf16SpanOffset += len;
    utf16Len -= len;
  }

  utf16Len = utf16SpanOffset;
  if (utf8SpanOffset != 0 && utf16Len == 0)
  {
    throw MyException("Invalid UTF-8 character", -1, MyException::ConversionType::FromUtf8ToUtf16);

  }

  return utf8SpanOffset;
}




size_t CopyUTF16(const unsigned short* src, size_t srcLen, unsigned short* dest, size_t& destLen)
{
  size_t len = (std::min)(destLen, srcLen);
  memcpy(dest, src, len * sizeof(UTF16));
  destLen = len;
  if (srcLen != 0 && len == 0)
  {
    throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::CopyUtf16);
  }
  return len;
}

size_t SwapUTF16(unsigned short* src, size_t srcLen, unsigned short* dest, size_t& destLen)
{
  size_t len = (std::min)(destLen, srcLen);
  const gsl::span<unsigned short> srcSpan(src, len);
  const gsl::span<unsigned short> destSpan(dest, len);
  for (size_t i = 0; i < len; i++)
    std::swap(srcSpan[i], destSpan[i]);
  destLen = len;
  if (srcLen != 0 && len == 0)
  {
    throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::SwapUtf16);
  }
  return len;
}

size_t UTF16ToUTF32(unsigned short* utf16Str, size_t utf16Len, unsigned long* utf32Str, size_t& utf32Len)
{
  const gsl::span<unsigned short> utf16Span(utf16Str, utf16Len);
  size_t utf16SpanOffset = 0;

  const gsl::span<unsigned long> utf32Span(utf32Str, utf32Len);// todo check utf32Len is valid
  size_t utf32SpanOffset = 0;

  while (utf16Len > 0 && utf32Len > 0)
  {
    UTF32 charTemp1 = utf16Span[utf16SpanOffset];

    // first of a surrogate pair?
    if (charTemp1 >= UNI_SUR_HIGH_START && charTemp1 < UNI_SUR_HIGH_END && utf16Len >= 2)
    {
      const UTF32 charTemp2 = utf16Span[utf16SpanOffset + 1];

      // valid trailing surrogate unit?
      if (charTemp2 >= UNI_SUR_LOW_START && charTemp1 < UNI_SUR_LOW_END)
      {
        charTemp1 = ((charTemp1 - UNI_SUR_HIGH_START) << 10) +
          ((charTemp2 - UNI_SUR_LOW_START) + 0x00010000);

        ++utf16SpanOffset;
        --utf16Len;
      }
      // illegal character
      else
      {
        charTemp1 = UNI_REPLACEMENT_CHAR;
      }
    }

    utf32Span[utf32SpanOffset++] = charTemp1;
    --utf32Len;

    ++utf16SpanOffset;
    --utf16Len;
  }

  if (utf16Len != 0 && utf32SpanOffset == 0)
  {
    throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::FromUtf16ToUtf32);

  }
  return utf16SpanOffset;
}

size_t UTF32ToUTF16(unsigned long* utf32Str, size_t utf32Len, unsigned short* utf16Str, size_t& utf16Len)
{
  const gsl::span<unsigned short> utf16Span(utf16Str, utf16Len);
  size_t utf16SpanOffset = 0;

  const gsl::span<unsigned long> utf32Span(utf32Str, utf32Len);// todo check utf32Len is valid
  size_t utf32SpanOffset = 0;


  while (utf32Len > 0 && utf16Len > 0)
  {
    UTF32 ch32 = utf32Span[utf32SpanOffset++];
    utf32Len--;

    // target is a character <= 0xffff
    if (ch32 < 0xfffe)
    {
      // make sure we don't represent anything in UTF16 surrogate range
      // (this helps protect against non-shortest forms)
      if (ch32 >= UNI_SUR_HIGH_START && ch32 <= UNI_SUR_LOW_END)
      {
        utf16Span[utf16SpanOffset++] = UNI_REPLACEMENT_CHAR;
        utf16Len--;
      }
      else
      {
        utf16Span[utf16SpanOffset++] = gsl::narrow_cast<WORD>(ch32);
        utf16Len--;
      }
    }
    // FFFE and FFFF are illegal mid-stream
    else if (ch32 == 0xfffe || ch32 == 0xffff)
    {
      utf16Span[utf16SpanOffset++] = UNI_REPLACEMENT_CHAR;
      utf16Len--;
    }
    // target is illegal Unicode value
    else if (ch32 > UNI_MAX_UTF16)
    {
      utf16Span[utf16SpanOffset++] = UNI_REPLACEMENT_CHAR;
      utf16Len--;
    }
    // target is in range 0xffff - 0x10ffff
    else if (utf16Len >= 2)
    {
      ch32 -= 0x0010000;

      utf16Span[utf16SpanOffset++] = gsl::narrow_cast<WORD>((ch32 >> 10) + UNI_SUR_HIGH_START);
      utf16Span[utf16SpanOffset++] = gsl::narrow_cast<WORD>((ch32 & 0x3ff) + UNI_SUR_LOW_START);

      utf16Len -= 2;
    }
    else
    {
      // no room to store result
      break;
    }
  }

  utf16Len = utf16SpanOffset;
  if (utf32SpanOffset != 0 && utf16Len == 0)
  {
    throw MyException("Invalid UTF-32 character", -1, MyException::ConversionType::FromUtf32ToUtf16);


  }
  return utf32SpanOffset;
}

size_t UTF16BEToUTF32(unsigned short* utf16Str, size_t utf16Len, unsigned long* utf32Str, size_t& utf32Len)
{
  if (utf16Str == nullptr || utf32Str == nullptr || utf16Len == 0 || utf32Len == 0) return 0;

  const gsl::span<unsigned short> utf16Span(utf16Str, utf16Len);
  size_t utf16SpanOffset = 0;

  const gsl::span<unsigned long> utf32Span(utf32Str, utf32Len);// todo check utf32Len is valid
  size_t utf32SpanOffset = 0;

  while (utf16Len > 0 && utf32Len > 0)
  {
    ULONG ch = SwapWord16(utf16Span[utf16SpanOffset]);

    // first of a surrogate pair?
    if (ch >= UNI_SUR_HIGH_START && ch < UNI_SUR_HIGH_END && utf16Len >= 2)
    {
      const ULONG ch2 = SwapWord16(utf16Span[utf16SpanOffset + 1]);

      // valid trailing surrogate unit?
      if (ch2 >= UNI_SUR_LOW_START && ch < UNI_SUR_LOW_END)
      {
        ch = ((ch - UNI_SUR_HIGH_START) << 10) +
          ((ch2 - UNI_SUR_LOW_START) + 0x00010000);

        ++utf16SpanOffset;
        --utf16Len;
      }
      // illegal character
      else
      {
        ch = UNI_REPLACEMENT_CHAR;
      }
    }

    utf32Span[utf32SpanOffset++] = ch;
    --utf32Len;

    ++utf16SpanOffset;
    --utf16Len;
  }

  utf32Len = utf32SpanOffset;
  if (utf16SpanOffset != 0 && utf32Len == 0)
  {
    throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::FromUtf16BEToUtf32);


  }
  return utf16SpanOffset;
}

size_t UTF16ToUTF8(unsigned short* utf16Str, size_t utf16Len, unsigned char* utf8Str, size_t& utf8Len)
{
  const gsl::span<unsigned short> utf16Span(utf16Str, utf16Len);
  size_t utf16SpanOffset = 0;

  const gsl::span<unsigned char> utf8Span(utf8Str, utf8Len);// todo check utf8Len is valid
  size_t utf8SpanOffset = 0;

  size_t  len = 0;
  UTF32 ch32 = 0;
  size_t  ch32len = 0;

  while (utf16Len > 0 && utf8Len > 0)
  {
    // convert to utf-32
    ch32len = 1;
    len = UTF16ToUTF32(&utf16Span[utf16SpanOffset], utf16Len, &ch32, ch32len);
    utf16SpanOffset += len;
    utf16Len -= len;

    // convert to utf-8
    len = UTF32ToUTF8(ch32, &utf8Span[utf8SpanOffset], utf8Len);
    utf8SpanOffset += len;
    utf8Len -= len;
  }

  utf8Len = utf8SpanOffset;
  if (utf16SpanOffset != 0 && utf8SpanOffset == 0)
  {
    throw MyException("Invalid UTF-16 character", -1, MyException::ConversionType::FromUtf16ToUtf8);

  }
  return utf16SpanOffset;
}



size_t UTF16ToAscii(const unsigned short* utf16Str, size_t utf16Len, unsigned char* asciiStr, size_t& asciiLen)
{
  size_t len = (std::min)(utf16Len, asciiLen);
  constexpr size_t intMaxVal = static_cast<size_t>((std::numeric_limits<int>::max)());
  if (len > intMaxVal)
  {
    throw std::overflow_error(
      "Input UTF16 string too long, size_t doesn't fit into int.");
  }
  WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<LPCWCH>(utf16Str), gsl::narrow_cast<int>(len), reinterpret_cast<LPSTR>(asciiStr), gsl::narrow_cast<int>(asciiLen), nullptr, nullptr);
  asciiLen = len;
  return len;
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
  //const gsl::span<const unsigned char> bufferSpan(buffer);
  unsigned char* bufferSpan = buffer.data();
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