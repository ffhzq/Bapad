#pragma once
#include "pch.h"

class TextDocument
{

public:
    TextDocument();
    ~TextDocument();

    //Initialize the TextDocument with the specified file
    bool    init(wchar_t* filename);
    //	Initialize using a file-handle
    bool    init(HANDLE hFile);

    bool    clear();
  
    //	Perform a reverse lookup - file-offset to line number
    bool    offset_to_line(size_t fileoffset, size_t* lineno, size_t* offset);
    bool    getlineinfo(size_t lineno, size_t* fileoff, size_t* length);

    size_t  getline(size_t lineno, wchar_t* buf, size_t len, size_t* fileoff = 0);
    size_t  getline(size_t lineno, size_t offset, wchar_t* buf, size_t len, size_t* fileoff = 0);
    size_t  getdata(size_t offset, wchar_t* buf, size_t len);

    const size_t getLinecount() const;
    const size_t getLongestline(int tabwidth) const;
    const size_t getDocLength() const;
    
private:
    bool    init_linebuffer();

    wchar_t*    buffer;
    size_t*     linebuffer;
    size_t      DocumentLength;
    size_t      numlines;
    
};