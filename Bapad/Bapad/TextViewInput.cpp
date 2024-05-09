#include "TextView.h"


LONG TextView::OnChar(UINT nChar, UINT nFlags)
{
	WCHAR ch = (WCHAR)nChar;

	if (nChar < 32 && nChar != '\t' && nChar != '\r' && nChar != '\n')
		return 0;
	// change CR into a CR/LF sequence
	if (nChar == '\r')
		PostMessageW(hWnd, WM_CHAR, '\n', 1);
	if (EnterText(&ch, 1))
	{
		//NotifyParent(TVN_CHANGED);用于通知文件被修改了
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
	cursorOffset += inputTextLength;
	selectionStart = selectionEnd = cursorOffset;
	RefreshWindow();
	Smeg(TRUE);
	return inputTextLength;
}

ULONG TextView::NotifyParent(UINT nNotifyCode, NMHDR* optional)
{
	UINT  nCtrlId = GetWindowLongW(hWnd, GWL_ID);
	NMHDR nmhdr = { hWnd, nCtrlId, nNotifyCode };
	NMHDR* nmptr = &nmhdr;

	if (optional)
	{
		nmptr = optional;
		*nmptr = nmhdr;
	}

	return SendMessage(GetParent(hWnd), WM_NOTIFY, (WPARAM)nCtrlId, (LPARAM)nmptr);
}

void TextView::Smeg(BOOL fAdvancing)
{
	pTextDoc->ReCalculateLineBuffer();
	lineCount = pTextDoc->GetLineCount();
	UpdateMetrics();
	SetupScrollbars();
	//UpdateCaretOffset(cursorOffset, fAdvancing, caretPosX, currentLine);

	anchorPosX = caretPosX;
	//ScrollToPosition(caretPosX, currentLine);
	RepositionCaret();
}