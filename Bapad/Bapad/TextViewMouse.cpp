#include "TextView.h"


int ScrollDir(int counter, int dir);

//
//	WM_MOUSEACTIVATE
//
//	Grab the keyboard input focus 
//	
LONG TextView::OnMouseActivate(HWND hwndTop, UINT nHitTest, UINT nMessage)
{
  SetFocus(this->hWnd);
  return MA_ACTIVATE;
}

//
//	WM_LBUTTONDOWN
//
//  Position caret to nearest text character under mouse
//
LONG TextView::OnLButtonDown(UINT nFlags, int mx, int my)
{
  size_t nLineNo;
  size_t nFileOff;

  // map the mouse-coordinates to a real file-offset-coordinate
  MouseCoordToFilePos(mx, my, nLineNo, nFileOff, caretPosX);
  anchorPosX = caretPosX;

  UpdateCaretXY(caretPosX, nLineNo);
  
  if (IsKeyPressed(VK_SHIFT) == false)
  {
    InvalidateRange(selectionStart, selectionEnd);
    // reset cursor and selection offsets to the same location
    selectionStart = nFileOff;
    selectionEnd = nFileOff;
    cursorOffset = nFileOff;
  }
  currentLine = nLineNo;
  // set capture for mouse-move selections
  mouseDown = true;
  SetCapture(hWnd);
  RefreshWindow();

  return 0;
}

//
//	Redraw the specified range of text/data in the control
//
LONG TextView::InvalidateRange(size_t nStart, size_t nFinish)
{
  size_t start = (std::min)(nStart, nFinish);
  const size_t finish = (std::max)(nStart, nFinish);

  // nothing to do?
  if (start == finish)
    return 0;

  size_t lineNo = pTextDoc->LineNumFromCharOffset(start);
  size_t offChars = 0, lenChars = 0;

  TextIterator itor;
  // clip to top of window
  if (lineNo < vScrollPos)
  {
    lineNo = vScrollPos;
    itor = pTextDoc->IterateLineByLineNumber(lineNo, &offChars, &lenChars);
    start = offChars;
  }
  else
  {
    itor = pTextDoc->IterateLineByLineNumber(lineNo, &offChars, &lenChars);
  }

  if (!itor || start >= finish)
    return 0;

  LONGLONG ypos = (lineNo - vScrollPos) * lineHeight;
  RECT  rect = { 0 };
  RECT client = { 0 };
  GetClientRect(hWnd, &client);

  while (itor && offChars < finish)
  {
    SetRect(&client, 0, ypos, client.right, ypos + lineHeight);
    rect.left -= hScrollPos * fontWidth;
    InvalidateRect(hWnd, &rect, FALSE);
    itor = pTextDoc->IterateLineByLineNumber(++lineNo, &offChars, &lenChars);
    ypos += lineHeight;
  }
  return 0;
}



//
//	WM_LBUTTONUP 
//
//	Release capture and cancel any mouse-scrolling
//
LONG TextView::OnLButtonUp(UINT nFlags, int mx, int my)
{
  if (mouseDown)
  {
    // cancel the scroll-timer if it is still running
    if (scrollTimer != 0)
    {
      KillTimer(hWnd, scrollTimer);
      scrollTimer = 0;
    }

    mouseDown = false;
    ReleaseCapture();
    RefreshWindow();
  }

  return 0;
}

//
//	WM_MOUSEMOVE
//
//	Set the selection end-point if we are dragging the mouse
//
LONG TextView::OnMouseMove(UINT nFlags, int mx, int my)
{
  if (mouseDown)
  {
    size_t	nLineNo, nFileOff;
    RECT	rect;
    const POINT	pt = {mx, my};
    // caret coordinates

    // get the non-scrolling area (an even no. of lines)
    GetClientRect(hWnd, &rect);
    rect.bottom -= rect.bottom % lineHeight;

    // If mouse is within this area, we don't need to scroll
    if (PtInRect(&rect, pt))
    {
      if (scrollTimer != 0)
      {
        KillTimer(hWnd, scrollTimer);
        scrollTimer = 0;
      }
    }
    // If mouse is outside window, start a timer in
    // order to generate regular scrolling intervals
    else
    {
      if (scrollTimer == 0)
      {
        scrollCounter = 0;
        scrollTimer = SetTimer(hWnd, 1, 30, nullptr);
      }
    }

    // get new cursor offset+coordinates
    MouseCoordToFilePos(mx, my, nLineNo, nFileOff, caretPosX);
    anchorPosX = caretPosX;
    currentLine = nLineNo;
    //caretPosX = mx + hScrollPos * fontWidth;
    // update the region of text that has changed selection state
    //if (selectionEnd != nFileOff)
    {
      // redraw from old selection-pos to new position
      InvalidateRange(selectionEnd, nFileOff);

      // adjust the cursor + selection *end* to the new offset
      selectionEnd = nFileOff;
      cursorOffset = nFileOff;
    }

    RefreshWindow();
    // always set the caret position because we might be scrolling
    UpdateCaretXY(caretPosX, currentLine);
  }

  return 0;
}


//
//	Convert mouse(client) coordinates to a file-relative offset
//
//	Currently only uses the main font so will not support other
//	fonts introduced by syntax highlighting
//
BOOL TextView::MouseCoordToFilePos( LONGLONG mx, // [in]  mouse x-coord 
                                    LONGLONG my, // [in]mouse y - coord
                                    size_t& pnLineNo, // [out] line number
                                    size_t& pfnFileOffset, // [out] char-offset from 0
                                    int& px // [out] adjusted x coord of caret
)
{
  size_t nLineNo;
  size_t charOff = 0;
  size_t  len;
  LONGLONG  curx = 0;
  RECT  rect;

  // get scrollable area
  GetClientRect(hWnd, &rect);
  rect.bottom -= rect.bottom % lineHeight;

  // clip mouse to edge of window
  if (mx < 0)				mx = 0;
  if (my < 0)				my = 0;
  if (my >= rect.bottom)  my = rect.bottom - 1;
  if (mx >= rect.right) mx = rect.right - 1;


  nLineNo = gsl::narrow_cast<size_t>((my / lineHeight)) + vScrollPos;

  // make sure we don't go outside of the document
  if (nLineNo >= lineCount)
  {
    nLineNo = lineCount ? lineCount - 1 : 0;
    charOff = pTextDoc->GetDocLength();
  }

  HDC    hdc = GetDC(hWnd);
  HANDLE hOldFont = SelectObject(hdc, gsl::at(fontAttr,0).hFont);

  mx += hScrollPos * fontWidth;

  TextIterator itor = pTextDoc->IterateLineByLineNumber(nLineNo, &charOff, nullptr);
  
  auto buf = itor.GetLine();
  len = buf.size();
  if (len > 0)
  {
    len = StripCRLF(buf, true);

    // find it's width
    int width = BaTextWidth(hdc, buf.data(), len, -(curx % TabWidth()));

    // does cursor fall within this segment?
    if (mx >= curx && mx < curx + width)
    {
      LONGLONG low = 0;
      LONGLONG high = len;
      LONGLONG lowx = 0;
      LONGLONG highx = width;

      while (low < high - 1)
      {
        const LONGLONG newlen = (high - low) / 2;

        width = BaTextWidth(hdc, buf.data() + low, newlen, -lowx - curx);

        if (mx - curx < width + lowx)
        {
          high = low + newlen;
          highx = lowx + width;
        }
        else
        {
          low = low + newlen;
          lowx = lowx + width;
        }
      }

      // base coordinates on centre of characters, not the edges
      if (mx - curx > highx - fontWidth / 2)
      {
        curx += highx;
        charOff += high;
      }
      else
      {
        curx += lowx;
        charOff += low;
      }
    }
    else
    {
      curx += width;
      charOff += len;
    }
  }

  SelectObject(hdc, hOldFont);
  ReleaseDC(hWnd, hdc);


  pnLineNo = nLineNo;
  pfnFileOffset = charOff;
  px = curx - gsl::narrow_cast<LONGLONG>(hScrollPos) * static_cast<LONGLONG>(fontWidth);
  return 0;
}


//
//	WM_TIMER handler
//
//	Used to create regular scrolling 
//
LONG TextView::OnTimer(UINT nTimerId)
{
  int	  dx = 0, dy = 0;	// scrolling vectors
  RECT  rect;
  POINT pt;

  // find client area, but make it an even no. of lines
  GetClientRect(hWnd, &rect);
  rect.bottom -= rect.bottom % lineHeight;

  // get the mouse's client-coordinates
  GetCursorPos(&pt);
  ScreenToClient(hWnd, &pt);

  //
  // scrolling up / down??
  //
  if (pt.y < rect.top)
    dy = ScrollDir(scrollCounter, pt.y - rect.top);

  else if (pt.y >= rect.bottom)
    dy = ScrollDir(scrollCounter, pt.y - rect.bottom);

  //
  // scrolling left / right?
  //
  if (pt.x < rect.left)
    dx = ScrollDir(scrollCounter, pt.x - rect.left);

  else if (pt.x > rect.right)
    dx = ScrollDir(scrollCounter, pt.x - rect.right);

  //
  // Scroll the window but don't update any invalid
  // areas - we will do this manually after we have 
  // repositioned the caret
  //
  HRGN hrgnUpdate = ScrollRgn(dx, dy, true);

  //
  // do the redraw now that the selection offsets are all 
  // pointing to the right places and the scroll positions are valid.
  //
  if (hrgnUpdate != nullptr)
  {
    OnMouseMove(0, pt.x, pt.y);

    InvalidateRgn(hWnd, hrgnUpdate, FALSE);
    DeleteObject(hrgnUpdate);

    UpdateWindow(hWnd);
  }
  scrollCounter++;

  return 0;
}

void TextView::RepositionCaret()
{
  UpdateCaretXY(caretPosX, currentLine);
}

//
//	return direction to scroll (+1,-1 or 0) based on 
//  distance of mouse from window edge
//
//	"counter" is used to achieve variable-speed scrolling
//
int ScrollDir(int counter, int distance)
{
  // amount to scroll based on distance of mouse from window
  if      (abs(distance) > 48)  return 5 * (distance/abs(distance));
  else if (abs(distance) > 16)  return 2 * (distance / abs(distance));
  else if (abs(distance) > 3)   return 1 * (distance / abs(distance));
  if (counter % 5 == 0)
    return distance < 0 ? -1 : 1;
  else
    return 0;
}