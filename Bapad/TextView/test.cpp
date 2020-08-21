#include "TextDocument.h"
#include <iostream>

void test_TextDocument_Openfile()
{
	TextDocument t;
	text T;
	wchar_t p[] = L"C:\\Users\\90314\\Desktop\\niao.txt";
	t.init(p);
	std::cout << 1 << '\n' << T.getbuffer(t) << 23 << std::endl;
}


int main()
{
	
}