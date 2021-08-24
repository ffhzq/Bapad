#pragma once
#include "pch.h"

class TextDocument
{

public:
    TextDocument();
    ~TextDocument();

    //Initialize the TextDocument with the specified file
    bool    Initialize(wchar_t* filename);
    //	Initialize using a file-handle
    bool    Initialize(HANDLE hFile);

    bool    Clear();
  
    //	Perform a reverse lookup - file-offset to line number
    bool    OffsetToLine(size_t fileoffset, size_t* lineno, size_t* offset);
    bool    GetLineInfo(size_t lineno, size_t* fileoff, size_t* length);

    size_t  GetLine(size_t lineno, wchar_t* buf, size_t len, size_t* fileoff = 0);
    size_t  GetLine(size_t lineno, size_t offset, wchar_t* buf, size_t len, size_t* fileoff = 0);
    size_t  GetData(size_t offset, wchar_t* buf, size_t len);


    const size_t GetLineCount() const;
    const size_t GetLongestLine(int tabwidth) const;
    const size_t GetDocLength() const;
    
private:
    bool    InitLineBuffer();

    wchar_t*    Buffer;
    size_t*     LineBuffer;
    size_t      DocumentLength;
    size_t      NumLines;
    
};

//Byte Order Mark
struct BOM
{
    uint32_t UTF8 = 0xEFBBBF00;//后缀0去掉
    uint32_t UTF16LE = 0xFFFE0000;//后缀0去掉
    uint32_t UTF16BE = 0xFEFF0000;//后缀0去掉
    uint32_t UTF32LE = 0xFFFE0000;
    uint32_t UTF32BE = 0x0000FEFF;
};