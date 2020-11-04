#include "pch.h"
#include "TextView.h"


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

LONG TextView::SetFont(HFONT hFont, int idx)
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

LONG TextView::SetLineSpacing(int nAbove, int nBelow)
{
	return 0;
}

LONG TextView::InvalidateRange(ULONG nStart, ULONG nFinish)
{
	return 0;
}

int TextView::TabWidth()
{
	return tabWidthchars * fontWidth;
}