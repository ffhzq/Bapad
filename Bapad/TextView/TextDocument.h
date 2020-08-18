#pragma once
#include <windows.h>

class TextDocument
{
    friend class text;
public:
    bool init(wchar_t* filename);
    bool init(HANDLE hFile);
    bool clear();
    size_t getline(size_t lineno, wchar_t* buf, size_t len);
    size_t linecount();
    size_t longestline(int tabwidth);

private:
    bool init_linebuffer();

    char* buffer;
    DWORD DocumentLength;

    size_t* linebuffer;
    size_t  numlines;

};

class text
{
public:
    char* getbuffer(TextDocument t)
    {
        return t.buffer;
    }
};
bool TextDocument::init(wchar_t* filename)
{
    HANDLE hFile;
    hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    return init(hFile);
}

bool TextDocument::init(HANDLE hFile)
{
    ULONG numread;

    if ((DocumentLength = GetFileSize(hFile, 0)) == 0)
        return false;

    // allocate new file-buffer
    if ((buffer = new char[DocumentLength]) == 0)
        return false;

    // read entire file into memory
    if (ReadFile(hFile, buffer, DocumentLength, &numread, 0) == FALSE)
    {
        //

    }

    // work out where each line of text starts
    init_linebuffer();

    CloseHandle(hFile);
    return true;
}

bool TextDocument::init_linebuffer()
{
    size_t i = 0;
    size_t linestart = 0;

    // allocate the line-buffer
    if ((linebuffer = new size_t[DocumentLength]) == 0)
        return false;

    numlines = 0;

    // loop through every byte in the file
    for (i = 0; i < DocumentLength; )
    {
        if (buffer[i++] == '\r')
        {
            // carriage-return / line-feed combination
            if (buffer[i] == '\n')
                i++;

            // record where the line starts
            linebuffer[numlines++] = linestart;
            linestart = i;
        }
    }
    
    //linebuffer[numlines] = DocumentLength;//wrong
    return true;
}

size_t TextDocument::getline(size_t lineno, wchar_t* buf, size_t len)
{
    char* lineptr;
    size_t linelen;

    // find the start of the specified line
    lineptr = buffer + linebuffer[lineno];

    // work out how long it is, by looking at the next line's starting point
    linelen = linebuffer[lineno + 1] - linebuffer[lineno];

    // make sure we don't overflow caller's buffer
    linelen = min(len, linelen);

    memcpy(buf, lineptr, linelen);

    return linelen;
}

size_t TextDocument::linecount()
{
    return numlines;
}

size_t TextDocument::longestline(int tabwidth)
{//???
    return 100;
}