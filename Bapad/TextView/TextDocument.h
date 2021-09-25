#pragma once
#include "pch.h"

class TextDocument
{
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
    ULONG lineno_from_offset(ULONG offset);

    bool  lineinfo_from_offset(ULONG offset_chars, ULONG* lineno, ULONG* lineoff_chars, ULONG* linelen_chars, ULONG* lineoff_bytes, ULONG* linelen_bytes);
    bool  lineinfo_from_lineno(ULONG lineno, ULONG* lineoff_chars, ULONG* linelen_chars, ULONG* lineoff_bytes, ULONG* linelen_bytes);

    TextIterator iterate_line(ULONG lineno, ULONG* linestart = 0, ULONG* linelen = 0);
    TextIterator iterate_line_offset(ULONG offset_chars, ULONG* lineno, ULONG* linestart = 0);


    size_t  GetLine(size_t lineno, char* buf, size_t len, size_t* fileoff = 0);
    size_t  GetLine(size_t lineno, size_t offset, char* buf, size_t len, size_t* fileoff = 0);
    size_t  GetData(size_t offset, char* buf, size_t len);


    const uint32_t  GetFileFormat() const;
    const size_t    GetLineCount() const;
    const size_t    GetLongestLine(int tabwidth) const;
    const size_t    GetDocLength() const;
    
private:
    bool            InitLineBuffer();
    uint32_t        DetectFileFormat(int & headerSize);
    int             GetText(size_t offset, size_t lenbytes, wchar_t* buf, int* len);
    int             GetChar(size_t offset, size_t lenbytes, size_t* pch32);

    char*       buffer;


    size_t      lengthChars;
    size_t      lengthBytes;//documentLength

    uint32_t    fileFormat;
    int         headerSize;

    size_t*     lineBufferByte;
    size_t*     lineBufferChar;
    
    size_t      numLines;
    


};


class TextIterator
{
public:
    // default constructor sets all members to zero
    TextIterator()
        : textDoc(0), offsetBytes(0), lengthBytes(0)
    {
    }

    TextIterator(ULONG off, ULONG len, TextDocument* td)
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
        textDoc = ti.textDoc;
        offsetBytes = ti.offsetBytes;
        lengthBytes = ti.lengthBytes;
        return *this;
    }

    int gettext(TCHAR* buf, int buflen)
    {
        if (textDoc)
        {
            // get text from the TextDocument at the specified byte-offset
            int len = textDoc->GetText(offsetBytes, lengthBytes, buf, &buflen);

            // adjust the iterator's internal position
            offsetBytes += len;
            lengthBytes -= len;

            return buflen;
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

private:

    TextDocument* textDoc;

    ULONG offsetBytes;
    ULONG lengthBytes;
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
