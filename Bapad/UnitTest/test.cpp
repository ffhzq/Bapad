#include "pch.h"


static union { char c[4]; unsigned long mylong; } endian_test = { { 'l', '?', '?', 'b' } };
#define ENDIANNESS ((char)endian_test.mylong)


TEST(TestCaseName, TestName) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

TEST(TestEndianSimple, TestEndian)
{ 
    ASSERT_EQ(ENDIANNESS, 'l');
}

//TEST(LittleToBig16, FormatConvertion)
//{
//    union {
//        char c[2];
//        char16_t ch16;
//    } e = { {'1','2'}};
//    zq::LittleToBig16(e.ch16);
//    
//    ASSERT_EQ(e.c[0], '2');
//    ASSERT_EQ(e.c[1], '1');
//
//}
TEST(SWAPWORD, FormatConvertion)
{
    union {
        char c[2];
        char16_t ch16;
    } e = { {'1','2'} };
    zq::SwapWord(e.ch16);

    ASSERT_EQ(e.c[0], '2');
    ASSERT_EQ(e.c[1], '1');

    wchar_t ch16;
    zq::SwapWord16(ch16);
}