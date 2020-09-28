#include "TextView.h"

//
//	Perform a full redraw of the entire window
//
VOID TextView::RefreshWindow()
{
	InvalidateRect(m_hWnd, NULL, FALSE);
}

//
//	Painting procedure for TextView objects
//
LONG TextView::OnPaint()
{
	PAINTSTRUCT ps;

	ULONG i, first, last;

	BeginPaint(m_hWnd, &ps);

	// select which font we will be using
	SelectObject(ps.hdc, font);

	// figure out which lines to redraw
	first = vScrollPos + ps.rcPaint.top / fontHeight;
	last = vScrollPos + ps.rcPaint.bottom / fontHeight;

	// make sure we never wrap around the 4gb boundary
	if (last < first) last = -1;

	// draw the display line-by-line
	for (i = first; i <= last; i++)
	{
		PaintLine(ps.hdc, i);
	}

	EndPaint(m_hWnd, &ps);

	return 0;
}

//
//	Draw the specified line
//
void TextView::PaintLine(HDC hdc, ULONG nLineNo)
{
	RECT  rect;

	WCHAR buf[LONGEST_LINE];
	size_t   len;

	// Get the area we want to update
	GetClientRect(m_hWnd, &rect);

	// calculate rectangle for entire length of line in window
	rect.left = (long)(-hScrollPos * fontWidth);
	rect.top = (long)((nLineNo - vScrollPos) * fontHeight);
	rect.right = (long)(rect.right);
	rect.bottom = (long)(rect.top + fontHeight);

	// check we have data to draw on this line
	if (nLineNo >= lineCount)
	{
		SetBkColor(hdc, GetColour(TXC_BACKGROUND));
		ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
		return;
	}

	// get the data for this single line of text
	len = m_pTextDoc->getline(nLineNo, buf, LONGEST_LINE);

	// set the colours
	SetTextColor(hdc, GetColour(TXC_FOREGROUND));
	SetBkColor(hdc, GetColour(TXC_BACKGROUND));

	// draw text and fill line background at the same time
	TabbedExtTextOut(hdc, &rect, buf, len);
}

//
//	Emulates ExtTextOut, but draws text using tabs using TabbedTextOut
//
void TextView::TabbedExtTextOut(HDC hdc, RECT* rect, WCHAR* buf, size_t len)
{
	int  tab = 4 * fontWidth;
	int  width;
	RECT fill = *rect;

	// Draw line and expand tabs
	width = TabbedTextOutW(hdc, rect->left, rect->top, buf, static_cast<int>(len), 1, &tab, rect->left);

	// Erase the rest of the line with the background colour
	fill.left += LOWORD(width);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &fill, 0, 0, 0);
}

//
//	Return an RGB value corresponding to the specified HVC_xxx index
//
COLORREF TextView::GetColour(UINT idx)
{
	switch (idx)
	{
		case TXC_BACKGROUND: return GetSysColor(COLOR_WINDOW);
		case TXC_FOREGROUND: return GetSysColor(COLOR_WINDOWTEXT);
		default:			 return 0;
	}
}