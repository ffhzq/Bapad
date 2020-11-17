#include "resource.h"
#include "framework.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "../TextView/TextView.h"

/*
#include "..\TextView\TextDocument.h"
*/


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

const wchar_t ClassName[] = L"Bapad";



extern HWND		g_hwndMain;
extern HWND		g_hwndTextView;
extern HFONT	g_hFont;


// Forward declarations of functions included in this code module:
ATOM                RegisterMainWindow(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL ShowOpenFileDlg(HWND hWnd, wchar_t* fileName, wchar_t* titleName);
void ShowAboutDlg(HWND hWndParent);
void SetWindowFileName(HWND hWnd, wchar_t* fileName);
BOOL DoOpenFile(HWND hWnd, WCHAR* fileName, WCHAR* fileTitle);
void HandleDropFiles(HWND hWnd, HDROP hDrop);

//
//	Global functions
//
HFONT EasyCreateFont(int nPointSize, BOOL fBold, DWORD dwQuality, TCHAR* szFace);

void ApplyRegSettings();
//void ShowProperties(HWND hwndParent);
void LoadRegSettings();
void SaveRegSettings();