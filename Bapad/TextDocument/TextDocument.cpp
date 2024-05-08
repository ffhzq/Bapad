#include "pch.h"
#include "TextDocument.h"
#include "formatConversion.h"

struct _BOM_LOOKUP BOMLOOK[] =
{
    // define longest headers first
    //bom, headerlen, encoding form
    { 0x0000FEFF, 4, BCP_UTF32    },
    { 0xFFFE0000, 4, BCP_UTF32BE  },
    { 0xBFBBEF,	  3, BCP_UTF8	  },
    { 0xFFFE,	  2, BCP_UTF16BE  },
    { 0xFEFF,	  2, BCP_UTF16    },
    { 0,          0, BCP_ASCII	  },
};


TextDocument::TextDocument()
    : 
    docBuffer(),
    byteOffsetLineBuffer(nullptr),
    charOffsetLineBuffer(nullptr)
{
    docLengthByBytes = 0;
    docLengthByChars = 0;
    lineCount = 0;

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
    docBuffer = std::move(std::vector<unsigned char>(bufferSize, 0));
    ULONG numOfBytesRead = 0;
    // read entire file into memory
    ReadFile(hFile, &docBuffer[0], static_cast<DWORD>(docLengthByBytes), &numOfBytesRead, NULL);


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

    lineCount = 0;

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
            if (lineCount >= bufLen)
            {
                return false;
            }
            //记录行起始位置
            byteOffsetLineBuffer[lineCount] = lineStartBytes;
            charOffsetLineBuffer[lineCount] = lineStartChars;
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

            lineCount++;
        }
        else if (ch32 == '\n')// || ch32 == '\x0b' || ch32 == '\x0c' || ch32 == 0x0085 || ch32 == 0x2029 || ch32 == 0x2028)
        {
            if (lineCount >= bufLen)
            {
                return false;
            }
            //记录行起始位置
            byteOffsetLineBuffer[lineCount] = lineStartBytes;
            charOffsetLineBuffer[lineCount] = lineStartChars;
            lineStartBytes = offsetBytes;
            lineStartChars = offsetChars;
            lineCount++;
        }
    }
    if (lineCount >= bufLen)
    {
        return false;
    }
    if (bufLen > 0)
    {
        byteOffsetLineBuffer[lineCount] = lineStartBytes;
        charOffsetLineBuffer[lineCount] = lineStartChars;
        lineCount++;
    }
    //SIZE_MAX
    if (lineCount >= bufLen)
    {
        return false;
    }
    byteOffsetLineBuffer[lineCount] = bufLen;
    charOffsetLineBuffer[lineCount] = offsetChars;

    
    return true;
}

int TextDocument::DetectFileFormat()
{
    int res=-1;
    for (auto i : BOMLOOK)
    {
        if (docLengthByBytes >= i.headerLength
            && memcmp(&docBuffer[0], &i.bom, i.headerLength) == 0)
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
    rawdata=reinterpret_cast<Byte*>(&docBuffer[0] + offset + headerSize);
    UTF16* rawdata_w = (UTF16*)rawdata;
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
        return UTF8ToUTF32(rawdata, lenBytes, reinterpret_cast<UTF32*>(&pch32));

    default:
        return 0;
    }
}

size_t TextDocument::GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t & bufLen)
{
    Byte* rawData = reinterpret_cast<Byte*>(&docBuffer[0] + offset + headerSize);
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

size_t TextDocument::RawDataToUTF16(BYTE* rawdata, size_t rawlen, WCHAR* utf16str, size_t& utf16len)
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

size_t TextDocument::UTF16ToRawData(WCHAR* utf16Str, size_t utf16Len, BYTE* rawData, size_t& rawLen)
{
    switch (fileFormat)
    {
    case BCP_ASCII:
        return UTF16ToAscii(reinterpret_cast<UTF16*>(utf16Str), utf16Len, rawData, rawLen);
    case BCP_UTF8:
        return UTF16ToUTF8(reinterpret_cast<UTF16*>(utf16Str), utf16Len, rawData, rawLen);
    case BCP_UTF16:
        rawLen /= sizeof(WCHAR);
        utf16Len = CopyUTF16(reinterpret_cast<UTF16*>(utf16Str), utf16Len, reinterpret_cast<UTF16*>(rawData), rawLen);
        rawLen *= sizeof(WCHAR);
        return utf16Len;
    case BCP_UTF16BE:
        rawLen /= sizeof(WCHAR);
        utf16Len = SwapUTF16(reinterpret_cast<UTF16*>(utf16Str), utf16Len, reinterpret_cast<UTF16*>(rawData), rawLen);
        rawLen *= sizeof(WCHAR);
        return utf16Len;
    default:
        rawLen = 0;
        return 0;
    }
}



const uint32_t TextDocument::GetFileFormat() const
{
    return fileFormat;
}

const size_t TextDocument::GetLineCount() const
{
    return lineCount;
}

const size_t TextDocument::GetLongestLine(int tabwidth = 4) const
{
    size_t longest = 0;
    size_t xpos = 0;
    if (docBuffer.empty())return 0;
    char* bufPtr = (char*)(&docBuffer[0] + headerSize);


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
    if (!docBuffer.empty())
    {
        docLengthByBytes = 0;
        docBuffer.resize(0);
        docBuffer.shrink_to_fit();
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
    lineCount = 0;
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
    size_t high = lineCount - 1;
    size_t line = 0;

    if (lineCount == 0)
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
    if (lineno < lineCount)
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


size_t TextDocument::InsertText(size_t offsetChars, WCHAR* text, size_t length)
{
    size_t offsetBytes = CharOffsetToByteOffset(offsetChars);
    return InsertTextRaw(offsetBytes,text,length); 
}
size_t TextDocument::ReplaceText(size_t offsetChars, WCHAR* text, size_t length, size_t eraseLen)
{
    size_t offsetBytes = CharOffsetToByteOffset(offsetChars);
    return ReplaceTextRaw(offsetBytes,text,length,eraseLen);
}
size_t TextDocument::EraseText(size_t offsetChars, size_t length)
{
    size_t offsetBytes = CharOffsetToByteOffset(offsetChars);
    return EraseTextRaw(offsetBytes,length);
}

size_t TextDocument::InsertTextRaw(size_t offsetBytes, WCHAR* text, size_t textLength)
{
    const size_t LEN = 0x100;
    unsigned char buf[LEN];
    size_t processedChars=0, rawLen=0, offset=offsetBytes+headerSize, bufLen = LEN;
    docBuffer.reserve((docBuffer.size() + textLength * sizeof(WCHAR)) * 1.5);
    while (offsetBytes)
    {
        rawLen = bufLen;
        processedChars = UTF16ToRawData(text, textLength, buf, bufLen);

        docBuffer.insert(docBuffer.begin() + offset, std::begin(buf), std::begin(buf)+bufLen);

        text += processedChars;
        textLength -= processedChars;
        rawLen += bufLen;
        offset += bufLen;
    }
    docBuffer.shrink_to_fit();
    docLengthByBytes = docBuffer.size();
    return rawLen;

}
size_t TextDocument::ReplaceTextRaw(size_t offsetBytes, WCHAR* text, size_t textLength, size_t eraseLen)
{
    const size_t LEN = 0x100;
    unsigned char buf[LEN];
    size_t processedChars = 0, rawLen = 0, offset = offsetBytes + headerSize, bufLen = LEN;
    docBuffer.reserve((docBuffer.size() - eraseLen + textLength * sizeof(WCHAR)) * 1.5);
    size_t eraseBytes = CharOffsetToByteOffsetAt(offsetBytes, eraseLen);
    auto beginIter = docBuffer.begin() + offsetBytes, endIter = beginIter + eraseBytes;
    docBuffer.erase(beginIter, endIter);
    auto insertIter = docBuffer.begin() + offsetBytes;
    while (offsetBytes)
    {
        rawLen = bufLen;
        processedChars = UTF16ToRawData(text, textLength, buf, bufLen);
        
        insertIter=docBuffer.insert(insertIter, std::begin(buf), std::begin(buf) + bufLen);

        text += processedChars;
        textLength -= processedChars;
        rawLen += bufLen;
        offset += bufLen;
    }
    docBuffer.shrink_to_fit();
    docLengthByBytes = docBuffer.size();
    return rawLen;
    

}
size_t TextDocument::EraseTextRaw(size_t offsetBytes, size_t textLength)
{
    size_t eraseBytes = CharOffsetToByteOffsetAt(offsetBytes, textLength);
    auto beginIter = docBuffer.begin() + offsetBytes, endIter = beginIter + eraseBytes;
    docBuffer.erase(beginIter, endIter);
    return textLength;
}

size_t TextDocument::CharOffsetToByteOffsetAt(size_t offsetBytes, size_t charCount)
{
    switch (fileFormat)
    {
    case BCP_ASCII:
        return charCount;
    case BCP_UTF16:case BCP_UTF16BE:
        return charCount * sizeof(WCHAR);

    default:
        break;
    }
    // case UTF-8
    size_t start = offsetBytes;
    while (charCount && offsetBytes < docLengthByBytes)//todo: +headrsize ?
    {
        const size_t bufLen = 0x100;
        WCHAR buf[bufLen];
        size_t charLen = min(charCount, bufLen);//因为底层默认输入为UTF-16所以直接定义WCHAR数组
        size_t byteLen = GetText(offsetBytes, docLengthByBytes - offsetBytes, buf, charLen);
        charCount -= charLen;
        offsetBytes += byteLen;

    }
    return offsetBytes - start;

}

size_t TextDocument::CharOffsetToByteOffset(size_t offsetChars)
{
    switch (fileFormat)
    {
    case BCP_ASCII:
        return offsetChars;
    case BCP_UTF16:case BCP_UTF16BE:
        return offsetChars * sizeof(WCHAR);

    default:
        break;
    }
    // case UTF-8 ....
    size_t lineOffChars;
    size_t lineOffBytes;
    if (LineInfoFromOffset(offsetChars, 0, &lineOffChars, 0, &lineOffBytes, 0))
    {
        return CharOffsetToByteOffsetAt(lineOffBytes, offsetChars - lineOffChars)
            + lineOffBytes;
    }
    else
    {
        return 0;
    }
}
