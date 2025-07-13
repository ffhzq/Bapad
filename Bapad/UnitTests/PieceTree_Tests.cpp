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
  std::vector<size_t> expected = {0, 7, 15, 23}; // 'F' @0, 'S' @7, 'T' @15, empty line @23
  EXPECT_EQ(createLineStarts(str), expected);
}

TEST(CreateLineStartsTest, MixedNewlines)
{
  std::vector<unsigned char> str = toUCharVector("Hello\nWorld\r\nTest\rAnother\n");
  std::vector<size_t> expected = {0, 6, 13, 19, 27};
  // "Hello\n" -> next line starts at 6
  // "World\r\n" -> next line starts at 6+5+2 = 13
  // "Test\r" -> next line starts at 13+4+1 = 18. No, 13+4 = 17, next line starts at 17+1 = 18. Ah, string is "Test\rAnother\n"
  // Let's re-calculate:
  // H @0
  // L @6 (after \n)
  // T @13 (after \r\n)
  // A @18 (after \r) - Your code does +1 for \r, so if 'Test' ends at 16, \r is 17, then next line is 18.
  // The next line starts AFTER the newline character(s).
  // "Hello" (0-4) \n (5) -> next line starts at 6
  // "World" (6-10) \r (11) \n (12) -> next line starts at 13
  // "Test" (13-16) \r (17) -> next line starts at 18
  // "Another" (18-24) \n (25) -> next line starts at 26
  expected = {0, 6, 13, 18, 26};
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

TEST(CreateLineStartsTest, StringWithLongLines)
{
  std::vector<unsigned char> str = toUCharVector(
    "This is a very long line that should not be split.\n"
    "This is another long line ending with CRLF.\r\n"
    "And a final line."
  );
  std::vector<size_t> expected = {
      0,                                   // Line 1
      52,                                  // Line 2 (after 51-char line + \n at 51)
      52 + 37                             // Line 3 (after 36-char line + \r\n at 36+51 = 88)
      // 52 + 37 = 89. After "This is another long line ending with CRLF."
      // "This is a very long line that should not be split.\n" -> length 52. Next line starts at 52.
      // "This is another long line ending with CRLF.\r\n" -> length 39. Next line starts at 52 + 39 = 91.
  };
  expected = {0, 52, 91};
  EXPECT_EQ(createLineStarts(str), expected);
}