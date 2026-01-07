#include "pch.h"
#include "../TextDocument/PieceTree.h"
#include "../TextDocument/FormatConversionV2.h"

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

  } // namespace internal
} // namespace testing

static std::vector<char16_t> toWCharVector(const std::string& s)
{
  std::vector<char> str(s.begin(), s.end());
  std::vector<char16_t> utf16_contnet = RawToUtf16(str, CP_TYPE::UTF8);
  return utf16_contnet;
}
// Test fixture for createLineStarts
TEST(CreateLineStartsTest, EmptyString)
{
  std::vector<char16_t> empty_str;
  std::vector<size_t> expected = { 0 }; // For empty string, no lines.
  EXPECT_EQ(createLineStarts(empty_str), expected);
}

TEST(CreateLineStartsTest, NoNewlineCharacters)
{
  std::vector<char16_t> str = toWCharVector("Hello World");
  std::vector<size_t> expected = {0}; // Single line, starts at 0
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, SingleLF)
{
  auto str = toWCharVector("Line1\nLine2");
  std::vector<size_t> expected = {0, 6}; // Line1 starts at 0, Line2 starts after '\n' at index 5 -> 5+1 = 6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, MultipleLFs)
{
  auto str = toWCharVector("A\nB\nC\n");
  std::vector<size_t> expected = {0, 2, 4, 6}; // 'A' @0, 'B' @2, 'C' @4, empty line @6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, SingleCRLF)
{
  auto str = toWCharVector("Line1\r\nLine2");
  std::vector<size_t> expected = {0, 7}; // Line1 @0, Line2 starts after '\r\n' at index 5+2 = 7
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, MultipleCRLFs)
{
  auto str = toWCharVector("First\r\nSecond\r\nThird\r\n");
  std::vector<size_t> expected = {0, 7, 15, 22}; // 'F' @0, 'S' @7, 'T' @15, empty line @22
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, MixedNewlines)
{
  auto str = toWCharVector("Hello\nWorld\r\nTest\rAnother\n");
  std::vector<size_t> expected = {0, 6, 13, 18, 26};
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringEndingWithNewlineLF)
{
  auto str = toWCharVector("Line1\n");
  std::vector<size_t> expected = {0, 6}; // Line1 @0, empty line after \n @6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringEndingWithNewlineCRLF)
{
  auto str = toWCharVector("Line1\r\n");
  std::vector<size_t> expected = {0, 7}; // Line1 @0, empty line after \r\n @7
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringEndingWithNewlineCR)
{
  auto str = toWCharVector("Line1\r");
  std::vector<size_t> expected = {0, 6}; // Line1 @0, empty line after \r @6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringWithOnlyNewlines)
{
  auto str = toWCharVector("\n\r\n\r");
  std::vector<size_t> expected = {0, 1, 3, 4};
  // "" @0
  // \n (0) -> next line starts at 1
  // \r (1) \n (2) -> next line starts at 3
  // \r (3) -> next line starts at 4
  EXPECT_EQ(createLineStarts(str), expected);
}

// ===========================================================================
// Tests for PieceTree Constructor and Destructor
// ===========================================================================

// Test fixture for PieceTree
// Using a fixture allows setup/teardown code to be shared
class PieceTreeTest : public ::testing::Test {
protected:
  // Any setup that needs to happen before each test
  void SetUp() override
  {
    // You might use this later for common PieceTree constructions
  }

  // Any teardown that needs to happen after each test
  void TearDown() override
  {
  }
protected:
  // Helper to create a PieceTree from a string
  std::unique_ptr<PieceTree> CreatePieceTree(const std::string& s)
  {
    return std::make_unique<PieceTree>(toWCharVector(s));
  }
};

TEST_F(PieceTreeTest, EmptyInputStringConstructor)
{
  std::vector<char16_t> empty_input;
  PieceTree pt(empty_input);

  //rootNode
  EXPECT_TRUE(pt.rootNode->left == nullptr);
  EXPECT_TRUE(pt.rootNode->right.get() == nullptr);
  EXPECT_EQ(pt.rootNode->piece.length, 0); // Root's piece should be empty

  // buffers and others
  EXPECT_EQ(pt.length, 0);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ(pt.buffers.size(), 2); // Initial empty buffer + the empty input buffer
}

TEST_F(PieceTreeTest, NonEmptyInputStringConstructor)
{
  auto input = toWCharVector("Hello\nWorld");
  PieceTree pt(input);

  // Check overall length and line count
  EXPECT_EQ(pt.length, input.size()); // Should be 11
  EXPECT_EQ(pt.lineCount, 2); // 2 lines

  // Check buffers content
  ASSERT_EQ(pt.buffers.size(), 2); // Initial empty buffer (index 0) + input buffer (index 1)
  EXPECT_EQ(pt.buffers[1].value, input); // Ensure input buffer is correctly stored

  // Check the rootNode.right's piece
  ASSERT_NE(pt.rootNode.get()->right, nullptr);
  const Piece& initialPiece = pt.rootNode.get()->right->piece;
  EXPECT_EQ(initialPiece.bufferIndex, 1); // Should refer to the input buffer
  EXPECT_EQ(initialPiece.length, input.size());
  EXPECT_EQ(initialPiece.lineFeedCnt, 1);

  // Check BufferPosition for start and end
  EXPECT_EQ(initialPiece.start.line, 0);
  EXPECT_EQ(initialPiece.start.column, 0);

  // For end position: lineStarts.size() - 1 for index, initialPiece.size() - lineStarts.back() for offset
  // "Hello\nWorld" -> createLineStarts returns {0, 6}. Size is 2.
  // lineStarts.size() - 1 = 1
  // lineStarts.back() = 6
  // initialPiece.size() = 11
  EXPECT_EQ(initialPiece.end.line, 1); // Index of the last line in piece (the second line, index 1)
  EXPECT_EQ(initialPiece.end.column, 5); // Offset of the start of the last line (the 'W' in World)
}


// ===========================================================================
// Tests for InsertText
// ===========================================================================

TEST_F(PieceTreeTest, InsertAtBeginning_EmptyTree)
{
  PieceTree pt(toWCharVector("")); // Start with an empty tree
  auto textToInsert = toWCharVector("NewText");
  bool success = pt.InsertText(0, textToInsert);
  EXPECT_TRUE(success);

  EXPECT_EQ(pt.length, textToInsert.size());
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ((pt.GetText(0, 99)), textToInsert);
}

TEST_F(PieceTreeTest, InsertAtBeginning_NonEmptyTree_NoSplit)
{
  PieceTree pt(toWCharVector("World"));
  auto textToInsert = toWCharVector("Hello ");
  bool success = pt.InsertText(0, textToInsert);
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 5 + 6); // "Hello World"
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("Hello World"));
}

TEST_F(PieceTreeTest, InsertInMiddle_SplitsPiece)
{
  PieceTree pt(toWCharVector("HelloWorld")); // Length 10
  auto textToInsert = toWCharVector(" "); // Insert space at offset 5 ("Hello_World")
  bool success = pt.InsertText(5, textToInsert);
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 11);
  EXPECT_EQ(pt.lineCount, 1);
  // This test will require verifying the internal tree structure:
  // The original piece ("HelloWorld") should be split into two pieces ("Hello" and "World"),
  // and a new piece (" ") should be inserted between them.
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("Hello World"));
}

TEST_F(PieceTreeTest, InsertTextWithNewlines)
{
  PieceTree pt(toWCharVector("Before\nAfter"));
  auto textToInsert = toWCharVector("Middle\n");
  bool success = pt.InsertText(6, textToInsert); // Insert at 'Befor|e\nAfter'
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 12 + 7); // Original (6 + 1 + 5) + Inserted (6 + 1)
  EXPECT_EQ(pt.lineCount, 2 + 1); // Original 1 lines, inserted 1 newline
}

// ===========================================================================
// Tests for EraseText
// ===========================================================================

TEST_F(PieceTreeTest, EraseFullContent)
{
  PieceTree pt(toWCharVector("Erase Me"));
  bool success = pt.EraseText(0, pt.length);
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 0);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector(""));
}

TEST_F(PieceTreeTest, EraseFromBeginning)
{
  PieceTree pt(toWCharVector("123456789"));
  bool success = pt.EraseText(0, 3); // Erase "123"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("456789"));
}

TEST_F(PieceTreeTest, EraseFromMiddle)
{
  PieceTree pt(toWCharVector("ABCDEFGHI"));
  bool success = pt.EraseText(3, 3); // Erase "DEF"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("ABCGHI"));
}

TEST_F(PieceTreeTest, EraseAcrossMultiplePieces_ComplexScenario)
{
  // This test requires InsertText to be working first to build a complex tree.
  // You might create a helper to build a tree with specific pieces.
  // Example: original "Hello World", insert "Cruel" at 5 -> "HelloCruel World"
  // Then erase "loCruel W" (part of original, new, part of original)
  // This is where GetText() becomes crucial for validation.
  PieceTree pt(toWCharVector("Hello World"));
  auto textToInsert = toWCharVector("Cruel");
  bool success = pt.InsertText(5, textToInsert);
  EXPECT_TRUE(success);
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("HelloCruel World"));
  success = pt.EraseText(3, 9); // Erase "loCruel W"
  EXPECT_TRUE(success);
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("Helorld"));
}

TEST_F(PieceTreeTest, EraseWithNewlines)
{
  PieceTree pt(toWCharVector("Line1\nLine2\nLine3"));
  bool success = pt.EraseText(4, 7); // Erase "1\nLine2"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 10);
  EXPECT_EQ(pt.lineCount, 2); // 2 lines remains
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("Line\nLine3"));
}


// ===========================================================================
// Tests for ReplaceText
// ===========================================================================

TEST_F(PieceTreeTest, ReplaceWithSameLength)
{
  PieceTree pt(toWCharVector("ABCDEF"));
  auto newText = toWCharVector("123");
  bool success = pt.ReplaceText(1, newText, 3); // Replace "BCD" with "123"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6); // Length remains same
  EXPECT_EQ(pt.lineCount, 1);
  // Expected content "A123EF"
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("A123EF"));
}

TEST_F(PieceTreeTest, ReplaceWithLongerText)
{
  PieceTree pt(toWCharVector("ABCDEF"));
  auto newText = toWCharVector("12345");
  bool success = pt.ReplaceText(1, newText, 3); // Replace "BCD" with "12345"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6 - 3 + 5); // 8
  EXPECT_EQ(pt.lineCount, 1);
  // Expected content "A12345EF"
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("A12345EF"));
}

TEST_F(PieceTreeTest, ReplaceWithShorterText)
{
  PieceTree pt(toWCharVector("ABCDEF"));
  auto newText = toWCharVector("1");
  bool success = pt.ReplaceText(1, newText, 3); // Replace "BCD" with "1"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6 - 3 + 1); // 4
  EXPECT_EQ(pt.lineCount, 1);
  // Expected content "A1EF"
  EXPECT_EQ((pt.GetText(0, 99)), toWCharVector("A1EF"));
}

// ===========================================================================
// Tests for GetText
// ===========================================================================
TEST_F(PieceTreeTest, GetText_FullContent_SinglePiece) {
  std::string original = "Hello World!";
  auto pt = CreatePieceTree(original);
  EXPECT_EQ(toWCharVector(original), pt->GetText(0, original.length()));
}

TEST_F(PieceTreeTest, GetText_PartialContent_SinglePiece_Start) {
  std::string original = "Hello World!";
  auto pt = CreatePieceTree(original);
  EXPECT_EQ(toWCharVector("Hello"), pt->GetText(0, 5));
}

TEST_F(PieceTreeTest, GetText_PartialContent_SinglePiece_Middle) {
  std::string original = "Hello World!";
  auto pt = CreatePieceTree(original);
  EXPECT_EQ(toWCharVector("lo Wor"), pt->GetText(3, 6));
}

TEST_F(PieceTreeTest, GetText_PartialContent_SinglePiece_End) {
  std::string original = "Hello World!";
  auto pt = CreatePieceTree(original);
  EXPECT_EQ(toWCharVector("World!"), pt->GetText(6, 6));
}

TEST_F(PieceTreeTest, GetText_ZeroLength) {
  auto pt = CreatePieceTree("Test");
  EXPECT_TRUE(pt->GetText(1, 0).empty());
}

TEST_F(PieceTreeTest, GetText_OffsetOutOfBounds) {
  auto pt = CreatePieceTree("Test");
  EXPECT_TRUE(pt->GetText(10, 5).empty()); // Offset completely out of bounds
}

TEST_F(PieceTreeTest, GetText_LengthExceedsDocument) {
  auto pt = CreatePieceTree("Test"); // Length 4
  EXPECT_EQ(toWCharVector("Test"), pt->GetText(0, 10)); // Should return full "Test"
  EXPECT_EQ(toWCharVector("est"), pt->GetText(1, 10)); // Should return "est"
}

TEST_F(PieceTreeTest, GetText_AcrossMultiplePieces) {
  // Scenario: "FirstPart" + "MiddlePart" + "LastPart"
  // Create "FirstPartLastPart"
  auto pt = CreatePieceTree("FirstPartLastPart"); // One piece: length 17
  // Insert "MiddlePart" at offset 9 ("FirstPart|LastPart")
  pt->InsertText(9, toWCharVector("MiddlePart")); // "FirstPartMiddlePartLastPart"
  // Now you have (at least) three pieces:
  // P1("FirstPart") len:9, P2("MiddlePart") len:10, P3("LastPart") len:8

  // Get text that spans P1 and P2
  EXPECT_EQ(toWCharVector("rtMiddle"), (pt->GetText(7, 8)));

  // Get text that spans P2 and P3
  EXPECT_EQ(toWCharVector("PartLastP"), (pt->GetText(15, 9)));

  // Get text that spans all three pieces
  EXPECT_EQ(toWCharVector("artMiddlePartLas"), (pt->GetText(6, 16)));
}

TEST_F(PieceTreeTest, GetText_WithNewlines) {
  auto pt = CreatePieceTree("Line1\nLine2\nLine3"); // 3 lines
  // Insert "MID\n" at offset 7 (after \n)
  pt->InsertText(6, toWCharVector("MID\n")); // "Line1\nMID\nLine2\nLine3"

  // Get "MID\nLi" from offset 7
  EXPECT_EQ(toWCharVector("ID\nLin"), pt->GetText(7, 6));

  // Get "Line2\n" from offset 11
  EXPECT_EQ(toWCharVector("ine2\nL"), pt->GetText(11, 6));
}

TEST_F(PieceTreeTest, GetText_FromEmptyTree) {
  auto pt = CreatePieceTree("");
  EXPECT_TRUE(pt->GetText(0, 10).empty());
  EXPECT_TRUE(pt->GetText(5, 5).empty());
}

// ===========================================================================
// Tests for GetLine
// ===========================================================================
TEST_F(PieceTreeTest, GetLine_FromEmptyTree)
{
  auto pt = CreatePieceTree("");
  EXPECT_TRUE(pt->GetLine(0, 0, nullptr).empty() == true);
}
TEST_F(PieceTreeTest, GetLine_Complicated)
{
  auto pt = CreatePieceTree("123\n456\n789");
  EXPECT_EQ(pt->lineCount, 3);
  EXPECT_EQ(toWCharVector("123\n"), (pt->GetLine(1, 0, nullptr)));
  EXPECT_EQ(toWCharVector("456\n"), (pt->GetLine(2, 0, nullptr)));
  EXPECT_EQ(toWCharVector("789"), (pt->GetLine(3, 0, nullptr)));

  pt->InsertText(2, toWCharVector("c"));
  EXPECT_EQ(toWCharVector("12c3\n"), (pt->GetLine(1, 0, nullptr)));
  EXPECT_EQ(toWCharVector("12c3\n"), (pt->GetText(0, 5)));
  pt->InsertText(2, toWCharVector("s"));
  EXPECT_EQ(toWCharVector("12sc3\n"), (pt->GetLine(1, 0, nullptr)));
  EXPECT_EQ(toWCharVector("12sc3\n"), (pt->GetText(0, 6)));
  pt->InsertText(2, toWCharVector("v"));
  EXPECT_EQ(toWCharVector("12vsc3\n"), (pt->GetLine(1, 0, nullptr)));
  EXPECT_EQ(toWCharVector("12vsc3\n"), (pt->GetText(0, 7)));
  pt->InsertText(0, toWCharVector("0"));
  EXPECT_EQ(toWCharVector("012vsc3\n"), (pt->GetLine(1, 0, nullptr)));
  EXPECT_EQ(toWCharVector("012vsc3\n"), (pt->GetText(0, 8)));
  pt->InsertText(0, toWCharVector("\n"));
  pt->InsertText(0, toWCharVector("\r\n"));
  pt->InsertText(0, toWCharVector("\r"));
  EXPECT_EQ(toWCharVector("\r"), (pt->GetLine(1, 0, nullptr)));
  EXPECT_EQ(toWCharVector("\r\n"), (pt->GetLine(2, 0, nullptr)));
  EXPECT_EQ(toWCharVector("\n"), (pt->GetLine(3, 0, nullptr)));
  EXPECT_EQ(toWCharVector("012vsc3\n"), (pt->GetLine(4, 0, nullptr)));
}