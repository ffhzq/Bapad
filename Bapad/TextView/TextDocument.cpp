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
    

    //PLARGE_INTEGER Represents a 64-bit signed integer value; and
    std::unique_ptr<LARGE_INTEGER> TmpDocumentLength(new LARGE_INTEGER());
    TmpDocumentLength->QuadPart = 0;


    //PLARGE_INTEGER TmpDocumentLength = new LARGE_INTEGER(); delete TmpDocumentLength;
    
    

    //Retrieves the size of the specified file.
    if (GetFileSizeEx(hFile, TmpDocumentLength.get()) == 0)
    {
        DocumentLength = 0;
        //DWORD dwError = GetLastError();
        return false;
    }
    DocumentLength = TmpDocumentLength->QuadPart;

    //1 wchar_t is 2 bytes long 
    // allocate new file-buffer
    if ((buffer = new WCHAR[DocumentLength]) == 0)
    {
        return false;
    }

    //NumberOfBytesRead
    ULONG numread = 0;

    DWORD DWError = 0;

    // read entire file into memory
    if ((DWError = ReadFile(hFile, buffer, DocumentLength, &numread, 0)) != TRUE)
    {
        if(DWError == ERROR_INVALID_USER_BUFFER || DWError == ERROR_NOT_ENOUGH_MEMORY)
        {/*too many outstanding asynchronous I/O requests*/ }
        else if (DWError == ERROR_IO_PENDING)
        {/*
         is not a failure; it designates the read operation is pending completion asynchronously
         */
        }
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

    linebuffer[numlines] = DocumentLength;
    return true;
}

ULONG TextDocument::getline(ULONG lineno, WCHAR* buf, size_t len)
{
    WCHAR* lineptr=nullptr;
    ULONG linelen=0;

    // find the start of the specified line
    lineptr = buffer + linebuffer[lineno];

    // work out how long it is, by looking at the next line's starting point
    linelen = linebuffer[lineno + 1] - linebuffer[lineno];

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
