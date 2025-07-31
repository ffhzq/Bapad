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
        NotifyParent(TVN_CHANGED);//用于通知文件被修改了
    }
    return 0;
}

ULONG TextView::EnterText(WCHAR* inputText, ULONG inputTextLength)
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

LRESULT TextView::NotifyParent(UINT nNotifyCode, NMHDR* optional)
{
    UINT  nCtrlId = GetWindowLongW(hWnd, GWL_ID);
    NMHDR nmhdr = { hWnd, nCtrlId, nNotifyCode };
    NMHDR* nmptr = &nmhdr;

    if (optional)
    {
        nmptr = optional;
        *nmptr = nmhdr;
    }

    return SendMessageW(GetParent(hWnd), WM_NOTIFY, (WPARAM)nCtrlId, (LPARAM)nmptr);
}

void TextView::Smeg(BOOL fAdvancing)
{
    lineCount = pTextDoc->GetLineCount();
    UpdateMetrics();
    SetupScrollbars();

    UpdateCaretOffset(fAdvancing);
    anchorPosX = caretPosX;
    ScrollToPosition(caretPosX, currentLine);
    RepositionCaret();
}


void TextView::UpdateCaretOffset(BOOL fAdvancing)
{
  caretPosX = cursorOffset * fontWidth;
  if (fAdvancing)
  {

  }

  size_t		lineno = 0;
  size_t		charoff = 0;
  size_t		offset = 0;
  LONGLONG	xpos = 0;
  LONGLONG	ypos = 0;
  ULONG64		len = 0;
  wchar_t buf[TEXTBUFSIZE]{{0}};


  // get start-of-line information from cursor-offset
  TextIterator itor = pTextDoc->IterateLineByCharOffset(cursorOffset, &lineno, &charoff);

  if (!itor)
    return ;

  // make sure we are using the right font
  HDC hdc = GetDC(hWnd);
  SelectObject(hdc, fontAttr[0].hFont);



  // y-coordinate from line-number
  ypos = (lineno - vScrollPos) * lineHeight;

  // now find the x-coordinate on the specified line
  while ((len = itor.GetText(buf, TEXTBUFSIZE)) > 0 && charoff < cursorOffset)
  {
    len = min(cursorOffset - charoff, len);
    xpos += BaTextWidth(hdc, buf, len, -xpos);
    offset += len;

  }

  ReleaseDC(hWnd, hdc);

  // take horizontal scrollbar into account
  xpos -= hScrollPos * fontWidth;
  //xpos += LeftMarginWidth();

  caretPosX = xpos;

  //SetCaretPos(static_cast<int>(xpos), static_cast<int>(ypos));


}
