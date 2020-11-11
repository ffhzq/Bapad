#include "pch.h"
#include "TextView.h"

static const wchar_t* CtrlStr(DWORD ch)
{
	static const wchar_t * reps[] =
	{
		L"NUL", L"SOH", L"STX", L"ETX", L"EOT", L"ENQ", L"ACK", L"BEL",
		L"BS",  L"HT",  L"LF",  L"VT",  L"FF",  L"CR",  L"SO",  L"SI",
		L"DLE", L"DC1", L"DC2", L"DC3", L"DC4", L"NAK", L"SYN", L"ETB",
		L"CAN", L"EM",  L"SUB", L"ESC", L"FS",  L"GS",  L"RS",  L"US"
	};

	return ch < ' ' ? reps[ch] : L"???";
}
//
//	Return width of specified control-character
//
int TextView::CtrlCharWidth(HDC hdc, ULONG chValue, FONT* font)
{
	SIZE sz;
	const wchar_t* str = CtrlStr(chValue % 32);
	GetTextExtentPoint32W(hdc, str, wcslen(str), &sz);
	return sz.cx + 4;
}


//
//	Wrapper for GetTextExtentPoint32. Takes into account
//	control-characters, tabs etc.
//
int TextView::BaTextWidth(HDC hdc, WCHAR* buf, int len, int nTabOrigin)
{
	SIZE	sz;
	int		width = 0;

	const int TABWIDTHPIXELS = tabWidthChars * fontWidth;

	for (int i = 0, lasti = 0; i <= len; i++)
	{
		if (i == len || buf[i] == '\t' || buf[i] < 32)
		{
			GetTextExtentPoint32W(hdc, buf + lasti, i - lasti, &sz);
			width += sz.cx;

			if (i < len && buf[i] == '\t')
			{
				width += TABWIDTHPIXELS - ((width - nTabOrigin) % TABWIDTHPIXELS);
				lasti = i + 1;
			}
			else if (i < len && buf[i] < 32)
			{
				width += CtrlCharWidth(hdc, buf[i], &fontAttr[0]);
				lasti = i + 1;
			}
		}
	}

	return width;
}
void PaintRect(HDC hdc, RECT* rect, COLORREF fill)
{
	fill = SetBkColor(hdc, fill);

	ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, rect, 0, 0, 0);

	SetBkColor(hdc, fill);
}


//
//	Set default font, WM_SETFONT handler: set a new default font
//
LONG TextView::OnSetFont(HFONT hFont)
{
	// default font is always #0
	SetFont(hFont, 0);

	fontWidth = fontAttr[0].tm.tmAveCharWidth;

	UpdateMetrics();

	return 0;
}

LONG TextView::AddFont(HFONT hFont)
{
	size_t idx = fontAttr.size();
	fontAttr.resize(idx + 1);
	SetFont(hFont, idx);
	UpdateMetrics();
	return 0;
}

LONG TextView::SetFont(HFONT hFont, size_t idx)
{

	FONT* font = &fontAttr[idx];
	// need a DC to query font data
	HDC    hdc = GetDC(0);
	HANDLE hold = SelectObject(hdc, hFont);

	// get font settings
	font->hFont = hFont;
	GetTextMetrics(hdc, &font->tm);

	// pre-calc the control-characters for this font
	//InitCtrlCharFontAttr(hdc, font);

	// cleanup
	SelectObject(hdc, hold);
	ReleaseDC(0, hdc);

	RecalcLineHeight();

	return 0;
}

//
//	Display an ASCII control character in inverted colours
//  to what is currently set in the DC
//
int TextView::PaintCtrlChar(HDC hdc, int xpos, int ypos, ULONG chValue, FONT* font)
{
	SIZE  sz;
	RECT  rect;
	const wchar_t* str = CtrlStr(chValue % 32);

	int yoff = maxAscent + heightAbove - font->tm.tmAscent;

	COLORREF fg = GetTextColor(hdc);
	COLORREF bg = GetBkColor(hdc);

	// find out how big the text will be
	GetTextExtentPoint32W(hdc, str, wcslen(str), &sz);
	SetRect(&rect, xpos, ypos, xpos + sz.cx + 4, ypos + lineHeight);

	// paint the background white
	PaintRect(hdc, &rect, bg);

	// adjust rectangle for first black block
	rect.right -= 1;
	rect.top += font->nInternalLeading + yoff;
	rect.bottom = rect.top + font->tm.tmHeight - font->nDescent - font->nInternalLeading;

	// paint the first black block
	PaintRect(hdc, &rect, fg);

	// prepare device context
	fg = SetTextColor(hdc, bg);
	bg = SetBkColor(hdc, fg);

	// paint the text and the second "black" block at the same time
	InflateRect(&rect, -1, 1);
	ExtTextOutW(hdc, xpos + 1, ypos + yoff, ETO_OPAQUE | ETO_CLIPPED, &rect, str, wcslen(str), 0);

	// restore device context
	SetTextColor(hdc, fg);
	SetBkColor(hdc, bg);

	return sz.cx + 4;
}

//
//	Manually calculate the internal-leading and descent
//  values for a font by parsing a small bitmap of a single letter "E"
//	and locating the top and bottom of this letter.
//
void TextView::InitCtrlCharFontAttr(HDC hdc, FONT* font)
{
	// create a temporary off-screen bitmap
	HDC		hdcTemp = CreateCompatibleDC(hdc);
	HBITMAP hbmTemp = CreateBitmap(font->tm.tmAveCharWidth, font->tm.tmHeight, 1, 1, 0);
	HANDLE  hbmOld = SelectObject(hdcTemp, hbmTemp);
	HANDLE  hfnOld = SelectObject(hdcTemp, font->hFont);

	// black-on-white text
	SetTextColor(hdcTemp, RGB(0, 0, 0));
	SetBkColor(hdcTemp, RGB(255, 255, 255));
	SetBkMode(hdcTemp, OPAQUE);

	TextOutW(hdcTemp, 0, 0, L"E", 1);

	// give default values just in case the scan fails
	font->nInternalLeading = font->tm.tmInternalLeading;
	font->nDescent = font->tm.tmDescent;

	// scan downwards looking for the top of the letter 'E'
	for (int y = 0; y < font->tm.tmHeight; y++)
	{
		COLORREF col;

		if ((col = GetPixel(hdcTemp, font->tm.tmAveCharWidth / 2, y)) == RGB(0, 0, 0))
		{
			font->nInternalLeading = y;
			break;
		}
	}

	// scan upwards looking for the bottom of the letter 'E'
	for (LONG y = font->tm.tmHeight - 1; y >= 0; y--)
	{
		COLORREF col;

		if ((col = GetPixel(hdcTemp, font->tm.tmAveCharWidth / 2, y)) == RGB(0, 0, 0))
		{
			font->nDescent = font->tm.tmHeight - y - 1;
			break;
		}
	}

	// give larger fonts a thicker border
	if (font->nInternalLeading > 1 && font->nDescent > 1 && font->tm.tmHeight > 18)
	{
		font->nInternalLeading--;
		font->nDescent--;
	}

	// cleanup
	SelectObject(hdcTemp, hbmOld);
	SelectObject(hdcTemp, hfnOld);
	DeleteDC(hdcTemp);
	DeleteObject(hbmTemp);
}

//
//	Update the lineheight based on current font settings
//
VOID TextView::RecalcLineHeight()
{
	lineHeight = 0;
	maxAscent = 0;

	// find the tallest font in the TextView
	for (int i = 0; i < fontAttr.size(); i++)
	{
		// always include a font's external-leading
		int fontheight = fontAttr[i].tm.tmHeight +
			fontAttr[i].tm.tmExternalLeading;

		lineHeight = max(lineHeight, fontheight);
		maxAscent = max(maxAscent, fontAttr[i].tm.tmAscent);
	}

	// add on the above+below spacings
	lineHeight += heightAbove + heightBelow;

	// force caret resize if we've got the focus
	if (GetFocus() == hWnd)
	{
		OnKillFocus(0);
		OnSetFocus(0);
	}
}




//
//	Set spacing (in pixels) above and below each line - 
//  this is in addition to the external-leading of a font
//
LONG TextView::SetLineSpacing(int nAbove, int nBelow)
{
	heightAbove = nAbove;
	heightBelow = nBelow;
	RecalcLineHeight();
	return TRUE;
}



int TextView::TabWidth()
{
	return tabWidthChars * fontWidth;
}