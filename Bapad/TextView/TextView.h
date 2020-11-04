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


	void RefreshWindow();
	void PaintLine(HDC hdc, ULONG line);
	void PaintText(HDC hdc, ULONG nLineNo, RECT* rect);
	


	void TabbedExtTextOut(HDC hdc, RECT* rect, WCHAR* buf, size_t len);


	
	LONG InvalidateRange(ULONG nStart, ULONG nFinish);

	int	 TabWidth();


	COLORREF GetColour(UINT idx);

	VOID	SetupScrollbars();
	VOID	UpdateMetrics();
	VOID	RecalcLineHeight();
	bool    PinToBottomCorner();
	void	Scroll(int dx, int dy);

	BOOL  MouseCoordToFilePos(int x, int y, ULONG& pnLineNo, ULONG& pnCharOffset, ULONG& pnFileOffset, int& px);

	int  TextWidth(HDC hdc, TCHAR* buf, int len, int nTabOrigin);


	HWND	hWnd;

	// Font-related data
	std::vector<FONT> fontAttr;
	HFONT	font;
	int		fontWidth;
	int		lineHeight;
	int		maxAscent;	
	int		heightAbove;
	int		heightBelow;

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

	COLORREF rgbColourList[TXC_MAX_COLOURS];

	// File-related data
	ULONG	lineCount;


	// Runtime related data
	bool	mouseDown;

	
	std::unique_ptr<TextDocument> pTextDoc;
};