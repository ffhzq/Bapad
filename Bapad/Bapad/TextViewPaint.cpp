#include "TextView.h"

size_t	StripCRLF(WCHAR* szText, size_t nLength);
void	PaintRect(HDC hdc, int x, int y, int width, int height, COLORREF fill);
void	DrawCheckedRect(HDC hdc, RECT* rect, COLORREF fg, COLORREF bg);

extern "C" COLORREF MixRGB(COLORREF, COLORREF);

size_t TextView::ApplyTextAttributes(size_t nLineNo, size_t nOffset, WCHAR* szText, int nTextLen, ATTR* attr)
{
	size_t	 font = nLineNo % fontAttr.size();
	COLORREF fg = RGB(rand() % 200, rand() % 200, rand() % 200);

	int i;

	size_t selstart = min(selectionStart, selectionEnd);
	size_t selend = max(selectionStart, selectionEnd);


	for (i = 0; i < nTextLen; i++)
	{
		// apply highlight colours 
		if (nOffset + i >= selstart && nOffset + i < selend)
		{
			if (GetFocus() == hWnd)
			{
				attr[i].fg = GetColour(TXC_HIGHLIGHTTEXT);
				attr[i].bg = GetColour(TXC_HIGHLIGHT);
			}
			else
			{
				attr[i].fg = GetColour(TXC_HIGHLIGHTTEXT2);
				attr[i].bg = GetColour(TXC_HIGHLIGHT2);
			}
		}
		// normal text colours
		else
		{
			attr[i].fg = GetColour(TXC_FOREGROUND);
			attr[i].bg = GetColour(TXC_BACKGROUND);
		}

		if (szText[i] == ' ')
			font = (font + 1) % fontAttr.size();

		attr[i].style = font;
	}

	//
	//	Turn any CR/LF at the end of a line into a single 'space' character
	//
	return StripCRLF(szText, nTextLen);
}

//
//	Strip CR/LF combinations from the end of a line and
//  replace with a single space character (for drawing purposes)
//
size_t StripCRLF(WCHAR* szText, size_t nLength)
{
	if (nLength >= 2)
	{
		if (szText[nLength - 2] == '\r' && szText[nLength - 1] == '\n')
		{
			szText[nLength - 2] = ' ';
			return --nLength;
		}
	}

	if (nLength >= 1)
	{
		if (szText[nLength - 1] == '\r' || szText[nLength - 1] == '\n')
		{
			szText[nLength - 1] = ' ';
			nLength--;
		}
	}

	return nLength;
}

//
//	Perform a full redraw of the entire window
//
VOID TextView::RefreshWindow()
{
	InvalidateRect(hWnd, NULL, FALSE);
}



//
//	Draw tabbed text using specified colours and font, return width of output text
//
//	We could just use TabbedTextOut - however, we need to parse the text because
//  it might contain ascii-control characters which must be output separately.
//  because of this we'll just handle the tabs at the same time.
//	
int TextView::BaTextOut(HDC hdc, int xpos, int ypos, WCHAR* szText, int nLen, int nTabOrigin, ATTR* attr)
{
	int   i;
	int   xold = xpos;
	int   lasti = 0;
	SIZE  sz;

	const int TABWIDTHPIXELS = tabWidthChars * fontWidth;

	FONT* font = &fontAttr[attr->style];

	// configure the device context
	SetTextColor(hdc, attr->fg);
	SetBkColor(hdc, attr->bg);
	SelectObject(hdc, font->hFont);

	// loop over each character
	for (i = 0; i <= nLen; i++)
	{
		int  yoff = maxAscent + heightAbove - font->tm.tmAscent;

		// output any "deferred" text before handling tab/control chars
		if (i == nLen || szText[i] == '\t' || szText[i] < 32)
		{
			RECT rect;

			// get size of text
			GetTextExtentPoint32(hdc, szText + lasti, i - lasti, &sz);
			SetRect(&rect, xpos, ypos, xpos + sz.cx, ypos + lineHeight);

			// draw the text and erase it's background at the same time
			ExtTextOut(hdc, xpos, ypos + yoff, ETO_OPAQUE, &rect, szText + lasti, i - lasti, 0);

			xpos += sz.cx;
		}

		// special case for TAB and Control characters
		if (i < nLen)
		{
			// TAB characters
			if (szText[i] == '\t')
			{
				// calculate distance in pixels to the next tab-stop
				int width = TABWIDTHPIXELS - ((xpos - nTabOrigin) % TABWIDTHPIXELS);

				// draw a blank space 
				PaintRect(hdc, xpos, ypos, width, lineHeight, attr->bg);

				xpos += width;
				lasti = i + 1;
			}
			// ASCII-CONTROL characters
			else if (szText[i] < 32)
			{
				xpos += PaintCtrlChar(hdc, xpos, ypos, szText[i], font);
				lasti = i + 1;
			}
		}
	}

	// return the width of text drawn
	return xpos - xold;
}



//
//	Painting procedure for TextView objects
//
LONG TextView::OnPaint()
{
	PAINTSTRUCT ps;

	ULONG i, first, last;

	BeginPaint(hWnd, &ps);

	// select which font we will be using
	//SelectObject(ps.hdc, font);

	// figure out which lines to redraw
	first = vScrollPos + ps.rcPaint.top / lineHeight;
	last = vScrollPos + ps.rcPaint.bottom / lineHeight;

	// make sure we never wrap around the 4gb boundary
	if (last < first) last = -1;

	// draw the display line-by-line
	for (i = first; i <= last; i++)
	{
		PaintLine(ps.hdc, i);
	}

	EndPaint(hWnd, &ps);

	return 0;
}

//
//	Draw the specified line (including margins etc)
//
void TextView::PaintLine(HDC hdc, ULONG64 nLineNo)
{
	RECT  rect;

	// Get the area we want to update
	GetClientRect(hWnd, &rect);

	// calculate rectangle for entire length of line in window
	rect.left = (long)(-hScrollPos * fontWidth);
	rect.top = (long)((nLineNo - vScrollPos) * lineHeight);
	rect.right = (long)(rect.right);
	rect.bottom = (long)(rect.top + lineHeight);

	//
	//	do things like margins, line numbers etc. here
	//

	//
	//	check we have data to draw on this line
	//
	if (nLineNo >= lineCount)
	{
		SetBkColor(hdc, GetColour(TXC_BACKGROUND));
		ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
		return;
	}

	//
	//	paint the line's text
	//
	PaintText(hdc, nLineNo, &rect);
}

//
//	Draw a line of text into the TextView window
//
void TextView::PaintText(HDC hdc, ULONG64 nLineNo, RECT* rect)
{
	wchar_t		buff[TEXTBUFSIZE]{ 0 };
	ATTR		attr[TEXTBUFSIZE];
	size_t		charoff = 0;

	size_t		colNo = 0;

	int			xpos = rect->left;
	int			ypos = rect->top;
	int			xtab = rect->left;

	//
	//	TODO: Clip text to left side of window
	//
	TextIterator itor = pTextDoc->IterateLineByLineNumber(nLineNo, &charoff);


	//
	//	Keep drawing until we reach the edge of the window
	//

	size_t	len = 0;
	while ((len = itor.GetText(buff,TEXTBUFSIZE))  > 0)
	{
		//
		//	Apply text attributes - 
		//	i.e. syntax highlighting, mouse selection colours etc.
		//
		//len = ApplyTextAttributes(nLineNo, fileoff+charoff, buff, len, attr);
		len = ApplyTextAttributes(nLineNo, charoff, buff, len, attr);

		if (len == 0)
			Sleep(0);

		//
		//	Display the text by breaking it into spans of colour/style
		//
		for (int i = 0, lasti = 0; i <= len; i++)
		{
			// if the colour or font changes, then need to output 
			if (i == len ||
				attr[i].fg != attr[lasti].fg ||
				attr[i].bg != attr[lasti].bg ||
				attr[i].style != attr[lasti].style)
			{
				xpos += BaTextOut(hdc, xpos, ypos, buff + lasti, i - lasti, xtab, &attr[lasti]);

				lasti = i;
			}
		}
		charoff += len;
	}

	//
	// Erase to the end of the line
	//
	rect->left = xpos;
	SetBkColor(hdc, GetColour(TXC_BACKGROUND));
	ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, rect, 0, 0, 0);
}

void PaintRect(HDC hdc, int x, int y, int width, int height, COLORREF fill)
{
	RECT rect = { x, y, x + width, y + height };

	fill = SetBkColor(hdc, fill);
	ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
	SetBkColor(hdc, fill);
}



//
//	Return an RGB value corresponding to the specified HVC_xxx index
//
//	If the RGB value has the top bit set (0x80000000) then it is
//  not a real RGB value - instead the low 31bits specify one
//  of the GetSysColor COLOR_xxx indices. This allows us to use
//	system colours without worrying about colour-scheme changes etc.
//
COLORREF TextView::GetColour(UINT idx)
{
	if (idx >= TXC_MAX_COLOURS)
		return 0;

	return REALIZE_SYSCOL(rgbColourList[idx]);
	
}

COLORREF TextView::SetColour(UINT idx, COLORREF rgbColour)
{
	COLORREF rgbOld;

	if (idx >= TXC_MAX_COLOURS)
		return 0;

	rgbOld = rgbColourList[idx];
	rgbColourList[idx] = rgbColour;

	return rgbOld;
}

//
//	Paint a checkered rectangle, with each alternate
//	pixel being assigned a different colour
//
void DrawCheckedRect(HDC hdc, RECT* rect, COLORREF fg, COLORREF bg)
{
	static WORD wCheckPat[8] =
	{
		0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555
	};

	HBITMAP hbmp;
	HBRUSH  hbr, hbrold;
	COLORREF fgold, bgold;

	hbmp = CreateBitmap(8, 8, 1, 1, wCheckPat);
	hbr = CreatePatternBrush(hbmp);

	hbrold = (HBRUSH)SelectObject(hdc, hbr);

	fgold = SetTextColor(hdc, fg);
	bgold = SetBkColor(hdc, bg);

	PatBlt(hdc, rect->left, rect->top,
		rect->right - rect->left,
		rect->bottom - rect->top,
		PATCOPY);

	SetBkColor(hdc, bgold);
	SetTextColor(hdc, fgold);

	SelectObject(hdc, hbrold);
	DeleteObject(hbr);
	DeleteObject(hbmp);
}

COLORREF MixRGB(COLORREF rgbCol1, COLORREF rgbCol2)
{
	return RGB(
		(GetRValue(rgbCol1) + GetRValue(rgbCol2)) / 2,
		(GetGValue(rgbCol1) + GetGValue(rgbCol2)) / 2,
		(GetBValue(rgbCol1) + GetBValue(rgbCol2)) / 2
	);
}

COLORREF RealizeColour(COLORREF col)
{
	COLORREF result = col;

	if (col & 0x80000000)
		result = GetSysColor(col & 0xff);

	if (col & 0x40000000)
		result = MixRGB(GetSysColor((col & 0xff00) >> 8), result);

	if (col & 0x20000000)
		result = MixRGB(GetSysColor((col & 0xff00) >> 8), result);

	return result;
}