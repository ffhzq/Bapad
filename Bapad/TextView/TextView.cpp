#include "TextView.h"

TextView::TextView(HWND hwnd) : m_pTextDoc(new TextDocument())
{
	m_hWnd = hwnd;

	// Set the default font
	OnSetFont((HFONT)GetStockObject(ANSI_FIXED_FONT));

	// Scrollbar related data
	vScrollPos = 0;
	hScrollPos = 0;
	vScrollMax = 0;
	hScrollMax = 0;

	// File-related data
	lineCount = 0;
	longestLine = 0;


	SetupScrollbars();
}

//
//	Destructor for TextView class
//
TextView::~TextView()
{
	if (m_pTextDoc)
		m_pTextDoc.~unique_ptr();
		//delete m_pTextDoc;
}

VOID TextView::UpdateMetrics()
{
	RECT rect;
	GetClientRect(m_hWnd, &rect);

	OnSize(0, rect.right, rect.bottom);
	RefreshWindow();
}
//
//	Set a new font
//
LONG TextView::OnSetFont(HFONT hFont)
{
	HDC hdc;
	TEXTMETRIC tm;
	HANDLE hOld;

	font = hFont;

	hdc = GetDC(m_hWnd);
	hOld = SelectObject(hdc, hFont);

	GetTextMetricsW(hdc, &tm);

	fontHeight = tm.tmHeight;
	fontWidth = tm.tmAveCharWidth;

	// Restoring the original object 
	SelectObject(hdc, hOld);
	
	ReleaseDC(m_hWnd, hdc);

	UpdateMetrics();

	return 0;
}