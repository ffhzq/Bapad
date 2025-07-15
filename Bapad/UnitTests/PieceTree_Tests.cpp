#include "pch.h"
#include "../TextDocument/PieceTree.h"

// Helper to convert std::string to std::vector<unsigned char>
std::vector<unsigned char> toUCharVector(const std::string& s)
{
  return std::vector<unsigned char>(s.begin(), s.end());
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
  EXPECT_EQ(initialPiece.start.index, 0);
  EXPECT_EQ(initialPiece.start.offset, 0);

  // For end position: lineStarts.size() - 1 for index, initialPiece.size() - lineStarts.back() for offset
  // "Hello\nWorld" -> createLineStarts returns {0, 6}. Size is 2.
  // lineStarts.size() - 1 = 1
  // lineStarts.back() = 6
  // initialPiece.size() = 11
  EXPECT_EQ(initialPiece.end.index, 1); // Index of the last line in piece (the second line, index 1)
  EXPECT_EQ(initialPiece.end.offset, 5); // Offset of the start of the last line (the 'W' in World)
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
}

TEST_F(PieceTreeTest, InsertAtBeginning_NonEmptyTree_NoSplit)
{
  PieceTree pt(toUCharVector("World"));
  std::vector<unsigned char> textToInsert = toUCharVector("Hello ");
  bool success = pt.InsertText(0, textToInsert);
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 5 + 6); // "Hello World"
  EXPECT_EQ(pt.lineCount, 1);

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
  // Tree should logically be empty (e.g., rootNode.right points to a piece with length 0)
}

TEST_F(PieceTreeTest, EraseFromBeginning)
{
  PieceTree pt(toUCharVector("123456789"));
  bool success = pt.EraseText(0, 3); // Erase "123"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6);
  EXPECT_EQ(pt.lineCount, 1);
  // Need to verify the remaining content (e.g., "456789") via GetText() later
}

TEST_F(PieceTreeTest, EraseFromMiddle)
{
  PieceTree pt(toUCharVector("ABCDEFGHI"));
  bool success = pt.EraseText(3, 3); // Erase "DEF"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 6);
  EXPECT_EQ(pt.lineCount, 1);
  // Result should be "ABCGHI"
}

TEST_F(PieceTreeTest, EraseAcrossMultiplePieces_ComplexScenario)
{
  // This test requires InsertText to be working first to build a complex tree.
  // You might create a helper to build a tree with specific pieces.
  // Example: original "Hello World", insert "Cruel" at 5 -> "HelloCruel World"
  // Then erase "loCruel W" (part of original, new, part of original)
  // This is where GetText() becomes crucial for validation.
}

TEST_F(PieceTreeTest, EraseWithNewlines)
{
  PieceTree pt(toUCharVector("Line1\nLine2\nLine3"));
  bool success = pt.EraseText(4, 7); // Erase "1\nLine2"
  EXPECT_TRUE(success);
  EXPECT_EQ(pt.length, 10);
  EXPECT_EQ(pt.lineCount, 2); // 2 lines remains
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
}