#include "pch.h"
#include "TextDocument.h"

//initial all var
TextDocument::TextDocument()
    :   buffer(nullptr),
        linebuffer(nullptr),
        DocumentLength(0),
        numlines(0)
{
}
TextDocument::~TextDocument()
{
    clear();
}



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
    
    //PLARGE_INTEGER Represents a 64-bit signed integer value
    LARGE_INTEGER TmpDocumentLength = {0};

    //Retrieves the size of the specified file.
    if (GetFileSizeEx(hFile, &TmpDocumentLength) == 0)
    {
        DocumentLength = 0;
        return false;
    }
    DocumentLength = TmpDocumentLength.QuadPart;

    // allocate new file-buffer
    const size_t bufferSize = DocumentLength + 1;
    if ((buffer = new wchar_t[bufferSize]) == 0)
    {
        return false;
    }
    wmemset(buffer, 0, bufferSize);

    ULONG numread = 0;

    // read entire file into memory
    if (ReadFile(hFile, buffer, DocumentLength, &numread, NULL))
    {
        ;
    }
    


    // work out where each line of text starts
    if (!init_linebuffer())
    {
        clear();
    }

    CloseHandle(hFile);
    return true;
}

bool TextDocument::init_linebuffer()
{
    size_t i = 0;
    size_t linestart = 0;

    const size_t linebufferSize = DocumentLength + 1;

    // allocate the line-buffer
    if ((linebuffer = new size_t[linebufferSize]) == 0)
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
    if (numlines == 0)linebuffer[numlines++] = linestart;
    linebuffer[numlines] = DocumentLength;

    return true;
}

size_t TextDocument::getline(size_t lineno, wchar_t* buf, size_t len, ULONG* fileoff)
{
    /*
    // find the start of the specified line
    wchar_t* lineptr = buffer + (lineno == 0 ? 0 : linebuffer[lineno]);
    // work out how long it is, by looking at the next line's starting point
    ULONG linelen = (lineno == 0 ? linebuffer[lineno] - 0 : (linebuffer[lineno] - linebuffer[lineno - 1]));
    */

    // find the start of the specified line
    wchar_t* lineptr = buffer + linebuffer[lineno];
    // work out how long it is, by looking at the next line's starting point
    size_t linelen = linebuffer[lineno + 1] - linebuffer[lineno];



    // make sure we don't overflow caller's buffer OR use wmemcpy_s
    linelen = min(len, linelen);
    wmemcpy(buf, lineptr, linelen);

    return linelen;
}

LONG TextDocument::getline(ULONG lineno, ULONG offset, wchar_t* buf, size_t len, ULONG* fileoff)
{
    return 0;
}

ULONG TextDocument::getdata(ULONG offset, wchar_t* buf, size_t len)
{
    return 0;
}

size_t TextDocument::getLinecount()
{
    return numlines;
}

size_t TextDocument::getLongestline(int tabwidth = 4)
{
    size_t longest = 0;
    size_t xpos = 0;

    for (size_t i = 0; i < DocumentLength; i++)
    {
        if (buffer[i] == '\r')
        {
            if (buffer[i + 1] == '\n')
                i++;

            longest = max(longest, xpos);
            xpos = 0;
        }
        else if (buffer[i] == '\t')
        {
            xpos += tabwidth - (xpos % tabwidth);
        }
        else
        {
            xpos++;
        }
    }

    longest = max(longest, xpos);
    return longest;
}

const size_t TextDocument::getDocLength() const
{
    return DocumentLength;
}

bool TextDocument::clear()
{

    if (buffer)
    {
        DocumentLength = 0;
        delete[] buffer;
    }
    if (linebuffer)
    {
        numlines = 0;
        delete[] linebuffer;
    }
    return true;
}

bool TextDocument::offset_to_line(ULONG fileoffset, ULONG* lineno, ULONG* offset)
{
    return false;
}

bool TextDocument::getlineinfo(ULONG lineno, ULONG* fileoff, ULONG* length)
{
    return false;
}
