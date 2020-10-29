#include "pch.h"
#include "TextView.h"


//
//	Set default font
//
LONG TextView::OnSetFont(HFONT hFont)
{
	/*HDC hdc;
	TEXTMETRIC tm;
	HANDLE hOld;

	font = hFont;

	hdc = GetDC(hWnd);
	hOld = SelectObject(hdc, hFont);

	GetTextMetricsW(hdc, &tm);

	fontHeight = tm.tmHeight;
	fontWidth = tm.tmAveCharWidth;

	// Restoring the original object 
	SelectObject(hdc, hOld);

	ReleaseDC(hWnd, hdc);*/

	// default font is always #0
	SetFont(hFont, 0);

	fontWidth = fontAttr[0].tm.tmAveCharWidth;

	UpdateMetrics();

	return 0;
}

LONG SetFont(HFONT, int idx)
{
	return 0;
}
LONG SetLineSpacing(int nAbove, int nBelow)
{
	return 0;
}