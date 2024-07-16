#include "resource.h"
#include "framework.h"
#include "TextView.h"



#define MAX_LOADSTRING 100

// Global Variables:


extern LONG		g_nFontSize;
extern BOOL		g_fFontBold;
extern TCHAR	g_szFontName[];
extern LONG		g_nFontSmoothing;

extern LONG		g_nPaddingAbove;
extern LONG		g_nPaddingBelow;
extern LONG		g_fPaddingFlags;
extern COLORREF g_rgbColourList[];
extern COLORREF g_rgbCustColours[];


#define COURIERNEW	1
#define LUCIDACONS	2

#define REGLOC _T("SOFTWARE\\ffhzq\\Bapad")

const wchar_t CLASS_NAME[] = L"Bapad";



extern HWND		g_hwndMain;
extern HWND		g_hwndTextView;
extern HFONT	g_hFont;


// Forward declarations of functions included in this code module:
void                RegisterMainWindow();
BOOL                CreateMainWnd(int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL ShowOpenFileDlg(HWND hWnd, wchar_t* fileName, wchar_t* titleName);
void ShowAboutDlg(HWND hWndParent);
void SetWindowFileName(HWND hWnd, wchar_t* fileName);
BOOL DoOpenFile(HWND hWnd, WCHAR* fileName, WCHAR* fileTitle);
void HandleDropFiles(HWND hWnd, HDROP hDrop);
