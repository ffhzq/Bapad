#ifndef TEXTDOCUMENT_TEXTDOCUMENT_H_
#define TEXTDOCUMENT_TEXTDOCUMENT_H_

#include "FormatConversionV2.h"
#include "PieceTree.h"

constexpr size_t GetUtf8CharSize(const char ch) noexcept;

class TextIterator;
enum class ActionType {
  ActionInvalid,
  ActionInsert,
  ActionErase,
  ActionReplace
};
struct EditAction {
  std::vector<char16_t> insertedText;
  std::vector<char16_t> erasedText;
  size_t actionOffsetBytes; // OffsetBytes in piece.
  ActionType actionType;
};

class TextDocument {
  friend class TextIterator;
public:
  TextDocument() noexcept;

  bool Initialize(const wchar_t* filename);

  bool Clear();
  size_t LineNumFromCharOffset(size_t offset);

  TextIterator IterateLineByLineNumber(size_t lineno, size_t* linestartCharOffset, size_t* lineLengthCharOffset);
  TextIterator IterateLineByCharOffset(size_t charOffset, size_t* lineno, size_t* linestartCharOffset);

  size_t  InsertText(size_t offsetChars, std::vector<char16_t> text);
  size_t  ReplaceText(size_t offsetChars, std::vector<char16_t> text, size_t eraseLen);
  size_t  EraseText(size_t offsetChars, size_t length);

  bool CanUndo() const noexcept;
  bool CanRedo() const noexcept;
  int Undo();
  int Redo();

  CP_TYPE GetFileFormat() const noexcept;
  const size_t GetLineCount() const noexcept;
  const size_t GetLongestLine(int tabwidth) const noexcept;
  const size_t GetDocLength() const noexcept;

private:

  size_t CharOffsetToIndexOffsetAt(const size_t startOffset, const size_t charCount) noexcept;
  size_t IndexOffsetToCharOffset(size_t offset) noexcept;

  int DoCommand(EditAction action, std::stack<EditAction> & record);

  PieceTree docBuffer;// raw txt data
  std::stack<EditAction> undoStack;
  std::stack<EditAction> redoStack;
  CP_TYPE fileFormat;
  int  headerSize;

};


class TextIterator {
private:
  std::vector<char16_t> lineContent;
  TextDocument* textDoc;
public:
  TextIterator() noexcept :lineContent(), textDoc(nullptr)
  {}
  TextIterator(const std::vector<char16_t>& lineContent, TextDocument* textDoc)
    : lineContent(lineContent), textDoc(textDoc)
  {}
  ~TextIterator() noexcept = default;
  TextIterator(const TextIterator&) = default;
  TextIterator& operator=(const TextIterator&) = default;
  TextIterator(TextIterator&&) = default;
  TextIterator& operator=(TextIterator&&) = default;

  std::vector<char16_t> GetLine()
  {
    if (textDoc)
    {
      return lineContent;
    }
    return std::vector<char16_t>();
  }
  operator bool() noexcept
  {
    return textDoc != nullptr ? true : false;
  }
};

#endif
