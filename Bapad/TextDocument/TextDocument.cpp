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
  ifs.read(reinterpret_cast<char*>(&gsl::at(read_buffer, 0)), docLengthByBytes);
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

TextIterator TextDocument::IterateLineByLineNumber(size_t lineno, size_t* linestart, size_t* linelen)
{
  size_t offset_bytes;
  size_t length_bytes;
  size_t nodeOffset = 0;
  auto retVal = docBuffer.GetLine(lineno + 1, 0, &nodeOffset);
  const size_t lineStartCharOffset = ByteOffsetToCharOffset(nodeOffset),
    lineLengthCharOffset = ByteOffsetToCharOffset(nodeOffset + retVal.size()) - lineStartCharOffset;

  if (linestart) *linestart = lineStartCharOffset;
  if (linelen) *linelen = lineLengthCharOffset;
  if (retVal.empty())
    return TextIterator();
  return TextIterator(retVal, this, 0);
}

TextIterator TextDocument::IterateLineByCharOffset(size_t offset_chars, size_t* lineno, size_t* linestart_char)
{
  if (offset_chars == 0) return TextIterator();
  const size_t bytes_offset = CharOffsetToByteOffsetAt(0, offset_chars);

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
    gsl::at(lineStarts, lineIndex_pos) - (gsl::at(lineStarts, lineIndex_begin) + piece.start.column);

  const size_t lineNumber = lineIndex_pos - lineIndex_begin + node->lf_left;
  if (lineno) *lineno = lineNumber;
  if (linestart_char) *linestart_char = ByteOffsetToCharOffset(bytes_offset_pos_line_start);

  auto lineContent = docBuffer.GetLine(lineNumber + 1);
  return TextIterator(lineContent, this, 0);
}


size_t TextDocument::InsertText(size_t offsetChars, wchar_t* text, size_t length)
{
  const size_t offsetBytes = CharOffsetToByteOffsetAt(0, offsetChars);
  return InsertTextRaw(offsetBytes, text, length);
}
size_t TextDocument::ReplaceText(size_t offsetChars, wchar_t* text, size_t length, size_t eraseLen)
{
  const size_t offsetBytes = CharOffsetToByteOffsetAt(0, offsetChars);
  return ReplaceTextRaw(offsetBytes, text, length, eraseLen);
}
size_t TextDocument::EraseText(size_t offsetChars, size_t length)
{
  const size_t offsetBytes = CharOffsetToByteOffsetAt(0, offsetChars);
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
  if (docBuffer.EraseText(offsetBytes, eraseBytes))
  {
    return textLength;
  }
  return 0;
}

size_t TextDocument::CharOffsetToByteOffsetAt(const size_t startOffsetBytes, const size_t charCount)
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
  size_t offsetBytes = startOffsetBytes;
  size_t offsetChars = charCount;
  wchar_t buf[LEN];
  while (charCount && startOffsetBytes < docBuffer.length)
  {
    size_t charLen = (std::min)(charCount, LEN);
    const size_t byteLen = GetText(startOffsetBytes, docBuffer.length - startOffsetBytes, buf, charLen);
    offsetChars -= charLen;
    offsetBytes += byteLen;

  }
  return offsetBytes - startOffsetBytes;

}
size_t TextDocument::ByteOffsetToCharOffset(size_t offsetBytes)
{
  switch (fileFormat)
  {
  case BCP_ASCII:
    return offsetBytes;

  case BCP_UTF16:
  case BCP_UTF16BE:
    return offsetBytes / sizeof(wchar_t);

  case BCP_UTF8:
  case BCP_UTF32:
  case BCP_UTF32BE:
    // bug! need to implement this. 
  default:
    break;
  }

  return 0;

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

size_t TextDocument::LineNumFromCharOffset(size_t offset)
{
  const auto byteOffset = CharOffsetToByteOffsetAt(0, offset);
  const auto nodePos = docBuffer.GetNodePosition(byteOffset);
  const auto lineIndex = GetLineIndexFromNodePosistion(gsl::at(docBuffer.buffers, nodePos.node->piece.bufferIndex).lineStarts, nodePos);
  const auto lineNumber = nodePos.node->lf_left + (nodePos.node->piece.start.line - lineIndex);
  return lineNumber;
}
