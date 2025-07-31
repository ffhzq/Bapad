#include "TextView.h"

//
//	Set scrollbar positions and range
//
VOID TextView::SetupScrollbars()
{
    SCROLLINFO si = { sizeof(si) };

    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;

    //
    //	Vertical scrollbar
    //
    si.nPos = vScrollPos;		// scrollbar thumb position
    si.nPage = windowLines;		// number of lines in a page
    si.nMin = 0;
    si.nMax = lineCount - 1;	// total number of lines in file

    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

    //
    //	Horizontal scrollbar
    //
    si.nPos = static_cast<int>(hScrollPos);		// scrollbar thumb position
    si.nPage = windowColumns;	// number of lines in a page
    si.nMin = 0;
    si.nMax = longestLine - 1;	// total number of lines in file

    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

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

    if (hScrollPos > longestLine - windowColumns)
    {
        hScrollPos = longestLine - windowColumns;
        repos = true;
    }

    if (vScrollPos > lineCount - windowLines)
    {
        vScrollPos = lineCount - windowLines;
        repos = true;
    }

    return repos;
}

//
//	The window has changed size - update the scrollbars
//
LONG TextView::OnSize(UINT nFlags, int width, int height)
{
    windowLines = min((unsigned)height / lineHeight, lineCount);
    windowColumns = min(width / fontWidth, longestLine);

    if (PinToBottomCorner())
    {
        RefreshWindow();
        RepositionCaret();
    }

    SetupScrollbars();

    return 0;
}

//
//	ScrollRgn
//
//	Scrolls the viewport in specified direction. If fReturnUpdateRgn is true, 
//	then a HRGN is returned which holds the client-region that must be redrawn 
//	manually. This region must be deleted by the caller using DeleteObject.
//
//  Otherwise ScrollRgn returns NULL and updates the entire window 
//
HRGN TextView::ScrollRgn(LONG64 dx, LONG64 dy, bool fReturnUpdateRgn)
{
    RECT clip;

    GetClientRect(hWnd, &clip);

    //
    // make sure that dx,dy don't scroll us past the edge of the document!
    //

    // scroll up
    if (dy < 0)
    {
        dy = -1 * min((ULONG64)-dy, vScrollPos);
        clip.top = -dy * lineHeight;
    }
    // scroll down
    else if (dy > 0)
    {
        dy = min((ULONG64)dy, vScrollMax - vScrollPos);
        clip.bottom = (windowLines - dy) * lineHeight;
    }


    // scroll left
    if (dx < 0)
    {
        dx = -(int)min(-dx, hScrollPos);
        clip.left = -dx * fontWidth * 4;
    }
    // scroll right
    else if (dx > 0)
    {
        dx = min((unsigned)dx, (unsigned)hScrollMax - hScrollPos);
        clip.right = static_cast<LONG>((windowColumns - dx - 4) * fontWidth);
    }

    // adjust the scrollbar thumb position
    hScrollPos += dx;
    vScrollPos += dy;

    // perform the scroll
    if (dx != 0 || dy != 0)
    {
        // do the scroll!
        ScrollWindowEx(
            hWnd,
            -dx * fontWidth,					// scale up to pixel coords
            -dy * lineHeight,
            NULL,								// scroll entire window
            fReturnUpdateRgn ? &clip : NULL,	// clip the non-scrolling part
            0,
            0,
            SW_INVALIDATE
        );

        SetupScrollbars();

        if (fReturnUpdateRgn)
        {
            RECT client;

            GetClientRect(hWnd, &client);

            HRGN hrgnClient = CreateRectRgnIndirect(&client);
            HRGN hrgnUpdate = CreateRectRgnIndirect(&clip);

            // create a region that represents the area outside the
            // clipping rectangle (i.e. the part that is never scrolled)
            CombineRgn(hrgnUpdate, hrgnClient, hrgnUpdate, RGN_XOR);

            DeleteObject(hrgnClient);

            return hrgnUpdate;
        }
    }

    return NULL;
}


//
//	Scroll viewport in specified direction
//
VOID TextView::Scroll(LONG64 dx, LONG64 dy)
{

    // do a "normal" scroll - don't worry about invalid regions,
    // just scroll the whole window 
    ScrollRgn(dx, dy, false);
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
    auto oldpos = vScrollPos;

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
        Scroll(0, -1 * static_cast<LONG64>(windowLines));
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:

        vScrollPos = GetTrackPos32(hWnd, SB_VERT);
        RefreshWindow();

        break;
    }

    if (oldpos != vScrollPos)
    {
        SetupScrollbars();
        RepositionCaret();
    }


    return 0;
}

//
//	Horizontal scrollbar support
//
LONG TextView::OnHScroll(UINT nSBCode, UINT nPos)
{
    auto oldpos = hScrollPos;

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
        Scroll(-1 * static_cast<LONG64>(windowColumns), 0);
        break;

    case SB_PAGERIGHT:
        Scroll(windowColumns, 0);
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:

        hScrollPos = GetTrackPos32(hWnd, SB_HORZ);
        RefreshWindow();
        break;
    }

    if (oldpos != hScrollPos)
    {
        SetupScrollbars();
        RepositionCaret();
    }

    return 0;
}

LONG TextView::OnMouseWheel(int nDelta)
{
#ifndef	SPI_GETWHEELSCROLLLINES	
#define SPI_GETWHEELSCROLLLINES   104
#endif

    int nScrollLines = 0;

    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &nScrollLines, 0);

    if (nScrollLines <= 1)
        nScrollLines = 3;

    Scroll(0, (-nDelta / 120) * nScrollLines);
    RepositionCaret();
    return 0;
}

VOID TextView::ScrollToPosition(int xpos, size_t lineno)
{
  bool fRefresh = false;
  RECT rect;
  int  marginWidth = 0;

  GetClientRect(hWnd, &rect);

  xpos -= hScrollPos * fontWidth;
  xpos += marginWidth;

  if (xpos < marginWidth)
  {
    hScrollPos -= (marginWidth - xpos) / fontWidth;
    fRefresh = true;
  }

  if (xpos >= rect.right)
  {
    hScrollPos += (xpos - rect.right) / fontWidth + 1;
    fRefresh = true;
  }

  if (lineno < vScrollPos)
  {
    vScrollPos = lineno;
    fRefresh = true;
  }
  else if (lineno > vScrollPos + windowLines - 1)
  {
    vScrollPos = lineno - windowLines + 1;
    fRefresh = true;
  }


  if (fRefresh)
  {
    SetupScrollbars();
    RefreshWindow();
    RepositionCaret();
  }
}