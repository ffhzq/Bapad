#pragma once
#include "TextDocument.h"
#include "FontStruct.h"
#include "TextViewWin32.h"

//#define LONGEST_LINE 0x100

const int	LONGEST_LINE = 0x100;
const int	TEXTBUFSIZE	= 32;
const int	MAX_FONTS = 32;

class TextView
{
public:

	TextView(HWND hwnd);
	~TextView();

	//滚动等操作后重新绘制
	LONG OnPaint();
	LONG OnSetFont(HFONT hFont);
	LONG OnSize(UINT nFlags, int width, int height);
	LONG OnVScroll(UINT nSBCode, UINT nPos);
	LONG OnHScroll(UINT nSBCode, UINT nPos);
	LONG OnMouseWheel(int nDelta);

	//鼠标
	LONG OnMouseActivate(HWND hwndTop, UINT nHitTest, UINT nMessage);
	LONG OnLButtonDown(UINT nFlags, int x, int y);
	LONG OnLButtonUp(UINT nFlags, int x, int y);
	LONG OnMouseMove(UINT nFlags, int x, int y);


	LONG OnSetFocus(HWND hwndOld);
	LONG OnKillFocus(HWND hwndNew);


	LONG OpenFile(WCHAR* szFileName);
	LONG ClearFile();

	LONG AddFont(HFONT);
	LONG SetFont(HFONT, int idx);
	LONG SetLineSpacing(int nAbove, int nBelow);


private:

	void PaintLine(HDC hdc, ULONG line);

	void TabbedExtTextOut(HDC hdc, RECT* rect, WCHAR* buf, size_t len);


	void RefreshWindow();
	LONG InvalidateRange(ULONG nStart, ULONG nFinish);

	int	 TabWidth();


	COLORREF GetColour(UINT idx);

	VOID	SetupScrollbars();
	VOID	UpdateMetrics();
	bool    PinToBottomCorner();
	void	Scroll(int dx, int dy);




	BOOL  MouseCoordToFilePos(int x, int y, ULONG* pnLineNo, ULONG* pnCharOffset, ULONG* pnFileOffset, int* px);

	HWND	hWnd;

	// Font-related data	
	FONT	fontAttr[MAX_FONTS];
	HFONT	font;
	int		fontWidth;
	int		fontHeight;

	// Scrollbar related data
	ULONG	vScrollPos;
	ULONG   vScrollMax;
	int		hScrollPos;
	int		hScrollMax;

	int		longestLine;
	int		windowLines;
	int		windowColumns;

	// Display related data
	int		tabWidthchars;
	ULONG	selectionStart;
	ULONG	selectionEnd;
	ULONG	cursorOffset;


	// File-related data
	ULONG	lineCount;


	// Runtime related data
	bool	mouseDown;

	//should use smart pointer?
	//TextDocument* m_pTextDoc;
	std::unique_ptr<TextDocument> pTextDoc;
};