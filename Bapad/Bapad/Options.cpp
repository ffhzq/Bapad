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