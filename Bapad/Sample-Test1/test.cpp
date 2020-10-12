#include "pch.h"
//#include "../TextView/TextDocument.h"
#include "../TextView/TextView.h"

TEST(TestTest1,TestTest)
{

	EXPECT_EQ(1, 1);
	EXPECT_TRUE(true);
}


TEST(TextDocumentOpenAndGetText1, TextDocumentOpenAndGetText) {
	TextDocument t;
	//GetModuleFileNameW()
	const int bufferSize = 256;
	wchar_t fileName[bufferSize] = L"0";//C:\\Users\\Public\\Desktop\\niao.txt
	if (!_wgetcwd(fileName, bufferSize))
	{
		EXPECT_TRUE(false);
	}
	if (t.init(fileName))
	{
		wchar_t outputBuffer[bufferSize] = { 0 };
		t.getline(0, outputBuffer, bufferSize);
		{
			if (outputBuffer != L"")
			{
				EXPECT_TRUE(true);
			}
			
			using namespace std;
			wcout << outputBuffer << endl;
		}
	}
}