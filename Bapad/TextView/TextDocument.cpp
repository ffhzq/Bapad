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
    numlines = 0;

    const size_t linebufferSize = DocumentLength + 1;

    // allocate the line-buffer
    if ((linebuffer = new size_t[linebufferSize]) == 0)
        return false;

    size_t linestart = 0;
    // loop through every byte in the file
    for (size_t i = 0; i < DocumentLength; )
    {
        if (buffer[i++] == '\r')
        {
            // carriage-return / line-feed combination
            if (buffer[i] == '\n')
                i++;

            // record where the line starts
            if(numlines < linebufferSize)
                linebuffer[numlines++] = linestart;
            linestart = i;
        }
    }
    if(DocumentLength>0&&numlines < linebufferSize)
        linebuffer[numlines++] = linestart;
    if (numlines < linebufferSize)
    linebuffer[numlines] = DocumentLength;

    return true;
}


size_t TextDocument::getline(size_t lineno, size_t offset, wchar_t* buf, size_t len, size_t* fileoff)
{
    if (lineno >= numlines || buffer == nullptr || DocumentLength == 0)
    {
        return 0;
    }

    // find the start of the specified line
    wchar_t* lineptr = buffer + linebuffer[lineno];
    // work out how long it is, by looking at the next line's starting point
    size_t linelen = linebuffer[lineno + 1] - linebuffer[lineno];

    
    offset = min(offset, linelen);
    
    // make sure the CR/LF is always fetched in one go
    if (linelen - (offset + len) < 2 && len > 2)
        len -= 2;

    // make sure we don't overflow caller's buffer
    linelen = min(len, linelen - offset);

    lineptr += offset;

    wmemcpy(buf, lineptr, linelen);

    if (fileoff)
        *fileoff = linebuffer[lineno];// + offset;

    return linelen;
}

size_t TextDocument::getline(size_t lineno, wchar_t* buf, size_t len, size_t* fileoff)
{
    return getline(lineno, 0, buf, len, fileoff);
}

size_t TextDocument::getdata(size_t offset, wchar_t* buf, size_t len)
{
    wmemcpy(buf, buffer + offset, len);
    return len;
}

const size_t TextDocument::getLinecount() const
{
    return numlines;
}

const size_t TextDocument::getLongestline(int tabwidth = 4) const
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
        buffer = nullptr;
    }
    if (linebuffer)
    {
        numlines = 0;
        delete[] linebuffer;
        linebuffer = nullptr;
    }
    return true;
}

bool TextDocument::offset_to_line(size_t fileoffset, size_t* lineno, size_t* offset)
{
    size_t low = 0, high = numlines - 1;
    size_t line = 0;

    if (numlines == 0)
    {
        if (lineno) *lineno = 0;
        if (offset) *offset = 0;
        return false;
    }

    while (low <= high)
    {
        line = (high + low) / 2;

        if (fileoffset >= linebuffer[line] && fileoffset < linebuffer[line + 1])
        {
            break;
        }
        else if (fileoffset < linebuffer[line])
        {
            high = line - 1;
        }
        else
        {
            low = line + 1;
        }
    }

    if (lineno)  *lineno = line;
    if (offset)	*offset = fileoffset - linebuffer[line];

    return true;
}

bool TextDocument::getlineinfo(size_t lineno, size_t* fileoff, size_t* length)
{
    if (lineno < numlines)
    {
        if (length)
            *length = linebuffer[lineno + 1] - linebuffer[lineno];

        if (fileoff)
            *fileoff = linebuffer[lineno];

        return true;
    }
    else
    {
        return false;
    }
}
