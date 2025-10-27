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
  return docBuffer.getLongestLine();//size_t{100};
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

TextIterator TextDocument::IterateLineByCharOffset(size_t charOffset, size_t* lineno, size_t* linestartCharOffset)
{
  //if (charOffset == 0) return TextIterator();
  const size_t indexOffset = CharOffsetToIndexOffsetAt(0, charOffset);

  const auto node_pos = docBuffer.GetNodePosition(indexOffset);
  const TreeNode* node = node_pos.node;
  if (node == nullptr)
    return TextIterator();
  const auto& piece = node->piece;
  const auto& lineStarts = gsl::at(docBuffer.buffers, piece.bufferIndex).lineStarts;
  const size_t lineIndexBegin = piece.start.line,
    lineIndexPos = GetLineIndexFromNodePosistion(lineStarts, node_pos);

  const size_t lineNumber = lineIndexPos - lineIndexBegin + node->lf_left;
  size_t lineStartIndexOffset = 0;
  auto lineContent = docBuffer.GetLine(lineNumber + 1, 0, &lineStartIndexOffset);
  if (lineno) *lineno = lineNumber;
  if (linestartCharOffset) *linestartCharOffset = IndexOffsetToCharOffset(lineStartIndexOffset);


  return TextIterator(lineContent, this);
}

size_t TextDocument::InsertText(size_t offsetChars, wchar_t* text, size_t length)
{
  const size_t indexOffset = CharOffsetToIndexOffsetAt(0, offsetChars);
  const gsl::span<wchar_t> textSpan(text, length);
  std::vector<wchar_t> textVec(textSpan.begin(), textSpan.end());
  if (length && docBuffer.InsertText(indexOffset, textVec))
  {
    EditAction action;
    action.actionOffsetBytes = indexOffset;
    action.actionType = ActionType::ActionInsert;
    action.insertedText = (textVec);//std::move
    undoStack.push(action);
    return textVec.size();
  }
  return 0;
}

size_t TextDocument::ReplaceText(size_t offsetChars, wchar_t* text, size_t length, size_t eraseLen)
{
  const size_t indexOffset = CharOffsetToIndexOffsetAt(0, offsetChars);
  const size_t eraseBytes = CharOffsetToIndexOffsetAt(indexOffset, eraseLen);
  const gsl::span<wchar_t> textSpan(text, length);
  std::vector<wchar_t> textVec(textSpan.begin(), textSpan.end());
  auto erasedText = docBuffer.GetText(indexOffset, eraseBytes);
  if (length && docBuffer.ReplaceText(indexOffset, textVec, eraseBytes))
  {
    EditAction action;
    action.actionOffsetBytes = indexOffset;
    action.actionType = ActionType::ActionReplace;
    action.insertedText = (textVec);
    action.erasedText = (erasedText);
    undoStack.push(action);
    return textVec.size();
  }
  return 0;
}

size_t TextDocument::EraseText(size_t offsetChars, size_t length)
{
  const size_t offset = CharOffsetToIndexOffsetAt(0, offsetChars);
  const size_t eraseLength = CharOffsetToIndexOffsetAt(offset, length);
  auto erasedText = docBuffer.GetText(offset, eraseLength);
  if (docBuffer.EraseText(offset, eraseLength))
  {
    EditAction action;
    action.actionOffsetBytes = offset;
    action.actionType = ActionType::ActionErase;
    action.erasedText = (erasedText);
    undoStack.push(action);
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
int TextDocument::DoCommand(EditAction action, std::stack<EditAction>& record)
{
  int newCursorOffset = -1;
  switch (action.actionType)
  {
  case ActionType::ActionInsert:
    docBuffer.EraseText(action.actionOffsetBytes, action.insertedText.size());
    action.actionType = ActionType::ActionErase;
    break;
  case ActionType::ActionErase:
    docBuffer.InsertText(action.actionOffsetBytes, action.erasedText);
    action.actionType = ActionType::ActionInsert;
    break;
  case ActionType::ActionReplace:
    docBuffer.ReplaceText(action.actionOffsetBytes, action.erasedText, action.insertedText.size());
    action.actionType = ActionType::ActionReplace;
    break;
  case ActionType::ActionInvalid:
    throw;
  default:
    throw;
  }
  newCursorOffset = action.actionOffsetBytes + action.erasedText.size();
  std::swap(action.erasedText, action.insertedText);
  record.push(action);
  return newCursorOffset;
}
bool TextDocument::Clear()
{
  docBuffer._lastChangeBufferPos = BufferPosition(0, 0);
  docBuffer.length = 0;
  docBuffer.lineCount = 1;
  docBuffer.buffers.clear();
  auto input = std::vector<wchar_t>();
  docBuffer.Init(input);
  std::stack<EditAction>().swap(undoStack);
  std::stack<EditAction>().swap(redoStack);
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

bool TextDocument::CanUndo() const noexcept
{
  return undoStack.size() != 0;
}

bool TextDocument::CanRedo() const noexcept
{
  return redoStack.size() != 0;
}

int TextDocument::Undo()
{
  if (!CanUndo())
    return -1;
  EditAction action = undoStack.top();
  undoStack.pop();
  return DoCommand(action, redoStack);
}

int TextDocument::Redo()
{
  if (!CanRedo())
    return -1;
  EditAction action = redoStack.top();
  redoStack.pop();
  return DoCommand(action, undoStack);
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
