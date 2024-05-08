#include "TextView.h"


LONG TextView::OnChar(UINT nChar, UINT nFlags)
{
	WCHAR ch = (WCHAR)nChar;

	if (nChar < 32 && nChar != '\t' && nChar != '\r' && nChar != '\n')
		return 0;
	// change CR into a CR/LF sequence
	if (nChar == '\r')
		PostMessage(hWnd, WM_CHAR, '\n', 1);
	if (EnterText(&ch, 1))
	{
		
	}
	return 0;
}

ULONG TextView::EnterText(WCHAR * inputText, ULONG inputTextLength)
{
	bool existSelectedText = selectionStart != selectionEnd;

	//INSERT MODE, TWO STEPS    ;  TODO:READONLY,OVERWRITE
	//
	if (existSelectedText)
	{
		size_t start = min(selectionStart, selectionEnd);
		size_t end = max(selectionStart, selectionEnd);
		size_t eraseLen = end - start;
		pTextDoc->EraseText(start, eraseLen);
		cursorOffset = start;
	}
	if (pTextDoc->InsertText(cursorOffset, inputText, inputTextLength) == 0)
	{
		return 0;
	}
}