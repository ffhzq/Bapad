#include "pch.h"

static union { char c[4]; unsigned long mylong; } endian_test = { { 'l', '?', '?', 'b' } };
#define ENDIANNESS ((char)endian_test.mylong)

TEST(DataEndian, ByteOrder)
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
TEST(SWAPWORDCh16, FormatConvertion)
{
    union {
        char c[2];
        char16_t ch16;
    } e = { {'1','2'} };
    zq::SwapWord16(e.ch16);
    ASSERT_EQ(e.c[0], '2');
    ASSERT_EQ(e.c[1], '1');
}
TEST(SWAPWORDWCh, FormatConvertion)
{
union {
    char c[2];
    wchar_t wCh;
} e = { 0x12 };
zq::SwapWord16(e.wCh);
ASSERT_EQ(e.c[0], 2);
ASSERT_EQ(e.c[1], 1);
}
TEST(SWAPWORDMACRO, FormatConvertion)
{
    union {
        char c[2];
        wchar_t wCh;
    } e = { '1','2'};
    //ASSERT_EQ(SwapWord(e.wCh), '2');
}
