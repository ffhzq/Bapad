#pragma once
#include "lib.h"

class TextDocument
{
    //for testing openfile
    friend class text;
public:
    TextDocument();
    ~TextDocument();

    bool init(wchar_t* filename);
    bool init(HANDLE hFile);
    bool clear();
    size_t getline(size_t lineno, wchar_t* buf, size_t len);
    size_t linecount();
    size_t longestline(int tabwidth);

private:
    bool init_linebuffer();
    //std::unique_ptr<char*> buffer;
    char* buffer;

    size_t DocumentLength;

    size_t* linebuffer;
    size_t  numlines;

};

//TODO:delete it
class text
{
public:
    char* getbuffer(TextDocument t)
    {
        return t.buffer;
    }
};