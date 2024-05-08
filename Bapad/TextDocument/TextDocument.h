#include "pch.h"

#define BCP_ASCII		0
#define BCP_UTF8		1
#define BCP_UTF16		2
#define BCP_UTF16BE		3
#define BCP_UTF32		4
#define BCP_UTF32BE		5



class TextIterator;

class TextDocument
{
    friend class TextIterator;
public:
    TextDocument();
    ~TextDocument();

    //TextDocument(TextDocument &a) = delete;
    //TextDocument& operator=(TextDocument& a) = delete;


    bool    Initialize(wchar_t* filename);
    bool    Initialize(HANDLE hFile);
    bool    Clear();

    size_t  LineNumFromOffset(size_t offset);

    bool    LineInfoFromOffset(size_t offset_chars, size_t* lineNo, size_t* lineoffChars, size_t* linelenChars, size_t* lineoffBytes, size_t* linelenBytes);//定位对应offset所在的行并返回行号、字符偏移量、行字符数、字节偏移量、行字节数这些信息
    bool    LineInfoFromLineNumber(size_t lineno, size_t* lineoffChars, size_t* linelenChars, size_t* lineoffBytes, size_t* linelenBytes);

    TextIterator    IterateLineByLineNumber(size_t lineno, size_t* linestart = 0, size_t* linelen = 0);
    TextIterator    IterateLineByOffset(size_t offset_chars, size_t* lineno, size_t* linestart = 0);

    size_t	InsertText(size_t offsetChars, WCHAR* text, size_t length);
    size_t	ReplaceText(size_t offsetChars, WCHAR* text, size_t length, size_t eraseLen);
    size_t	EraseText(size_t offsetChars, size_t length);

    const uint32_t  GetFileFormat() const;
    const size_t    GetLineCount() const;
    const size_t    GetLongestLine(int tabwidth) const;
    const size_t    GetDocLength() const;

private:
    bool InitLineBuffer();
    int DetectFileFormat();
    size_t GetUTF32Char(size_t offset, size_t lenBytes, char32_t& pch32);

    // GetText: read 'lenBytes'or'bufLen'(use the smaller one) bytes wchar from the position (docBuffer+offset) to 'buf'
    //
    size_t  GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t& bufLen);

    size_t  RawDataToUTF16(BYTE* rawdata, size_t rawlen, WCHAR* utf16str, size_t& utf16len);
    size_t  UTF16ToRawData(WCHAR* utf16Str, size_t utf16Len, BYTE* rawData, size_t& rawLen);

    size_t	InsertTextRaw(size_t offsetBytes, WCHAR* text, size_t textLength);
    size_t	ReplaceTextRaw(size_t offsetBytes, WCHAR* text, size_t textLength, size_t eraseLen);
    size_t	EraseTextRaw(size_t offsetBytes, size_t textLength);

    size_t CharOffsetToByteOffsetAt(size_t offsetBytes, size_t charCount);
    size_t CharOffsetToByteOffset(size_t offsetChars);
    std::vector<unsigned char> docBuffer;// raw txt data TODO:should change to unsigned char? 
    size_t  docLengthByChars;//
    size_t  docLengthByBytes;// size of txt data

    int fileFormat;
    size_t  headerSize;

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
    TextIterator()
        : textDoc(nullptr), offsetBytes(0), lengthBytes(0)
    {
    }

    TextIterator(size_t off, size_t len, TextDocument* td)
        : textDoc(td), offsetBytes(off), lengthBytes(len)
    {

    }

    TextIterator(const TextIterator& ti)
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

//Byte Order Mark
struct _BOM_LOOKUP
{
    DWORD  bom;
    ULONG  headerLength;
    int    type;
};
