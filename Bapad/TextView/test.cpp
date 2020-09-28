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
	wchar_t fileName[] = L"C:\\Users\\90314\\Desktop\\niao.txt";
	t->init(fileName);
	const size_t bufferSize = 100;
	WCHAR outputBuffer[bufferSize] = {0};

	
	t->getline(0, outputBuffer, bufferSize-1);
	using namespace std;
	for(size_t i = 0; i < bufferSize; ++i)
		std::wcout << outputBuffer[i];
	std::wcout << std::endl;
	std::wcout << "Íê±Ï" << endl;
}


int main()
{
	test_TextDocument_Openfile();
}