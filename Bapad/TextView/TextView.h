#pragma once
#include "lib.h"
#include "TextDocument.h"

#define LONGEST_LINE 0x100


class TextView
{
public:

	TextView(HWND hwnd);
	~TextView();

	LONG OnPaint();
	LONG OnSetFont(HFONT hFont);
	LONG OnSize(int width, int height);
	//LONG OnSize(unsigned __int64 nFlags, int width, int height);

	LONG OnVScroll(UINT nSBCode, UINT nPos);
	LONG OnHScroll(UINT nSBCode, UINT nPos);
	LONG OnMouseWheel(int nDelta);

	LONG OpenFile(TCHAR* szFileName);
	LONG ClearFile();

private:

	void PaintLine(HDC hdc, ULONG line);
	void TabbedExtTextOut(HDC hdc, RECT* rect, TCHAR* buf, size_t len);
	void RefreshWindow();

	COLORREF GetColour(UINT idx);

	VOID	SetupScrollbars();
	VOID	UpdateMetrics();
	bool    PinToBottomCorner();
	void	Scroll(int dx, int dy);

	HWND	m_hWnd;

	// Font-related data	
	HFONT	font;
	int		fontWidth;
	int		fontHeight;

	// Scrollbar related data
	ULONG	vScrollPos;
	ULONG   vScrollMax;
	int		hScrollPos;
	int		hScrollMax;

	size_t		longestLine;
	int		windowLines;
	int		windowColumns;

	// File-related data
	size_t	lineCount;

	//should use smart pointer?
	//TextDocument* m_pTextDoc;
	std::unique_ptr<TextDocument> m_pTextDoc;
};


//
//	TextView API declared here
//
BOOL InitTextView();
HWND CreateTextView(HWND hwndParent);

//
//	TextView Window Messages defined here
//
#define TXM_BASE         (WM_USER)
#define TXM_OPENFILE     (TXM_BASE + 0)
#define TXM_CLEAR        (TXM_BASE + 1)

//
//	TextView Message Macros defined here
//
#define TextView_OpenFile(hwndTV, szFile)	SendMessage((hwndTV), TXM_OPENFILE, 0, (LPARAM)(szFile))
#define TextView_Clear(hwndTV)				SendMessage((hwndTV), TXM_CLEAR, 0, 0)

//
//	TextView Macros defined here
//
#define TEXTVIEW_CLASS L"TextView32"

//
//	TextView colours
//
#define TXC_BACKGROUND		0			// normal background colour
#define TXC_FOREGROUND		1			// normal foreground colour

#define TXC_MAX_COLOURS		2			// keep this updated!
