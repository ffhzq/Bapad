#include "windows.h"
#include "framework.h"
#include "Bapad.h"
#include "TextView.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

LONGLONG CALLBACK FontOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONGLONG CALLBACK MiscOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LONG  g_nFontSize;
BOOL  g_fFontBold;
wchar_t g_szFontName[LF_FACESIZE];
LONG  g_nFontSmoothing;

LONG  g_nPaddingAbove;
LONG  g_nPaddingBelow;
LONG  g_fPaddingFlags;



COLORREF g_rgbColourList[TXC_MAX_COLOURS];
COLORREF g_rgbCustColours[16];

// Get a binary buffer from the registry
BOOL GetSettingBin(HKEY hkey, const wchar_t szKeyName[], PVOID pBuffer, ULONG nLength)
{
	ZeroMemory(pBuffer, nLength);
	return !RegQueryValueExW(hkey, szKeyName, 0, 0, (BYTE*)pBuffer, &nLength);
}

// Get an integer value from the registry
BOOL GetSettingInt(HKEY hkey, const wchar_t szKeyName[], void* pnReturnVal, LONG nDefault)// LONG* pnReturnVal
{
	ULONG len = sizeof(nDefault);

	pnReturnVal = &nDefault;//*pnReturnVal = nDefault

	return !RegQueryValueExW(hkey, szKeyName, 0, 0, (BYTE*)pnReturnVal, &len);
}

// Get a string buffer from the registry
BOOL GetSettingStr(HKEY hkey, const wchar_t szKeyName[], wchar_t pszReturnStr[], DWORD nLength, const wchar_t szDefault[])
{
	ULONG len = nLength * sizeof(wchar_t);

	lstrcpynW(pszReturnStr, szDefault, nLength);

	return !RegQueryValueExW(hkey, szKeyName, 0, 0, (BYTE*)pszReturnStr, &len);
}

// Write a binary value from the registry
BOOL WriteSettingBin(HKEY hkey, const wchar_t szKeyName[], PVOID pData, ULONG nLength)
{
	return !RegSetValueExW(hkey, szKeyName, 0, REG_BINARY, (BYTE*)pData, nLength);
}

// Write an integer value from the registry
BOOL WriteSettingInt(HKEY hkey, const wchar_t szKeyName[], LONG nValue)
{
	return !RegSetValueExW(hkey, szKeyName, 0, REG_DWORD, (BYTE*)&nValue, sizeof(nValue));
}

// Get a string buffer from the registry
BOOL WriteSettingStr(HKEY hkey, const wchar_t szKeyName[], wchar_t szString[])
{
	return !RegSetValueExW(hkey, szKeyName, 0, REG_SZ, (BYTE*)szString, static_cast<DWORD>((wcslen(szString) + 1) * sizeof(wchar_t)));
}

void LoadRegSettings()
{
	HKEY hKey, hColKey;

	// open registry location for reading
	RegCreateKeyExW(HKEY_CURRENT_USER, REGLOC, 0, 0, 0, KEY_READ, 0, &hKey, 0);

	GetSettingInt(hKey, _T("FontSize"), &g_nFontSize, 10);
	GetSettingInt(hKey, _T("FontBold"), &((g_fFontBold)), FALSE);
	GetSettingStr(hKey, _T("FontName"), g_szFontName, LF_FACESIZE, _T("Courier New"));
	GetSettingInt(hKey, _T("FontSmooth"), &g_nFontSmoothing, DEFAULT_QUALITY);

	GetSettingInt(hKey, _T("PaddingAbove"), &g_nPaddingAbove, 0);
	GetSettingInt(hKey, _T("PaddingBelow"), &g_nPaddingBelow, 1);
	GetSettingInt(hKey, _T("PaddingFlags"), &g_fPaddingFlags, COURIERNEW | LUCIDACONS);

	// read the display colours
	RegCreateKeyExW(hKey, _T("Colours"), 0, 0, 0, KEY_READ, 0, &hColKey, 0);

	GetSettingInt(hColKey, _T("Foreground"), &g_rgbColourList[TXC_FOREGROUND], SYSCOL(COLOR_WINDOWTEXT));
	GetSettingInt(hColKey, _T("Background"), &g_rgbColourList[TXC_BACKGROUND], SYSCOL(COLOR_WINDOW));
	GetSettingInt(hColKey, _T("SelFG"), &g_rgbColourList[TXC_HIGHLIGHTTEXT], SYSCOL(COLOR_HIGHLIGHTTEXT));
	GetSettingInt(hColKey, _T("SelBG"), &g_rgbColourList[TXC_HIGHLIGHT], SYSCOL(COLOR_HIGHLIGHT));
	GetSettingInt(hColKey, _T("SelFG2"), &g_rgbColourList[TXC_HIGHLIGHTTEXT2], SYSCOL(COLOR_INACTIVECAPTIONTEXT));
	GetSettingInt(hColKey, _T("SelBG2"), &g_rgbColourList[TXC_HIGHLIGHT2], SYSCOL(COLOR_INACTIVECAPTION));

	GetSettingBin(hColKey, _T("Custom"), g_rgbCustColours, sizeof(g_rgbCustColours));

	RegCloseKey(hColKey);
	RegCloseKey(hKey);
}

void SaveRegSettings()
{
	HKEY hKey, hColKey;

	// open registry location for writing
	RegCreateKeyEx(HKEY_CURRENT_USER, REGLOC, 0, 0, 0, KEY_WRITE, 0, &hKey, 0);

	WriteSettingInt(hKey, _T("FontSize"), g_nFontSize);
	WriteSettingInt(hKey, _T("FontBold"), g_fFontBold);
	WriteSettingStr(hKey, _T("FontName"), g_szFontName);
	WriteSettingInt(hKey, _T("FontSmooth"), g_nFontSmoothing);

	WriteSettingInt(hKey, _T("PaddingAbove"), g_nPaddingAbove);
	WriteSettingInt(hKey, _T("PaddingBelow"), g_nPaddingBelow);
	WriteSettingInt(hKey, _T("PaddingFlags"), g_fPaddingFlags);

	// write the display colours
	RegCreateKeyExW(hKey, _T("Colours"), 0, 0, 0, KEY_WRITE, 0, &hColKey, 0);

	WriteSettingInt(hColKey, _T("Foreground"), g_rgbColourList[TXC_FOREGROUND]);
	WriteSettingInt(hColKey, _T("Background"), g_rgbColourList[TXC_BACKGROUND]);
	WriteSettingInt(hColKey, _T("SelFG"), g_rgbColourList[TXC_HIGHLIGHTTEXT]);
	WriteSettingInt(hColKey, _T("SelBG"), g_rgbColourList[TXC_HIGHLIGHT]);
	WriteSettingInt(hColKey, _T("SelFG2"), g_rgbColourList[TXC_HIGHLIGHTTEXT2]);
	WriteSettingInt(hColKey, _T("SelBG2"), g_rgbColourList[TXC_HIGHLIGHT2]);

	WriteSettingBin(hColKey, _T("Custom"), g_rgbCustColours, sizeof(g_rgbCustColours));

	RegCloseKey(hColKey);
	RegCloseKey(hKey);
}

void ApplyRegSettings()
{
	int i;

	if (g_hFont)
		DeleteObject(g_hFont);

	g_hFont = EasyCreateFont(g_nFontSize, g_fFontBold, g_nFontSmoothing, g_szFontName);

	TextView_SetLineSpacing(g_hwndTextView, g_nPaddingAbove, g_nPaddingBelow);

	SendMessageW(g_hwndTextView, WM_SETFONT, (WPARAM)g_hFont, 0);

	for (i = 0; i < TXC_MAX_COLOURS; i++)
	{
		TextView_SetColor(g_hwndTextView, i, g_rgbColourList[i]);
	}
}

void ShowProperties(HWND hwndParent)
{
	PROPSHEETHEADERW psh = { sizeof(psh) };
	PROPSHEETPAGEW   psp[3] = { { sizeof(psp[0]) },  { sizeof(psp[1]) }, { sizeof(psp[2]) } };
	
	CoInitialize(0);

	// configure property sheet
	psh.dwFlags = PSH_PROPSHEETPAGE;
	psh.hwndParent = hwndParent;
	psh.nPages = sizeof(psp) / sizeof(psp[0]);
	psh.ppsp = psp;
	psh.pszCaption = _T("Options");

	// configure property sheet page
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = GetModuleHandle(0);
	psp[0].pfnDlgProc = FontOptionsDlgProc;
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_FONT);
	psp[0].pszTitle = _T("Font");

	psp[1].dwFlags = PSP_USETITLE;
	psp[1].hInstance = GetModuleHandle(0);
	//psp[1].pfnDlgProc = DisplayOptionsDlgProc;
	//psp[1].pszTemplate = MAKEINTRESOURCE(IDD_DISPLAY);
	psp[1].pszTitle = _T("Display");

	psp[2].dwFlags = PSP_USETITLE;
	psp[2].hInstance = GetModuleHandle(0);
	psp[2].pfnDlgProc = MiscOptionsDlgProc;
	psp[2].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS);
	psp[2].pszTitle = _T("Settings");
	
	if (PropertySheetW(&psh))
	{
		ApplyRegSettings();
	}
	
	CoUninitialize();
}
