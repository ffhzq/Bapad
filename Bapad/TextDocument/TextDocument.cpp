#include "pch.h"
#include "TextDocument.h"
#include "formatConversion.h"

struct _BOM_LOOKUP BOMLOOK[] =
{
    // define longest headers first
    { 0x0000FEFF, 4, BCP_UTF32    },
    { 0xFFFE0000, 4, BCP_UTF32BE  },
    { 0xBFBBEF,	  3, BCP_UTF8	  },
    { 0xFFFE,	  2, BCP_UTF16BE  },
    { 0xFEFF,	  2, BCP_UTF16    },
    { 0,          0, BCP_ASCII	  },
};


TextDocument::TextDocument()
    : 
    docBuffer(nullptr),
    byteOffsetLineBuffer(nullptr),
    charOffsetLineBuffer(nullptr)
{
    docLengthByBytes = 0;
    docLengthByChars = 0;
    LineCount = 0;

    fileFormat = BCP_ASCII;
    headerSize = 0;
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

bool TextDocument::Initialize(HANDLE hFile)//跨平台要改?
{
    //PLARGE_INTEGER Represents a 64-bit signed integer value
    LARGE_INTEGER TmpDocumentLength = { 0 };

    //Retrieves the size of the specified file.
    if (GetFileSizeEx(hFile, &TmpDocumentLength) == 0)
    {
        docLengthByBytes = 0;
        return false;
    }
    docLengthByBytes = TmpDocumentLength.QuadPart;

    //if (GetFileSize(hFile, 0) != docLengthByBytes)abort();

    // allocate new file-buffer
    const size_t bufferSize = docLengthByBytes;
    if ((docBuffer = new char[bufferSize]) == 0)
        return false;

    ULONG numOfBytesRead = 0;
    // read entire file into memory
    ReadFile(hFile, docBuffer, static_cast<DWORD>(docLengthByBytes), &numOfBytesRead, NULL);


    fileFormat = DetectFileFormat();

    // work out where each line of text starts
    if (!InitLineBuffer())
        Clear();
    
    CloseHandle(hFile);
    return true;
}


bool TextDocument::InitLineBuffer()
{
    const size_t bufLen = docLengthByBytes - headerSize;
    //NEED TO BE UPDATED 用std::vector 吗？
    byteOffsetLineBuffer = new size_t[bufLen+1];
    if (byteOffsetLineBuffer == nullptr)
        return false;
    charOffsetLineBuffer = new size_t[bufLen+1];
    if (charOffsetLineBuffer == nullptr)
        return false;

    LineCount = 0;

    size_t offsetBytes = 0, offsetChars = 0;
    size_t lineStartBytes = 0, lineStartChars = 0;
    while (offsetBytes < bufLen)
    {
        char32_t ch32;
        size_t len = GetUTF32Char(offsetBytes, bufLen - offsetBytes, ch32);
        offsetBytes += len;
        offsetChars += 1;

        //TODO:判断语句的变量类型不同,需要验证
        if (ch32 == '\r')
        {
            if (LineCount >= bufLen)
            {
                return false;
            }
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
        else if (ch32 == '\n')// || ch32 == '\x0b' || ch32 == '\x0c' || ch32 == 0x0085 || ch32 == 0x2029 || ch32 == 0x2028)
        {
            if (LineCount >= bufLen)
            {
                return false;
            }
            //记录行起始位置
            byteOffsetLineBuffer[LineCount] = lineStartBytes;
            charOffsetLineBuffer[LineCount] = lineStartChars;
            lineStartBytes = offsetBytes;
            lineStartChars = offsetChars;
            LineCount++;
        }
    }
    if (LineCount >= bufLen)
    {
        return false;
    }
    if (bufLen > 0)
    {
        byteOffsetLineBuffer[LineCount] = lineStartBytes;
        charOffsetLineBuffer[LineCount] = lineStartChars;
        LineCount++;
    }
    //SIZE_MAX
    if (LineCount >= bufLen)
    {
        return false;
    }
    byteOffsetLineBuffer[LineCount] = bufLen;
    charOffsetLineBuffer[LineCount] = offsetChars;

    
    return true;
}

int TextDocument::DetectFileFormat()
{
    int res=-1;
    for (auto i : BOMLOOK)
    {
        if (docLengthByBytes >= i.headerLength
            && memcmp(docBuffer, &i.bom, i.headerLength) == 0)
        {
            headerSize = i.headerLength;
            res = i.type;
            break;
        }
    }
    if (res == -1)
    {
        headerSize = 0;
        res = BCP_ASCII;
    }
    return res;

}

size_t TextDocument::GetUTF32Char(size_t offset, size_t lenBytes, char32_t & pch32)
{
    //Byte* rawData = reinterpret_cast<Byte*>(docBuffer + offset + headerSize);
    //Word* rawDataW = reinterpret_cast<Word*>(docBuffer + offset + headerSize);
    Byte* rawdata;

    lenBytes = min(16, lenBytes);
    rawdata=reinterpret_cast<Byte*>(docBuffer + offset + headerSize);
    //m_seq.render(offset + m_nHeaderSize, rawdata, lenbytes);


    UTF16* rawdata_w = (UTF16*)rawdata;//(WCHAR*)(buffer + offset + m_nHeaderSize);
    WCHAR     ch16;
    size_t   ch32len = 1;

    switch (fileFormat)
    {
    case BCP_ASCII:
        MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<CCHAR*>(rawdata), 1, &ch16, 1);
        pch32 = ch16;
        return 1;

    case BCP_UTF16:
        return UTF16ToUTF32(rawdata_w, lenBytes / 2, reinterpret_cast<UTF32*>(&pch32), ch32len) * sizeof(WCHAR);

    case BCP_UTF16BE:
        return UTF16BEToUTF32(rawdata_w, lenBytes / 2, reinterpret_cast<UTF32*>(&pch32), ch32len) * sizeof(WCHAR);

    case BCP_UTF8:
        return UTF8ToUTF32(rawdata, lenBytes, reinterpret_cast<UTF32&>(pch32));

    default:
        return 0;
    }
}

size_t TextDocument::GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t & bufLen)
{
    Byte* rawData = reinterpret_cast<Byte*>(docBuffer + offset + headerSize);
    //size_t  len;

    if (offset >= docLengthByBytes)
    {
        bufLen = 0;
        return 0;
    }

    switch (fileFormat)
    {
        // convert from ANSI->UNICODE
    case BCP_ASCII:
        return AsciiToUTF16((UTF8*)rawData, lenBytes, (UTF16*)buf, bufLen);

    case BCP_UTF8:
        return UTF8ToUTF16((UTF8*)rawData, lenBytes, (UTF16*)buf, bufLen);

        // already unicode, do a straight memory copy
    case BCP_UTF16:
        return CopyUTF16((UTF16*)(rawData), lenBytes / sizeof(wchar_t), (UTF16*)buf, bufLen) * sizeof(WCHAR);

        // need to convert from big-endian to little-endian
    case BCP_UTF16BE:
        return SwapUTF16((UTF16*)(rawData), lenBytes / sizeof(wchar_t), (UTF16*)buf, bufLen) * sizeof(WCHAR);

        // error! we should *never* reach this point
    default:
        bufLen = 0;
        return 0;
    }
    
}

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


    for (size_t i = 0; i < docLengthByBytes; i++)
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
            ++xpos;
        }
    }

    longest = max(longest, xpos);
    return longest;
}

const size_t TextDocument::GetDocLength() const
{
    return docLengthByBytes;
}

bool TextDocument::Clear()
{
    if (docBuffer != nullptr)
    {
        delete[] docBuffer;
        docBuffer = nullptr;
        docLengthByBytes = 0;
    }
    if (byteOffsetLineBuffer != nullptr)
    {
        delete[] byteOffsetLineBuffer;
        byteOffsetLineBuffer = nullptr;
    }
    if (charOffsetLineBuffer != nullptr)
    {
        delete[] charOffsetLineBuffer;
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
        if (lineNo != nullptr)          *lineNo = 0;
        if (lineoffChars != nullptr)    *lineoffChars = 0;
        if (linelenChars != nullptr)    *linelenChars = 0;
        if (lineoffBytes != nullptr)    *lineoffBytes = 0;
        if (linelenBytes != nullptr)    *linelenBytes = 0;

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

    if (lineNo != nullptr)	        *lineNo = line;
    if (lineoffBytes != nullptr)	*lineoffBytes = byteOffsetLineBuffer[line];
    if (linelenBytes != nullptr)	*linelenBytes = byteOffsetLineBuffer[line + 1] - byteOffsetLineBuffer[line];
    if (lineoffChars != nullptr)	*lineoffChars = charOffsetLineBuffer[line];
    if (linelenChars != nullptr)	*linelenChars = charOffsetLineBuffer[line + 1] - charOffsetLineBuffer[line];

    return true;
}

bool TextDocument::LineInfoFromLineNumber(size_t lineno, size_t * lineoffChars, size_t * linelenChars, size_t * lineoffBytes, size_t * linelenBytes)
{
    if (lineno < LineCount)
    {
        if (linelenChars != nullptr) *linelenChars = charOffsetLineBuffer[lineno + 1] - charOffsetLineBuffer[lineno];
        if (lineoffChars != nullptr) *lineoffChars = charOffsetLineBuffer[lineno];
        if (linelenBytes != nullptr) *linelenBytes = byteOffsetLineBuffer[lineno + 1] - byteOffsetLineBuffer[lineno];
        if (lineoffBytes != nullptr) *lineoffBytes = byteOffsetLineBuffer[lineno];

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


size_t TextDocument::rawdata_to_utf16(BYTE* rawdata, size_t rawlen, WCHAR* utf16str, size_t& utf16len)
{
    switch (fileFormat)
    {
    // convert from ANSI->UNICODE
    case BCP_ASCII:
        
        return AsciiToUTF16(rawdata, rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len);

    case BCP_UTF8:
        return UTF8ToUTF16(rawdata, rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len);

        // already unicode, do a straight memory copy
    case BCP_UTF16:
        rawlen /= sizeof(TCHAR);
        return CopyUTF16(reinterpret_cast<UTF16*>(rawdata), rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len) * sizeof(TCHAR);

        // need to convert from big-endian to little-endian
    case BCP_UTF16BE:
        rawlen /= sizeof(TCHAR);
        return SwapUTF16(reinterpret_cast<UTF16*>(rawdata), rawlen, reinterpret_cast<UTF16*>(utf16str), utf16len) * sizeof(TCHAR);
    default:
        utf16len = 0;
        return 0;

    }
}