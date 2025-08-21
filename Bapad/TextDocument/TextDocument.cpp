#include "pch.h"
#include "TextDocument.h"

constexpr size_t LEN = 0x100;

TextDocument::TextDocument() noexcept
  :
  docBuffer(),
  fileFormat(CP_TYPE::ANSI),
  headerSize(0)
{}
bool TextDocument::Initialize(const wchar_t* filename)
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
    throw e;
  }
  ifs.read(reinterpret_cast<char*>(&gsl::at(read_buffer, 0)), docLengthByBytes);
  if (!ifs)
  {
    return false;
  }

  // Detect file format
  fileFormat = DetectFileFormat(read_buffer, headerSize);
  read_buffer.erase(read_buffer.begin(), read_buffer.begin() + headerSize);
  auto utf16Text = RawToUtf16(read_buffer, fileFormat);
  docBuffer = std::move(PieceTree(utf16Text));

  ifs.close();
  return true;
}

size_t TextDocument::RawDataToUTF16(unsigned char* rawdata, size_t rawlen, wchar_t* utf16str, size_t& utf16len)
{
  switch (fileFormat)
  {
    // convert from ANSI->UNICODE
  case CP_TYPE::ANSI:

    return AsciiToUTF16(rawdata, rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len);

  case CP_TYPE::UTF8:
    return UTF8ToUTF16(rawdata, rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len);

    // already unicode, do a straight memory copy
  case CP_TYPE::UTF16:
    rawlen /= sizeof(wchar_t);
    return CopyUTF16(reinterpret_cast<UTF16*>(rawdata), rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len) * sizeof(wchar_t);

    // need to convert from big-endian to little-endian
  case CP_TYPE::UTF16BE:
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
  case CP_TYPE::ANSI:
    return UTF16ToAscii(reinterpret_cast<UTF16*>(utf16Str), utf16Len, rawData, rawLen);
  case CP_TYPE::UTF8:
    return UTF16ToUTF8(reinterpret_cast<UTF16*>(utf16Str), utf16Len, rawData, rawLen);
  case CP_TYPE::UTF16:
    rawLen /= sizeof(wchar_t);
    utf16Len = CopyUTF16(reinterpret_cast<UTF16*>(utf16Str), utf16Len, reinterpret_cast<UTF16*>(rawData), rawLen);
    rawLen *= sizeof(wchar_t);
    return utf16Len;
  case CP_TYPE::UTF16BE:
    rawLen /= sizeof(wchar_t);
    utf16Len = SwapUTF16(reinterpret_cast<UTF16*>(utf16Str), utf16Len, reinterpret_cast<UTF16*>(rawData), rawLen);
    rawLen *= sizeof(wchar_t);
    return utf16Len;
  default:
    rawLen = 0;
    return 0;
  }
}

CP_TYPE TextDocument::GetFileFormat() const noexcept
{
  return fileFormat;
}

const size_t TextDocument::GetLineCount() const noexcept
{
  if (docBuffer.length == 0)return 0;
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

TextIterator TextDocument::IterateLineByLineNumber(size_t lineno, size_t* linestart, size_t* linelen)
{
  size_t nodeOffset = 0;
  auto retVal = docBuffer.GetLine(lineno + 1, 0, &nodeOffset);
  const size_t lineStartCharOffset = IndexOffsetToCharOffset(nodeOffset),
    lineLengthCharOffset = IndexOffsetToCharOffset(nodeOffset + retVal.size()) - lineStartCharOffset;

  if (linestart) *linestart = lineStartCharOffset;
  if (linelen) *linelen = lineLengthCharOffset;
  if (retVal.empty())
    return TextIterator();
  return TextIterator(retVal, this);
}

TextIterator TextDocument::IterateLineByCharOffset(size_t offset_chars, size_t* lineno, size_t* linestart_char)
{
  if (offset_chars == 0) return TextIterator();
  const size_t bytes_offset = CharOffsetToIndexOffsetAt(0, offset_chars);

  const auto node_pos = docBuffer.GetNodePosition(bytes_offset);
  const TreeNode* node = node_pos.node;
  if (node == nullptr)
    return TextIterator();
  const size_t in_piece_offset = node_pos.in_piece_offset;
  auto& lineStarts = gsl::at(docBuffer.buffers, node->piece.bufferIndex).lineStarts;
  const auto& piece = node->piece;
  const size_t lineIndex_begin = piece.start.line, lineIndex_end = piece.end.line,
    lineIndex_pos = GetLineIndexFromNodePosistion(lineStarts, node_pos),
    bytes_offset_pos_line_start = node->lf_left +
    gsl::at(lineStarts, lineIndex_pos) + (gsl::at(lineStarts, lineIndex_begin) + piece.start.column);

  const size_t lineNumber = lineIndex_pos - lineIndex_begin + node->lf_left;
  size_t lineStartByteOffset = 0;
  auto lineContent = docBuffer.GetLine(lineNumber + 1, 0, &lineStartByteOffset);
  if (lineno) *lineno = lineNumber;
  if (linestart_char) *linestart_char = IndexOffsetToCharOffset(lineStartByteOffset);


  return TextIterator(lineContent, this);
}

size_t TextDocument::InsertText(size_t offsetChars, wchar_t* text, size_t length)
{
  const size_t offsetBytes = CharOffsetToIndexOffsetAt(0, offsetChars);
  const gsl::span<wchar_t> textSpan(text, length);
  std::vector<wchar_t> textVec(textSpan.begin(), textSpan.end());
  if (length)
  {
    docBuffer.InsertText(offsetBytes, textVec);
    return textVec.size();
  }
  return 0;
}

size_t TextDocument::ReplaceText(size_t offsetChars, wchar_t* text, size_t length, size_t eraseLen)
{
  const size_t offsetBytes = CharOffsetToIndexOffsetAt(0, offsetChars);
  const size_t eraseBytes = CharOffsetToIndexOffsetAt(offsetBytes, eraseLen);
  const gsl::span<wchar_t> textSpan(text, length);
  std::vector<wchar_t> textVec(textSpan.begin(), textSpan.end());
  if (length)
  {
    docBuffer.ReplaceText(offsetBytes, textVec, eraseBytes);
    return textVec.size();
  }
  return 0;
}

size_t TextDocument::EraseText(size_t offsetChars, size_t length)
{
  const size_t offset = CharOffsetToIndexOffsetAt(0, offsetChars);
  const size_t eraseLength = CharOffsetToIndexOffsetAt(offset, length);
  if (docBuffer.EraseText(offset, eraseLength))
  {
    return length;
  }
  return 0;
}

size_t TextDocument::CharOffsetToIndexOffsetAt(const size_t startOffset, const size_t charCount) noexcept
{
  // todo: utf16 char maybe 1 or 2 wchar_t long.
  return charCount;
}
size_t TextDocument::IndexOffsetToCharOffset(size_t offset) noexcept
{
  // todo: utf16 char maybe 1 or 2 wchar_t long.
  return offset;
}
size_t TextDocument::CountByteAnsi(const size_t startByteOffset, const size_t charCount) noexcept
{
  std::vector<unsigned char> rawText; //docBuffer.GetText(startByteOffset, docBuffer.length - startByteOffset); // todo: iterate text piece by piece
  const gsl::span<unsigned char> textSpan(rawText);
  size_t currentCharCount = 0;
  size_t byteOffset = 0;
  while (currentCharCount < charCount)
  {
    const size_t charSize = IsDBCSLeadByte(textSpan[byteOffset]) ? 2 : 1;
    byteOffset += charSize;
    ++currentCharCount;
  }
  return byteOffset;
}
size_t TextDocument::CountByteUtf8(const size_t startByteOffset, const size_t charCount) noexcept
{
  std::vector<unsigned char> rawText;// = docBuffer.GetText(startByteOffset, docBuffer.length - startByteOffset); // todo: iterate text piece by piece
  const gsl::span<unsigned char> textSpan(rawText);
  size_t currentCharCount = 0;
  size_t byteOffset = 0;
  while (currentCharCount < charCount)
  {
    const size_t charSize = GetUtf8CharSize(textSpan[byteOffset]);
    byteOffset += charSize;
    ++currentCharCount;
  }
  return byteOffset;
}
size_t TextDocument::CountByte(const size_t startByteOffset, const size_t charCount) noexcept
{
  size_t byteCount = 0;
  switch (fileFormat)
  {
  case CP_TYPE::ANSI:
    byteCount = CountByteAnsi(startByteOffset, charCount);
    break;
  case CP_TYPE::UTF8:
    byteCount = CountByteUtf8(startByteOffset, charCount);
    break;
  default:
    break;
  }
  return byteCount;
}
size_t TextDocument::CountCharAnsi(const size_t byteLength) noexcept
{
  std::vector<unsigned char> rawText;// = docBuffer.GetText(0, byteLength);
  const gsl::span<unsigned char> textSpan(rawText);
  size_t charCount = 0;
  size_t byteOffset = 0;
  while (byteOffset < byteLength)
  {
    const size_t charSize = IsDBCSLeadByte(textSpan[byteOffset]) ? 2 : 1;
    byteOffset += charSize;
    ++charCount;
  }
  return charCount;
}
size_t TextDocument::CountCharUtf8(const size_t byteLength) noexcept
{
  std::vector<unsigned char> rawText;// = docBuffer.GetText(0, byteLength);
  const gsl::span<unsigned char> textSpan(rawText);
  size_t charCount = 0;
  size_t byteOffset = 0;
  while (byteOffset < byteLength)
  {
    const size_t charSize = GetUtf8CharSize(textSpan[byteOffset]);
    byteOffset += charSize;
    ++charCount;
  }
  return charCount;
}
size_t TextDocument::CountChar(const size_t byteLength) noexcept
{
  size_t charCount = 0;
  switch (fileFormat)
  {
  case CP_TYPE::ANSI:
    charCount = CountCharAnsi(byteLength);
    break;
  case CP_TYPE::UTF8:
    charCount = CountCharUtf8(byteLength);
    break;
  default:
    break;
  }
  return charCount;
}
bool TextDocument::Clear()
{
  docBuffer.length = 0;
  docBuffer.lineCount = 1;
  docBuffer.buffers.clear();
  auto input = std::vector<wchar_t>();
  docBuffer.Init(input);
  return true;
}

size_t TextDocument::LineNumFromCharOffset(size_t offset)
{
  const auto indexOffset = CharOffsetToIndexOffsetAt(0, offset);
  const auto nodePos = docBuffer.GetNodePosition(indexOffset);
  const auto lineIndex = GetLineIndexFromNodePosistion(gsl::at(docBuffer.buffers, nodePos.node->piece.bufferIndex).lineStarts, nodePos);
  const auto lineNumber = nodePos.node->lf_left + (nodePos.node->piece.start.line - lineIndex);
  return lineNumber;
}

constexpr size_t GetUtf8CharSize(const unsigned char ch) noexcept
{
  const uint8_t byte = ch;
  if ((byte & 0x80) == 0) return 1;       // 0xxxxxxx
  if ((byte & 0xE0) == 0xC0) return 2;    // 110xxxxx
  if ((byte & 0xF0) == 0xE0) return 3;    // 1110xxxx
  if ((byte & 0xF8) == 0xF0) return 4;    // 11110xxx
  return 1;
}
