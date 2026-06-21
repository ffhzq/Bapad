#ifndef TEXTDOCUMENT_TEXTDOCUMENT_H_
#define TEXTDOCUMENT_TEXTDOCUMENT_H_

#include <stack>
#include <vector>

#include "PieceTree.h"

constexpr size_t GetUtf8CharSize(const char ch) noexcept;

class TextIterator;

/// Type of an edit action tracked by the undo/redo system.
enum class ActionType {
  ActionInvalid,
  ActionInsert,
  ActionErase,
  ActionReplace
};

/// A single edit action recorded for undo/redo.
struct EditAction {
  std::vector<char16_t> insertedText;  ///< Text that was inserted.
  std::vector<char16_t> erasedText;    ///< Text that was erased.
  size_t actionOffsetBytes;            ///< Offset (in chars) where the action occurred.
  ActionType actionType;
};

/// High-level text document API backed by a PieceTable.
///
/// Provides insert/erase/replace operations with undo/redo, line-based
/// iteration, and offset-to-line mapping. The underlying storage is a
/// PieceTree, which stores edits efficiently as piece references instead
/// of copying data.
class TextDocument {
  friend class TextIterator;

 public:
  TextDocument() noexcept;

  /// Load content into the document. Replaces any existing content.
  /// Line endings are normalized (CR, CRLF -> LF) during initialization.
  bool Initialize(const std::vector<char16_t> utf16Content);

  /// Clear all content and reset undo/redo stacks.
  bool Clear();

  /// @return 0-based line number for a given character offset.
  size_t LineNumFromCharOffset(size_t offset);

  /// Iterate over a line by its 0-based line number.
  /// @param lineno              0-based line number.
  /// @param linestartCharOffset [out] Character offset of the line start.
  /// @param lineLengthCharOffset[out] Length of the line in characters.
  /// @return TextIterator containing the line (with trailing \\n, if any).
  ///         A default-constructed (false) iterator if the line is empty or
  ///         out of range.
  TextIterator IterateLineByLineNumber(size_t lineno,
                                       size_t* linestartCharOffset,
                                       size_t* lineLengthCharOffset);

  /// Iterate over the line that contains a given character offset.
  /// @param charOffset          Character offset to locate.
  /// @param lineno              [out] 0-based line number.
  /// @param linestartCharOffset [out] Character offset of the line start.
  /// @return TextIterator containing the line (with trailing \\n, if any).
  TextIterator IterateLineByCharOffset(size_t charOffset, size_t* lineno,
                                       size_t* linestartCharOffset);

  /// Retrieve a substring of the document content.
  std::vector<char16_t> GetText(size_t offset, size_t len);

  /// Insert text at a given offset.
  /// @return Number of characters inserted (0 if input was empty).
  ///         Records the action on the undo stack.
  size_t InsertText(size_t offsetChars, std::vector<char16_t> text);

  /// Replace a range of text with new text (erase then insert).
  /// @return Number of characters inserted (0 if input was empty).
  ///         Records the action on the undo stack.
  size_t ReplaceText(size_t offsetChars, std::vector<char16_t> text,
                     size_t eraseLen);

  /// Erase a range of text.
  /// @return Length erased (0 if no-op).
  ///         Records the action on the undo stack.
  size_t EraseText(size_t offsetChars, size_t length);

  bool CanUndo() const noexcept;
  bool CanRedo() const noexcept;

  /// Undo the last edit action.
  /// @return New cursor offset, or -1 if undo stack is empty.
  int Undo();

  /// Redo the last undone action.
  /// @return New cursor offset, or -1 if redo stack is empty.
  int Redo();

  const size_t GetLineCount() const noexcept;
  const size_t GetLongestLine(int tabwidth) const noexcept;
  const size_t GetDocLength() const noexcept;

 private:
  // Note: CharOffsetToIndexOffsetAt and IndexOffsetToCharOffset are stubs
  // that assume one char16_t == one character (no surrogate pair support yet).
  size_t CharOffsetToIndexOffsetAt(const size_t startOffset,
                                   const size_t charCount) noexcept;
  size_t IndexOffsetToCharOffset(size_t offset) noexcept;

  int DoCommand(EditAction action, std::stack<EditAction>& record);

  PieceTree docBuffer;
  std::stack<EditAction> undoStack;
  std::stack<EditAction> redoStack;
};

/// Lightweight line iterator returned by TextDocument.
/// Holds a copy of the line content. Evaluate as bool to check validity.
class TextIterator {
 private:
  std::vector<char16_t> lineContent;
  TextDocument* textDoc;

 public:
  TextIterator() noexcept : lineContent(), textDoc(nullptr) {}
  TextIterator(const std::vector<char16_t>& lineContent, TextDocument* textDoc)
      : lineContent(lineContent), textDoc(textDoc) {}
  ~TextIterator() noexcept = default;
  TextIterator(const TextIterator&) = default;
  TextIterator& operator=(const TextIterator&) = default;
  TextIterator(TextIterator&&) = default;
  TextIterator& operator=(TextIterator&&) = default;

  /// @return A copy of the line content (includes trailing \\n if present).
  std::vector<char16_t> GetLine() {
    if (textDoc) {
      return lineContent;
    }
    return std::vector<char16_t>();
  }

  /// @c true if the iterator points to a valid line.
  operator bool() noexcept { return textDoc != nullptr ? true : false; }
};

#endif
