#include "pch.h"
#include "TextDocument.h"

//initial all var
TextDocument::TextDocument()
    :   Buffer(nullptr),
        LineBuffer(nullptr),
        DocumentLength(0),
        NumLines(0)
{
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
        DocumentLength = 0;
        return false;
    }
    DocumentLength = TmpDocumentLength.QuadPart;

    // allocate new file-buffer
    const size_t bufferSize = DocumentLength + 1;
    if ((Buffer = new wchar_t[bufferSize]) == 0)
    {
        return false;
    }
    wmemset(Buffer, 0, bufferSize);

    ULONG numread = 0;

    // read entire file into memory
    if (ReadFile(hFile, Buffer, DocumentLength, &numread, NULL))
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
    NumLines = 0;

    const size_t linebufferSize = DocumentLength + 1;

    // allocate the line-buffer
    if ((LineBuffer = new size_t[linebufferSize]) == 0)
        return false;

    size_t linestart = 0;
    // loop through every byte in the file
    for (size_t i = 0; i < DocumentLength; )
    {
        if (Buffer[i++] == '\r')
        {
            // carriage-return / line-feed combination
            if (Buffer[i] == '\n')
                i++;

            // record where the line starts
            if(NumLines < linebufferSize)
                LineBuffer[NumLines++] = linestart;
            linestart = i;
        }
    }
    if(DocumentLength>0&&NumLines < linebufferSize)
        LineBuffer[NumLines++] = linestart;
    if (NumLines < linebufferSize)
    LineBuffer[NumLines] = DocumentLength;

    return true;
}


size_t TextDocument::GetLine(size_t lineno, size_t offset, wchar_t* buf, size_t len, size_t* fileoff)
{
    if (lineno >= NumLines || Buffer == nullptr || DocumentLength == 0)
    {
        return 0;
    }

    // find the start of the specified line
    wchar_t* lineptr = Buffer + LineBuffer[lineno];
    // work out how long it is, by looking at the next line's starting point
    size_t linelen = LineBuffer[lineno + 1] - LineBuffer[lineno];

    
    offset = min(offset, linelen);
    
    // make sure the CR/LF is always fetched in one go
    if (linelen - (offset + len) < 2 && len > 2)
        len -= 2;

    // make sure we don't overflow caller's buffer
    linelen = min(len, linelen - offset);

    lineptr += offset;

    wmemcpy(buf, lineptr, linelen);

    if (fileoff)
        *fileoff = LineBuffer[lineno];// + offset;

    return linelen;
}

size_t TextDocument::GetLine(size_t lineno, wchar_t* buf, size_t len, size_t* fileoff)
{
    return GetLine(lineno, 0, buf, len, fileoff);
}

size_t TextDocument::GetData(size_t offset, wchar_t* buf, size_t len)
{
    wmemcpy(buf, Buffer + offset, len);
    return len;
}

const size_t TextDocument::GetLineCount() const
{
    return NumLines;
}

const size_t TextDocument::GetLongestLine(int tabwidth = 4) const
{
    size_t longest = 0;
    size_t xpos = 0;

    for (size_t i = 0; i < DocumentLength; i++)
    {
        if (Buffer[i] == '\r')
        {
            if (Buffer[i + 1] == '\n')
                i++;

            longest = max(longest, xpos);
            xpos = 0;
        }
        else if (Buffer[i] == '\t')
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
    return DocumentLength;
}

bool TextDocument::Clear()
{

    if (Buffer)
    {
        DocumentLength = 0;
        delete[] Buffer;
        Buffer = nullptr;
    }
    if (LineBuffer)
    {
        NumLines = 0;
        delete[] LineBuffer;
        LineBuffer = nullptr;
    }
    return true;
}

bool TextDocument::OffsetToLine(size_t fileoffset, size_t* lineno, size_t* offset)
{
    size_t low = 0, high = NumLines - 1;
    size_t line = 0;

    if (NumLines == 0)
    {
        if (lineno) *lineno = 0;
        if (offset) *offset = 0;
        return false;
    }

    while (low <= high)
    {
        line = (high + low) / 2;

        if (fileoffset >= LineBuffer[line] && fileoffset < LineBuffer[line + 1])
        {
            break;
        }
        else if (fileoffset < LineBuffer[line])
        {
            high = line - 1;
        }
        else
        {
            low = line + 1;
        }
    }

    if (lineno)  *lineno = line;
    if (offset)	*offset = fileoffset - LineBuffer[line];

    return true;
}

bool TextDocument::GetLineInfo(size_t lineno, size_t* fileoff, size_t* length)
{
    if (lineno < NumLines)
    {
        if (length)
            *length = LineBuffer[lineno + 1] - LineBuffer[lineno];

        if (fileoff)
            *fileoff = LineBuffer[lineno];

        return true;
    }
    else
    {
        return false;
    }
}
