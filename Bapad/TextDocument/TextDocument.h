#pragma once
#include "pch.h"

class TextIterator;

class TextDocument
{
    friend class TextIterator;
public:
    TextDocument() noexcept;
    ~TextDocument() noexcept;

    bool Initialize(wchar_t* filename);
    
    bool Clear() noexcept;
    bool ReCalculateLineBuffer();
    size_t LineNumFromOffset(size_t offset);

    bool LineInfoFromOffset(size_t offset_chars, size_t* lineNo, size_t* lineoffChars, size_t* linelenChars, size_t* lineoffBytes, size_t* linelenBytes);//定位对应offset所在的行并返回行号、字符偏移量、行字符数、字节偏移量、行字节数这些信息
    bool LineInfoFromLineNumber(size_t lineno, size_t* lineoffChars, size_t* linelenChars, size_t* lineoffBytes, size_t* linelenBytes);

    TextIterator IterateLineByLineNumber(size_t lineno, size_t* linestart = 0, size_t* linelen = 0);
    TextIterator IterateLineByOffset(size_t offset_chars, size_t* lineno, size_t* linestart = 0);

    size_t	InsertText(size_t offsetChars, wchar_t* text, size_t length);
    size_t	ReplaceText(size_t offsetChars, wchar_t * text, size_t length, size_t eraseLen);
    size_t	EraseText(size_t offsetChars, size_t length);

    const int GetFileFormat() const;
    const size_t GetLineCount() const;
    const size_t GetLongestLine(int tabwidth) const;
    const size_t GetDocLength() const;

private:
    bool InitLineBuffer();
    bool ReleaseLineBuffer();

    size_t GetUTF32Char(size_t offset, size_t lenBytes, char32_t& pch32);

    // GetText: read 'lenBytes'or'bufLen'(use the smaller one) bytes wchar from the position (docBuffer+offset) to 'buf'
    size_t  GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t& bufLen);

    size_t  RawDataToUTF16(unsigned char * rawdata, size_t rawlen, wchar_t * utf16str, size_t& utf16len);
    size_t  UTF16ToRawData(wchar_t * utf16Str, size_t utf16Len, unsigned char * rawData, size_t& rawLen);

    size_t	InsertTextRaw(size_t offsetBytes, wchar_t * text, size_t textLength);
    size_t	ReplaceTextRaw(size_t offsetBytes, wchar_t  * text, size_t textLength, size_t eraseLen);
    size_t	EraseTextRaw(size_t offsetBytes, size_t textLength);

    size_t CharOffsetToByteOffsetAt(size_t offsetBytes, size_t charCount);
    size_t CharOffsetToByteOffset(size_t offsetChars);
    std::vector<unsigned char> docBuffer;// raw txt data
    size_t  docLengthByChars;//
    size_t  docLengthByBytes;// size of txt data

    int fileFormat;
    int  headerSize;

    size_t* byteOffsetLineBuffer;
    size_t* charOffsetLineBuffer;

    size_t  lineCount;



};


class TextIterator
{
private:
    TextDocument* textDoc;
    size_t  offsetBytes;
    size_t  lengthBytes;//bytes remaining
public:
    TextIterator() noexcept
        : textDoc(nullptr), offsetBytes(0), lengthBytes(0)
    {
    }

    TextIterator(size_t off, size_t len, TextDocument* td) noexcept
        : textDoc(td), offsetBytes(off), lengthBytes(len)
    {

    }

    TextIterator(const TextIterator& ti) noexcept
        : textDoc(ti.textDoc), offsetBytes(ti.offsetBytes), lengthBytes(ti.lengthBytes)
    {
    }

    TextIterator& operator= (TextIterator ti)
    {
        if (ti == *this)
        {
            return *this;
        }

        textDoc = ti.textDoc;
        offsetBytes = ti.offsetBytes;
        lengthBytes = ti.lengthBytes;
        return *this;
    }

    size_t GetText(wchar_t* buf, size_t bufLen)
    {
        if (textDoc)
        {
            memset(buf, 0, bufLen * sizeof(wchar_t));
            // get text from the TextDocument at the specified byte-offset
            size_t len = textDoc->GetText(offsetBytes, lengthBytes, buf, bufLen);

            // adjust the iterator's internal position
            offsetBytes += len;
            lengthBytes -= len;

            return bufLen;
        }
        else
        {
            return 0;
        }
    }

    operator bool()
    {
        return textDoc != nullptr ? true : false;
    }
};


