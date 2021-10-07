#include "pch.h"
#include "TextDocument.h"
#include "formatConversion.h"

//initial all var
TextDocument::TextDocument()
    : docBuffer(nullptr),
    lineOffsetByte(nullptr),
    lineOffsetChar(nullptr)
{
    lengthBytes = 0;
    LineCount = 0;
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
    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    return Initialize(hFile);
}

bool TextDocument::Initialize(HANDLE hFile)
{

    //PLARGE_INTEGER Represents a 64-bit signed integer value
    LARGE_INTEGER TmpDocumentLength = { 0 };

    //Retrieves the size of the specified file.
    if (GetFileSizeEx(hFile, &TmpDocumentLength) == 0)
    {
        lengthBytes = 0;
        return false;
    }
    lengthBytes = TmpDocumentLength.QuadPart;

    // allocate new file-buffer
    const size_t bufferSize = lengthBytes;// +1;
    if ((docBuffer = new char[bufferSize]) == 0)
    {
        return false;
    }

    //memset(buffer, 0, bufferSize);

    ULONG numread = 0;
    // read entire file into memory
    if (ReadFile(hFile, docBuffer, lengthBytes, &numread, NULL))
    {
        ;
    }

    fileFormat = DetectFileFormat(headerSize);

    // work out where each line of text starts
    if (!InitLineBuffer())
        Clear();

    CloseHandle(hFile);
    return true;
}


bool TextDocument::InitLineBuffer()
{
    const size_t bufLen = lengthBytes - headerSize;
    //NEED TO BE UPDATED 用std::vector
    if ((lineOffsetByte = new size_t[bufLen]) == 0)
        return false;

    if ((lineOffsetChar = new size_t[bufLen]) == 0)
        return false;

    LineCount = 0;

    size_t offsetBytes = 0, offsetChars = 0;
    size_t lineStartBytes = 0, lineStartChars = 0;
    for (; offsetBytes < bufLen;)
    {
        char32_t ch32;
        size_t len = GetChar(offsetBytes, bufLen - offsetBytes, ch32);
        offsetBytes += len;
        offsetChars += 1;

        // NEED TO BE UPDATED 判断语句的变量类型不同！
        if (ch32 == '\r')
        {
            lineOffsetByte[LineCount] = lineStartBytes;
            lineOffsetChar[LineCount] = lineStartChars;
            lineStartBytes = offsetBytes;
            lineStartChars = offsetChars;

            len = GetChar(offsetBytes, bufLen - offsetBytes, ch32);
            offsetBytes += len;
            offsetChars += 1;

            if (ch32 == '\n')
            {
                lineStartBytes = offsetBytes;
                lineStartChars = offsetChars;
            }
            LineCount++;
        }
        else if (ch32 == '\n')
        {
            lineOffsetByte[LineCount] = lineStartBytes;
            lineOffsetChar[LineCount] = lineStartChars;
            lineStartBytes = offsetBytes;
            lineStartChars = offsetChars;
            LineCount++;
        }
    }

    if (bufLen > 0)
    {
        lineOffsetByte[LineCount] = lineStartBytes;
        lineOffsetChar[LineCount] = lineStartChars;
        LineCount++;
    }
    //SIZE_MAX
    lineOffsetByte[LineCount] = bufLen;
    lineOffsetChar[LineCount] = offsetChars;
    return true;
}

uint32_t TextDocument::DetectFileFormat(int& headerSize)
{
    const char* p = docBuffer;
    auto BOMLOOK = BOMLookupList().GetInstances();
    for (auto i : BOMLOOK)
    {
        if (lengthBytes >= i.headerLength
            && memcmp(p, &i.bom, i.headerLength) == 0)
        {
            headerSize = i.headerLength;
            return (uint32_t)i.bom;
        }
    }

    headerSize = 0;
    return (uint32_t)BOM::ASCII;

}

int TextDocument::GetChar(size_t offset, size_t lenBytes, char32_t& pch32)
{

    Byte* rawData = (Byte*)(docBuffer + offset + headerSize);
    Word* rawDataW = (Word*)(docBuffer + offset + headerSize);
    Word ch16;
    size_t ch32Len = 1;


    switch (fileFormat)
    {
        // convert from ANSI->UNICODE
    case (uint32_t)BOM::ASCII:
        MultiByteToWideChar(CP_ACP, 0, (char*)rawData, 1, (wchar_t*)&ch16, 1);
        pch32 = ch16;
        return 1;

    case (uint32_t)BOM::UTF16LE:
        //*pch32 = (ULONG)(WORD)rawdata_w[0];
        //return 2;

        return UTF16ToUTF32((wchar_t*)rawDataW, lenBytes / 2, (ULONG*)pch32, ch32Len) * 2;

    case (uint32_t)BOM::UTF16BE:
        //*pch32 = (ULONG)(WORD)SWAPWORD((WORD)rawdata_w[0]);
        //return 2;

        return UTF16BEToUTF32((wchar_t*)rawDataW, lenBytes / 2, (ULONG*)pch32, ch32Len) * 2;


    case (uint32_t)BOM::UTF8:
        return UTF8ToUTF32(rawData, lenBytes, pch32);

    default:
        return 0;
    }

    return 0;
}

int TextDocument::GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t & bufLen)
{
    Byte* rawdata = (Byte*)(docBuffer + offset + headerSize);
    size_t  len;

    if (offset >= lengthBytes)
    {
        bufLen = 0;
        return 0;
    }

    switch (fileFormat)
    {
        // convert from ANSI->UNICODE
    case (uint32_t)BOM::ASCII:
        return AsciiToUTF16(rawdata, lenBytes, buf, bufLen);

    case (uint32_t)BOM::UTF8:
        return UTF8ToUTF16(rawdata, lenBytes, buf, bufLen);

        // already unicode, do a straight memory copy
    case (uint32_t)BOM::UTF16LE:
        return CopyUTF16(reinterpret_cast<wchar_t*>(rawdata), lenBytes / sizeof(wchar_t), buf, bufLen);

        // need to convert from big-endian to little-endian
    case (uint32_t)BOM::UTF16BE:
        return SwapUTF16(reinterpret_cast<wchar_t*>(rawdata), lenBytes / sizeof(wchar_t), buf, bufLen);

        // error! we should *never* reach this point
    default:
        bufLen = 0;
        return 0;
    }
}


//size_t TextDocument::GetLine(size_t lineno, size_t offset, char* buf, size_t len, size_t* fileoff)
//{
//    if (lineno >= numLines || buffer == nullptr || lengthBytes == 0)
//    {
//        return 0;
//    }
//
//    // find the start of the specified line
//    char* lineptr = buffer + lineBufferByte[lineno];
//    // work out how long it is, by looking at the next line's starting point
//    size_t linelen = lineBufferByte[lineno + 1] - lineBufferByte[lineno];
//
//    
//    offset = min(offset, linelen);
//    
//    // make sure the CR/LF is always fetched in one go
//    if (linelen - (offset + len) < 2 && len > 2)
//        len -= 2;
//
//    // make sure we don't overflow caller's buffer
//    linelen = min(len, linelen - offset);
//
//    lineptr += offset;
//
//    memcpy(buf, lineptr, linelen);
//
//    if (fileoff)
//        *fileoff = lineBufferByte[lineno];// + offset;
//
//    return linelen;
//}
//
//size_t TextDocument::GetLine(size_t lineno, char* buf, size_t len, size_t* fileoff)
//{
//    return GetLine(lineno, 0, buf, len, fileoff);
//}

//size_t TextDocument::GetData(size_t offset, char* buf, size_t len)
//{
//    memcpy(buf, docBuffer + offset, len);
//    return len;
//}

const uint32_t TextDocument::GetFileFormat() const
{
    return fileFormat;
}

const size_t TextDocument::GetLineCount() const
{
    return LineCount;
}

const size_t TextDocument::GetLongestLine(int tabwidth = 4) const
{
    size_t longest = 0;
    size_t xpos = 0;
    char* bufPtr = (char*)(docBuffer + headerSize);


    for (size_t i = 0; i < lengthBytes; i++)
    {
        if (bufPtr[i] == '\r')
        {
            if (bufPtr[i + 1] == '\n')
                i++;

            longest = max(longest, xpos);
            xpos = 0;
        }
        else if (bufPtr[i] == '\n')
        {
            longest = max(longest, xpos);
            xpos = 0;
        }
        else if (bufPtr[i] == '\t')
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
    return lengthBytes;
}

bool TextDocument::Clear()
{

    if (docBuffer != nullptr)
    {
        delete docBuffer;
        docBuffer = nullptr;
        lengthBytes = 0;
    }
    if (lineOffsetByte != nullptr)
    {

        delete lineOffsetByte;
        lineOffsetByte = nullptr;
    }
    if (lineOffsetChar != nullptr)
    {
        delete lineOffsetChar;
        lineOffsetChar = nullptr;
    }

    LineCount = 0;
    return true;
}

bool TextDocument::OffsetToLine(size_t fileoffset, size_t* lineno, size_t* offset)
{
    size_t low = 0, high = LineCount - 1;
    size_t line = 0;

    if (LineCount == 0)
    {
        if (lineno) *lineno = 0;
        if (offset) *offset = 0;
        return false;
    }

    while (low <= high)
    {
        line = (high + low) / 2;

        if (fileoffset >= lineOffsetByte[line] && fileoffset < lineOffsetByte[line + 1])
        {
            break;
        }
        else if (fileoffset < lineOffsetByte[line])
        {
            high = line - 1;
        }
        else
        {
            low = line + 1;
        }
    }

    if (lineno)  *lineno = line;
    if (offset)	*offset = fileoffset - lineOffsetByte[line];

    return true;
}

bool TextDocument::GetLineInfo(size_t lineno, size_t* fileoff, size_t* length)
{
    if (lineno < LineCount)
    {
        if (length)
            *length = lineOffsetByte[lineno + 1] - lineOffsetByte[lineno];

        if (fileoff)
            *fileoff = lineOffsetByte[lineno];

        return true;
    }
    else
    {
        return false;
    }
}

ULONG TextDocument::lineno_from_offset(ULONG offset)
{
    return 0;
}

bool TextDocument::LineInfoFromOffset(ULONG offset_chars, size_t* lineNo, size_t* lineoff_chars, size_t* linelen_chars, size_t* lineoff_bytes, size_t* linelen_bytes)
{
    ULONG low = 0;
    ULONG high = LineCount - 1;
    ULONG line = 0;

    if (LineCount == 0)
    {
        if (lineNo)			*lineNo = 0;
        if (lineoff_chars)	*lineoff_chars = 0;
        if (linelen_chars)	*linelen_chars = 0;
        if (lineoff_bytes)	*lineoff_bytes = 0;
        if (linelen_bytes)	*linelen_bytes = 0;

        return false;
    }

    while (low <= high)
    {
        line = (high + low) / 2;

        if (offset_chars >= lineOffsetChar[line] && offset_chars < lineOffsetChar[line + 1])
        {
            break;
        }
        else if (offset_chars < lineOffsetChar[line])
        {
            high = line - 1;
        }
        else
        {
            low = line + 1;
        }
    }

    if (lineNo)			*lineNo = line;
    if (lineoff_bytes)	*lineoff_bytes = lineOffsetByte[line];
    if (linelen_bytes)	*linelen_bytes = lineOffsetByte[line + 1] - lineOffsetByte[line];
    if (lineoff_chars)	*lineoff_chars = lineOffsetChar[line];
    if (linelen_chars)	*linelen_chars = lineOffsetChar[line + 1] - lineOffsetChar[line];

    return true;
}

bool TextDocument::LineInfoFromLineNo(size_t lineno, size_t* lineoff_chars, size_t* linelen_chars, size_t* lineoff_bytes, size_t* linelen_bytes)
{
    if (lineno < LineCount)
    {
        if (linelen_chars) *linelen_chars = lineOffsetChar[lineno + 1] - lineOffsetChar[lineno];
        if (lineoff_chars) *lineoff_chars = lineOffsetChar[lineno];

        if (linelen_bytes) *linelen_bytes = lineOffsetByte[lineno + 1] - lineOffsetByte[lineno];
        if (lineoff_bytes) *lineoff_bytes = lineOffsetByte[lineno];

        return true;
    }
    else
    {
        return false;
    }
}


TextIterator TextDocument::iterate_line(size_t lineno, size_t* linestart, size_t* linelen)
{
    size_t offset_bytes;
    size_t length_bytes;

    if (!LineInfoFromLineNo(lineno, linestart, linelen, &offset_bytes, &length_bytes))
        return TextIterator();

    return TextIterator(offset_bytes, length_bytes, this);
}

TextIterator TextDocument::iterate_line_offset(size_t offset_chars, size_t* lineno, size_t* linestart)
{
    size_t offset_bytes;
    size_t length_bytes;

    if (!LineInfoFromOffset(offset_chars, lineno, linestart, 0, &offset_bytes, &length_bytes))
        return TextIterator();

    return TextIterator(offset_bytes, length_bytes, this);
}
