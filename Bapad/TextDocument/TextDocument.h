#pragma once
#include "pch.h"
#include "PieceTree.h"

class TextIterator;

class TextDocument {
  friend class TextIterator;
public:
  TextDocument() noexcept;

  bool Initialize(wchar_t* filename);

  bool Clear();
  size_t LineNumFromCharOffset(size_t offset);

  TextIterator IterateLineByLineNumber(size_t lineno, size_t* linestart = nullptr, size_t* linelen = nullptr);
  TextIterator IterateLineByCharOffset(size_t offset_chars, size_t* lineno = nullptr, size_t* linestart = nullptr);

  size_t	InsertText(size_t offsetChars, wchar_t* text, size_t length);
  size_t	ReplaceText(size_t offsetChars, wchar_t* text, size_t length, size_t eraseLen);
  size_t	EraseText(size_t offsetChars, size_t length);

  const int GetFileFormat() const noexcept;
  const size_t GetLineCount() const noexcept;
  const size_t GetLongestLine(int tabwidth) const noexcept;
  const size_t GetDocLength() const noexcept;

private:

  // GetText: read 'lenBytes'or'bufLen'(use the smaller one) bytes wchar from the position (docBuffer+offset) to 'buf'
  size_t  GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t& bufLen);

  // return processed raw charcount
  size_t  RawDataToUTF16(unsigned char* rawdata, size_t rawlen, wchar_t* utf16str, size_t& utf16len);
  // return processed UTF16 charcount
  size_t  UTF16ToRawData(wchar_t* utf16Str, size_t utf16Len, unsigned char* rawData, size_t& rawLen);

  size_t	InsertTextRaw(size_t offsetBytes, wchar_t* text, size_t textLength);
  size_t	ReplaceTextRaw(size_t offsetBytes, wchar_t* text, size_t textLength, size_t eraseLen);
  size_t	EraseTextRaw(size_t offsetBytes, size_t textLength);

  size_t CharOffsetToByteOffsetAt(const size_t startOffsetBytes, const size_t charCount);
  size_t ByteOffsetToCharOffset(size_t offsetBytes);
  PieceTree docBuffer;// raw txt data

  int fileFormat;
  int  headerSize;

};


class TextIterator {
private:
  std::vector<unsigned char> lineContent;
  TextDocument* textDoc;
  size_t processedBytes;
public:
  TextIterator() noexcept :lineContent(), textDoc(nullptr), processedBytes(0)
  {}
  TextIterator(const std::vector<unsigned char>& lineContent, TextDocument* textDoc, const size_t& processedBytes)
    : lineContent(lineContent), textDoc(textDoc), processedBytes(processedBytes)
  {}
  ~TextIterator() noexcept = default;
  TextIterator(const TextIterator&) = default;
  TextIterator& operator=(const TextIterator&) = default;
  TextIterator(TextIterator&&) = default;
  TextIterator& operator=(TextIterator&&) = default;


  size_t GetText(wchar_t* buf, size_t bufLen)
  {
    if (textDoc)
    {
      memset(buf, 0, bufLen * sizeof(wchar_t));
      // get text from the TextDocument at the specified byte-offset
      const auto startPos = lineContent.data() + processedBytes;
      const size_t strLen = lineContent.size() - processedBytes;
      const size_t len = textDoc->RawDataToUTF16(startPos, strLen, buf, bufLen);

      // adjust the iterator's internal position
      processedBytes += len;
      return bufLen;
    }
    else
    {
      return 0;
    }
  }

  operator bool() noexcept
  {
    return textDoc != nullptr ? true : false;
  }
};


