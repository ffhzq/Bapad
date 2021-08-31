#include "pch.h"
#include "TextDocument.h"

//initial all var
TextDocument::TextDocument()
    :   buffer(nullptr),
        lineBufferByte(nullptr),
        lineBufferChar(nullptr)
{
    documentLength = 0;
    numLines = 0;
    lengthChars = 0;
    fileFormat = (uint32_t)BOMLookupList().GetInstances().begin()->bom;
    headerSize = BOMLookupList().GetInstances().begin()->headerLength;
}
TextDocument::~TextDocument()
{
    Clear(); 
}



bool TextDocument::Initialize(wchar_t* filename)
{
    HANDLE hFile;
    hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    return Initialize(hFile);
}

bool TextDocument::Initialize(HANDLE hFile)
{
    
    //PLARGE_INTEGER Represents a 64-bit signed integer value
    LARGE_INTEGER TmpDocumentLength = {0};

    //Retrieves the size of the specified file.
    if (GetFileSizeEx(hFile, &TmpDocumentLength) == 0)
    {
        documentLength = 0;
        return false;
    }
    documentLength = TmpDocumentLength.QuadPart;

    // allocate new file-buffer
    const size_t bufferSize = documentLength + 1;
    if ((buffer = new char[bufferSize]) == 0)
    {
        return false;
    }
    memset(buffer, 0, bufferSize);

    ULONG numread = 0;

    // read entire file into memory
    if (ReadFile(hFile, buffer, documentLength, &numread, NULL))
    {
        ;
    }
    


    // work out where each line of text starts
    if (!InitLineBuffer())
    {
        Clear();
    }

    CloseHandle(hFile);
    return true;
}

bool TextDocument::InitLineBuffer()
{
    numLines = 0;

    const size_t linebufferSize = documentLength + 1;

    // allocate the line-buffer
    if ((lineBufferByte = new size_t[linebufferSize]) == 0)
        return false;

    size_t linestart = 0;
    // loop through every byte in the file
    for (size_t i = 0; i < documentLength; )
    {
        if (buffer[i++] == '\r')
        {
            // carriage-return / line-feed combination
            if (buffer[i] == '\n')
                i++;

            // record where the line starts
            if(numLines < linebufferSize)
                lineBufferByte[numLines++] = linestart;
            linestart = i;
        }
    }
    if(documentLength>0&&numLines < linebufferSize)
        lineBufferByte[numLines++] = linestart;
    if (numLines < linebufferSize)
    lineBufferByte[numLines] = documentLength;

    return true;
}

uint32_t TextDocument::DetectFileFormat(int& headerLength)
{
    for (;;);
}


size_t TextDocument::GetLine(size_t lineno, size_t offset, char* buf, size_t len, size_t* fileoff)
{
    if (lineno >= numLines || buffer == nullptr || documentLength == 0)
    {
        return 0;
    }

    // find the start of the specified line
    char* lineptr = buffer + lineBufferByte[lineno];
    // work out how long it is, by looking at the next line's starting point
    size_t linelen = lineBufferByte[lineno + 1] - lineBufferByte[lineno];

    
    offset = min(offset, linelen);
    
    // make sure the CR/LF is always fetched in one go
    if (linelen - (offset + len) < 2 && len > 2)
        len -= 2;

    // make sure we don't overflow caller's buffer
    linelen = min(len, linelen - offset);

    lineptr += offset;

    memcpy(buf, lineptr, linelen);

    if (fileoff)
        *fileoff = lineBufferByte[lineno];// + offset;

    return linelen;
}

size_t TextDocument::GetLine(size_t lineno, char* buf, size_t len, size_t* fileoff)
{
    return GetLine(lineno, 0, buf, len, fileoff);
}

size_t TextDocument::GetData(size_t offset, char* buf, size_t len)
{
    memcpy(buf, buffer + offset, len);
    return len;
}

const uint32_t TextDocument::GetFileFormat() const
{
    return fileFormat;
}

const size_t TextDocument::GetLineCount() const
{
    return numLines;
}

const size_t TextDocument::GetLongestLine(int tabwidth = 4) const
{
    size_t longest = 0;
    size_t xpos = 0;

    for (size_t i = 0; i < documentLength; i++)
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

const size_t TextDocument::GetDocLength() const
{
    return documentLength;
}

bool TextDocument::Clear()
{

    if (buffer)
    {
        documentLength = 0;
        delete[] buffer;
        buffer = nullptr;
    }
    if (lineBufferByte)
    {
        numLines = 0;
        delete[] lineBufferByte;
        lineBufferByte = nullptr;
    }
    return true;
}

bool TextDocument::OffsetToLine(size_t fileoffset, size_t* lineno, size_t* offset)
{
    size_t low = 0, high = numLines - 1;
    size_t line = 0;

    if (numLines == 0)
    {
        if (lineno) *lineno = 0;
        if (offset) *offset = 0;
        return false;
    }

    while (low <= high)
    {
        line = (high + low) / 2;

        if (fileoffset >= lineBufferByte[line] && fileoffset < lineBufferByte[line + 1])
        {
            break;
        }
        else if (fileoffset < lineBufferByte[line])
        {
            high = line - 1;
        }
        else
        {
            low = line + 1;
        }
    }

    if (lineno)  *lineno = line;
    if (offset)	*offset = fileoffset - lineBufferByte[line];

    return true;
}

bool TextDocument::GetLineInfo(size_t lineno, size_t* fileoff, size_t* length)
{
    if (lineno < numLines)
    {
        if (length)
            *length = lineBufferByte[lineno + 1] - lineBufferByte[lineno];

        if (fileoff)
            *fileoff = lineBufferByte[lineno];

        return true;
    }
    else
    {
        return false;
    }
}
