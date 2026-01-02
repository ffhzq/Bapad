#include "pch.h"
#include "TextView.h"
#include <string>

LONG TextView::OpenFile(WCHAR* szFileName)
{
    ClearFile();
    std::wstring ws;
    std::vector<char16_t> filePath(ws.begin(), ws.end());
    if (pTextDoc->Initialize(filePath))
    {
        lineCount = pTextDoc->GetLineCount();
        longestLine = GetLongestLine();
        UpdateMetrics();
        return TRUE;
    }

    return FALSE;
}

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