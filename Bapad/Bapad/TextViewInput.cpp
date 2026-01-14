#include "pch.h"
#include "TextView.h"


LONG TextView::OnChar(UINT nChar, UINT nFlags) {
  WCHAR ch[2]{nChar, 0};
  int inputLength = 1;
  if (nChar < 32 && nChar != '\t' && nChar != '\r' && nChar != '\n') return 0;
  // change CR to LF
  if (nChar == '\r') {
    ch[0] = '\n';
  }
  if (EnterText(&ch[0], inputLength))
    NotifyParent(TVN_CHANGED);  // 用于通知文件被修改了
  return 0;
}

ULONG TextView::EnterText(WCHAR* inputText, ULONG inputTextLength) {
  const bool existSelectedText = selectionStart != selectionEnd;
  std::vector<char16_t> inputVec(inputText, inputText + inputTextLength);
  if (existSelectedText) {
    const size_t start = (std::min)(selectionStart, selectionEnd);
    const size_t end = (std::max)(selectionStart, selectionEnd);
    const size_t eraseLen = end - start;

    pTextDoc->ReplaceText(start, inputVec, eraseLen);
    cursorOffset = start;
  } else if (pTextDoc->InsertText(cursorOffset, inputVec) == 0) {
    return 0;
  }
  cursorOffset += inputTextLength;
  selectionStart = selectionEnd = cursorOffset;
  RefreshWindow();
  SyncMetrics(TRUE);
  return inputTextLength;
}

int TextView::ForwardDelete() {
  const size_t start = (std::min)(selectionStart, selectionEnd);
  const size_t end = (std::max)(selectionStart, selectionEnd);
  if (start != end) {
    pTextDoc->EraseText(start, end - start);
    cursorOffset = start;
  } else {
    const size_t oldOffset = cursorOffset;
    MoveCharNext();
    pTextDoc->EraseText(oldOffset, cursorOffset - oldOffset);
    cursorOffset = oldOffset;
  }
  selectionStart = selectionEnd = cursorOffset;
  RefreshWindow();
  SyncMetrics(FALSE);
  return TRUE;
}

int TextView::BackwardDelete() {
  const size_t start = (std::min)(selectionStart, selectionEnd);
  const size_t end = (std::max)(selectionStart, selectionEnd);
  if (start != end) {
    pTextDoc->EraseText(start, end - start);
    cursorOffset = start;
  } else if (cursorOffset > 0) {
    const size_t oldOffset = cursorOffset;
    MoveCharPrev();
    pTextDoc->EraseText(cursorOffset, oldOffset - cursorOffset);
    // cursorOffset = oldOffset;
  }
  selectionStart = selectionEnd = cursorOffset;
  // RefreshWindow();
  SyncMetrics(FALSE);
  return TRUE;
}

LRESULT TextView::NotifyParent(UINT nNotifyCode, NMHDR* optional) {
  UINT nCtrlId = GetWindowLongW(hWnd, GWL_ID);
  NMHDR nmhdr = {hWnd, nCtrlId, nNotifyCode};
  NMHDR* nmptr = &nmhdr;

  if (optional) {
    nmptr = optional;
    *nmptr = nmhdr;
  }

  return SendMessageW(GetParent(hWnd), WM_NOTIFY, (WPARAM)nCtrlId,
                      (LPARAM)nmptr);
}

void TextView::SyncMetrics(BOOL fAdvancing) {
  lineCount = pTextDoc->GetLineCount();
  UpdateMetrics();
  SetupScrollbars();

  UpdateCaretOffset(fAdvancing);

  ScrollToPosition(caretPosX, currentLine);
  RepositionCaret();
}

void TextView::UpdateCaretOffset(BOOL fAdvancing) {
  size_t lineno = 0;
  size_t linestartCharOffset = 0;
  TextIterator itor = pTextDoc->IterateLineByCharOffset(cursorOffset, &lineno,
                                                        &linestartCharOffset);
  // if (!itor)
  // return;

  HDC hdc = GetDC(hWnd);
  SelectObject(hdc, gsl::at(fontAttr, 0).hFont);
  // find the x-coordinate on the specified line
  auto buf = itor.GetLine();
  const size_t len = buf.size();
  LONGLONG xpos = 0;

  if (!buf.empty() && linestartCharOffset < cursorOffset) {
    const size_t offsetChars = cursorOffset - linestartCharOffset;
    // if (fAdvancing && offsetChars > 0)
    //{
    //   ++offsetChars;
    // }
    const std::span<char16_t> bufSpan(buf.begin(), offsetChars);
    xpos += BaTextWidth(hdc, bufSpan, -xpos);
  }

  ReleaseDC(hWnd, hdc);

  caretPosX = xpos;
  currentLine = lineno;
  UpdateCaretXY(xpos, lineno);
}
