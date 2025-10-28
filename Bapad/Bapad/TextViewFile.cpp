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
        longestLine = GetLongestLine();
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

    lineCount = pTextDoc->GetLineCount();
    longestLine = GetLongestLine();

    vScrollPos = 0;
    hScrollPos = 0; 

    cursorOffset = 0;
    selectionStart = 0;
    selectionEnd = 0;

    currentLine = 0;
    caretPosX = 0;

    UpdateMetrics();

    return TRUE;
}