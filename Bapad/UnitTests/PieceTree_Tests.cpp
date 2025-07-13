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