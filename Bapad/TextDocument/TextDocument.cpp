#include "pch.h"
#include "TextDocument.h"
#include "formatConversion.h"

TextDocument::TextDocument()
    : 
    docBuffer(nullptr),
    byteOffsetLineBuffer(nullptr),
    charOffsetLineBuffer(nullptr)
{
    lengthByBytes = 0;
    LineCount = 0;
    lengthByChars = 0;

    fileFormat = (uint32_t)(BOMLookupList().GetInstances().rbegin()->bom);
    headerSize = BOMLookupList().GetInstances().begin()->headerLength;
}
TextDocument::~TextDocument()
{
    Clear();
}

bool TextDocument::Initialize(wchar_t* filename)//跨平台要改
{
    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    return Initialize(hFile);
}

bool TextDocument::Initialize(HANDLE hFile)//跨平台要改
{
    //PLARGE_INTEGER Represents a 64-bit signed integer value
    LARGE_INTEGER TmpDocumentLength = { 0 };

    //Retrieves the size of the specified file.
    if (GetFileSizeEx(hFile, &TmpDocumentLength) == 0)
    {
        lengthByBytes = 0;
        return false;
    }
    lengthByBytes = TmpDocumentLength.QuadPart;

    // allocate new file-buffer
    const size_t bufferSize = lengthByBytes;
    if ((docBuffer = new char[bufferSize]) == 0)
    {
        return false;
    }

    //memset(buffer, 0, bufferSize);

    ULONG numOfBytesRead = 0;
    // read entire file into memory
    if (ReadFile(hFile, docBuffer, lengthByBytes, &numOfBytesRead, NULL))
    {
        ;
    }

    fileFormat = DetectFileFormat();

    // work out where each line of text starts
    if (!InitLineBuffer())
        Clear();

    CloseHandle(hFile);
    return true;
}


bool TextDocument::InitLineBuffer()
{
    const size_t bufLen = lengthByBytes - headerSize;
    //NEED TO BE UPDATED 用std::vector
    if ((byteOffsetLineBuffer = new size_t[bufLen]) == 0)
        return false;

    if ((charOffsetLineBuffer = new size_t[bufLen]) == 0)
        return false;

    LineCount = 0;

    size_t offsetBytes = 0, offsetChars = 0;
    size_t lineStartBytes = 0, lineStartChars = 0;
    for (; offsetBytes < bufLen;)
    {
        char32_t ch32;
        size_t len = GetUTF32Char(offsetBytes, bufLen - offsetBytes, ch32);
        offsetBytes += len;
        offsetChars += 1;

        //TODO:判断语句的变量类型不同,需要验证
        if (ch32 == '\r')
        {
            //记录行起始位置
            byteOffsetLineBuffer[LineCount] = lineStartBytes;
            charOffsetLineBuffer[LineCount] = lineStartChars;
            lineStartBytes = offsetBytes;
            lineStartChars = offsetChars;

            //找下一字符
            len = GetUTF32Char(offsetBytes, bufLen - offsetBytes, ch32);
            offsetBytes += len;
            offsetChars += 1;
            
            //判断"\r\n"组和
            if (ch32 == '\n')
            {
                lineStartBytes = offsetBytes;
                lineStartChars = offsetChars;
            }

            LineCount++;
        }
        else if (ch32 == '\n')
        {
            //记录行起始位置
            byteOffsetLineBuffer[LineCount] = lineStartBytes;
            charOffsetLineBuffer[LineCount] = lineStartChars;
            lineStartBytes = offsetBytes;
            lineStartChars = offsetChars;

            LineCount++;
        }
    }

    if (bufLen > 0)
    {
        byteOffsetLineBuffer[LineCount] = lineStartBytes;
        charOffsetLineBuffer[LineCount] = lineStartChars;
        LineCount++;
    }
    //SIZE_MAX
    byteOffsetLineBuffer[LineCount] = bufLen;
    charOffsetLineBuffer[LineCount] = offsetChars;
    return true;
}

uint32_t TextDocument::DetectFileFormat()
{
    auto BOMLOOK = BOMLookupList().GetInstances();
    for (auto i : BOMLOOK)
    {
        if (lengthByBytes >= i.headerLength
            && memcmp(docBuffer, &i.bom, i.headerLength) == 0)
        {
            headerSize = i.headerLength;
            return static_cast<uint32_t>(i.bom);
        }
    }

    headerSize = 0;
    return static_cast<uint32_t>(BOM::ASCII);

}

size_t TextDocument::GetUTF32Char(size_t offset, size_t lenBytes, char32_t& pch32)
{
    Byte* rawData = reinterpret_cast<Byte*>(docBuffer + offset + headerSize);
    Word* rawDataW = reinterpret_cast<Word*>(docBuffer + offset + headerSize);
    Word ch16;
    size_t ch32Len = 1;

    switch (fileFormat)
    {
    case (uint32_t)BOM::ASCII:
        MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<Cchar *>(rawData), 1, reinterpret_cast<wchar_t*>(&ch16), 1);
        pch32 = ch16;
        return 1;
    case (uint32_t)BOM::UTF16LE:
        return UTF16ToUTF32(reinterpret_cast<wchar_t*>(rawDataW), lenBytes / 2, reinterpret_cast<ULONG*>(pch32), ch32Len) * 2;
    case (uint32_t)BOM::UTF16BE:
        return UTF16BEToUTF32(reinterpret_cast<wchar_t*>(rawDataW), lenBytes / 2, reinterpret_cast<ULONG*>(pch32), ch32Len) * 2;
    case (uint32_t)BOM::UTF8:
        return UTF8ToUTF32(rawData, lenBytes, pch32);
    default:
        return 0;
    }

    return 0;
}

size_t TextDocument::GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t & bufLen)
{
    Byte* rawData = reinterpret_cast<Byte*>(docBuffer + offset + headerSize);
    size_t  len;

    if (offset >= lengthByBytes)
    {
        bufLen = 0;
        return 0;
    }

    switch (fileFormat)
    {
        // convert from ANSI->UNICODE
    case (uint32_t)BOM::ASCII:
        return AsciiToUTF16(rawData, lenBytes, buf, bufLen);

    case (uint32_t)BOM::UTF8:
        return UTF8ToUTF16(rawData, lenBytes, buf, bufLen);

        // already unicode, do a straight memory copy
    case (uint32_t)BOM::UTF16LE:
        return CopyUTF16(reinterpret_cast<wchar_t*>(rawData), lenBytes / sizeof(wchar_t), buf, bufLen);

        // need to convert from big-endian to little-endian
    case (uint32_t)BOM::UTF16BE:
        return SwapUTF16(reinterpret_cast<wchar_t*>(rawData), lenBytes / sizeof(wchar_t), buf, bufLen);

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


    for (size_t i = 0; i < lengthByBytes; i++)
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
    return lengthByBytes;
}

bool TextDocument::Clear()
{
    if (docBuffer != NULL)
    {
        delete docBuffer;
        docBuffer = nullptr;
        lengthByBytes = 0;
    }
    if (byteOffsetLineBuffer != NULL)
    {
        delete byteOffsetLineBuffer;
        byteOffsetLineBuffer = nullptr;
    }
    if (charOffsetLineBuffer != NULL)
    {
        delete charOffsetLineBuffer;
        charOffsetLineBuffer = nullptr;
    }
    LineCount = 0;
    return true;
}


size_t TextDocument::LineNumFromOffset(size_t offset)
{
    size_t lineNum = 0;
    LineInfoFromOffset(offset, &lineNum, nullptr, nullptr, nullptr, nullptr);
    return lineNum;
}

bool TextDocument::LineInfoFromOffset(size_t offset_chars, size_t * lineNo, size_t * lineoffChars, size_t * linelenChars, size_t * lineoffBytes, size_t * linelenBytes)
{
    size_t low = 0;
    size_t high = LineCount - 1;
    size_t line = 0;

    if (LineCount == 0)
    {
        if (lineNo != NULL)          *lineNo = 0;
        if (lineoffChars != NULL)    *lineoffChars = 0;
        if (linelenChars != NULL)    *linelenChars = 0;
        if (lineoffBytes != NULL)    *lineoffBytes = 0;
        if (linelenBytes != NULL)    *linelenBytes = 0;

        return false;
    }

    while (low <= high)
    {
        line = (high + low) / 2;

        if (offset_chars >= charOffsetLineBuffer[line] && offset_chars < charOffsetLineBuffer[line + 1])
        {
            break;
        }
        else if (offset_chars < charOffsetLineBuffer[line])
        {
            high = line - 1;
        }
        else
        {
            low = line + 1;
        }
    }

    if (lineNo != NULL)	        *lineNo = line;
    if (lineoffBytes != NULL)	*lineoffBytes = byteOffsetLineBuffer[line];
    if (linelenBytes != NULL)	*linelenBytes = byteOffsetLineBuffer[line + 1] - byteOffsetLineBuffer[line];
    if (lineoffChars != NULL)	*lineoffChars = charOffsetLineBuffer[line];
    if (linelenChars != NULL)	*linelenChars = charOffsetLineBuffer[line + 1] - charOffsetLineBuffer[line];

    return true;
}

bool TextDocument::LineInfoFromLineNumber(size_t lineno, size_t * lineoffChars, size_t * linelenChars, size_t * lineoffBytes, size_t * linelenBytes)
{
    if (lineno < LineCount)
    {
        if (linelenChars != NULL) *linelenChars = charOffsetLineBuffer[lineno + 1] - charOffsetLineBuffer[lineno];
        if (lineoffChars != NULL) *lineoffChars = charOffsetLineBuffer[lineno];
        if (linelenBytes != NULL) *linelenBytes = byteOffsetLineBuffer[lineno + 1] - byteOffsetLineBuffer[lineno];
        if (lineoffBytes != NULL) *lineoffBytes = byteOffsetLineBuffer[lineno];

        return true;
    }
    else
    {
        return false;
    }
}


TextIterator TextDocument::IterateLineByLineNumber(size_t lineno, size_t * linestart, size_t * linelen)
{
    size_t offset_bytes;
    size_t length_bytes;

    if (!LineInfoFromLineNumber(lineno, linestart, linelen, &offset_bytes, &length_bytes))
        return TextIterator();

    return TextIterator(offset_bytes, length_bytes, this);
}

TextIterator TextDocument::IterateLineByOffset(size_t offset_chars, size_t * lineno, size_t * linestart)
{
    size_t offset_bytes;
    size_t length_bytes;

    if (!LineInfoFromOffset(offset_chars, lineno, linestart, nullptr, &offset_bytes, &length_bytes))
        return TextIterator();

    return TextIterator(offset_bytes, length_bytes, this);
}