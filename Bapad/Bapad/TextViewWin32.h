#pragma once
#include <CommCtrl.h>

//	TextView API declared here
//
BOOL RegisterTextView();
HWND	 CreateTextView(HWND hwndParent);
COLORREF RealizeColour(COLORREF col);


//
//	TextView Window Messages defined here
//
#define TXM_BASE				 (WM_USER)
#define TXM_OPENFILE			 (TXM_BASE + 0)
#define TXM_CLEAR				 (TXM_BASE + 1)
#define TXM_SETLINESPACING		 (TXM_BASE + 2)
#define TXM_ADDFONT				 (TXM_BASE + 3)
#define TXM_SETCOLOR			 (TXM_BASE + 5)
#define TXM_SETSTYLE			 (TXM_BASE + 6)
#define TXM_GETSTYLE			 (TXM_BASE + 7)
#define TXM_SETCARETWIDTH		 (TXM_BASE + 8)
#define TXM_SETIMAGELIST		 (TXM_BASE + 9)
#define TXM_SETLONGLINE			 (TXM_BASE + 10)
#define TXM_SETLINEIMAGE		 (TXM_BASE + 11)
#define TXM_GETFORMAT			 (TXM_BASE + 12)

//
//	TextView Macros defined here
//
#define TEXTVIEW_CLASS L"TextView32"


//
//	TextView Notification Messages defined here - 
//	sent via the WM_NOTIFY message
//
#define TVN_BASE				(WM_USER)
#define TVN_CURSOR_CHANGE		(TVN_BASE + 0)
#define TVN_SELECTION_CHANGE	(TVN_BASE + 1)
#define TVN_EDITMODE_CHANGE		(TVN_BASE + 2)
#define TVN_CHANGED				(TVN_BASE + 3)




//
//	TextView colours
//
#define TXC_FOREGROUND			0			// normal foreground colour
#define TXC_BACKGROUND			1			// normal background colour
#define TXC_HIGHLIGHTTEXT		2			// normal text highlight colour
#define TXC_HIGHLIGHT			3			// normal background highlight colour
#define TXC_HIGHLIGHTTEXT2		4			// inactive text highlight colour
#define TXC_HIGHLIGHT2			5			// inactive background highlight colour
#define TXC_SELMARGIN1			6			// selection margin colour#1
#define TXC_SELMARGIN2			7			// selection margin colour#2
#define TXC_LINENUMBERTEXT		8			// line number text
#define TXC_LINENUMBER			9			// line number background
#define TXC_LONGLINETEXT		10			// long-line text
#define TXC_LONGLINE			11			// long-line background
#define TXC_CURRENTLINETEXT		12			// active line text
#define TXC_CURRENTLINE			13			// active line background

const size_t TXC_MAX_COLOURS = 6;			// keep this updated!

#define SYSCOL(COLOR_IDX)   ( 0x80000000 | COLOR_IDX)
#define SYSCOLIDX(COLREF)   (~0x80000000 & COLREF)


#define REALIZE_SYSCOL(col) (RealizeColour(col))



//
//	TextView Message Macros defined here
//
auto inline TextView_OpenFile(HWND hwndTV, wchar_t* szFile)
{
	return SendMessageW((hwndTV), TXM_OPENFILE, 0, (LPARAM)(szFile));
}


auto inline TextView_Clear(HWND hwndTV)
{
	return SendMessageW((hwndTV), TXM_CLEAR, 0, 0);
}

auto inline TextView_SetLineSpacing(HWND hwndTV, int nAbove, int nBelow)
{
	return SendMessageW((hwndTV), TXM_SETLINESPACING, (int)(nAbove), (int)(nBelow));
}


auto inline TextView_AddFont(HWND hwndTV, HFONT hFont, HWND g_hwndTextView)
{
	return SendMessageW((hwndTV), TXM_ADDFONT, (WPARAM)(HFONT)(g_hwndTextView), 0);
}

auto inline TextView_SetColor(HWND hwndTV, WPARAM nIdx, LPARAM rgbColor)
{
	return SendMessageW((hwndTV), TXM_SETCOLOR, (WPARAM)(nIdx), (LPARAM)(rgbColor));
}


auto inline TextView_SetStyle(HWND hwndTV, WPARAM uMask, LPARAM uStyles)
{
	return SendMessageW((hwndTV), TXM_SETSTYLE, (WPARAM)(uMask), (LPARAM)(uStyles));
}

auto inline TextView_SetStyleBool(HWND hwndTV, WPARAM uStyle, LPARAM fBoolean)
{
	return SendMessageW((hwndTV), TXM_SETSTYLE, (WPARAM)(uStyle), (LPARAM)(fBoolean ? uStyle : 0));
}

auto inline TextView_SetCaretWidth(HWND hwndTV, WPARAM nWidth)
{
	return SendMessageW((hwndTV), TXM_SETCARETWIDTH, (WPARAM)(nWidth), 0);
}

auto inline TextView_SetImageList(HWND hwndTV, HIMAGELIST hImgList)
{
	return SendMessageW((hwndTV), TXM_SETIMAGELIST, (WPARAM)(HIMAGELIST)(hImgList), 0);
}

auto inline TextView_SetLongLine(HWND hwndTV, LPARAM nLength)
{
	return SendMessageW((hwndTV), TXM_SETLONGLINE, (WPARAM)(0), (LPARAM)(nLength));
}

auto inline TextView_SetLineImage(HWND hwndTV, ULONG nLineNo, ULONG nImageIdx)
{
	return SendMessageW((hwndTV), TXM_SETLINEIMAGE, (WPARAM)(ULONG)(nLineNo), (LPARAM)(ULONG)nImageIdx);
}

auto inline TextView_GetFormat(HWND hwndTV)
{
	return SendMessageW((hwndTV), TXM_GETFORMAT, 0, 0);
}
