#include "pch.h"
#include "../TextDocument/PieceTree.h"

// Helper to convert std::string to std::vector<unsigned char>
std::vector<unsigned char> toUCharVector(const std::string& s)
{
  return std::vector<unsigned char>(s.begin(), s.end());
}
std::string UCVtoString(const std::vector<unsigned char> uchar_vec)
{
  return std::string(uchar_vec.begin(), uchar_vec.end());
}
// Test fixture for createLineStarts
TEST(CreateLineStartsTest, EmptyString)
{
  std::vector<unsigned char> empty_str;
  std::vector<size_t> expected = {0}; // For empty string, no lines.
  EXPECT_EQ(createLineStarts(empty_str), expected);
}

TEST(CreateLineStartsTest, NoNewlineCharacters)
{
  std::vector<unsigned char> str = toUCharVector("Hello World");
  std::vector<size_t> expected = {0}; // Single line, starts at 0
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, SingleLF)
{
  std::vector<unsigned char> str = toUCharVector("Line1\nLine2");
  std::vector<size_t> expected = {0, 6}; // Line1 starts at 0, Line2 starts after '\n' at index 5 -> 5+1 = 6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, MultipleLFs)
{
  std::vector<unsigned char> str = toUCharVector("A\nB\nC\n");
  std::vector<size_t> expected = {0, 2, 4, 6}; // 'A' @0, 'B' @2, 'C' @4, empty line @6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, SingleCRLF)
{
  std::vector<unsigned char> str = toUCharVector("Line1\r\nLine2");
  std::vector<size_t> expected = {0, 7}; // Line1 @0, Line2 starts after '\r\n' at index 5+2 = 7
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, MultipleCRLFs)
{
  std::vector<unsigned char> str = toUCharVector("First\r\nSecond\r\nThird\r\n");
  std::vector<size_t> expected = {0, 7, 15, 22}; // 'F' @0, 'S' @7, 'T' @15, empty line @22
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, MixedNewlines)
{
  std::vector<unsigned char> str = toUCharVector("Hello\nWorld\r\nTest\rAnother\n");
  std::vector<size_t> expected = {0, 6, 13, 18, 26};
  // "Hello" (0-4) \n (5) -> next line starts at 6
  // "World" (6-10) \r (11) \n (12) -> next line starts at 13
  // "Test" (13-16) \r (17) -> next line starts at 18
  // "Another" (18-24) \n (25) -> next line starts at 26
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringEndingWithNewlineLF)
{
  std::vector<unsigned char> str = toUCharVector("Line1\n");
  std::vector<size_t> expected = {0, 6}; // Line1 @0, empty line after \n @6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringEndingWithNewlineCRLF)
{
  std::vector<unsigned char> str = toUCharVector("Line1\r\n");
  std::vector<size_t> expected = {0, 7}; // Line1 @0, empty line after \r\n @7
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringEndingWithNewlineCR)
{
  std::vector<unsigned char> str = toUCharVector("Line1\r");
  std::vector<size_t> expected = {0, 6}; // Line1 @0, empty line after \r @6
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, StringWithOnlyNewlines)
{
  std::vector<unsigned char> str = toUCharVector("\n\r\n\r");
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
  {}
protected:
  // Helper to create a PieceTree from a string
  std::unique_ptr<PieceTree> CreatePieceTree(const std::string& s)
  {
    return std::make_unique<PieceTree>(toUCharVector(s));
  }
};

TEST_F(PieceTreeTest, EmptyInputStringConstructor)
{
  std::vector<unsigned char> empty_input;
  PieceTree pt(empty_input);

  //rootNode
  ASSERT_EQ(pt.rootNode.get()->left, nullptr);
  ASSERT_NE(pt.rootNode.get()->right, nullptr); // rootNode.right should not be nullptr
  EXPECT_EQ(pt.rootNode.get()->piece.length, 0); // Root's piece should be empty
  EXPECT_EQ(pt.rootNode.get()->right->piece.length, 0); // The piece in the first actual node should be empty

  // buffers and others
  EXPECT_EQ(pt.length, 0);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ(pt.buffers.size(), 2); // Initial empty buffer + the empty input buffer
}

TEST_F(PieceTreeTest, NonEmptyInputStringConstructor)
{
  std::vector<unsigned char> input = toUCharVector("Hello\nWorld");
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
  PieceTree pt(toUCharVector("")); // Start with an empty tree
  std::vector<unsigned char> textToInsert = toUCharVector("NewText");
  bool success = pt.InsertText(0, textToInsert);
  EXPECT_TRUE(success);

  EXPECT_EQ(pt.length, textToInsert.size());
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "NewText");
}

TEST_F(PieceTreeTest, InsertAtBeginning_NonEmptyTree_NoSplit)
{
  PieceTree pt(toUCharVector("World"));
  std::vector<unsigned char> textToInsert = toUCharVector("Hello ");
  bool success = pt.InsertText(0, textToInsert);
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 5 + 6); // "Hello World"
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "Hello World");
}

TEST_F(PieceTreeTest, InsertInMiddle_SplitsPiece)
{
  PieceTree pt(toUCharVector("HelloWorld")); // Length 10
  std::vector<unsigned char> textToInsert = toUCharVector(" "); // Insert space at offset 5 ("Hello_World")
  bool success = pt.InsertText(5, textToInsert);
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 11);
  EXPECT_EQ(pt.lineCount, 1);
  // This test will require verifying the internal tree structure:
  // The original piece ("HelloWorld") should be split into two pieces ("Hello" and "World"),
  // and a new piece (" ") should be inserted between them.
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "Hello World");
}

TEST_F(PieceTreeTest, InsertTextWithNewlines)
{
  PieceTree pt(toUCharVector("Before\nAfter"));
  std::vector<unsigned char> textToInsert = toUCharVector("Middle\n");
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
  PieceTree pt(toUCharVector("Erase Me"));
  bool success = pt.EraseText(0, pt.length);
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 0);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "");
}

TEST_F(PieceTreeTest, EraseFromBeginning)
{
  PieceTree pt(toUCharVector("123456789"));
  bool success = pt.EraseText(0, 3); // Erase "123"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "456789");
}

TEST_F(PieceTreeTest, EraseFromMiddle)
{
  PieceTree pt(toUCharVector("ABCDEFGHI"));
  bool success = pt.EraseText(3, 3); // Erase "DEF"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6);
  EXPECT_EQ(pt.lineCount, 1);
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "ABCGHI");
}

TEST_F(PieceTreeTest, EraseAcrossMultiplePieces_ComplexScenario)
{
  // This test requires InsertText to be working first to build a complex tree.
  // You might create a helper to build a tree with specific pieces.
  // Example: original "Hello World", insert "Cruel" at 5 -> "HelloCruel World"
  // Then erase "loCruel W" (part of original, new, part of original)
  // This is where GetText() becomes crucial for validation.
  PieceTree pt(toUCharVector("Hello World"));
  std::vector<unsigned char> textToInsert = toUCharVector("Cruel");
  bool success = pt.InsertText(5, textToInsert);
  EXPECT_TRUE(success);
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "HelloCruel World");
  success = pt.EraseText(3, 9); // Erase "loCruel W"
  EXPECT_TRUE(success);
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "Helorld");
}

TEST_F(PieceTreeTest, EraseWithNewlines)
{
  PieceTree pt(toUCharVector("Line1\nLine2\nLine3"));
  bool success = pt.EraseText(4, 7); // Erase "1\nLine2"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 10);
  EXPECT_EQ(pt.lineCount, 2); // 2 lines remains
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "Line\nLine3");
}


// ===========================================================================
// Tests for ReplaceText
// ===========================================================================

TEST_F(PieceTreeTest, ReplaceWithSameLength)
{
  PieceTree pt(toUCharVector("ABCDEF"));
  std::vector<unsigned char> newText = toUCharVector("123");
  bool success = pt.ReplaceText(1, newText, 3); // Replace "BCD" with "123"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6); // Length remains same
  EXPECT_EQ(pt.lineCount, 1);
  // Expected content "A123EF"
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "A123EF");
}

TEST_F(PieceTreeTest, ReplaceWithLongerText)
{
  PieceTree pt(toUCharVector("ABCDEF"));
  std::vector<unsigned char> newText = toUCharVector("12345");
  bool success = pt.ReplaceText(1, newText, 3); // Replace "BCD" with "12345"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6 - 3 + 5); // 8
  EXPECT_EQ(pt.lineCount, 1);
  // Expected content "A12345EF"
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "A12345EF");
}

TEST_F(PieceTreeTest, ReplaceWithShorterText)
{
  PieceTree pt(toUCharVector("ABCDEF"));
  std::vector<unsigned char> newText = toUCharVector("1");
  bool success = pt.ReplaceText(1, newText, 3); // Replace "BCD" with "1"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6 - 3 + 1); // 4
  EXPECT_EQ(pt.lineCount, 1);
  // Expected content "A1EF"
  EXPECT_EQ(UCVtoString(pt.GetText(0, 99)), "A1EF");
}

// ===========================================================================
// Tests for GetText
// ===========================================================================
TEST_F(PieceTreeTest, GetText_FullContent_SinglePiece) {
    std::string original = "Hello World!";
    auto pt = CreatePieceTree(original);
    EXPECT_EQ(toUCharVector(original), pt->GetText(0, original.length()));
}

TEST_F(PieceTreeTest, GetText_PartialContent_SinglePiece_Start) {
    std::string original = "Hello World!";
    auto pt = CreatePieceTree(original);
    EXPECT_EQ(toUCharVector("Hello"), pt->GetText(0, 5));
}

TEST_F(PieceTreeTest, GetText_PartialContent_SinglePiece_Middle) {
    std::string original = "Hello World!";
    auto pt = CreatePieceTree(original);
    EXPECT_EQ(toUCharVector("lo Wor"), pt->GetText(3, 6));
}

TEST_F(PieceTreeTest, GetText_PartialContent_SinglePiece_End) {
    std::string original = "Hello World!";
    auto pt = CreatePieceTree(original);
    EXPECT_EQ(toUCharVector("World!"), pt->GetText(6, 6));
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
    EXPECT_EQ(toUCharVector("Test"), pt->GetText(0, 10)); // Should return full "Test"
    EXPECT_EQ(toUCharVector("est"), pt->GetText(1, 10)); // Should return "est"
}

TEST_F(PieceTreeTest, GetText_AcrossMultiplePieces) {
    // Scenario: "FirstPart" + "MiddlePart" + "LastPart"
    // Create "FirstPartLastPart"
    auto pt = CreatePieceTree("FirstPartLastPart"); // One piece: length 17
    // Insert "MiddlePart" at offset 9 ("FirstPart|LastPart")
    pt->InsertText(9, toUCharVector("MiddlePart")); // "FirstPartMiddlePartLastPart"
    // Now you have (at least) three pieces: 
    // P1("FirstPart") len:9, P2("MiddlePart") len:10, P3("LastPart") len:8

    // Get text that spans P1 and P2
    EXPECT_EQ(std::string("rtMiddle"), UCVtoString(pt->GetText(7, 8)));

    // Get text that spans P2 and P3
    EXPECT_EQ(std::string("PartLastP"), UCVtoString(pt->GetText(15, 9)));

    // Get text that spans all three pieces
    EXPECT_EQ(std::string("artMiddlePartLas"), UCVtoString(pt->GetText(6, 16)));
}

TEST_F(PieceTreeTest, GetText_WithNewlines) {
    auto pt = CreatePieceTree("Line1\nLine2\nLine3"); // 3 lines
    // Insert "MID\n" at offset 7 (after \n)
    pt->InsertText(6, toUCharVector("MID\n")); // "Line1\nMID\nLine2\nLine3"

    // Get "MID\nLi" from offset 7
    EXPECT_EQ(toUCharVector("ID\nLin"), pt->GetText(7, 6));

    // Get "Line2\n" from offset 11
    EXPECT_EQ(toUCharVector("ine2\nL"), pt->GetText(11, 6));
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
  EXPECT_TRUE(pt->GetLine(0).empty());
}
TEST_F(PieceTreeTest, GetLine_Complicated)
{
  auto pt = CreatePieceTree("123\n456\n789");
  EXPECT_EQ(pt->lineCount, 3);
  EXPECT_EQ(std::string("123\n"), UCVtoString(pt->GetLine(1)));
  EXPECT_EQ(std::string("456\n"), UCVtoString(pt->GetLine(2)));
  EXPECT_EQ(std::string("789"), UCVtoString(pt->GetLine(3)));

  pt->InsertText(2, toUCharVector("c"));
  EXPECT_EQ(std::string("12c3\n"), UCVtoString(pt->GetLine(1)));
  pt->InsertText(2, toUCharVector("s"));
  EXPECT_EQ(std::string("12sc3\n"), UCVtoString(pt->GetLine(1)));
  pt->InsertText(2, toUCharVector("v"));
  EXPECT_EQ(std::string("12vsc3\n"), UCVtoString(pt->GetLine(1)));
  pt->InsertText(0, toUCharVector("1"));
  EXPECT_EQ(std::string("012vsc3\n"), UCVtoString(pt->GetLine(1)));
}