#include "pch.h"
#include "TestHelpers.h"
#include "../TextDocument/FormatConversionV2.h"

// ===========================================================================
// Tests: DetectFileFormat
// ===========================================================================

TEST(DetectFileFormatTest, EmptyBuffer) {
  std::vector<char> buf;
  int headerSize = -1;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::UNKNOWN);
  // headerSize is unspecified for empty; just check no crash
}

TEST(DetectFileFormatTest, UTF8BOM) {
  // BOM: EF BB BF
  std::vector<char> buf = {static_cast<char>(0xEF), static_cast<char>(0xBB),
                           static_cast<char>(0xBF), 'H', 'i'};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::UTF8);
  EXPECT_EQ(headerSize, 3);
}

TEST(DetectFileFormatTest, UTF16LEBOM) {
  // BOM: FF FE
  std::vector<char> buf = {static_cast<char>(0xFF), static_cast<char>(0xFE),
                           'H', '\0', 'i', '\0'};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::UTF16);
  EXPECT_EQ(headerSize, 2);
}

TEST(DetectFileFormatTest, UTF16BEBOM) {
  // BOM: FE FF
  std::vector<char> buf = {static_cast<char>(0xFE), static_cast<char>(0xFF),
                           '\0', 'H', '\0', 'i'};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::UTF16BE);
  EXPECT_EQ(headerSize, 2);
}

TEST(DetectFileFormatTest, UTF32LEBOM) {
  // BOM: FF FE 00 00
  std::vector<char> buf = {static_cast<char>(0xFF), static_cast<char>(0xFE),
                           static_cast<char>(0x00), static_cast<char>(0x00),
                           'H', '\0', '\0', '\0'};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::UTF32);
  EXPECT_EQ(headerSize, 4);
}

TEST(DetectFileFormatTest, UTF32BEBOM) {
  // BOM: 00 00 FE FF
  std::vector<char> buf = {static_cast<char>(0x00), static_cast<char>(0x00),
                           static_cast<char>(0xFE), static_cast<char>(0xFF),
                           '\0', '\0', '\0', 'H'};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::UTF32BE);
  EXPECT_EQ(headerSize, 4);
}

TEST(DetectFileFormatTest, NoBOMValidUTF8) {
  // Pure ASCII → valid UTF-8 with no BOM
  std::vector<char> buf = {'H', 'e', 'l', 'l', 'o'};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  // Matches {0, 0, ANSI} first, then IsUTF8 returns true → UTF8
  EXPECT_EQ(type, CP_TYPE::UTF8);
  EXPECT_EQ(headerSize, 0);
}

TEST(DetectFileFormatTest, NoBOMInvalidUTF8) {
  // 0xFF is not valid UTF-8 start byte
  std::vector<char> buf = {static_cast<char>(0xFF), static_cast<char>(0xFE)};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::ANSI);
  EXPECT_EQ(headerSize, 0);
}

TEST(DetectFileFormatTest, UTF16LEBOMShortBuffer) {
  // 2 bytes {0xFF, 0xFE} — this IS a valid UTF-16 LE BOM (not too short)
  std::vector<char> buf = {static_cast<char>(0xFF), static_cast<char>(0xFE)};
  int headerSize = 0;
  auto type = DetectFileFormat(buf, headerSize);
  EXPECT_EQ(type, CP_TYPE::UTF16);
  EXPECT_EQ(headerSize, 2);
}

// ===========================================================================
// Tests: IsUTF8
// ===========================================================================

TEST(IsUTF8Test, EmptyBuffer) {
  EXPECT_FALSE(IsUTF8(std::vector<char>()));
}

TEST(IsUTF8Test, PureASCII) {
  std::vector<char> buf = {'H', 'e', 'l', 'l', 'o'};
  EXPECT_TRUE(IsUTF8(buf));
}

TEST(IsUTF8Test, SingleByteOnly) {
  std::vector<char> buf(128);
  for (int i = 0; i < 128; ++i) buf[i] = static_cast<char>(i);
  EXPECT_TRUE(IsUTF8(buf));
}

TEST(IsUTF8Test, Valid2ByteSequence) {
  // U+00A9 © = 0xC2 0xA9 in UTF-8
  std::vector<char> buf = {static_cast<char>(0xC2), static_cast<char>(0xA9)};
  EXPECT_TRUE(IsUTF8(buf));
}

TEST(IsUTF8Test, Valid3ByteSequence) {
  // U+4E2D 中 = 0xE4 0xB8 0xAD in UTF-8
  std::vector<char> buf = {static_cast<char>(0xE4), static_cast<char>(0xB8),
                           static_cast<char>(0xAD)};
  EXPECT_TRUE(IsUTF8(buf));
}

TEST(IsUTF8Test, Valid4ByteSequence) {
  // U+1F600 😀 = 0xF0 0x9F 0x98 0x80 in UTF-8
  std::vector<char> buf = {static_cast<char>(0xF0), static_cast<char>(0x9F),
                           static_cast<char>(0x98), static_cast<char>(0x80)};
  EXPECT_TRUE(IsUTF8(buf));
}

TEST(IsUTF8Test, Truncated2ByteSequence) {
  // 0xC2 without continuation byte
  std::vector<char> buf = {static_cast<char>(0xC2)};
  EXPECT_FALSE(IsUTF8(buf));
}

TEST(IsUTF8Test, InvalidContinuationByte) {
  // 0xC2 followed by 0x00 (not 10xxxxxx)
  std::vector<char> buf = {static_cast<char>(0xC2), static_cast<char>(0x00)};
  EXPECT_FALSE(IsUTF8(buf));
}

TEST(IsUTF8Test, Truncated3ByteSequence) {
  // 0xE4 without enough continuation bytes
  std::vector<char> buf = {static_cast<char>(0xE4), static_cast<char>(0xB8)};
  EXPECT_FALSE(IsUTF8(buf));
}

TEST(IsUTF8Test, Truncated4ByteSequence) {
  // 0xF0 without enough continuation bytes
  std::vector<char> buf = {static_cast<char>(0xF0), static_cast<char>(0x9F),
                           static_cast<char>(0x98)};
  EXPECT_FALSE(IsUTF8(buf));
}

TEST(IsUTF8Test, InvalidStartByte) {
  // 0xF8 is not a valid UTF-8 start byte (0xF8 & 0xF8 == 0xF8, not 0xF0)
  std::vector<char> buf = {static_cast<char>(0xF8)};
  EXPECT_FALSE(IsUTF8(buf));
}

TEST(IsUTF8Test, OverlongSequence) {
  // Overlong encoding of '/' (0x2F) as 2-byte: 0xC0 0xAF
  // 0xC0 → (0xC0 & 0xE0) == 0xC0 ✓
  // 0xAF → (0xAF & 0xC0) == 0x80 ✓
  // IsUTF8 does NOT check for overlong, so this returns true
  std::vector<char> buf = {static_cast<char>(0xC0), static_cast<char>(0xAF)};
  // Note: current implementation doesn't reject overlong sequences
  EXPECT_TRUE(IsUTF8(buf));
}

// ===========================================================================
// Tests: RawToUtf16
// ===========================================================================

TEST(RawToUtf16Test, EmptyInputUTF8) {
  std::vector<char> input;
  auto result = RawToUtf16(input, CP_TYPE::UTF8);
  EXPECT_TRUE(result.empty());
}

TEST(RawToUtf16Test, ASCIIviaUTF8) {
  std::vector<char> input = {'H', 'e', 'l', 'l', 'o'};
  auto result = RawToUtf16(input, CP_TYPE::UTF8);
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], u'H');
  EXPECT_EQ(result[1], u'e');
  EXPECT_EQ(result[2], u'l');
  EXPECT_EQ(result[3], u'l');
  EXPECT_EQ(result[4], u'o');
}

TEST(RawToUtf16Test, UTF8WithNonASCII) {
  // "中" in UTF-8: 0xE4 0xB8 0xAD → U+4E2D
  std::vector<char> input = {static_cast<char>(0xE4), static_cast<char>(0xB8),
                             static_cast<char>(0xAD)};
  auto result = RawToUtf16(input, CP_TYPE::UTF8);
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], static_cast<char16_t>(0x4E2D));
}

TEST(RawToUtf16Test, UTF16LEInput) {
  // "Hi" in UTF-16 LE: H\0 i\0
  std::vector<char> input = {'H', '\0', 'i', '\0'};
  auto result = RawToUtf16(input, CP_TYPE::UTF16);
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], u'H');
  EXPECT_EQ(result[1], u'i');
}

TEST(RawToUtf16Test, UTF16BEInput) {
  // "Hi" in UTF-16 BE: \0H \0i
  std::vector<char> input = {'\0', 'H', '\0', 'i'};
  auto result = RawToUtf16(input, CP_TYPE::UTF16BE);
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], u'H');
  EXPECT_EQ(result[1], u'i');
}

TEST(RawToUtf16Test, UnsupportedCodepageThrows) {
  std::vector<char> input = {'H', 'i'};
  EXPECT_THROW(RawToUtf16(input, CP_TYPE::UTF32), std::runtime_error);
  EXPECT_THROW(RawToUtf16(input, CP_TYPE::UTF32BE), std::runtime_error);
  EXPECT_THROW(RawToUtf16(input, CP_TYPE::UNKNOWN), std::runtime_error);
}

// Note: Utf16toRaw uses a bare `throw;` (rethrow) with no active exception,
// which calls std::terminate(). Cannot test until implemented properly.

// ===========================================================================
// Tests: SwapWord16
// ===========================================================================

TEST(SwapWord16Test, SwapsBytes) {
  uint16_t val = 0x1234;
  SwapWord16(val);
  EXPECT_EQ(val, 0x3412);
}

TEST(SwapWord16Test, SwapsBack) {
  uint16_t val = 0x3412;
  SwapWord16(val);
  EXPECT_EQ(val, 0x1234);
}

TEST(SwapWord16Test, Zero) {
  uint16_t val = 0x0000;
  SwapWord16(val);
  EXPECT_EQ(val, 0x0000);
}

TEST(SwapWord16Test, SameHighLow) {
  uint16_t val = 0xFFFF;
  SwapWord16(val);
  EXPECT_EQ(val, 0xFFFF);
}
