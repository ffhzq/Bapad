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
	//RepositionCaret();
}

/*
VOID TextView::UpdateCaretOffset(ULONG offset, BOOL fTrailing, int* outx = 0, ULONG* outlineno = 0)
{
	size_t lineno = 0;
	int xpos = 0;
	size_t off_chars;
	USPDATA* uspData;

	// get line information from cursor-offset
	if (pTextDoc->LineInfoFromOffset(offset, &lineno, &off_chars, 0, 0, 0))
	{
		// locate the USPDATA for this line
		if ((uspData = GetUspData(NULL, lineno)) != 0)
		{
			// convert character-offset to x-coordinate
			off_chars = cursorOffset - off_chars;

			if (fTrailing && off_chars > 0)
				ScriptCPtoX(off_chars - 1, TRUE, off_chars, uspData, &xpos);
			//UspOffsetToX(uspData, off_chars - 1, TRUE, &xpos);
			else
				ScriptCPtoX(off_chars, FALSE, off_chars, uspData, &xpos);
			//UspOffsetToX(uspData, off_chars, FALSE, &xpos);

		// update caret position
			UpdateCaretXY(xpos, lineno);
		}
	}

	if (outx)	  *outx = xpos;
	if (outlineno) *outlineno = lineno;
}
*/