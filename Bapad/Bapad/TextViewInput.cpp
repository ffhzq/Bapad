#include "TextView.h"


LONG TextView::OnChar(UINT nChar, UINT nFlags)
{
  WCHAR ch[2]{ nChar , 0 };
  int inputLength = 1;
  if (nChar < 32 && nChar != '\t' && nChar != '\r' && nChar != '\n')
    return 0;
  // change CR into a CR/LF sequence
  if (nChar == '\r')
  {
    ch[1] = '\n';
    inputLength += 1;
  }
  if (EnterText(&ch[0], inputLength))
    NotifyParent(TVN_CHANGED);//用于通知文件被修改了
  return 0;
}

ULONG TextView::EnterText(WCHAR* inputText, ULONG inputTextLength)
{
  const bool existSelectedText = selectionStart != selectionEnd;


  if (existSelectedText)
  {
    const size_t start = (std::min)(selectionStart, selectionEnd);
    const size_t end = (std::max)(selectionStart, selectionEnd);
    const size_t eraseLen = end - start;
    pTextDoc->ReplaceText(start, inputText, inputTextLength, eraseLen);
    cursorOffset = start;
  }
  else if (pTextDoc->InsertText(cursorOffset, inputText, inputTextLength) == 0)
  {
    return 0;
  }
  cursorOffset += inputTextLength;
  selectionStart = selectionEnd = cursorOffset;
  RefreshWindow();
  SyncMetrics(TRUE);
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

void TextView::SyncMetrics(BOOL fAdvancing)
{
  lineCount = pTextDoc->GetLineCount();
  UpdateMetrics();
  SetupScrollbars();

  UpdateCaretOffset(fAdvancing);

  ScrollToPosition(caretPosX, currentLine);
  RepositionCaret();
}

void TextView::UpdateCaretOffset(BOOL fAdvancing)
{
  size_t  lineno = 0;
  size_t  linestartCharOffset = 0;
  TextIterator itor = pTextDoc->IterateLineByCharOffset(cursorOffset, &lineno, &linestartCharOffset);
  if (!itor)
    return;

  HDC hdc = GetDC(hWnd);
  SelectObject(hdc, gsl::at(fontAttr, 0).hFont);
  // find the x-coordinate on the specified line
  auto buf = itor.GetLine();
  const size_t len = buf.size();
  LONGLONG  xpos = 0;

  if (!buf.empty() && linestartCharOffset < cursorOffset)
  {
    size_t offsetChars = cursorOffset - linestartCharOffset;
    if (fAdvancing && offsetChars > 0)
    {
      ++offsetChars;
    }
    xpos += BaTextWidth(hdc, buf, -xpos);
  }

  ReleaseDC(hWnd, hdc);

  // take horizontal scrollbar into account
  xpos -= hScrollPos * fontWidth;

  caretPosX = xpos;
  currentLine = lineno;
  UpdateCaretXY(xpos, lineno);


}
