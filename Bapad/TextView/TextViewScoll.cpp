#include "TextView.h"

//
//	Set scrollbar positions and range
//
VOID TextView::SetupScrollbars()
{//size_t to int ULONG
	SCROLLINFO si = { sizeof(si) };

	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;

	//
	//	Vertical scrollbar
	//
	si.nPos = vScrollPos;		// scrollbar thumb position
	si.nPage = windowLines;		// number of lines in a page
	si.nMin = 0;
	si.nMax = lineCount - 1;	// total number of lines in file

	SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	//
	//	Horizontal scrollbar
	//
	si.nPos = hScrollPos;		// scrollbar thumb position
	si.nPage = windowColumns;	// number of lines in a page
	si.nMin = 0;
	si.nMax = longestLine - 1;	// total number of lines in file

	SetScrollInfo(m_hWnd, SB_HORZ, &si, TRUE);

	// adjust our interpretation of the max scrollbar range to make
	// range-checking easier. The scrollbars don't use these values, they
	// are for our own use.
	vScrollMax = lineCount - windowLines;
	hScrollMax = longestLine - windowColumns;
}

//
//	Ensure that we never scroll off the end of the file
//
bool TextView::PinToBottomCorner()
{
	bool repos = false;

	if (hScrollPos + windowColumns > longestLine)
	{
		hScrollPos = longestLine - windowColumns;
		repos = true;
	}

	if (vScrollPos + windowLines > lineCount)
	{
		vScrollPos = lineCount - windowLines;
		repos = true;
	}

	return repos;
}

//
//	The window has changed size - update the scrollbars
//
LONG TextView::OnSize(int width, int height)
{
	windowLines = min((unsigned)height / fontHeight, lineCount);
	windowColumns = min(width / fontWidth, longestLine);

	if (PinToBottomCorner())
	{
		RefreshWindow();
	}

	SetupScrollbars();

	return 0;
}

//
//	Scroll viewport in specified direction
//
VOID TextView::Scroll(int dx, int dy)
{
	//
	// make sure that dx,dy don't scroll us past the edge of the document!
	//

	// scroll up
	if (dy < 0)
	{
		dy = -(int)min((ULONG)-dy, vScrollPos);
	}
	// scroll down
	else if (dy > 0)
	{
		dy = min((ULONG)dy, vScrollMax - vScrollPos);
	}


	// scroll left
	if (dx < 0)
	{
		dx = -(int)min(-dx, hScrollPos);
	}
	// scroll right
	else if (dx > 0)
	{
		dx = min((unsigned)dx, (unsigned)hScrollMax - hScrollPos);
	}

	// adjust the scrollbar thumb position
	hScrollPos += dx;
	vScrollPos += dy;

	// perform the scroll
	if (dx != 0 || dy != 0)
	{
		ScrollWindowEx(
			m_hWnd,
			-dx * fontWidth,
			-dy * fontHeight,
			NULL,
			NULL,
			0, 0, SW_INVALIDATE
		);

		SetupScrollbars();
	}
}

LONG GetTrackPos32(HWND hwnd, int nBar)
{
	SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
	GetScrollInfo(hwnd, nBar, &si);
	return si.nTrackPos;
}

//
//	Vertical scrollbar support
//
LONG TextView::OnVScroll(UINT nSBCode, UINT nPos)
{
	ULONG oldpos = vScrollPos;

	switch (nSBCode)
	{
		case SB_TOP:
			vScrollPos = 0;
			RefreshWindow();
			break;

		case SB_BOTTOM:
			vScrollPos = vScrollMax;
			RefreshWindow();
			break;

		case SB_LINEUP:
			Scroll(0, -1);
			break;

		case SB_LINEDOWN:
			Scroll(0, 1);
			break;

		case SB_PAGEDOWN:
			Scroll(0, windowLines);
			break;

		case SB_PAGEUP:
			Scroll(0, -windowLines);
			break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:

			vScrollPos = GetTrackPos32(m_hWnd, SB_VERT);
			RefreshWindow();

			break;
	}

	if (oldpos != vScrollPos)
	{
		SetupScrollbars();
	}


	return 0;
}

//
//	Horizontal scrollbar support
//
LONG TextView::OnHScroll(UINT nSBCode, UINT nPos)
{
	int oldpos = hScrollPos;

	switch (nSBCode)
	{
		case SB_LEFT:
			hScrollPos = 0;
			RefreshWindow();
			break;

		case SB_RIGHT:
			hScrollPos = hScrollMax;
			RefreshWindow();
			break;

		case SB_LINELEFT:
			Scroll(-1, 0);
			break;

		case SB_LINERIGHT:
			Scroll(1, 0);
			break;

		case SB_PAGELEFT:
			Scroll(-windowColumns, 0);
			break;

		case SB_PAGERIGHT:
			Scroll(windowColumns, 0);
			break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:

			hScrollPos = GetTrackPos32(m_hWnd, SB_HORZ);
			RefreshWindow();
			break;
	}

	if (oldpos != hScrollPos)
	{
		SetupScrollbars();
	}

	return 0;
}

LONG TextView::OnMouseWheel(int nDelta)
{
#ifndef	SPI_GETWHEELSCROLLLINES	
#define SPI_GETWHEELSCROLLLINES   104
#endif

	int nScrollLines;

	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nScrollLines, 0);

	if (nScrollLines <= 1)
		nScrollLines = 3;

	Scroll(0, (-nDelta / 120) * nScrollLines);

	return 0;
}
