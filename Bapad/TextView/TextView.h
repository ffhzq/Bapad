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

	//������Ϣ
	LONG OnPaint();
	LONG OnSetFont(HFONT hFont);
	LONG OnSize(UINT nFlags, int width, int height);
	LONG OnVScroll(UINT nSBCode, UINT nPos);
	LONG OnHScroll(UINT nSBCode, UINT nPos);
	LONG OnMouseWheel(int nDelta);
	LONG OnTimer(UINT nTimer);

	//���
	LONG OnMouseActivate(HWND hwndTop, UINT nHitTest, UINT nMessage);
	LONG OnLButtonDown(UINT nFlags, int x, int y);
	LONG OnLButtonUp(UINT nFlags, int x, int y);
	LONG OnMouseMove(UINT nFlags, int x, int y);

	LONG OnSetFocus(HWND hwndOld);
	LONG OnKillFocus(HWND hwndNew);

	//�û�����

	LONG OpenFile(WCHAR* szFileName);
	LONG ClearFile();

	LONG AddFont(HFONT);
	LONG SetFont(HFONT, size_t idx);
	LONG SetLineSpacing(int nAbove, int nBelow);
	COLORREF SetColour(UINT idx, COLORREF rgbColour);

private:

	void	PaintLine(HDC hdc, ULONG line);
	void	PaintText(HDC hdc, ULONG nLineNo, RECT* rect);
	
	size_t	ApplyTextAttributes(size_t nLineNo, size_t offset, WCHAR* szText, int nTextLen, ATTR* attr);
	int		BaTextOut(HDC hdc, int xpos, int ypos, WCHAR* szText, int nLen, int nTabOrigin, ATTR* attr);

	int		PaintCtrlChar(HDC hdc, int xpos, int ypos, ULONG chValue, FONT* fa);
	void	InitCtrlCharFontAttr(HDC hdc, FONT* fa);

	void	RefreshWindow();
	LONG	InvalidateRange(size_t nStart, size_t nFinish);

	int		GetCtrlCharWidth(HDC hdc, ULONG chValue, FONT* fa);
	int		BaTextWidth(HDC hdc, WCHAR* buf, int len, int nTabOrigin);
	int		TabWidth();

	BOOL	MouseCoordToFilePos(int x, int y, 
		size_t& pnLineNo, size_t& pnCharOffset,
		size_t& pnFileOffset, int& px);
	ULONG  RepositionCaret();


	COLORREF GetColour(UINT idx);

	VOID	SetupScrollbars();
	VOID	UpdateMetrics();
	VOID	RecalcLineHeight();
	bool    PinToBottomCorner();

	void	Scroll(int dx, int dy);
	HRGN	ScrollRgn(int dx, int dy, bool fReturnUpdateRgn);

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

	size_t	longestLine;
	int		windowLines;
	int		windowColumns;

	// Display related data
	int		tabWidthChars;
	size_t	selectionStart;
	size_t	selectionEnd;
	size_t	cursorOffset;

	COLORREF rgbColourList[TXC_MAX_COLOURS];

	// File-related data
	size_t	lineCount;


	// Runtime related data
	bool	mouseDown;
	UINT	scrollTimer;
	int		scrollCounter;
	
	std::unique_ptr<TextDocument> pTextDoc;
};