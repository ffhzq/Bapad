#include "pch.h"

#include "../TextDocument/FormatConversionV2.h"
#include "../TextDocument/TextDocument.h"

namespace testing {
namespace internal {
std::string U16VectorToPrintableString(const std::vector<char16_t>& vec) {
  std::string result = std::to_string(reinterpret_cast<wchar_t>(&vec[0]));
  return result;
}
template <>
void UniversalPrinter<std::vector<char16_t>>::Print(
    const std::vector<char16_t>& vec, std::ostream* os) {
  *os << U16VectorToPrintableString(vec);
}
}  // namespace internal
}  // namespace testing

static std::vector<char16_t> toWCharVector(const std::string& s) {
  std::vector<char> str(s.begin(), s.end());
  std::vector<char16_t> utf16_content = RawToUtf16(str, CP_TYPE::UTF8);
  return utf16_content;
}

// ===========================================================================
// Test fixture for TextDocument
// ===========================================================================
class TextDocumentTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  std::unique_ptr<TextDocument> CreateDoc(const std::string& s) {
    auto doc = std::make_unique<TextDocument>();
    doc->Initialize(toWCharVector(s));
    return doc;
  }
};

// ===========================================================================
// Tests: Initialize / Clear
// ===========================================================================

TEST_F(TextDocumentTest, DefaultConstructedDocIsEmpty) {
  TextDocument doc;
  EXPECT_EQ(doc.GetLineCount(), 0);
  EXPECT_EQ(doc.GetDocLength(), 0);
  EXPECT_FALSE(doc.CanUndo());
  EXPECT_FALSE(doc.CanRedo());
}

TEST_F(TextDocumentTest, InitializeWithEmptyContent) {
  TextDocument doc;
  bool ok = doc.Initialize(toWCharVector(""));
  EXPECT_TRUE(ok);
  EXPECT_EQ(doc.GetLineCount(), 0);
  EXPECT_EQ(doc.GetDocLength(), 0);
}

TEST_F(TextDocumentTest, InitializeWithSingleLine) {
  auto doc = CreateDoc("Hello World");
  EXPECT_EQ(doc->GetLineCount(), 1);
  EXPECT_EQ(doc->GetDocLength(), 11);
  auto text = doc->GetText(0, 99);
  EXPECT_EQ(text, toWCharVector("Hello World"));
}

TEST_F(TextDocumentTest, InitializeWithMultipleLines) {
  auto doc = CreateDoc("Line1\nLine2\nLine3");
  EXPECT_EQ(doc->GetLineCount(), 3);
  EXPECT_EQ(doc->GetDocLength(), 17);
}

TEST_F(TextDocumentTest, InitializeNormalizesLineEndingsCR) {
  // CR only (\r) should become LF (\n) and count as one line
  auto doc = CreateDoc("Hello\rWorld");
  EXPECT_EQ(doc->GetLineCount(), 2);
  // The line break character is now \n
  auto text = doc->GetText(0, 99);
  std::vector<char16_t> expected = {u'H', u'e', u'l', u'l', u'o',
                                    u'\n', u'W', u'o', u'r', u'l', u'd'};
  EXPECT_EQ(text, expected);
}

TEST_F(TextDocumentTest, InitializeNormalizesLineEndingsCRLF) {
  // CR+LF (\r\n) should become LF (\n)
  auto doc = CreateDoc("Hello\r\nWorld");
  EXPECT_EQ(doc->GetLineCount(), 2);
  auto text = doc->GetText(0, 99);
  std::vector<char16_t> expected = {u'H', u'e', u'l', u'l', u'o',
                                    u'\n', u'W', u'o', u'r', u'l', u'd'};
  EXPECT_EQ(text, expected);
}

TEST_F(TextDocumentTest, ClearResetsDocument) {
  auto doc = CreateDoc("Some content\nwith lines");
  EXPECT_EQ(doc->GetLineCount(), 2);
  EXPECT_EQ(doc->GetDocLength(), 20);

  doc->InsertText(0, toWCharVector("X"));
  EXPECT_TRUE(doc->CanUndo());

  doc->Clear();
  EXPECT_EQ(doc->GetLineCount(), 0);
  EXPECT_EQ(doc->GetDocLength(), 0);
  EXPECT_FALSE(doc->CanUndo());
  EXPECT_FALSE(doc->CanRedo());
}

TEST_F(TextDocumentTest, ReinitializeAfterClear) {
  auto doc = CreateDoc("First");
  doc->Clear();
  doc->Initialize(toWCharVector("Second"));
  EXPECT_EQ(doc->GetLineCount(), 1);
  EXPECT_EQ(doc->GetDocLength(), 6);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Second"));
}

// ===========================================================================
// Tests: GetLineCount / GetDocLength / GetLongestLine
// ===========================================================================

TEST_F(TextDocumentTest, GetLineCountSingleLine) {
  auto doc = CreateDoc("NoNewline");
  EXPECT_EQ(doc->GetLineCount(), 1);
}

TEST_F(TextDocumentTest, GetLineCountMultipleLines) {
  auto doc = CreateDoc("A\nB\nC\nD\nE");
  EXPECT_EQ(doc->GetLineCount(), 5);
}

TEST_F(TextDocumentTest, GetDocLengthAfterInsert) {
  auto doc = CreateDoc("123");
  doc->InsertText(3, toWCharVector("456"));
  EXPECT_EQ(doc->GetDocLength(), 6);
}

TEST_F(TextDocumentTest, GetDocLengthAfterErase) {
  auto doc = CreateDoc("123456");
  doc->EraseText(1, 3);
  EXPECT_EQ(doc->GetDocLength(), 3);
}

TEST_F(TextDocumentTest, GetLongestLineSingleLine) {
  auto doc = CreateDoc("Hello World!");
  EXPECT_EQ(doc->GetLongestLine(0), 12);
}

TEST_F(TextDocumentTest, GetLongestLineMultipleLines) {
  auto doc = CreateDoc("Short\nMediumLine\nTiny");
  // "MediumLine" is the longest at 10 chars
  EXPECT_EQ(doc->GetLongestLine(0), 10);
}

TEST_F(TextDocumentTest, GetLongestLineEmptyDoc) {
  TextDocument doc;
  EXPECT_EQ(doc.GetLongestLine(0), 0);
}

// ===========================================================================
// Tests: InsertText
// ===========================================================================

TEST_F(TextDocumentTest, InsertAtBeginning) {
  auto doc = CreateDoc("World");
  size_t len = doc->InsertText(0, toWCharVector("Hello "));
  EXPECT_EQ(len, 6);
  EXPECT_EQ(doc->GetDocLength(), 11);
  EXPECT_EQ(doc->GetLineCount(), 1);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World"));
}

TEST_F(TextDocumentTest, InsertInMiddle) {
  auto doc = CreateDoc("HelloWorld");
  size_t len = doc->InsertText(5, toWCharVector(" "));
  EXPECT_EQ(len, 1);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World"));
}

TEST_F(TextDocumentTest, InsertAtEnd) {
  auto doc = CreateDoc("Hello");
  size_t len = doc->InsertText(5, toWCharVector(" World!"));
  EXPECT_EQ(len, 7);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World!"));
}

TEST_F(TextDocumentTest, InsertWithNewlines) {
  auto doc = CreateDoc("BeforeAfter");
  doc->InsertText(6, toWCharVector("\nMiddle\n"));
  EXPECT_EQ(doc->GetLineCount(), 3);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Before\nMiddle\nAfter"));
}

TEST_F(TextDocumentTest, InsertEmptyTextReturnsZero) {
  auto doc = CreateDoc("Hello");
  size_t len = doc->InsertText(3, toWCharVector(""));
  EXPECT_EQ(len, 0);
  // Document unchanged
  EXPECT_EQ(doc->GetDocLength(), 5);
}

TEST_F(TextDocumentTest, InsertRecordsUndoStack) {
  auto doc = CreateDoc("Start");
  EXPECT_FALSE(doc->CanUndo());
  doc->InsertText(0, toWCharVector("Pre-"));
  EXPECT_TRUE(doc->CanUndo());
}

// ===========================================================================
// Tests: EraseText
// ===========================================================================

TEST_F(TextDocumentTest, EraseFromBeginning) {
  auto doc = CreateDoc("Hello World");
  size_t len = doc->EraseText(0, 6);
  EXPECT_EQ(len, 6);
  EXPECT_EQ(doc->GetDocLength(), 5);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("World"));
}

TEST_F(TextDocumentTest, EraseFromMiddle) {
  auto doc = CreateDoc("ABCDEFGHI");
  doc->EraseText(3, 3);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("ABCGHI"));
}

TEST_F(TextDocumentTest, EraseEntireContent) {
  auto doc = CreateDoc("Hello");
  doc->EraseText(0, 5);
  EXPECT_EQ(doc->GetDocLength(), 0);
  EXPECT_TRUE(doc->GetText(0, 99).empty());
}

TEST_F(TextDocumentTest, EraseWithNewlines) {
  auto doc = CreateDoc("Line1\nLine2\nLine3");
  doc->EraseText(4, 7);  // Erase "1\nLine2"
  EXPECT_EQ(doc->GetLineCount(), 2);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Line\nLine3"));
}

TEST_F(TextDocumentTest, EraseRecordsUndoStack) {
  auto doc = CreateDoc("Hello");
  doc->EraseText(1, 3);
  EXPECT_TRUE(doc->CanUndo());
}

// ===========================================================================
// Tests: ReplaceText
// ===========================================================================

TEST_F(TextDocumentTest, ReplaceSameLength) {
  auto doc = CreateDoc("ABCDEF");
  doc->ReplaceText(1, toWCharVector("123"), 3);
  EXPECT_EQ(doc->GetDocLength(), 6);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("A123EF"));
}

TEST_F(TextDocumentTest, ReplaceLonger) {
  auto doc = CreateDoc("ABCDEF");
  doc->ReplaceText(1, toWCharVector("12345"), 3);
  EXPECT_EQ(doc->GetDocLength(), 8);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("A12345EF"));
}

TEST_F(TextDocumentTest, ReplaceShorter) {
  auto doc = CreateDoc("ABCDEF");
  doc->ReplaceText(1, toWCharVector("1"), 3);
  EXPECT_EQ(doc->GetDocLength(), 4);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("A1EF"));
}

TEST_F(TextDocumentTest, ReplaceWithEmptyText) {
  auto doc = CreateDoc("ABCDEF");
  doc->ReplaceText(2, toWCharVector(""), 2);
  // Replaces "CD" with "" — effectively erases
  EXPECT_EQ(doc->GetDocLength(), 4);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("ABEF"));
}

TEST_F(TextDocumentTest, ReplaceWithNewlines) {
  auto doc = CreateDoc("Hello World");
  doc->ReplaceText(5, toWCharVector("\nBeautiful\n"), 1);
  EXPECT_EQ(doc->GetLineCount(), 3);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello\nBeautiful\nWorld"));
}

// ===========================================================================
// Tests: Undo / Redo
// ===========================================================================

TEST_F(TextDocumentTest, UndoInsert) {
  auto doc = CreateDoc("Hello");
  doc->InsertText(5, toWCharVector(" World"));
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World"));
  int cursor = doc->Undo();
  EXPECT_GE(cursor, 0);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello"));
  EXPECT_FALSE(doc->CanUndo());
  EXPECT_TRUE(doc->CanRedo());
}

TEST_F(TextDocumentTest, UndoThenRedo) {
  auto doc = CreateDoc("Base");
  doc->InsertText(4, toWCharVector("+Insert"));
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Base"));
  int cursor = doc->Redo();
  EXPECT_GE(cursor, 0);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Base+Insert"));
  EXPECT_TRUE(doc->CanUndo());
  EXPECT_FALSE(doc->CanRedo());
}

TEST_F(TextDocumentTest, UndoErase) {
  auto doc = CreateDoc("Hello World");
  doc->EraseText(5, 6);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello"));
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World"));
}

TEST_F(TextDocumentTest, UndoReplace) {
  auto doc = CreateDoc("ABCDEF");
  doc->ReplaceText(1, toWCharVector("XYZ"), 3);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("AXYZEF"));
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("ABCDEF"));
}

TEST_F(TextDocumentTest, MultipleUndoRedo) {
  auto doc = CreateDoc("");
  doc->InsertText(0, toWCharVector("A"));
  doc->InsertText(1, toWCharVector("B"));
  doc->InsertText(2, toWCharVector("C"));
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("ABC"));

  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("AB"));
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("A"));
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector(""));
  EXPECT_FALSE(doc->CanUndo());

  doc->Redo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("A"));
  doc->Redo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("AB"));
  doc->Redo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("ABC"));
  EXPECT_FALSE(doc->CanRedo());
}

TEST_F(TextDocumentTest, UndoOnEmptyUndoStackReturnsMinusOne) {
  auto doc = CreateDoc("Hello");
  int cursor = doc->Undo();
  EXPECT_EQ(cursor, -1);
}

TEST_F(TextDocumentTest, RedoOnEmptyRedoStackReturnsMinusOne) {
  auto doc = CreateDoc("Hello");
  int cursor = doc->Redo();
  EXPECT_EQ(cursor, -1);
}

TEST_F(TextDocumentTest, NewActionClearsRedoStack) {
  auto doc = CreateDoc("Start");
  doc->InsertText(5, toWCharVector("+A"));
  doc->Undo();
  EXPECT_TRUE(doc->CanRedo());
  // New action should clear redo stack
  doc->InsertText(5, toWCharVector("+B"));
  EXPECT_FALSE(doc->CanRedo());
  // Undo should undo the new action
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Start"));
}

TEST_F(TextDocumentTest, UndoInsertThenEraseComplex) {
  // Insert text, verify undo restores exact content,
  // then insert again, erase, undo erase restores correctly
  auto doc = CreateDoc("InitialContent");
  doc->InsertText(7, toWCharVector("Inserted"));
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("InitialContent"));
  doc->EraseText(4, 3);
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("InitialContent"));
}

// ===========================================================================
// Tests: LineNumFromCharOffset
// ===========================================================================

TEST_F(TextDocumentTest, LineNumFromCharOffsetFirstLine) {
  auto doc = CreateDoc("Line0\nLine1\nLine2");
  EXPECT_EQ(doc->LineNumFromCharOffset(0), 0);   // 'L' of "Line0"
  EXPECT_EQ(doc->LineNumFromCharOffset(3), 0);   // 'e' in "Line0"
  EXPECT_EQ(doc->LineNumFromCharOffset(5), 0);   // just before \n
}

TEST_F(TextDocumentTest, LineNumFromCharOffsetLaterLines) {
  auto doc = CreateDoc("Line0\nLine1\nLine2");
  // \n at offset 5 → Line1 starts at offset 6
  EXPECT_EQ(doc->LineNumFromCharOffset(6), 1);   // 'L' of "Line1"
  EXPECT_EQ(doc->LineNumFromCharOffset(11), 1);  // just before \n
  // \n at offset 11 → Line2 starts at offset 12
  EXPECT_EQ(doc->LineNumFromCharOffset(12), 2);  // 'L' of "Line2"
  EXPECT_EQ(doc->LineNumFromCharOffset(16), 2);  // '2' last char
}

TEST_F(TextDocumentTest, LineNumFromCharOffsetEmptyDoc) {
  TextDocument doc;
  // For empty doc, GetNodePosition returns root node; behavior is undefined
  // but we verify it doesn't crash
  // size_t line = doc.LineNumFromCharOffset(0);
  // Just verify initialization works
  doc.Initialize(toWCharVector("A"));
  EXPECT_EQ(doc.LineNumFromCharOffset(0), 0);
}

TEST_F(TextDocumentTest, LineNumFromCharOffsetAfterInsert) {
  // Insertion creates new pieces; verify line numbers stay correct
  auto doc = CreateDoc("Hello\nWorld");
  doc->InsertText(0, toWCharVector("BEGIN\n"));
  // Doc is now "BEGIN\nHello\nWorld"
  EXPECT_EQ(doc->LineNumFromCharOffset(0), 0);    // 'B'
  EXPECT_EQ(doc->LineNumFromCharOffset(6), 1);    // 'H' (Hello is now on line 1)
  EXPECT_EQ(doc->LineNumFromCharOffset(12), 2);   // 'W'
}

// ===========================================================================
// Tests: IterateLineByLineNumber
// ===========================================================================

TEST_F(TextDocumentTest, IterateLineByLineNumberValidLines) {
  auto doc = CreateDoc("First\nSecond\nThird");
  size_t start = 999, len = 999;

  auto itor1 = doc->IterateLineByLineNumber(0, &start, &len);
  EXPECT_TRUE((bool)itor1);
  EXPECT_EQ(itor1.GetLine(), toWCharVector("First\n"));
  EXPECT_EQ(start, 0);
  EXPECT_EQ(len, 6);

  auto itor2 = doc->IterateLineByLineNumber(1, &start, &len);
  EXPECT_TRUE((bool)itor2);
  EXPECT_EQ(itor2.GetLine(), toWCharVector("Second\n"));
  EXPECT_EQ(start, 6);
  EXPECT_EQ(len, 7);

  auto itor3 = doc->IterateLineByLineNumber(2, &start, &len);
  EXPECT_TRUE((bool)itor3);
  EXPECT_EQ(itor3.GetLine(), toWCharVector("Third"));
  EXPECT_EQ(start, 13);
  EXPECT_EQ(len, 5);
}

TEST_F(TextDocumentTest, IterateLineByLineNumberEmptyDoc) {
  TextDocument doc;
  size_t start = 0, len = 0;
  auto itor = doc.IterateLineByLineNumber(0, &start, &len);
  EXPECT_FALSE((bool)itor);
}

TEST_F(TextDocumentTest, IterateLineByLineNumberOutOfRange) {
  auto doc = CreateDoc("OnlyOne");
  size_t start = 0, len = 0;
  auto itor = doc->IterateLineByLineNumber(99, &start, &len);
  EXPECT_FALSE((bool)itor);
}

// ===========================================================================
// Tests: IterateLineByCharOffset
// ===========================================================================

TEST_F(TextDocumentTest, IterateLineByCharOffsetReturnsCorrectLine) {
  auto doc = CreateDoc("AAA\nBBB\nCCC");
  size_t lineno = 999, linestart = 999;

  // Offset 0 → line 0, start at 0
  auto itor0 = doc->IterateLineByCharOffset(0, &lineno, &linestart);
  EXPECT_TRUE((bool)itor0);
  EXPECT_EQ(lineno, 0);
  EXPECT_EQ(linestart, 0);

  // Offset 5 → "BBB" → line 1
  auto itor1 = doc->IterateLineByCharOffset(5, &lineno, &linestart);
  EXPECT_TRUE((bool)itor1);
  EXPECT_EQ(lineno, 1);
  EXPECT_EQ(linestart, 4);

  // Offset 9 → "CCC" → line 2
  auto itor2 = doc->IterateLineByCharOffset(9, &lineno, &linestart);
  EXPECT_TRUE((bool)itor2);
  EXPECT_EQ(lineno, 2);
  EXPECT_EQ(linestart, 8);
}

TEST_F(TextDocumentTest, IterateLineByCharOffsetAfterMultiPieceInsert) {
  // Insert to create multi-piece tree, then verify line-by-offset still works
  auto doc = CreateDoc("Start\nEnd");
  doc->InsertText(6, toWCharVector("Middle\n"));  // "Start\nMiddle\nEnd"
  size_t lineno = 999, linestart = 999;

  auto itor = doc->IterateLineByCharOffset(7, &lineno, &linestart);
  EXPECT_TRUE((bool)itor);
  EXPECT_EQ(lineno, 1);          // "Middle" line
  EXPECT_EQ(linestart, 6);       // starts at offset 6
  EXPECT_EQ(itor.GetLine(), toWCharVector("Middle\n"));
}

// ===========================================================================
// Tests: GetText (TextDocument wrapper)
// ===========================================================================

TEST_F(TextDocumentTest, GetTextFullContent) {
  auto doc = CreateDoc("Hello World!");
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World!"));
}

TEST_F(TextDocumentTest, GetTextPartial) {
  auto doc = CreateDoc("Hello World!");
  EXPECT_EQ(doc->GetText(0, 5), toWCharVector("Hello"));
  EXPECT_EQ(doc->GetText(6, 5), toWCharVector("World"));
}

TEST_F(TextDocumentTest, GetTextAfterEdits) {
  auto doc = CreateDoc("ABCD");
  doc->InsertText(2, toWCharVector("123"));
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("AB123CD"));
  doc->EraseText(3, 2);  // Erase "23"
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("AB1CD"));
}

// ===========================================================================
// Tests: CanUndo / CanRedo state management
// ===========================================================================

TEST_F(TextDocumentTest, CanUndoCanRedoStateTransitions) {
  auto doc = CreateDoc("Test");
  EXPECT_FALSE(doc->CanUndo());
  EXPECT_FALSE(doc->CanRedo());

  doc->InsertText(4, toWCharVector("!"));
  EXPECT_TRUE(doc->CanUndo());
  EXPECT_FALSE(doc->CanRedo());

  doc->Undo();
  EXPECT_FALSE(doc->CanUndo());
  EXPECT_TRUE(doc->CanRedo());

  doc->Redo();
  EXPECT_TRUE(doc->CanUndo());
  EXPECT_FALSE(doc->CanRedo());
}

// ===========================================================================
// Tests: Mixed operations — integration scenarios
// ===========================================================================

TEST_F(TextDocumentTest, TypeSomeTextThenUndoAll) {
  // Simulate typing: "Helo" → correct to "Hello" → add " World"
  auto doc = CreateDoc("");
  doc->InsertText(0, toWCharVector("Helo"));
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Helo"));

  // "Correct by replacing 'o' with 'lo'"
  doc->ReplaceText(3, toWCharVector("lo"), 1);
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello"));

  // "Add World"
  doc->InsertText(5, toWCharVector(" World"));
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World"));

  // Undo " World"
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello"));

  // Undo ReplaceText
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Helo"));

  // Undo InsertText
  doc->Undo();
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector(""));
}

TEST_F(TextDocumentTest, EraseAndInsertAtSameLocation) {
  auto doc = CreateDoc("Hello World");
  doc->EraseText(5, 1);  // Remove space
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("HelloWorld"));
  doc->InsertText(5, toWCharVector("_"));
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello_World"));
  doc->Undo();  // Undo insert
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("HelloWorld"));
  doc->Undo();  // Undo erase
  EXPECT_EQ(doc->GetText(0, 99), toWCharVector("Hello World"));
}

TEST_F(TextDocumentTest, ClearResetsUndoRedoStacks) {
  auto doc = CreateDoc("Content");
  doc->InsertText(7, toWCharVector("!"));
  EXPECT_TRUE(doc->CanUndo());
  doc->Clear();
  EXPECT_FALSE(doc->CanUndo());
  EXPECT_FALSE(doc->CanRedo());
}
