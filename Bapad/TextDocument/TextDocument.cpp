#include "pch.h"
#include "formatConversion.h"
#include "TextDocument.h"

constexpr size_t LEN = 0x100;

TextDocument::TextDocument() noexcept
  :
  docBuffer(),
  fileFormat(0),
  headerSize(0)
{}
bool TextDocument::Initialize(wchar_t* filename)
{
  std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
  if (!ifs.is_open())
  {
    return false;
  }
  const size_t docLengthByBytes = ifs.tellg();
  std::vector<unsigned char> read_buffer;

  ifs.seekg(0, std::ios::beg);
  if (docLengthByBytes <= 0)
  {
    return false;
  }
  try
  {
    read_buffer.resize(docLengthByBytes);
  }
  catch (const std::bad_alloc& e)
  {
    return false;
  }
  ifs.read(reinterpret_cast<char*>(&gsl::at(read_buffer,0)), docLengthByBytes);
  if (!ifs)
  {
    return false;
  }

  // Detect file format
  fileFormat = DetectFileFormat(read_buffer.data(), docLengthByBytes, headerSize);
  read_buffer.erase(read_buffer.begin(), read_buffer.begin() + headerSize);
  docBuffer = std::move(PieceTree(read_buffer));

  ifs.close();
  return true;
}

size_t TextDocument::GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t& bufLen)
{
  const size_t offset_plus = offset;
  auto rawData = docBuffer.GetText(offset_plus, lenBytes);
  //size_t  len;

  if (offset >= docBuffer.length)
  {
    bufLen = 0;
    return 0;
  }
  return RawDataToUTF16(rawData.data(), lenBytes, buf, bufLen);
}

size_t TextDocument::RawDataToUTF16(unsigned char* rawdata, size_t rawlen, wchar_t* utf16str, size_t& utf16len)
{
  switch (fileFormat)
  {
    // convert from ANSI->UNICODE
  case BCP_ASCII:

    return AsciiToUTF16(rawdata, rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len);

  case BCP_UTF8:
    return UTF8ToUTF16(rawdata, rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len);

    // already unicode, do a straight memory copy
  case BCP_UTF16:
    rawlen /= sizeof(wchar_t);
    return CopyUTF16(reinterpret_cast<UTF16*>(rawdata), rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len) * sizeof(wchar_t);

    // need to convert from big-endian to little-endian
  case BCP_UTF16BE:
    rawlen /= sizeof(wchar_t);
    return SwapUTF16(reinterpret_cast<UTF16*>(rawdata), rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len) * sizeof(wchar_t);
  default:
    utf16len = 0;
    return 0;

  }
}

size_t TextDocument::UTF16ToRawData(wchar_t* utf16Str, size_t utf16Len, unsigned char* rawData, size_t& rawLen)
{
  switch (fileFormat)
  {
  case BCP_ASCII:
    return UTF16ToAscii(reinterpret_cast<UTF16*>(utf16Str), utf16Len, rawData, rawLen);
  case BCP_UTF8:
    return UTF16ToUTF8(reinterpret_cast<UTF16*>(utf16Str), utf16Len, rawData, rawLen);
  case BCP_UTF16:
    rawLen /= sizeof(wchar_t);
    utf16Len = CopyUTF16(reinterpret_cast<UTF16*>(utf16Str), utf16Len, reinterpret_cast<UTF16*>(rawData), rawLen);
    rawLen *= sizeof(wchar_t);
    return utf16Len;
  case BCP_UTF16BE:
    rawLen /= sizeof(wchar_t);
    utf16Len = SwapUTF16(reinterpret_cast<UTF16*>(utf16Str), utf16Len, reinterpret_cast<UTF16*>(rawData), rawLen);
    rawLen *= sizeof(wchar_t);
    return utf16Len;
  default:
    rawLen = 0;
    return 0;
  }
}

const int TextDocument::GetFileFormat() const noexcept
{
  return fileFormat;
}

const size_t TextDocument::GetLineCount() const noexcept
{
  return docBuffer.lineCount;
}

const size_t TextDocument::GetLongestLine(int tabwidth) const noexcept
{
  return size_t{100};
}

const size_t TextDocument::GetDocLength() const noexcept
{
  return docBuffer.length;
}

size_t TextDocument::LineNumFromOffset(size_t offset)
{
  size_t lineNum = 0;
  size_t byte_offset = 0;
  TreeNode* node = docBuffer.GetNodePosition(offset, byte_offset);
  LineInfoFromOffset(offset, &lineNum, nullptr, nullptr, nullptr, nullptr);
  return lineNum;
}

bool TextDocument::LineInfoFromOffset(size_t offset_chars, size_t* lineNo, size_t* lineoffChars, size_t* linelenChars, size_t* lineoffBytes, size_t* linelenBytes)
{
  size_t low = 0;
  size_t high = docBuffer.lineCount - 1;
  size_t line = 0;

  if (docBuffer.lineCount == 0)
  {
    if (lineNo != nullptr)          *lineNo = 0;
    if (lineoffChars != nullptr)    *lineoffChars = 0;
    if (linelenChars != nullptr)    *linelenChars = 0;
    if (lineoffBytes != nullptr)    *lineoffBytes = 0;
    if (linelenBytes != nullptr)    *linelenBytes = 0;

    return false;
  }
  
  /*while (low <= high)
  {
    line = (high + low) / 2;

    if (offset_chars >= charOffsetLineBuffer[line] && offset_chars < charOffsetLineBuffer[line + 1])
    {
      break;
    }
    else if (offset_chars < charOffsetLineBuffer[line])
    {
      high = line - 1;
    }
    else
    {
      low = line + 1;
    }
  }*/

  //if (lineNo != nullptr)	        *lineNo = line;
  //if (lineoffBytes != nullptr)	*lineoffBytes = byteOffsetLineBuffer[line];
  //if (linelenBytes != nullptr)	*linelenBytes = byteOffsetLineBuffer[line + 1] - byteOffsetLineBuffer[line];
  //if (lineoffChars != nullptr)	*lineoffChars = charOffsetLineBuffer[line];
  //if (linelenChars != nullptr)	*linelenChars = charOffsetLineBuffer[line + 1] - charOffsetLineBuffer[line];

  return true;
}

bool TextDocument::LineInfoFromLineNumber(size_t lineno, size_t* lineoffChars, size_t* linelenChars, size_t* lineoffBytes, size_t* linelenBytes)
{
  if (lineno < docBuffer.lineCount)
  {
    //if (linelenChars != nullptr) *linelenChars = charOffsetLineBuffer[lineno + 1] - charOffsetLineBuffer[lineno];
    //if (lineoffChars != nullptr) *lineoffChars = charOffsetLineBuffer[lineno];
    //if (linelenBytes != nullptr) *linelenBytes = byteOffsetLineBuffer[lineno + 1] - byteOffsetLineBuffer[lineno];
    //if (lineoffBytes != nullptr) *lineoffBytes = byteOffsetLineBuffer[lineno];

    return true;
  }
  else
  {
    return false;
  }
}


TextIterator TextDocument::IterateLineByLineNumber(size_t lineno, size_t* linestart, size_t* linelen)
{
  size_t offset_bytes;
  size_t length_bytes;

  if (!LineInfoFromLineNumber(lineno, linestart, linelen, &offset_bytes, &length_bytes))
    return TextIterator();

  return TextIterator(offset_bytes, length_bytes, this);
}

TextIterator TextDocument::IterateLineByOffset(size_t offset_chars, size_t* lineno, size_t* linestart)
{
  size_t offset_bytes = 0;
  size_t length_bytes = 0;

  if (!LineInfoFromOffset(offset_chars, lineno, linestart, nullptr, &offset_bytes, &length_bytes))
    return TextIterator();

  return TextIterator(offset_bytes, length_bytes, this);
}


size_t TextDocument::InsertText(size_t offsetChars, wchar_t* text, size_t length)
{
  const size_t offsetBytes = CharOffsetToByteOffset(offsetChars);
  return InsertTextRaw(offsetBytes, text, length);
}
size_t TextDocument::ReplaceText(size_t offsetChars, wchar_t* text, size_t length, size_t eraseLen)
{
  const size_t offsetBytes = CharOffsetToByteOffset(offsetChars);
  return ReplaceTextRaw(offsetBytes, text, length, eraseLen);
}
size_t TextDocument::EraseText(size_t offsetChars, size_t length)
{
  const size_t offsetBytes = CharOffsetToByteOffset(offsetChars);
  return EraseTextRaw(offsetBytes, length);
}

size_t TextDocument::InsertTextRaw(size_t offsetBytes, wchar_t* text, size_t textLength)
{
  unsigned char buf[LEN];
  size_t processedChars = 0, rawLen = 0, offset = offsetBytes, bufLen = LEN;
  while (textLength)
  {
    rawLen = bufLen;
    processedChars = UTF16ToRawData(text, textLength, buf, bufLen);

    docBuffer.InsertText(offset, std::vector<unsigned char>(buf, buf + processedChars));

    text += processedChars;
    textLength -= processedChars;
    rawLen += bufLen;
    offset += bufLen;
  }
  return rawLen;

}
size_t TextDocument::ReplaceTextRaw(size_t offsetBytes, wchar_t* text, size_t textLength, size_t eraseLen)
{
  
  unsigned char buf[LEN];
  size_t processedChars = 0, rawLen = 0, offset = offsetBytes, bufLen = LEN;
  size_t eraseBytes = CharOffsetToByteOffsetAt(offsetBytes, eraseLen);
  while (textLength)
  {
    rawLen = bufLen;
    processedChars = UTF16ToRawData(text, textLength, buf, bufLen);

    docBuffer.ReplaceText(offset, std::vector<unsigned char>(buf, buf + processedChars), eraseBytes);

    text += processedChars;
    textLength -= processedChars;
    rawLen += bufLen;
    offset += bufLen;
    eraseBytes = 0;
  }
  return rawLen;


}
size_t TextDocument::EraseTextRaw(size_t offsetBytes, size_t textLength)
{
  const size_t eraseBytes = CharOffsetToByteOffsetAt(offsetBytes, textLength);
  const size_t offset = offsetBytes;
  docBuffer.EraseText(offset, eraseBytes);
  return textLength;
}

size_t TextDocument::CharOffsetToByteOffsetAt(size_t offsetBytes, size_t charCount)
{
  switch (fileFormat)
  {
  case BCP_ASCII:
    return charCount;
  case BCP_UTF16:case BCP_UTF16BE:
    return charCount * sizeof(wchar_t);

  default:
    break;
  }
  // case UTF-8
  const size_t start = offsetBytes;
  while (charCount && offsetBytes < docBuffer.length)//todo: +headrsize ?
  {
    wchar_t buf[LEN];
    size_t charLen = (std::min)(charCount, LEN);//因为底层默认输入为UTF-16所以直接定义WCHAR数组
    const size_t byteLen = GetText(offsetBytes, docBuffer.length - offsetBytes, buf, charLen);
    charCount -= charLen;
    offsetBytes += byteLen;

  }
  return offsetBytes - start;

}

size_t TextDocument::CharOffsetToByteOffset(size_t offsetChars)
{
  switch (fileFormat)
  {
  case BCP_ASCII:
    return offsetChars;
  case BCP_UTF16:case BCP_UTF16BE:
    return offsetChars * sizeof(wchar_t);

  default:
    break;
  }
  // case UTF-8 ....
  size_t lineOffChars;
  size_t lineOffBytes;
  if (LineInfoFromOffset(offsetChars, nullptr, &lineOffChars, nullptr, &lineOffBytes, nullptr))
  {
    return CharOffsetToByteOffsetAt(lineOffBytes, offsetChars - lineOffChars)
      + lineOffBytes;
  }
  else
  {
    return 0;
  }
}
bool TextDocument::Clear()
{
  docBuffer.length = 0;
  docBuffer.lineCount = 0;
  docBuffer.buffers.clear();
  auto input = std::vector<unsigned char>();
  docBuffer.Init(input);
  return true;
}