#include "TextView.h"


//
//	
//
LONG TextView::OpenFile(WCHAR* szFileName)
{
    ClearFile();

    if (pTextDoc->Initialize(szFileName))
    {
        lineCount = pTextDoc->GetLineCount();
        longestLine = pTextDoc->GetLongestLine(4);

        vScrollPos = 0;
        hScrollPos = 0;

        selectionStart = 0;
        selectionEnd = 0;
        UpdateMetrics();
        return TRUE;
    }

    return FALSE;
}

//
//
//
LONG TextView::ClearFile()
{
    if (pTextDoc)
        pTextDoc->Clear();

    lineCount = 0;//pTextDoc->GetLineCount();
    longestLine = 0;//pTextDoc->GetLongestLine(4);

    vScrollPos = 0;
    hScrollPos = 0;

    UpdateMetrics();

    return TRUE;
}