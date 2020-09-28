#pragma once
#include "lib.h"

class TextDocument
{

public:
    TextDocument();
    ~TextDocument();
    //Get Handle of file then invoke init(HANDLE hFile) ,if success return true,or return false 
    bool init(wchar_t* filename);
    //
    bool init(HANDLE hFile);

    bool clear();

    ULONG getline(ULONG lineno, WCHAR* buf, size_t len);

    ULONG getLinecount();
    ULONG getLongestline(int tabwidth);

private:
    bool init_linebuffer();

    //std::unique_ptr<char*> buffer;
    WCHAR* buffer;
    ULONG DocumentLength;
    //point to an array which record the sequence's num of newline char
    ULONG* linebuffer;
    //num of lines
    ULONG  numlines;

};