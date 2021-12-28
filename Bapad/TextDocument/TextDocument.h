#include "pch.h"

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

    TextIterator IterateLineByLineNumber(size_t lineno, size_t * linestart = nullptr, size_t * linelen = nullptr);
    TextIterator IterateLineByOffset(size_t offset_chars, size_t * lineno, size_t * linestart = nullptr);

    const uint32_t  GetFileFormat() const;
    const size_t    GetLineCount() const;
    const size_t    GetLongestLine(int tabwidth) const;
    const size_t    GetDocLength() const;

private:
    bool            InitLineBuffer();
    uint32_t        DetectFileFormat(size_t& headerSize);
    size_t          GetUTF32Char(size_t offset, size_t lenBytes, char32_t& pch32);
    size_t          GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t& bufLen);

    char*       docBuffer;

    size_t      lengthByChars;
    size_t      lengthByBytes;

    uint32_t    fileFormat;
    size_t      headerSize;

    size_t*     byteOffsetBuffer;
    size_t*     charOffsetBuffer;

    size_t      LineCount;



};


class TextIterator
{
private:
    std::shared_ptr<TextDocument> textDoc;
    size_t offsetBytes;
    size_t lengthBytes;
public:
    // default constructor sets all members to zero
    TextIterator()
        : textDoc(nullptr), offsetBytes(0), lengthBytes(0)
    {
    }

    TextIterator(size_t off, size_t len, TextDocument* td)
        : textDoc(td), offsetBytes(off), lengthBytes(len)
    {

    }

    // default copy-constructor
    TextIterator(const TextIterator& ti)
        : textDoc(ti.textDoc), offsetBytes(ti.offsetBytes), lengthBytes(ti.lengthBytes)
    {
    }




    // assignment operator
    TextIterator& operator= (TextIterator& ti)
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
        return textDoc.use_count()!=0 ? true : false;
    }
};

//Byte Order Mark
enum class BOM : uint32_t //uint32_t
{                           //BOMHeaderLength
    ASCII = 0,              //0
    UTF8 = 0xEFBBBF,        //3
    UTF16LE = 0xFFFE,       //2
    UTF16BE = 0xFEFF,       //2
    UTF32LE = 0xFFFE0000,   //4
    UTF32BE = 0x0000FEFF    //4
};

struct BOMLookup
{
public:
    BOM    bom;
    //bytes
    size_t      headerLength;

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
        static std::vector<BOMLookup> instances;
        if (instances.empty())
        {
            instances = std::vector<BOMLookup>(
                {
                    BOMLookup(BOM::UTF32LE,4),
                    BOMLookup(BOM::UTF32BE,4),
                    BOMLookup(BOM::UTF8,3),
                    BOMLookup(BOM::UTF16LE,2),
                    BOMLookup(BOM::UTF16BE,2),
                    BOMLookup(BOM::ASCII,0)
                });
        }
        return instances;
    }
};