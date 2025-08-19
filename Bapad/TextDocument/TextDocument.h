#pragma once
#include "pch.h"
#include "FormatConversionV2.h"
#include "PieceTree.h"

constexpr size_t GetUtf8CharSize(const unsigned char ch) noexcept;

class TextIterator;

class TextDocument {
  friend class TextIterator;
public:
  TextDocument() noexcept;

  bool Initialize(const wchar_t* filename);

  bool Clear();
  size_t LineNumFromCharOffset(size_t offset);

  TextIterator IterateLineByLineNumber(size_t lineno, size_t* linestart = nullptr, size_t* linelen = nullptr);
  TextIterator IterateLineByCharOffset(size_t offset_chars, size_t* lineno = nullptr, size_t* linestart = nullptr);

  size_t	InsertText(size_t offsetChars, wchar_t* text, size_t length);
  size_t	ReplaceText(size_t offsetChars, wchar_t* text, size_t length, size_t eraseLen);
  size_t	EraseText(size_t offsetChars, size_t length);

  CP_TYPE GetFileFormat() const noexcept;
  const size_t GetLineCount() const noexcept;
  const size_t GetLongestLine(int tabwidth) const noexcept;
  const size_t GetDocLength() const noexcept;

private:


  // return processed raw charcount
  size_t  RawDataToUTF16(unsigned char* rawdata, size_t rawlen, wchar_t* utf16str, size_t& utf16len);
  // return processed UTF16 charcount
  size_t  UTF16ToRawData(wchar_t* utf16Str, size_t utf16Len, unsigned char* rawData, size_t& rawLen);

  size_t CharOffsetToIndexOffsetAt(const size_t startOffset, const size_t charCount) noexcept;
  size_t IndexOffsetToCharOffset(size_t offset) noexcept;

  size_t CountByteAnsi(const size_t startByteOffset, const size_t charCount) noexcept;
  size_t CountByteUtf8(const size_t startByteOffset, const size_t charCount) noexcept;
  size_t CountByte(const size_t startByteOffset, const size_t charCount) noexcept; // charCount to byteCount
  size_t CountCharAnsi(const size_t byteLength) noexcept;
  size_t CountCharUtf8(const size_t byteLength) noexcept;
  size_t CountChar(const size_t byteLength) noexcept; // byteCount to charCount;
  PieceTree docBuffer;// raw txt data

  CP_TYPE fileFormat;
  int  headerSize;

};


class TextIterator {
private:
  std::vector<wchar_t> lineContent;
  TextDocument* textDoc;
public:
  TextIterator() noexcept :lineContent(), textDoc(nullptr)
  {}
  TextIterator(const std::vector<wchar_t>& lineContent, TextDocument* textDoc)
    : lineContent(lineContent), textDoc(textDoc)
  {}
  ~TextIterator() noexcept = default;
  TextIterator(const TextIterator&) = default;
  TextIterator& operator=(const TextIterator&) = default;
  TextIterator(TextIterator&&) = default;
  TextIterator& operator=(TextIterator&&) = default;

  std::vector<wchar_t> GetLine()
  {
    if (textDoc)
    {
      return lineContent;
    }
    return std::vector<wchar_t>();
  }
  operator bool() noexcept
  {
    return textDoc != nullptr ? true : false;
  }
};


