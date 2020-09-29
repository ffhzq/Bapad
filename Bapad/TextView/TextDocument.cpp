#include "TextDocument.h"

//initial all var
TextDocument::TextDocument()
{
    buffer = nullptr;
    linebuffer = nullptr;
    DocumentLength = 0;
    numlines = 0;
    
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
        //DWORD dwError = GetLastError();
        return false;
    }
    DocumentLength = TmpDocumentLength.QuadPart;

    //1 wchar_t is 2 bytes long 
    // allocate new file-buffer
    //std::unique_ptr<char[]> readBuffer(new char[DocumentLength]);
    //memset(readBuffer.get(), 0, DocumentLength);
    if ((buffer = new wchar_t[DocumentLength+1]) == 0)
    {
        return false;
    }
    memset(buffer, 0, sizeof(wchar_t) * (DocumentLength + 1));

    ULONG numread = 0;
    // read entire file into memory
    if (ReadFile(hFile, buffer, DocumentLength, &numread, 0))
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
    ULONG i = 0;
    ULONG linestart = 0;

    // allocate the line-buffer
    if ((linebuffer = new ULONG[static_cast<LONGLONG>(DocumentLength)+1]) == 0)
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

ULONG TextDocument::getline(ULONG lineno, wchar_t* buf, size_t len)
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
    ULONG linelen = linebuffer[lineno + 1] - linebuffer[lineno];



    // make sure we don't overflow caller's buffer
    linelen = min(len, linelen);
    memcpy(buf, lineptr, linelen);

    return linelen;
}

ULONG TextDocument::getLinecount()
{
    return numlines;
}

ULONG TextDocument::getLongestline(int tabwidth = 4)
{
    ULONG longest = 0;
    ULONG xpos = 0;

    for (ULONG i = 0; i < DocumentLength; i++)
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
