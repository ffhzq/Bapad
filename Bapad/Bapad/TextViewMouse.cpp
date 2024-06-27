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
	size_t nCharOff;
	size_t nFileOff;
	LONGLONG   px;

	// map the mouse-coordinates to a real file-offset-coordinate
	MouseCoordToFilePos(mx, my, nLineNo, nCharOff, nFileOff, px);

	SetCaretPos(px, (nLineNo - vScrollPos) * lineHeight);

	// reset cursor and selection offsets to the same location
	selectionStart = nFileOff;
	selectionEnd = nFileOff;
	cursorOffset = nFileOff;

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
	size_t start = min(nStart, nFinish);
	size_t finish = max(nStart, nFinish);

	LONGLONG  xpos1 = 0, xpos2 = 0;
	LONGLONG  ypos = 0;



	RECT client = {0};
	RECT  rect = {};
	

	// nothing to do?
	if (start == finish)
		return 0;

	//
	//	Find the line number and character offset of specified file-offset
	//

	size_t lineNo = pTextDoc->LineNumFromOffset(start);
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

	ypos = (lineNo - vScrollPos) * lineHeight;
	GetClientRect(hWnd, &client);

	while (itor && offChars < finish)
	{
		SetRect(&client, 0, ypos, client.right, ypos + lineHeight);
		rect.left -= hScrollPos * fontWidth;
		InvalidateRect(hWnd, &rect, FALSE);
		itor = pTextDoc->IterateLineByLineNumber(++lineNo, &offChars, &lenChars);
		ypos += lineHeight;
	}



	
	/*	
	HDC hdc = GetDC(hWnd);
	SelectObject(hdc, fontAttr[0].hFont);
	// selection starts midline...
	if (offChars < start)
	{
		size_t	len = start - offChars;
		size_t	tlen = 0;
		int   width = 0;

		wchar_t buf[TEXTBUFSIZE];

		// loop until we get on-screen
		while ((tlen = itor.GetText(buf, min(len, TEXTBUFSIZE))))
		{
			len -= tlen;
			offChars += tlen;
			lenChars -= tlen;

			width = BaTextWidth(hdc, buf, tlen, -(xpos1 % TabWidth()));
			xpos1 += width;

			if (tlen == 0)
				break;
		}

		// xpos now equals the start of range
	}

	RECT rect;

	// Invalidate any lines that aren't part of the last line
	while (itor && finish >= offChars + lenChars)
	{
		SetRect(&rect, xpos1, ypos, client.right, ypos + lineHeight);

		rect.left -= static_cast<LONG>(hScrollPos * fontWidth);
		//rect.left += LeftMarginWidth();

		//InvertRect(hdc, &rect);
		InvalidateRect(hWnd, &rect, FALSE);

		xpos1 = 0;
		ypos += lineHeight;

		auto a = pTextDoc->IterateLineByLineNumber(++lineNo, &offChars, &lenChars);
		itor = a;
	}

	// erase up to the end of selection
	xpos2 = xpos1;
	if (offChars < finish)
	{
		wchar_t buf[TEXTBUFSIZE]{ {0} };
		int width{0};
		LONGLONG len{0};

		while ((len = itor.GetText(buf,min((finish - offChars),TEXTBUFSIZE))) && offChars < finish)
		{
			width = BaTextWidth(hdc, buf, len, -(xpos2 % TabWidth()));
			xpos2 += width;
			offChars += len;
		}
	}


	
	SetRect(&rect, xpos1, ypos, xpos2, ypos + lineHeight);
	OffsetRect(&rect, -hScrollPos * fontWidth, 0);
	//OffsetRect(&rect, LeftMarginWidth(), 0);

	//InvertRect(hdc, &rect);
	InvalidateRect(hWnd, &rect, FALSE);

	ReleaseDC(hWnd, hdc);
	*/
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
		size_t	nLineNo, nCharOff, nFileOff;
		RECT	rect;
		POINT	pt = { mx, my };
		// caret coordinates
		LONGLONG		cx;

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
				scrollTimer = SetTimer(hWnd, 1, 10, 0);
			}
		}

		// get new cursor offset+coordinates
		MouseCoordToFilePos(mx, my, nLineNo, nCharOff, nFileOff, cx);

		// update the region of text that has changed selection state
		if (selectionEnd != nFileOff)
		{
			// redraw from old selection-pos to new position
			InvalidateRange(selectionEnd, nFileOff);

			// adjust the cursor + selection *end* to the new offset
			selectionEnd = nFileOff;
			cursorOffset = nFileOff;
		}
		// always set the caret position because we might be scrolling
		SetCaretPos(cx, (nLineNo - vScrollPos) * lineHeight);
	}

	return 0;
}


//
//	Convert mouse(client) coordinates to a file-relative offset
//
//	Currently only uses the main font so will not support other
//	fonts introduced by syntax highlighting
//
BOOL TextView::MouseCoordToFilePos(
	LONGLONG mx, LONGLONG my,

			// [in]  mouse x-coord
	size_t&	pnLineNo,

		// [out] line number
	size_t&	pnCharOffset,

	// [out] char-offset from start of line
	size_t&	pfnFileOffset, LONGLONG & px				// [out] adjusted x coord of caret
)
{
	size_t nLineNo;

	size_t charOff = 0;
	size_t fileoff = 0;

	wchar_t buf[TEXTBUFSIZE];
	size_t  len;
	LONGLONG  curx = 0;
	RECT  rect;

	// get scrollable area
	GetClientRect(hWnd, &rect);
	rect.bottom -= rect.bottom % lineHeight;

	//mx -= LeftMarginWidth();

	// clip mouse to edge of window
	if (mx < 0)				mx = 0;
	if (my < 0)				my = 0;
	if (my >= rect.bottom)	my = rect.bottom - 1;
	if (mx >= rect.right)	mx = rect.right - 1;

	// It's easy to find the line-number: just divide 'y' by the line-height
	nLineNo = static_cast<size_t>((my / lineHeight)) + vScrollPos;

	// make sure we don't go outside of the document
	if (nLineNo >= lineCount)
	{
		nLineNo = lineCount ? lineCount - 1 : 0;
		fileoff = pTextDoc->GetDocLength();
	}

	HDC    hdc = GetDC(hWnd);
	HANDLE hOldFont = SelectObject(hdc, fontAttr[0].hFont);

	pnCharOffset = 0;

	mx += hScrollPos * fontWidth;

	TextIterator itor = pTextDoc->IterateLineByLineNumber(nLineNo, &charOff);
	// character offset within the line is more complicated. We have to 
	// parse the text.
	while ((len = itor.GetText(buf, TEXTBUFSIZE)) > 0)
	{
		//len = StripCRLF(buf, len, true);

		// find it's width
		int width = BaTextWidth(hdc, buf, len, -(curx % TabWidth()));

		// does cursor fall within this segment?
		if (mx >= curx && mx < curx + width)
		{
			//
			//	We have a range of text, with the mouse 
			//  somewhere in the middle. Perform a "binary chop" to
			//  locate the exact character that the mouse is positioned over
			//
			LONGLONG low = 0;
			LONGLONG high = len;
			LONGLONG lowx = 0;
			LONGLONG highx = width;

			while (low < high - 1)
			{
				LONGLONG newlen = (high - low) / 2;
				
				width = BaTextWidth(hdc, buf + low, newlen, -lowx - curx);

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

			break;
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
	pfnFileOffset = charOff;//fileoff + pnCharOffset;
	px = curx - static_cast<LONGLONG>(hScrollPos) * static_cast<LONGLONG>(fontWidth);
	//px += LeftMarginWidth();
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
	if (pt.y < 0)
		dy = ScrollDir(scrollCounter, pt.y);

	else if (pt.y >= rect.bottom)
		dy = ScrollDir(scrollCounter, pt.y - rect.bottom);

	//
	// scrolling left / right?
	//
	if (pt.x < 0)
		dx = ScrollDir(scrollCounter, pt.x);

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
	if (hrgnUpdate != NULL)
	{
		// We perform a "fake" WM_MOUSEMOVE for two reasons:
		//
		// 1. To get the cursor/caret/selection offsets set to the correct place
		//    *before* we redraw (so everything is synchronized correctly)
		//
		// 2. To invalidate any areas due to mouse-movement which won't
		//    get done until the next WM_MOUSEMOVE - and then it would
		//    be too late because we need to redraw *now*
		//
		OnMouseMove(0, pt.x, pt.y);

		// invalidate the area returned by ScrollRegion
		InvalidateRgn(hWnd, hrgnUpdate, FALSE);
		DeleteObject(hrgnUpdate);

		// the next time we process WM_PAINT everything 
		// should get drawn correctly!!
		UpdateWindow(hWnd);
	}

	// keep track of how many WM_TIMERs we process because
	// we might want to skip the next one
	scrollCounter++;

	return 0;
}


//
//	Set the caret position based on m_nCursorOffset,
//	typically used whilst scrolling 
//	(i.e. not due to mouse clicks/keyboard input)
//
ULONG TextView::RepositionCaret()
{
	size_t		lineno = 0;
	size_t		charoff = 0;
	size_t		offset = 0;
	LONGLONG	xpos = 0;
	LONGLONG	ypos = 0;
	ULONG64		len = 0;
	wchar_t buf[TEXTBUFSIZE]{ {0} };


	// get start-of-line information from cursor-offset
	TextIterator itor = pTextDoc->IterateLineByOffset(cursorOffset, &lineno, &charoff);

	if (!itor)
		return 0;

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


	SetCaretPos(static_cast<int>(xpos), static_cast<int>(ypos));
	return 0;
}

//
//	return direction to scroll (+1,-1 or 0) based on 
//  distance of mouse from window edge
//
//	"counter" is used to achieve variable-speed scrolling
//
int ScrollDir(int counter, int distance)
{
	int amt;

	// amount to scroll based on distance of mouse from window
	if (abs(distance) < 16)			amt = 8;
	else if (abs(distance) < 48)		amt = 3;
	else							amt = 1;

	if (counter % amt == 0)
		return distance < 0 ? -1 : 1;
	else
		return 0;
}