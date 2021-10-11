#pragma once
#include "pch.h"


class TextIterator;

class TextDocument
{
    //4 bytes long for ch32
    //using CH32 = char32_t;


    friend class TextIterator;
public:
    TextDocument();
    ~TextDocument();

    //Initialize the TextDocument with the specified file
    bool    Initialize(wchar_t* filename);
    //	Initialize using a file-handle
    bool    Initialize(HANDLE hFile);
    bool    Clear();
  
    //	Perform a reverse lookup - file-offset to line number
    bool    OffsetToLine(size_t fileoffset, size_t* lineno, size_t* offset);
    bool    GetLineInfo(size_t lineno, size_t* fileoff, size_t* length);

////////////////////////////////////////////////////NEED to UPDATED
    size_t LineNumFromOffset(size_t offset);

    bool  LineInfoFromOffset(size_t offset_chars, size_t& lineNo, size_t & lineoffChars, size_t & linelenChars, size_t & lineoffBytes, size_t & linelenBytes);
    bool  LineInfoFromLineNo(size_t lineno, size_t & lineoffChars, size_t & linelenChars, size_t & lineoffBytes, size_t & linelenBytes);

    TextIterator iterate_line(size_t lineno, size_t & linestart, size_t & linelen);
    TextIterator iterate_line_offset(size_t offset_chars, size_t & lineno, size_t & linestart);


    //size_t  GetLine(size_t lineno, char* buf, size_t len, size_t* fileoff = 0);
    //size_t  GetLine(size_t lineno, size_t offset, char* buf, size_t len, size_t* fileoff = 0);

    //size_t  GetData(size_t offset, char* buf, size_t len);


    const uint32_t  GetFileFormat() const;
    const size_t    GetLineCount() const;
    const size_t    GetLongestLine(int tabwidth) const;
    const size_t    GetDocLength() const;
    
private:
    bool            InitLineBuffer();
    uint32_t        DetectFileFormat(int & headerSize);
    int             GetChar(size_t offset, size_t lenBytes, char32_t& pch32);
    int             GetText(size_t offset, size_t lenBytes, wchar_t* buf, size_t & bufLen);

    char*       docBuffer;


    size_t      lengthChars;
    size_t      lengthBytes;//documentLength

    uint32_t    fileFormat;
    int         headerSize;

    size_t*     lineOffsetByte;
    size_t*     lineOffsetChar;
    
    size_t      LineCount;
    


};


class TextIterator
{
private:
    //use shared_ptr<TextDocument>
    TextDocument* textDoc;
    size_t offsetBytes;
    size_t lengthBytes;
public:
    // default constructor sets all members to zero
    TextIterator()
        : textDoc(0), offsetBytes(0), lengthBytes(0)
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
        //自赋值考虑一下 NEED TO BE UPDATED
        textDoc = ti.textDoc;
        offsetBytes = ti.offsetBytes;
        lengthBytes = ti.lengthBytes;
        return *this;
    }

    int GetText(wchar_t* buf, size_t bufLen)
    {
        if (textDoc)
        {
            // get text from the TextDocument at the specified byte-offset
            int len = textDoc->GetText(offsetBytes, lengthBytes, buf, bufLen);

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
        return textDoc ? true : false;
    }
};




//Byte Order Mark
enum class BOM //uint32_t
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

    static const std::vector<BOMLookup>& GetInstances()
    {
        static std::vector<BOMLookup> instances;
        if(instances.empty())
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