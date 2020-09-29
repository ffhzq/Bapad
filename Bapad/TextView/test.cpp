#include "TextDocument.h"
#include <iostream>
#include <memory>

/*class test
{
public:

	char* getbuffer()
	{
		return t.getline();
	}
private:
	TextDocument t;
};*/


void test_TextDocument_Openfile()
{
	std::unique_ptr<TextDocument> t(new TextDocument);
	wchar_t fileName[] = L"C:\\Users\\Public\\Desktop\\niao.txt";

	if (t->init(fileName))
	{
	const int bufferSize = 256;
	wchar_t outputBuffer[bufferSize] = {0};

	t->getline(0, outputBuffer, bufferSize);
	using namespace std;
	setlocale(LC_CTYPE, "zh-cn");
	wcout << outputBuffer << L"heyheyyouyouyouÍê±Ï!Á¹¹¬´ºÈÕOVER" << endl;
	
	//std::wcout << std::endl;
	//std::wcout <<  << endl;
	}
}


int main()
{
	test_TextDocument_Openfile();
}