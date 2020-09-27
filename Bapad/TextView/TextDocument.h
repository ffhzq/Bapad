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

    size_t getline(size_t lineno, wchar_t* buf, size_t len);

    size_t getLinecount();

    size_t getLongestline(int tabwidth);

private:
    bool init_linebuffer();

    //std::unique_ptr<char*> buffer;
    char* buffer;

    size_t DocumentLength;
    //point to an array which record the sequence's num of newline char
    size_t* linebuffer;
    //num of lines
    size_t  numlines;

};