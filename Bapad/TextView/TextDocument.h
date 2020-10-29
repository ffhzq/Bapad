#pragma once
#include "pch.h"

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
  
    bool  offset_to_line(ULONG fileoffset, ULONG* lineno, ULONG* offset);
    bool  getlineinfo(ULONG lineno, ULONG* fileoff, ULONG* length);

    size_t getline(size_t lineno, wchar_t* buf, size_t len, ULONG* fileoff = 0);
    LONG getline(ULONG lineno, ULONG offset, wchar_t* buf, size_t len, ULONG* fileoff = 0);
    ULONG getdata(ULONG offset, wchar_t* buf, size_t len);

    size_t getLinecount();

    size_t getLongestline(int tabwidth);

    const size_t getDocLength() const;
    
private:
    bool init_linebuffer();

    //std::unique_ptr<char*> buffer;
    //wchar_t* buffer;
    wchar_t* buffer;
    size_t DocumentLength;
    //point to an array which record the sequence's num of newline char
    size_t* linebuffer;
    //num of lines
    size_t  numlines;
    
};