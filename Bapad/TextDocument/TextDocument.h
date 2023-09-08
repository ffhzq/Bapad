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

    size_t LineNumFromOffset(size_t offset);

    bool  LineInfoFromOffset(size_t offset_chars, size_t * lineNo, size_t * lineoffChars, size_t * linelenChars, size_t * lineoffBytes, size_t * linelenBytes);
    bool  LineInfoFromLineNumber(size_t lineno, size_t * lineoffChars, size_t * linelenChars, size_t * lineoffBytes, size_t * linelenBytes);

    TextIterator IterateLineByLineNumber(size_t lineno, size_t * linestart = 0, size_t * linelen = 0);
    TextIterator IterateLineByOffset(size_t offset_chars, size_t * lineno, size_t * linestart = 0);

    const uint32_t  GetFileFormat() const;
    const size_t    GetLineCount() const;
    const size_t    GetLongestLine(int tabwidth) const;
    const size_t    GetDocLength() const;

private:
    bool            InitLineBuffer();
    int        DetectFileFormat();
    size_t          GetUTF32Char(size_t offset, size_t lenBytes, char32_t& pch32);
    size_t          GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t& bufLen);
    size_t      rawdata_to_utf16(BYTE* rawdata, size_t rawlen, WCHAR* utf16str, size_t& utf16len);
    char*       docBuffer;

    size_t      lengthByChars;
    size_t      lengthByBytes;

    int         fileFormat;
    size_t      headerSize;

    size_t*     byteOffsetLineBuffer;
    size_t*     charOffsetLineBuffer;

    size_t      LineCount;



};


class TextIterator
{
private:
    TextDocument* textDoc;
    size_t offsetBytes;
    size_t lengthBytes;
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


/*
enum class BOM : uint32_t
{                           //BOMHeaderLength
    ASCII = 0,              //0
    UTF8 = 0xBFBBEF,        //3
    UTF16LE = 0xFEFF,       //2
    UTF16BE = 0xFFFE,       //2
    UTF32LE = 0x0000FEFF,   //4
    UTF32BE = 0xFFFE0000    //4
};

struct BOMLookup
{
public:
    BOM    bom;
    //bytes
    size_t headerLength;

    BOMLookup(BOM _bom, size_t _headerLength)
        : bom(_bom), headerLength(_headerLength)
    {

    }
};


class BOMLookupList
{
public:
    BOMLookupList() {}
    static const std::vector<BOMLookup>& GetInstances() {
        static std::vector<BOMLookup> instances = std::vector<BOMLookup>(
                {
                    BOMLookup(BOM::UTF32LE,4),
                    BOMLookup(BOM::UTF32BE,4),
                    BOMLookup(BOM::UTF8,3),
                    BOMLookup(BOM::UTF16LE,2),
                    BOMLookup(BOM::UTF16BE,2),
                    BOMLookup(BOM::ASCII,0)
                });

        return instances;
    }
};*/